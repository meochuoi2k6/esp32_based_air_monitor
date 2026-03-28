idf_component_register(
                    SRCS "air_monitor.c"
                    INCLUDE_DIRS "."
                    REQUIRES bme680 ssd1306
)
