idf_component_register(
                    SRCS "air_monitor.c"
                    INCLUDE_DIRS "."
                    REQUIRES bme680 ssd1306 fatfs sdmmc driver esp_wifi nvs_flash icons esp_https_server
)
