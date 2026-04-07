#pragma once

#include <stdbool.h>

#include "esp_err.h"

typedef struct {
    int pm25;
    int pm10;
    float temperature;
    float humidity;
    float pressure;
    bool valid;
} sensor_sample_t;

extern QueueHandle_t sensor_queue;

esp_err_t sensor_task_init(void);
esp_err_t sensor_task_read_average(sensor_sample_t *out_sample);
esp_err_t sensor_task_get_latest(sensor_sample_t *out_sample);
void sensor_task(void *pvParameters);