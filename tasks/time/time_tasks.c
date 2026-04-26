#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "esp_sntp.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "time_tasks.h"
#include "wifi_task.h"

static char time_str[20];

static bool s_time_synced = false;
static bool isWifi = 0;

bool time_is_synced(void)
{
    return s_time_synced;
}

esp_err_t init_time(void)
{
    setenv("TZ", "ICT-7", 1);
    tzset();
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "pool.ntp.org");
    sntp_set_sync_mode(SNTP_SYNC_MODE_IMMED);
    sntp_init();

    return ESP_OK;
}

bool wait_for_time_sync(int max_wait_ms)
{
    time_t now = 0;
    struct tm timeinfo = {0};
    int waited_ms = 0;

    while (timeinfo.tm_year < (2020 - 1900) && waited_ms < max_wait_ms) {
        printf("Waiting for time sync...\n");
        vTaskDelay(pdMS_TO_TICKS(2000));
        waited_ms += 2000;
        time(&now);
        localtime_r(&now, &timeinfo);
    }

    return timeinfo.tm_year >= (2020 - 1900);
}

void get_time_str(char *buffer, int max_len)
{
    time_t now;
    struct tm timeinfo;

    time(&now);
    localtime_r(&now, &timeinfo);

    strftime(buffer, max_len, "%Y-%m-%d %H:%M:%S", &timeinfo);
}

void time_task(void *pvParameters)
{
    (void)pvParameters;

    while (1) {
        if (wifi_is_connected()&& !s_time_synced) {
            init_time();
            if (!wait_for_time_sync(30000)) {
                printf("Time sync timeout, continuing without SNTP time\n");
                s_time_synced = false;
            } else {
                get_time_str(time_str, sizeof(time_str));
                s_time_synced = true;
            }
            vTaskDelete(NULL);
            return;
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
