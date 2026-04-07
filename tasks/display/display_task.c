#include "display_task.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_log.h"

#include "ssd1306.h"
#include "i2cdev.h"
#include "font8x8_basic.h"
#include "sensor_task.h"


#define SDA_GPIO 21
#define SCL_GPIO 22
#define I2C_PORT I2C_NUM_0

static ssd1306_t oled;
static const char *TAG = "display_task";

void oled_init()
{
    memset(&oled, 0, sizeof(oled));

    ESP_ERROR_CHECK(
        ssd1306_init_desc(&oled, I2C_NUM_0, SSD1306_I2C_ADDR0, 21, 22)
    );

    oled.width = 128;
    oled.height = 64;

    ESP_ERROR_CHECK(ssd1306_init(&oled));
}

void draw_char(int x, int y, char c)
{
    if (c > 127) return;

    for (int col = 0; col < 8; col++) {
        uint8_t line = font8x8_basic_tr[(int)c][col];

        for (int row = 0; row < 8; row++) {
            if (line & (1 << row)) {
                ssd1306_set_pixel(&oled, x + col, y + row, OLED_COLOR_WHITE);
            }
        }
    }
}

void draw_string(int x, int y, const char *str)
{
    while (*str) {
        draw_char(x, y, *str++);
        x += 8;
        if (x > 120) break;
    }
}

void draw_bitmap(int x, int y, const uint8_t *bitmap)
{
    for (int i = 0; i < 16; i++) {
        for (int j = 0; j < 16; j++) {
            int byte = bitmap[i * 2 + j / 8];
            if (byte & (1 << (7 - (j % 8)))) {
                ssd1306_set_pixel(&oled, x + j, y + i, OLED_COLOR_WHITE);
            }
        }
    }
}

esp_err_t oled_show(sensor_sample_t sample, bool isWifi, bool isSd)
{
    char buf[32];

    if (ssd1306_clear(&oled) != ESP_OK) {
        return ESP_FAIL;
    }

    draw_bitmap(128 - 16, 0, isWifi ? wifi_on : wifi_off);
    draw_bitmap(128 - 35, 0, isSd ? sd_ok : sd_fail);

    //snprintf(buf, sizeof(buf), "PM2.5: %d", sample.pm25);
    draw_string(0, 16, buf);

    //snprintf(buf, sizeof(buf), "PM10 : %d", sample.pm10);
    draw_string(0, 24, buf);

    //snprintf(buf, sizeof(buf), "Temp : %.1f C", sample.temperature);
    draw_string(0, 32, buf);

    //snprintf(buf, sizeof(buf), "Hum  : %.1f %%", sample.humidity);
    draw_string(0, 40, buf);

    //snprintf(buf, sizeof(buf), "Pres : %.1f hPa", sample.pressure);
    draw_string(0, 48, buf);

    if (ssd1306_flush(&oled) != ESP_OK) {
        return ESP_FAIL;
    }

    return ESP_OK;
}

void display_task(void *pvParameters)
{
    oled_init;

    QueueHandle_t queue = (QueueHandle_t) pvParameters;
    sensor_sample_t sample;

    while (1) {
        if (xQueueReceive(queue, &sample, portMAX_DELAY)) {
            esp_err_t err = oled_show(sample, isWifi, isSd);
            if(err != ESP_OK){
                ESP_LOGE(TAG, "Display Error: %s", esp_err_to_name(err));
            } else {
                ESP_LOGI(TAG, "Display success\nPM2.5: %d\nPM10: %d\ntemp: %.1f\nhum: %.1f\n pres: %.1f", 
                    sample.pm25, 
                    sample.pm10, 
                    sample.temperature, 
                    sample.humidity,
                    sample.pressure);
            }
        }
    }
}