/**
 * @file ble_server.c
 * @brief Triển khai các chức năng Server của Bluetooth Low Energy (BLE).
 * 
 * @note File này thực hiện việc thiết lập phần cứng Bluetooth, cấu hình Bluedroid stack, 
 * và xử lý các sự kiện GATTS. Cụ thể, nó nhận dữ liệu JSON từ app (như lệnh cấu hình WiFi), 
 * phân tích (parse) để thực thi, đồng thời cũng cung cấp cơ chế gửi thông báo trạng thái 
 * (notify) ngược lại cho thiết bị kết nối.
 */

#include "ble_server.h"
#include "wifi_scan_task.h"
#include "wifi_task.h"

#include "esp_log.h"
#include "esp_gatts_api.h"
#include "esp_gap_ble_api.h"
#include "esp_bt_main.h"
#include "esp_bt.h"
#include "cJSON.h"

#include <string.h>
#include <stdio.h>
#include <stdbool.h>

#define TAG "BLE"
#define BLE_JSON_BUF_SIZE 256

#define TAG_WIFI_CMD "wifi_cmd"
#define TAG_WIFI_STATUS "wifi_status"
#define CMD_CONNECT_WIFI "connect_wifi"
#define CMD_WIFI_SCAN "wifi_scan"

// UUID demo
#define SERVICE_UUID   0x1234
#define WRITE_UUID     0x1235
#define NOTIFY_UUID    0x1236

static esp_gatt_if_t gatts_if_global;
static uint16_t conn_id_global;
static uint16_t notify_handle;

// ================= JSON PARSE =================

/**
 * @brief Gửi trạng thái kết nối WiFi qua BLE bằng định dạng JSON.
 * 
 * @param status Chuỗi mô tả trạng thái hiện tại (ví dụ: "connecting").
 * @param connected Trạng thái boolean xác định đã kết nối thành công hay chưa.
 * 
 * @note Hàm sử dụng thư viện cJSON để tạo một đối tượng JSON chứa tag, status và cờ connected. 
 * Sau đó, đối tượng được chuyển sang dạng chuỗi thô (unformatted string) và truyền đi qua BLE bằng 
 * hàm ble_send_notify. Cuối cùng, giải phóng bộ nhớ JSON và chuỗi đã cấp phát.
 */
static void ble_send_wifi_status(const char *status, bool connected)
{
    cJSON *root = cJSON_CreateObject();
    if (root == NULL) {
        ESP_LOGE(TAG, "Failed to create wifi_status JSON");
        return;
    }
    

    cJSON_AddStringToObject(root, "tag", TAG_WIFI_STATUS);
    cJSON_AddStringToObject(root, "status", status);
    cJSON_AddBoolToObject(root, "connected", connected);

    char *payload = cJSON_PrintUnformatted(root);//transfer JSON to text
    if (payload != NULL) {
        ble_send_notify(payload);
        cJSON_free(payload);//free the text after sending
    }

    cJSON_Delete(root);
}

/**
 * @brief Xử lý lệnh điều khiển WiFi nhận được từ BLE.
 * 
 * @param root Đối tượng cJSON chứa dữ liệu phân tích từ chuỗi nhận được.
 * 
 * @note Trích xuất trường "cmd" để xác định yêu cầu. Nếu là "wifi_scan", tiến hành yêu cầu quét 
 * mạng. Nếu là "connect_wifi", tiếp tục trích xuất "ssid" và "pass", sau đó gọi hàm wifi_connect 
 * để thực hiện kết nối. Cuối cùng phản hồi lại BLE là đang trong quá trình "connecting".
 */
static void handle_wifi_cmd(cJSON *root)
{
    cJSON *cmd = cJSON_GetObjectItem(root, "cmd");
    if (!cJSON_IsString(cmd)) {
        ESP_LOGE(TAG, "Missing cmd");
        return;
    }

    if (strcmp(cmd->valuestring, CMD_WIFI_SCAN) == 0) {
        wifi_scan_request();
        return;
    }

    if (strcmp(cmd->valuestring, CMD_CONNECT_WIFI) != 0) {
        ESP_LOGW(TAG, "Unknown wifi cmd: %s", cmd->valuestring);
        return;
    }

    cJSON *ssid = cJSON_GetObjectItem(root, "ssid");
    cJSON *pass = cJSON_GetObjectItem(root, "pass");


    if (!cJSON_IsString(ssid) || !cJSON_IsString(pass)) {
        ESP_LOGE(TAG, "Missing ssid or pass");
        return;
    }

    ESP_LOGI(TAG, "Connect WiFi request: %s", ssid->valuestring);
    wifi_connect(ssid->valuestring, pass->valuestring);
    ble_send_wifi_status("connecting", false);
}

