idf_component_register(
                    SRCS "display_task.c"
                    INCLUDE_DIRS "."
                    REQUIRES ssd1306 icons sensors wifi storage
)
