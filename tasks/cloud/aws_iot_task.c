/**
 * @file aws_iot_task.c
 * @brief Triển khai các chức năng kết nối và gửi dữ liệu lên AWS IoT Core.
 * 
 * @note File này chịu trách nhiệm khởi tạo kết nối MQTT, quản lý các sự kiện MQTT, 
 * và duy trì một task để liên tục đọc dữ liệu từ hàng đợi rồi gửi lên AWS IoT dưới 
 * định dạng JSON.
 */

#include "aws_iot_task.h"
#include "esp_err.h"

#include "esp_log.h"
#include "freertos/task.h"
#include "mqtt_client.h"

#include "aws_iot_config.h"
#include "wifi_task.h"
#include "time_tasks.h"

static const char *TAG = "aws_iot_task";
static bool s_cloud_connected = false;
extern const uint8_t AmazonRootCA1_pem_start[] asm("_binary_AmazonRootCA1_pem_start");
extern const uint8_t device_pem_crt_start[] asm("_binary_device_pem_crt_start");
extern const uint8_t private_pem_key_start[] asm("_binary_private_pem_key_start");

/**
 * @brief Kiểm tra trạng thái kết nối tới AWS IoT Cloud.
 * 
 * @note Trả về cờ s_cloud_connected được cập nhật trong hàm mqtt_event_handler.
 * @return true nếu đang kết nối MQTT ổn định, false nếu ngắt kết nối.
 */
bool aws_iot_is_connected(void)
{
    return s_cloud_connected;
}

/**
 * @brief Hàm callback để xử lý các sự kiện MQTT nội bộ.
 * 
 * @param handler_args Tham số con trỏ từ quá trình đăng ký sự kiện.
 * @param base Loại sự kiện cơ bản.
 * @param event_id Mã sự kiện (Ví dụ: connected, disconnected, data).
 * @param event_data Con trỏ dữ liệu chi tiết của sự kiện MQTT.
 * 
 * @note Cơ chế: Nếu kết nối thành công, set cờ `s_cloud_connected = true` và tự động gửi 
 * lệnh subscribe tới topic `AWS_IOT_COMMAND_TOPIC`. Nếu mất kết nối, set lại cờ bằng false. 
 * Khi nhận được data từ topic đã subscribe, log dữ liệu nhận được ra.
 */
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = event_data;
    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
            s_cloud_connected = true;
            esp_mqtt_client_subscribe(event->client, AWS_IOT_COMMAND_TOPIC, 1);
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
            s_cloud_connected = false;
            break;
        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG, "MQTT_EVENT_DATA");
            printf("Received message: %.*s\n", event->data_len, event->data);
            break;
        default:
            ESP_LOGI(TAG, "Other MQTT event id: %d", event->event_id);
            break;
    }
}

/**
 * @brief Khởi động client MQTT đã cấu hình.
 * 
 * @param client Con trỏ handle của MQTT client.
 * 
 * @note Sử dụng hàm gốc của ESP-IDF `esp_mqtt_client_start` để bắt đầu quá trình kết nối. 
 * Có check lỗi và in log nếu khởi động thất bại.
 * @return ESP_OK nếu không gặp sự cố, ngược lại là mã lỗi tương ứng.
 */
esp_err_t aws_iot_start_client(esp_mqtt_client_handle_t client){ //change since it kinda same with ESP API
    esp_err_t err = esp_mqtt_client_start(client);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start MQTT client: %s", esp_err_to_name(err));
            return err;
     }
     return ESP_OK;
}

/**
 * @brief Khởi tạo và thiết lập các thông số cơ bản cho client MQTT của AWS IoT.
 * 
 * @param config Con trỏ trỏ tới cấu trúc cấu hình MQTT client của ESP-IDF.
 * 
 * @note Hàm này sẽ điền thông tin về URI, Port, và gán các con trỏ trỏ tới chứng chỉ 
 * CA, chứng chỉ thiết bị, và khóa riêng tư (đã nhúng vào nhị phân) vào biến config. 
 * @return ESP_OK nếu gán thông số thành công, hoặc lỗi ESP_ERR_INVALID_ARG nếu config bị null.
 */
