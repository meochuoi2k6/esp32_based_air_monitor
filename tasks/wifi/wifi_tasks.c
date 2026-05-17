/**
 * @file wifi_tasks.c
 * @brief Triển khai chức năng quản lý, duy trì kết nối WiFi Station.
 * 
 * @note File này chịu trách nhiệm khởi tạo lớp NVS flash, khởi tạo thư viện Netif 
 * của ESP32, đăng ký bộ lắng nghe sự kiện để tự động kết nối lại khi mất mạng 
 * và xử lý cập nhật trạng thái kết nối.
 */

#include "wifi_task.h"

#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_log.h"
#include <stdbool.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "WIFI";
    static bool isWifi = 0;

/**
 * @brief Xử lý các sự kiện hệ thống liên quan đến WiFi và IP.
 * 
 * @param arg Các đối số chuyển vào lúc đăng ký.
 * @param event_base Nhóm sự kiện (WIFI_EVENT hoặc IP_EVENT).
 * @param event_id Mã sự kiện (Ví dụ kết nối, ngắt kết nối, nhận IP).
 * @param event_data Dữ liệu chi tiết của sự kiện.
 * 
 * @note Cơ chế: Nếu nhận sự kiện STA_START, tiến hành connect. Nếu nhận STA_DISCONNECTED, 
 * tiến hành reconnect đồng thời set isWifi=false. Khi nhận GOT_IP, báo hiệu isWifi=true.
 */
static void wifi_event_handler(void* arg,
                               esp_event_base_t event_base,
                               int32_t event_id,
                               void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGI(TAG, "Retrying...");
        isWifi = false;
        esp_wifi_connect();
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ESP_LOGI(TAG, "Got IP!");
        isWifi = true;
    }

}

/**
 * @brief Yêu cầu kết nối mạng WiFi với SSID và password chỉ định.
 * 
 * @param ssid Tên mạng WiFi muốn kết nối.
 * @param pass Mật khẩu của mạng.
 * 
 * @note Sử dụng struct `wifi_config_t`, gán chuỗi rồi set lại cấu hình thông qua 
 * `esp_wifi_set_config`. Tiếp theo ngắt kết nối hiện tại và gọi `esp_wifi_connect`.
 */
void wifi_connect(const char *ssid, const char *pass)
{
    wifi_config_t wifi_config = {0};

    strncpy((char*)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid));
    strncpy((char*)wifi_config.sta.password, pass, sizeof(wifi_config.sta.password));

    ESP_LOGI(TAG, "Connecting to SSID: %s", ssid);

    esp_wifi_disconnect();
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    esp_wifi_connect();
}

/**
 * @brief Khởi tạo các hệ thống hỗ trợ và bắt đầu tiến trình chạy WiFi.
 * 
 * @note Trình tự: 1. Khởi tạo NVS flash. 2. Khởi tạo Netif. 3. Tạo event loop. 
 * 4. Khởi tạo trạm wifi mặc định (STA). 5. Cài đặt wifi config mặc định. 6. Cài đặt chế 
 * độ STA. 7. Đăng ký các event handler cho WIFI_EVENT và IP_EVENT. 8. Bắt đầu wifi start.
 * @return true nếu lệnh gọi esp_wifi_connect trả về ESP_OK, ngược lại là false.
 */
bool wifi_init() {
    esp_err_t ret = nvs_flash_init();
    if (ret != ESP_OK) {
        nvs_flash_erase();
        nvs_flash_init();
    }

    esp_netif_init();

    esp_event_loop_create_default();

    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    esp_wifi_set_mode(WIFI_MODE_STA);

    esp_event_handler_instance_register(WIFI_EVENT,
                                    ESP_EVENT_ANY_ID,
                                    &wifi_event_handler,
                                    NULL,
                                    NULL);

    esp_event_handler_instance_register(IP_EVENT,
                                        IP_EVENT_STA_GOT_IP,
                                        &wifi_event_handler,
                                        NULL,
                                        NULL);

    esp_wifi_start();

    esp_err_t err = esp_wifi_connect();
    if(err==ESP_OK) return 1;
    return 0;
}

/**
 * @brief Trả về biến trạng thái boolean của kết nối mạng WiFi.
 * 
 * @note Giá trị isWifi dựa trên các sự kiện thu được trong `wifi_event_handler`.
 * @return true nếu đã kết nối và cấp IP thành công, ngược lại là false.
 */
bool wifi_is_connected(void)
{
    return isWifi;
}

/**
 * @brief Task wrapper để khởi động tiến trình WiFi.
 * 
 * @param pvParameters Tham số của task (bỏ qua).
 * 
 * @note Hàm task đóng vai trò gọi `wifi_init` để thiết lập ban đầu và chạy dịch vụ ngầm. 
 * Sau đó tự động xóa tác vụ này đi khi init hoàn thành.
 */
void wifi_task(void *pvParameters)
{
    (void)pvParameters;
    wifi_init();
    vTaskDelete(NULL);
}
