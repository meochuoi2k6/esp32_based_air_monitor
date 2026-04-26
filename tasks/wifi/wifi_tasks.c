#include "wifi_task.h"

#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_log.h"
#include <stdbool.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define WIFI_SSID ":)" //This part will be replaced by getting ssid and pass in the future
#define WIFI_PASS "88888888" //This part will be replaced by getting ssid and pass in the future

static const char *TAG = "WIFI";
    static bool isWifi = 0;

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

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
        },
    };

    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);



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

    esp_err_t err = esp_wifi_connect();{
        if(err==ESP_OK) return 1;
        return 0;
    }
}

bool wifi_is_connected(void)
{
    return isWifi;
}

void wifi_task(void *pvParameters)
{
    (void)pvParameters;
    wifi_init();
    vTaskDelete(NULL);
}