esp_err_t esp_configure_mqtt_client(esp_mqtt_client_config_t *config)
{
    if (config == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = "mqtts://" AWS_IOT_ENDPOINT,
        .broker.address.port = 8883,
        .broker.verification.certificate = (const char *)AmazonRootCA1_pem_start,
        .credentials.client_id = AWS_IOT_CLIENT_ID,
        .credentials.authentication.certificate = (const char *)device_pem_crt_start,
        .credentials.authentication.key = (const char *)private_pem_key_start,
        .session.keepalive = 60,
        .network.disable_auto_reconnect = false,
    };
    *config = mqtt_cfg;
    return ESP_OK;
}

/**
 * @brief Task chạy ngầm xử lý việc kết nối và đẩy dữ liệu lên AWS IoT.
 * 
 * @param pvParameters Tham số truyền vào cho task, ép kiểu về `aws_iot_task_params_t*`.
 * 
 * @note Cơ chế là đợi WiFi kết nối và thời gian được đồng bộ xong thì mới bắt đầu cấu hình 
 * và khởi động MQTT client (chỉ khởi động 1 lần duy nhất). Khi đã có kết nối MQTT thành công, 
 * vòng lặp chính sẽ liên tục đọc queue để lấy mẫu cảm biến, chuyển sang định dạng chuỗi JSON 
 * và publish lên topic Telemetry của AWS IoT với mức QoS là 1. Sau đó delay 100ms nhường CPU.
 */
void aws_iot_task(void *pvParameters)
{
    aws_iot_task_params_t *params = (aws_iot_task_params_t *)pvParameters;
    QueueHandle_t cloud_queue = params->cloud_queue;

    ESP_LOGI(TAG, "AWS IoT task start");

    static esp_mqtt_client_handle_t client = NULL;

    sensor_sample_t sample;
    char payload[256];

    while (1) {

        // 🔌 INIT MQTT (chỉ 1 lần)
        if (wifi_is_connected() && client == NULL && time_is_synced()) {

            ESP_LOGI(TAG, "Init MQTT...");

            esp_mqtt_client_config_t mqtt_cfg;
            esp_configure_mqtt_client(&mqtt_cfg);

            client = esp_mqtt_client_init(&mqtt_cfg);

            esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID,
                                           mqtt_event_handler, NULL);

            esp_mqtt_client_start(client);

            ESP_LOGI(TAG, "MQTT started");
        }

        // 📡 LẤY DATA TỪ QUEUE + PUBLISH
        if (client != NULL && aws_iot_is_connected()) {

            if (xQueueReceive(cloud_queue, &sample, pdMS_TO_TICKS(500))) {

                if (!sample.valid) {
                    ESP_LOGW(TAG, "Invalid sample, skip");
                    continue;
                }

                int len = snprintf(payload, sizeof(payload),
                    "{"
                    "\"pm25\": %d,"
                    "\"pm10\": %d,"
                    "\"temperature\": %.2f,"
                    "\"humidity\": %.2f,"
                    "\"pressure\": %.2f,"
                    "\"timestamp\": \"%s\""
                    "}",
                    sample.pm25,
                    sample.pm10,
                    sample.temperature,
                    sample.humidity,
                    sample.pressure,
                    sample.timestamp
                );

                if (len <= 0 || len >= sizeof(payload)) {
                    ESP_LOGE(TAG, "Payload overflow!");
                    continue;
                }

                ESP_LOGI(TAG, "Publish: %s", payload);

                int msg_send_id = esp_mqtt_client_publish(client,
                    AWS_IOT_TELEMETRY_TOPIC,
                    payload,
                    len,
                    1,   // QoS 1 (khuyên dùng)
                    0);

                if (msg_send_id == -1) {
                    ESP_LOGE(TAG, "Publish failed");
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
