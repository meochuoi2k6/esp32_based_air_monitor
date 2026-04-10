#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "display_task.h"
#include "i2cdev.h"
#include "logger_task.h"
#include "sensor_task.h"
#include "time_tasks.h"
#include "wifi_task.h"

#define SAMPLE_QUEUE_LENGTH 4

static const char *TAG = "air_monitor";

static QueueHandle_t display_queue;
static QueueHandle_t logger_queue;

static sensor_task_params_t sensor_params;
static display_params_t display_params;
static logger_task_params_t logger_params;

void app_main(void)
{
    ESP_ERROR_CHECK(i2cdev_init());

    display_queue = xQueueCreate(SAMPLE_QUEUE_LENGTH, sizeof(sensor_sample_t));
    logger_queue = xQueueCreate(SAMPLE_QUEUE_LENGTH, sizeof(sensor_sample_t));

    if (display_queue == NULL || logger_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create sample queues");
        return;
    }

    sensor_params.display_queue = display_queue;
    sensor_params.logger_queue = logger_queue;

    display_params.sensor_queue = display_queue;
    logger_params.sensor_queue = logger_queue;

    xTaskCreate(wifi_task, "wifi_task", 4096, NULL, 5, NULL);
    xTaskCreate(time_task, "time_task", 4096, NULL, 4, NULL);
    xTaskCreate(sensor_task, "sensor_task", 4096, &sensor_params, 5, NULL);
    xTaskCreate(display_task, "display_task", 4096, &display_params, 4, NULL);
    xTaskCreate(logger_task, "logger_task", 4096, &logger_params, 4, NULL);
}