/**
 * @brief Xử lý dữ liệu thô (raw data) nhận được qua sự kiện GATTS WRITE.
 * 
 * @param data Con trỏ chứa mảng dữ liệu byte nhận được.
 * @param len Chiều dài của dữ liệu.
 * 
 * @note Hàm thực hiện việc kiểm tra độ dài an toàn để chống tràn bộ đệm. Đưa dữ liệu byte vào 
 * chuỗi cục bộ để chèn ký tự kết thúc chuỗi ('\0'). Sau đó dùng thư viện cJSON để phân tích 
 * chuỗi thành đối tượng JSON và dựa vào trường "tag" để phân luồng xử lý tiếp (ví dụ: wifi_cmd).
 */
static void handle_write(uint8_t *data, uint16_t len)
{
    if (len == 0 || len >= BLE_JSON_BUF_SIZE) { //check length for safety
        ESP_LOGE(TAG, "Invalid BLE JSON length: %u", len);
        return;
    }

    char json[BLE_JSON_BUF_SIZE];
    memcpy(json, data, len);
    json[len] = 0;

    ESP_LOGI(TAG, "Recv: %s", json);

    cJSON *root = cJSON_Parse(json);
    if (root == NULL) {
        ESP_LOGE(TAG, "Invalid JSON");
        return;
    }

    cJSON *tag = cJSON_GetObjectItem(root, "tag");
    if (!cJSON_IsString(tag)) {
        ESP_LOGE(TAG, "Missing tag");
        cJSON_Delete(root);
        return;
    }

    if (strcmp(tag->valuestring, TAG_WIFI_CMD) == 0) {
        handle_wifi_cmd(root);
    } else {
        ESP_LOGW(TAG, "Unknown tag: %s", tag->valuestring);
    }

    cJSON_Delete(root);
}

// ================= GATT EVENT =================

/**
 * @brief Hàm callback để xử lý các sự kiện của profile GATTS.
 * 
 * @param event Loại sự kiện xảy ra (Connect, Write, Read...).
 * @param gatts_if Giao diện GATTS hiện hành.
 * @param param Con trỏ trỏ tới dữ liệu chi tiết của sự kiện.
 * 
 * @note Cơ chế: Khi client kết nối, hàm sẽ lưu lại `conn_id` và `gatts_if` vào biến toàn cục 
 * nhằm tái sử dụng cho việc gửi phản hồi sau này. Khi có dữ liệu được ghi xuống (Write), 
 * nó sẽ gọi hàm `handle_write` để phân tích và xử lý luồng dữ liệu.
 */
static void gatts_event_handler(esp_gatts_cb_event_t event,
                               esp_gatt_if_t gatts_if,
                               esp_ble_gatts_cb_param_t *param)
{
    switch (event)
    {
    case ESP_GATTS_CONNECT_EVT:
        conn_id_global = param->connect.conn_id;
        gatts_if_global = gatts_if;
        ESP_LOGI(TAG, "Connected");
        break;

    case ESP_GATTS_WRITE_EVT:
        handle_write(param->write.value, param->write.len);
        break;

    default:
        break;
    }
}

// ================= INIT =================

/**
 * @brief Khởi tạo giao thức Bluetooth Low Energy (BLE).
 * 
 * @note Hàm này cấu hình và kích hoạt controller Bluetooth của ESP ở chế độ BLE, giải phóng bộ 
 * nhớ của Classic BT để tiết kiệm tài nguyên. Sau đó, nó khởi tạo Bluedroid, đăng ký callback xử 
 * lý sự kiện GATTS và đăng ký ứng dụng BLE GATTS cơ bản.
 */
void ble_init(void)
{
    esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT);

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    esp_bt_controller_init(&bt_cfg);
    esp_bt_controller_enable(ESP_BT_MODE_BLE);

    esp_bluedroid_init();
    esp_bluedroid_enable();

    esp_ble_gatts_register_callback(gatts_event_handler);
    esp_ble_gatts_app_register(0);
}

// ================= SEND =================

/**
 * @brief Gửi thông báo (notify/indicate) qua BLE cho thiết bị client đã kết nối.
 * 
 * @param data Chuỗi dữ liệu (dạng string) cần gửi đi.
 * 
 * @note Cơ chế là sử dụng hàm `esp_ble_gatts_send_indicate` để truyền đi chuỗi byte tương ứng 
 * của dữ liệu, gửi qua interface (gatts_if) và ID kết nối (conn_id) đã được lưu khi có thiết bị 
 * kết nối thành công.
 */
void ble_send_notify(const char *data)
{
    if (!data) return;

    esp_ble_gatts_send_indicate(
        gatts_if_global,
        conn_id_global,
        notify_handle,
        strlen(data),
        (uint8_t *)data,
        false
    );
}
