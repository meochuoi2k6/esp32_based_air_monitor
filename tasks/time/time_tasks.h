#pragma once

#include <stdbool.h>
#include "esp_err.h"

bool time_is_synced(void);
esp_err_t init_time(void);
bool wait_for_time_sync(int max_wait_ms);
void get_time_str(char *buffer, int max_len);

void time_task(void *pvParameters);
