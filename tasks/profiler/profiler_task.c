#include "profiler_task.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>

#define PROFILER_PERIOD_MS 30000
#define MIN_FREE_STACK_THRESHOLD 512

static const char *TAG = "profiler";

void profiler_register(profiler_handles_t *p, TaskHandle_t handle, const char *name)
{
    if (p == NULL || handle == NULL || name == NULL) {
        return;
    }

    if (p->count >= PROFILER_MAX_TASKS) {
        ESP_LOGW(TAG, "profiler_handles full, cannot add %s", name);
        return;
    }

    p->entries[p->count].handle = handle;
    p->entries[p->count].name = name;
    p->count++;
}

void profiler_task(void *pvParameters)
{
    profiler_handles_t *handles = (profiler_handles_t *)pvParameters;

    if (handles == NULL) {
        ESP_LOGE(TAG, "NULL handles, deleting task");
        vTaskDelete(NULL);
        return;
    }

    ESP_LOGI(TAG, "Profiler started, %d tasks registered", handles->count);

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(PROFILER_PERIOD_MS));

        UBaseType_t free_heap = xPortGetFreeHeapSize();
        UBaseType_t min_free_heap = xPortGetMinimumEverFreeHeapSize();

        ESP_LOGI(TAG, "===== STACK USAGE REPORT (heap free: %u / min ever: %u) =====",
                 free_heap, min_free_heap);

        for (int i = 0; i < handles->count; i++) {
            if (handles->entries[i].handle == NULL) {
                continue;
            }

            UBaseType_t remaining = uxTaskGetStackHighWaterMark(handles->entries[i].handle);
            UBaseType_t total = 4096;
            UBaseType_t used = total - remaining;

            const char *warning = (remaining < MIN_FREE_STACK_THRESHOLD) ? " *** LOW ***" : "";

            ESP_LOGI(TAG, "  %-20s | used: %4u / %4u (%.0f%%) | free: %4u%s",
                     handles->entries[i].name,
                     used, total,
                     (float)used / total * 100.0f,
                     remaining,
                     warning);
        }
    }
}
