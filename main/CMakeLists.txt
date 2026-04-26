idf_component_register(
                    SRCS "air_monitor.c"
                    INCLUDE_DIRS "."
                    REQUIRES cloud display sensors storage time wifi i2cdev
)
