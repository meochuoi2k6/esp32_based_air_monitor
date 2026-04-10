idf_component_register(
                    SRCS "logger_task.c"
                    INCLUDE_DIRS "."
                    REQUIRES fatfs sdmmc driver sensors time
)
