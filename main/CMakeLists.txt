idf_component_register(
                    SRCS "air_monitor.c"
                    INCLUDE_DIRS "."
                    REQUIRES display sensors storage time wifi i2cdev
)
