idf_component_register(
                    SRCS "wifi_scan_task.c" "wifi_tasks.c"
                    INCLUDE_DIRS "."
                    REQUIRES esp_wifi nvs_flash esp_netif esp_event wifi_provisioning ble_app log
)
