#ifndef PROFILER_TASK_H
#define PROFILER_TASK_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PROFILER_MAX_TASKS 8

typedef struct {
    TaskHandle_t handle;
    const char *name;
} profiler_entry_t;

typedef struct {
    profiler_entry_t entries[PROFILER_MAX_TASKS];
    int count;
} profiler_handles_t;

void profiler_register(profiler_handles_t *p, TaskHandle_t handle, const char *name);

void profiler_task(void *pvParameters);

#ifdef __cplusplus
}
#endif

#endif
