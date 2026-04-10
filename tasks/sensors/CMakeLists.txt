idf_component_register(
                    SRCS "sensor_task.c"
                    INCLUDE_DIRS "."
                    REQUIRES bme680 driver
)
