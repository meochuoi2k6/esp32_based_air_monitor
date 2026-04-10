#include "sensor_task.h"

#include <string.h>

#include "driver/uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_log.h"

#include "bme680.h"

////////////////// UART TASK ///////////////////////

static const char *TAG = "sensor_task";

#define UART_PORT UART_NUM_2
#define TX_PIN 17
#define RX_PIN 16

#define SAMPLE_COUNT 5
#define SENSOR_TASK_PERIOD_MS 1000

static bme680_t bme;
static sensor_sample_t latest_sample;
static bool sensor_ready = false;

static void uart_init(void)
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

///////////////////BME680 TASK////////////////////////////

#define SDA_GPIO 21
#define SCL_GPIO 22
#define I2C_PORT I2C_NUM_0

static void bme_init(void)
{
    memset(&bme, 0, sizeof(bme));

    ESP_ERROR_CHECK(
         bme680_init_desc(&bme, BME680_I2C_ADDR_0, I2C_PORT, SDA_GPIO, SCL_GPIO)
    );

    ESP_ERROR_CHECK(bme680_init_sensor(&bme));
    ESP_ERROR_CHECK(bme680_use_heater_profile(&bme, BME680_HEATER_NOT_USED));
}

esp_err_t sensor_task_init(void)
{
    uart_init();
    bme_init();

    memset(&latest_sample, 0, sizeof(latest_sample));
    sensor_ready = true;

    return ESP_OK;
}

esp_err_t sensor_task_read_average(sensor_sample_t *out_sample)
{
    if (out_sample == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (!sensor_ready) {
        return ESP_ERR_INVALID_STATE;
    }

    uint8_t data[32];
    bme680_values_float_t values;
    int pm25_sum = 0;
    int pm10_sum = 0;
    float temp_sum = 0;
    float hum_sum = 0;
    float pres_sum = 0;
    int count = 0;

    memset(out_sample, 0, sizeof(*out_sample));

    while (count < SAMPLE_COUNT) {
        ESP_ERROR_CHECK(bme680_force_measurement(&bme));
        vTaskDelay(pdMS_TO_TICKS(200));
        ESP_ERROR_CHECK(bme680_get_results_float(&bme, &values));

        int len = uart_read_bytes(UART_PORT, data, sizeof(data), pdMS_TO_TICKS(200));

        if (len != sizeof(data) || data[0] != 0x42 || data[1] != 0x4D) {
            ESP_LOGW(TAG, "Invalid PMS7001 frame, retrying");
            continue;
        }

        pm25_sum += (data[12] << 8) | data[13];
        pm10_sum += (data[14] << 8) | data[15];
        temp_sum += values.temperature;
        hum_sum += values.humidity;
        pres_sum += values.pressure;
        count++;
    }

    out_sample->pm25 = pm25_sum / SAMPLE_COUNT;
    out_sample->pm10 = pm10_sum / SAMPLE_COUNT;
    out_sample->temperature = temp_sum / SAMPLE_COUNT;
    out_sample->humidity = hum_sum / SAMPLE_COUNT;
    out_sample->pressure = pres_sum / SAMPLE_COUNT;
    out_sample->valid = true;

    latest_sample = *out_sample;

    return ESP_OK;
}

esp_err_t sensor_task_get_latest(sensor_sample_t *out_sample)
{
    if (out_sample == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (!latest_sample.valid) {
        return ESP_ERR_INVALID_STATE;
    }

    *out_sample = latest_sample;
    return ESP_OK;
}

void sensor_task(void *pvParameters)
{
    sensor_sample_t sample;
    sensor_task_params_t *params = (sensor_task_params_t *)pvParameters;

    if (sensor_task_init() != ESP_OK) {
        ESP_LOGE(TAG, "Sensor task init failed");
        vTaskDelete(NULL);
        return;
    }

    while (1) {
        esp_err_t err = sensor_task_read_average(&sample);
        if (err == ESP_OK) {
            ESP_LOGI(TAG,
                     "PM2.5=%d PM10=%d Temp=%.2f Hum=%.2f Pres=%.2f",
                     sample.pm25,
                     sample.pm10,
                     sample.temperature,
                     sample.humidity,
                     sample.pressure);

            if (params != NULL && params->display_queue != NULL) {
                xQueueSend(params->display_queue, &sample, portMAX_DELAY);
            }

            if (params != NULL && params->logger_queue != NULL) {
                xQueueSend(params->logger_queue, &sample, portMAX_DELAY);
            }

        } else {
            ESP_LOGE(TAG, "sensor_task_read_average failed: %s", esp_err_to_name(err));
        }

        vTaskDelay(pdMS_TO_TICKS(SENSOR_TASK_PERIOD_MS));
    }
}
