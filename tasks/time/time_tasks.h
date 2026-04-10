#pragma once

#include <stdbool.h>

void init_time();
bool wait_for_time_sync(int max_wait_ms);
void get_time_str(char *buffer, int max_len);

void time_task(void *pvParameters);
