#pragma once

#include <stdbool.h>

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "sensor_task.h"
#include "esp_err.h"
#include "mqtt_client.h"

typedef struct {
    QueueHandle_t cloud_queue;
} aws_iot_task_params_t;

static esp_err_t esp_configure_mqtt_client(esp_mqtt_client_config_t *config);
extern const uint8_t AmazonRootCA1_pem_start[] asm("_binary_AmazonRootCA1_pem_start");
extern const uint8_t device_pem_crt_start[] asm("_binary_device_pem_crt_start");
extern const uint8_t private_pem_key_start[] asm("_binary_private_pem_key_start");
bool aws_iot_is_connected(void);
void aws_iot_task(void *pvParameters);
