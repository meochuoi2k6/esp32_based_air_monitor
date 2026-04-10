#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "sensor_task.h"

typedef struct {
    QueueHandle_t sensor_queue;
} display_params_t;

void oled_init();
void draw_char(int x, int y, char c);
void draw_string(int x, int y, const char *str);
void draw_bitmap(int x, int y, const uint8_t *bitmap);
esp_err_t oled_show(sensor_sample_t sample, bool isWifi, bool isSd);
void display_task(void *pvParameters);
