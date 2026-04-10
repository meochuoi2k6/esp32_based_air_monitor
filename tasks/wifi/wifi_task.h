#pragma once

#include <stdbool.h>

bool wifi_init();
bool wifi_is_connected(void);
void wifi_task(void *pvParameters);
