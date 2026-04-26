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


bool aws_iot_is_connected(void)
{
    return s_cloud_connected;
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = event_data;
    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
            s_cloud_connected = true;
            esp_mqtt_client_publish(event->client,
                AWS_IOT_TELEMETRY_TOPIC,
                "hello",
                0, 1, 0);
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
            s_cloud_connected = false;
            break;
        default:
            ESP_LOGI(TAG, "Other MQTT event id: %d", event->event_id);
            break;
    }
}

esp_err_t esp_mqtt_connect(esp_mqtt_client_handle_t client){
    esp_err_t err = esp_mqtt_client_start(client);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start MQTT client: %s", esp_err_to_name(err));
            return err;
     }
     return ESP_OK;
}

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

            esp_mqtt_connect(client);

            ESP_LOGI(TAG, "MQTT started");
        }

        // 📡 LẤY DATA TỪ QUEUE + PUBLISH
        if (client != NULL && aws_iot_is_connected()) {

            if (xQueueReceive(cloud_queue, &sample, pdMS_TO_TICKS(500))) {

                // ❌ bỏ sample không hợp lệ
                if (!sample.valid) {
                    ESP_LOGW(TAG, "Invalid sample, skip");
                    continue;
                }

                // 🧠 serialize JSON
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

                int msg_id = esp_mqtt_client_publish(client,
                    AWS_IOT_TELEMETRY_TOPIC,
                    payload,
                    len,
                    1,   // QoS 1 (khuyên dùng)
                    0);

                if (msg_id == -1) {
                    ESP_LOGE(TAG, "Publish failed");
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}