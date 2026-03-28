#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "esp_sleep.h"

#include "ssd1306.h"
#include "bme680.h"
#include "font8x8_basic.h"
#include "i2cdev.h"

#define UART_PORT UART_NUM_2
#define TX_PIN 17
#define RX_PIN 16

#define SDA_GPIO 21
#define SCL_GPIO 22
#define I2C_PORT I2C_NUM_0

#define SLEEP_TIME_SEC 60
#define SAMPLE_COUNT 5

ssd1306_t oled;
bme680_t bme;

//////////////////////////////////////////////////
// UART
//////////////////////////////////////////////////
void uart_init()
{
    uart_config_t cfg = {
        .baud_rate = 9600,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };

    uart_driver_install(UART_PORT, 1024, 0, 0, NULL, 0);
    uart_param_config(UART_PORT, &cfg);
    uart_set_pin(UART_PORT, TX_PIN, RX_PIN,
                 UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
}

//////////////////////////////////////////////////
// OLED
//////////////////////////////////////////////////
// void oled_init()
// {
//     memset(&oled, 0, sizeof(oled));

//     ESP_ERROR_CHECK(
//         ssd1306_init_desc(&oled, I2C_NUM_0, SSD1306_I2C_ADDR0, 21, 22)
//     );

//     // SET KÍCH THƯỚC MÀN HÌNH (BẮT BUỘC)
//     oled.width = 128;
//     oled.height = 64;   // hoặc 32 nếu là bản 0.96 nhỏ

//     oled.i2c_dev.cfg.sda_pullup_en = 1;
//     oled.i2c_dev.cfg.scl_pullup_en = 1;

//     ESP_ERROR_CHECK(ssd1306_init(&oled));
// }

void oled_init()
{
    memset(&oled, 0, sizeof(oled));

    ESP_ERROR_CHECK(
        ssd1306_init_desc(&oled, I2C_NUM_0, SSD1306_I2C_ADDR0, 21, 22)
    );

    oled.width = 128;
    oled.height = 64;

    // ❌ BỎ cái này
    // oled.i2c_dev.cfg.sda_pullup_en = 1;
    // oled.i2c_dev.cfg.scl_pullup_en = 1;

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

void oled_show(int pm25, int pm10, bme680_values_float_t v)
{
    char buf[32];

    ssd1306_clear(&oled);

    sprintf(buf, "PM2.5: %d", pm25);
    draw_string(0, 8, buf);
    sprintf(buf, "PM10 : %d", pm10);
    draw_string(0, 16, buf);

    sprintf(buf, "Temp : %.1f C", v.temperature);
    draw_string(0, 24, buf);

    sprintf(buf, "Hum  : %.1f %%", v.humidity);
    draw_string(0, 32, buf);

    sprintf(buf, "Pres : %.1f hPa", v.pressure / 100.0);
    draw_string(0, 40, buf);

    ssd1306_flush(&oled);
}

//////////////////////////////////////////////////
// BME680
//////////////////////////////////////////////////
void bme_init()
{
    memset(&bme, 0, sizeof(bme));

    ESP_ERROR_CHECK(
         bme680_init_desc(&bme, BME680_I2C_ADDR_0, I2C_NUM_0, 21, 22)
     );

    ESP_ERROR_CHECK(bme680_init_sensor(&bme));
}

//////////////////////////////////////////////////
// MAIN
//////////////////////////////////////////////////
void app_main(void)
{
    
    uart_init();

    i2cdev_init();
    

//     i2c_config_t cfg = {
//         .mode = I2C_MODE_MASTER,
//         .sda_io_num = 21,
//         .scl_io_num = 22,
//         .sda_pullup_en = GPIO_PULLUP_ENABLE,
//         .scl_pullup_en = GPIO_PULLUP_ENABLE,
//         .master.clk_speed = 100000   
//     };

// i2c_param_config(I2C_NUM_0, &cfg);
// i2c_driver_install(I2C_NUM_0, cfg.mode, 0, 0, 0);

    oled_init(); 
    bme_init();    

    for (int i = 0; i < 5; i++) {
        uint8_t id;
        esp_err_t err = i2c_dev_read_reg(&bme.i2c_dev, 0xD0, &id, 1);
        printf("Try %d: err=%d, id=0x%02X\n", i, err, id);
        vTaskDelay(pdMS_TO_TICKS(200));
    }
    
    printf("Sensor warmup...\n");
    vTaskDelay(pdMS_TO_TICKS(10000));
    


    uint8_t data[32];
    bme680_values_float_t values;

    int pm25_sum = 0;
    int pm10_sum = 0;
    int count = 0;

    while (count < SAMPLE_COUNT)
    {
        int len = uart_read_bytes(UART_PORT, data, 32, pdMS_TO_TICKS(200));

        if (len == 32 && data[0] == 0x42 && data[1] == 0x4D)
        {
            int pm25 = (data[12] << 8) | data[13];
            int pm10 = (data[14] << 8) | data[15];

            printf("PM2.5: %d\n", pm25);
            printf("PM10 : %d\n", pm10);
    
            pm25_sum += pm25;
            pm10_sum += pm10;
            count++;
        }
    }

    // 🔥 đọc BME680 đúng chuẩn
    ESP_ERROR_CHECK(bme680_force_measurement(&bme));
    vTaskDelay(pdMS_TO_TICKS(200));
    ESP_ERROR_CHECK(bme680_get_results_float(&bme, &values));

    int pm25_avg = pm25_sum / SAMPLE_COUNT;
    int pm10_avg = pm10_sum / SAMPLE_COUNT;

    printf("Average PM2.5: %d\n", pm25_avg);
    printf("Average PM10 : %d\n", pm10_avg);
    printf("Temp: %.2f C\n", values.temperature);
    printf("Humidity: %.2f %%\n", values.humidity);
    printf("Pressure: %.2f hPa\n", values.pressure);


    oled_show(pm25_avg, pm10_avg, values);

    vTaskDelay(pdMS_TO_TICKS(2000));

    printf("Going to sleep...\n");

    esp_sleep_enable_timer_wakeup((uint64_t)SLEEP_TIME_SEC * 1000000ULL);
    esp_deep_sleep_start();
}