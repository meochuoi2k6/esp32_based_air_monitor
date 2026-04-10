#pragma once

#include <stdbool.h>

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

typedef struct {
    int pm25;
    int pm10;
    float temperature;
    float humidity;
    float pressure;
    bool valid;
} sensor_sample_t;

typedef struct {
    QueueHandle_t display_queue;
    QueueHandle_t logger_queue;
} sensor_task_params_t;

esp_err_t sensor_task_init(void);
esp_err_t sensor_task_read_average(sensor_sample_t *out_sample);
esp_err_t sensor_task_get_latest(sensor_sample_t *out_sample);
void sensor_task(void *pvParameters);
