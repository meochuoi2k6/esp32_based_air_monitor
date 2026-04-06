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
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "driver/sdspi_host.h"

#include <time.h>
#include "esp_sntp.h"
#include "icons.h"

#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_log.h"
#include <stdbool.h>


#define WIFI_SSID "ANH TUAN 2G"
#define WIFI_PASS "0904543663"

//////////////TIME SET/////////////////

static const char *TAG = "WIFI";
static bool isWifi = false;

void init_time()
{
    setenv("TZ", "ICT-7", 1);
    tzset();

    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "pool.ntp.org");
    sntp_init();
}

static void wifi_event_handler(void* arg,
                               esp_event_base_t event_base,
                               int32_t event_id,
                               void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGI(TAG, "Retrying...");
        isWifi = false;
        esp_wifi_connect();
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ESP_LOGI(TAG, "Got IP!");
        isWifi = true;
        init_time(); //:> bat ngo chua 
    }
    
}


bool wifi_init() {
    esp_err_t ret = nvs_flash_init();
    if (ret != ESP_OK) {
        nvs_flash_erase();
        nvs_flash_init();
    }

    esp_netif_init();

    esp_event_loop_create_default();

    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
        },
    };

    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);


    
    esp_event_handler_instance_register(WIFI_EVENT,
                                    ESP_EVENT_ANY_ID,
                                    &wifi_event_handler,
                                    NULL,
                                    NULL);

    esp_event_handler_instance_register(IP_EVENT,
                                        IP_EVENT_STA_GOT_IP,
                                        &wifi_event_handler,
                                        NULL,
                                        NULL);
    
    esp_wifi_start();

    esp_err_t err = esp_wifi_connect();{
        if(err==ESP_OK) return 1;
        return 0;
    }
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

void oled_init()
{
    memset(&oled, 0, sizeof(oled));

    ESP_ERROR_CHECK(
        ssd1306_init_desc(&oled, I2C_NUM_0, SSD1306_I2C_ADDR0, 21, 22)
    );

    oled.width = 128;
    oled.height = 64;


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

void oled_show(int pm25, int pm10, bme680_values_float_t v, bool isWifi, bool isSd)
{
    char buf[32];

    ssd1306_clear(&oled);
    draw_bitmap(128 - 16, 0, isWifi ? wifi_on : wifi_off);
    draw_bitmap(128 - 35, 0, isSd ? sd_ok : sd_fail);

    sprintf(buf, "PM2.5: %d", pm25);
    draw_string(0, 16, buf);
    sprintf(buf, "PM10 : %d", pm10);
    draw_string(0, 24, buf);

    sprintf(buf, "Temp : %.1f C", v.temperature);
    draw_string(0, 32, buf);

    sprintf(buf, "Hum  : %.1f %%", v.humidity);
    draw_string(0, 40, buf);

    sprintf(buf, "Pres : %.1f hPa", v.pressure);
    draw_string(0, 48, buf);

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

///////////////////SD INIT////////////////////////

bool init_sd_card() {
    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    
    spi_bus_config_t bus_cfg = {
        .mosi_io_num = 23,
        .miso_io_num = 19,
        .sclk_io_num = 18,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
    };

    spi_bus_initialize(host.slot, &bus_cfg, SDSPI_DEFAULT_DMA);

    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = 5;
    slot_config.host_id = host.slot;

    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 5,
    };

    sdmmc_card_t *card;
    
    esp_err_t ret = esp_vfs_fat_sdspi_mount("/sdcard", &host, &slot_config, &mount_config, &card);
    if (ret != ESP_OK) {
        printf("SD mount failed: %d\n", ret);
        return 0;
    }
    else return 1;
}
 
void w_sd(char *time_s, int pm25, int pm10, float temp, float hum, float pres)
{
    FILE *f = fopen("/sdcard/log.csv", "a");
    if (f == NULL) {
        printf("Open file failed\n");
        return;
    }

    fprintf(f, "%s,%d,%d,%.2f,%.2f,%.2f\n",
            time_s,
            pm25,
            pm10,
            temp,
            hum,
            pres);

    fflush(f);
    fclose(f);
}

void r_sd(){
    FILE *f = fopen("/sdcard/log.csv", "r");
    if (f == NULL) {
        printf("Open file failed\n");
        return;
    }   

    char line[128];
    while (fgets(line, sizeof(line), f)) {
        printf("%s", line);
    }

    fclose(f);
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
    
    wifi_init();

    if (!wait_for_time_sync(20000)) {
        printf("Time sync timeout, continuing without SNTP time\n");
    }
    bool isSd = init_sd_card();

    printf("WiFi ready\n");

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
    float temp_sum = 0;
    float hum_sum = 0;
    float pres_sum = 0;
    int count = 0;

    while (count < SAMPLE_COUNT)
    {
        ESP_ERROR_CHECK(bme680_force_measurement(&bme));
        vTaskDelay(pdMS_TO_TICKS(200));
        ESP_ERROR_CHECK(bme680_get_results_float(&bme, &values));

        int len = uart_read_bytes(UART_PORT, data, 32, pdMS_TO_TICKS(200));

        if (len == 32 && data[0] == 0x42 && data[1] == 0x4D)
        {
            int pm25 = (data[12] << 8) | data[13];
            int pm10 = (data[14] << 8) | data[15];

            printf("PM2.5: %d\n", pm25);
            printf("PM10 : %d\n", pm10);
    
            pm25_sum += pm25;
            pm10_sum += pm10;
            temp_sum += values.temperature;
            hum_sum += values.humidity;
            pres_sum += values.pressure;
            count++;
        }
    }


    // ESP_ERROR_CHECK(bme680_force_measurement(&bme));
    // vTaskDelay(pdMS_TO_TICKS(200));
    // ESP_ERROR_CHECK(bme680_get_results_float(&bme, &values));

    int pm25_avg = pm25_sum / SAMPLE_COUNT;
    int pm10_avg = pm10_sum / SAMPLE_COUNT;
    float temp_avg = temp_sum / SAMPLE_COUNT;
    float hum_avg = hum_sum / SAMPLE_COUNT;
    float pres_avg = pres_sum / SAMPLE_COUNT;
    bme680_values_float_t avg_values = values;
    avg_values.temperature = temp_avg;
    avg_values.humidity = hum_avg;
    avg_values.pressure = pres_avg;

    printf("Average PM2.5: %d\n", pm25_avg);
    printf("Average PM10 : %d\n", pm10_avg);
    printf("Temp: %.2f C\n", temp_avg);
    printf("Humidity: %.2f %%\n", hum_avg);
    printf("Pressure: %.2f hPa\n", pres_avg);

    oled_show(pm25_avg, pm10_avg, avg_values, isWifi, isSd);
    
    char time_s[30];
    get_time_str(time_s, sizeof(time_s));
    
    w_sd(time_s, 
        pm25_avg, 
        pm10_avg, 
        temp_avg,
        hum_avg,
        pres_avg
    );
    
    r_sd();

    vTaskDelay(pdMS_TO_TICKS(2000));

    printf("Going to sleep...\n");

    esp_sleep_enable_timer_wakeup((uint64_t)SLEEP_TIME_SEC * 1000000ULL);
    esp_deep_sleep_start();
}
