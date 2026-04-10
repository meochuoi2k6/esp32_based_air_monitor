#pragma once

#include <stdbool.h>

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "sensor_task.h"

typedef struct {
    QueueHandle_t sensor_queue;
} logger_task_params_t;

bool logger_is_sd_ready(void);
bool init_sd_card(void);
esp_err_t logger_write_sample(const sensor_sample_t *sample);
void logger_task(void *pvParameters);
