idf_component_register(
                    SRCS "aws_iot_task.c"
                    INCLUDE_DIRS "."
                    REQUIRES mqtt sensors wifi time                   
)
target_add_binary_data(${COMPONENT_TARGET} "../../certs/AmazonRootCA1.pem" TEXT)
target_add_binary_data(${COMPONENT_TARGET} "../../certs/device.pem.crt" TEXT)
target_add_binary_data(${COMPONENT_TARGET} "../../certs/private.pem.key" TEXT)