idf_component_register(
                    SRCS "ble_server.c"
                    INCLUDE_DIRS "."
                    REQUIRES bt json wifi wifi_provisioning nvs_flash log
)
