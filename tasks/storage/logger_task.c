#include <stdio.h>
#include <string.h>

#include "driver/sdspi_host.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "logger_task.h"
#include "sensor_task.h"

static const char *TAG = "logger_task";

#define SD_MOUNT_POINT "/sdcard"
#define SD_LOG_FILE SD_MOUNT_POINT "/log.csv"
#define SD_RETRY_INTERVAL_MS 5000
#define SD_HOST_SLOT SDSPI_DEFAULT_HOST

static bool sd_ready = false;
static bool sd_bus_initialized = false;
static sdmmc_card_t *sd_card = NULL;

/**
 * @brief Kiểm tra xem thẻ SD có sẵn sàng không.
 * 
 * @note Hàm này chỉ trả về giá trị của biến cờ sd_ready để báo trạng thái hoạt động của thẻ SD.
 * @return true nếu sẵn sàng, false nếu chưa.
 */
bool logger_is_sd_ready(void)
{
    return sd_ready;
}

/**
 * @brief Ngắt kết nối và giải phóng tài nguyên của thẻ SD.
 * 
 * @note Cơ chế là thực hiện unmount hệ thống file FAT của thẻ SD trước, sau đó giải phóng 
 * phần cứng bus SPI nếu đang được khởi tạo, và cuối cùng đặt các cờ trạng thái về false.
 */
static void logger_unmount_sd_card(void)
{
    if (sd_card != NULL) {
        esp_vfs_fat_sdcard_unmount(SD_MOUNT_POINT, sd_card);
        sd_card = NULL;
    }

    // Do not free SPI bus to prevent conflict with other devices
    // and to avoid driver corruption from repeated init/free cycles.

    sd_ready = false;
}

/**
 * @brief Khởi tạo giao tiếp SPI và cấu hình thẻ SD.
 * 
 * @note Hàm sẽ kiểm tra cờ sd_ready để tránh khởi tạo nhiều lần. Cơ chế gồm: 
 * 1. Khởi tạo bus SPI dùng các GPIO định sẵn (MOSI:23, MISO:19, CLK:18). 
 * 2. Cấu hình thiết bị SPI cho thẻ SD dùng GPIO CS là 5. 
 * 3. Dùng thư viện esp_vfs_fat_sdspi_mount để mount thẻ nhớ.
 * @return true nếu thành công, ngược lại trả về false.
 */
bool init_sd_card(void)
{
    if (sd_ready) {
        return true;
    }

    sdmmc_host_t host = SDSPI_HOST_DEFAULT();

    spi_bus_config_t bus_cfg = {
        .mosi_io_num = 23,
        .miso_io_num = 19,
        .sclk_io_num = 18,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
    };

    if (!sd_bus_initialized) {
        esp_err_t ret = spi_bus_initialize(host.slot, &bus_cfg, SDSPI_DEFAULT_DMA);
        if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
            ESP_LOGE(TAG, "SPI bus initialize failed: %s", esp_err_to_name(ret));
            return false;
        }
        sd_bus_initialized = true;
    }

    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = 5;
    slot_config.host_id = host.slot;

    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 5,
    };

    esp_err_t ret = esp_vfs_fat_sdspi_mount(SD_MOUNT_POINT, &host, &slot_config, &mount_config, &sd_card);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "SD mount failed: %s", esp_err_to_name(ret));
        sd_card = NULL;
        sd_ready = false;
        return false;
    }

    sd_ready = true;
    ESP_LOGI(TAG, "SD card mounted");
    return true;
}

/**
 * @brief Ghi dữ liệu trực tiếp vào file.
 * 
 * @param time_s Chuỗi thời gian.
 * @param pm25 Giá trị PM2.5.
 * @param pm10 Giá trị PM10.
 * @param temp Giá trị nhiệt độ.
 * @param hum Giá trị độ ẩm.
 * @param pres Giá trị áp suất.
 * 
 * @note Cơ chế là mở file csv (chế độ "a" - append) trên thẻ SD, dùng fprintf để nối chuỗi 
 * các thông số cách nhau bởi dấu phẩy. Sau đó gọi fflush và fclose để đảm bảo dữ liệu 
 * được lưu xuống hoàn toàn.
 * @return ESP_OK nếu quá trình ghi file ổn định, ngược lại trả về ESP_FAIL.
 */
static esp_err_t w_sd(const char *time_s, int pm25, int pm10, float temp, float hum, float pres)
{
    FILE *f = fopen(SD_LOG_FILE, "a");
    if (f == NULL) {
        return ESP_FAIL;
    }

    int written = fprintf(f, "%s,%d,%d,%.2f,%.2f,%.2f\n",
                          time_s,
                          pm25,
                          pm10,
                          temp,
                          hum,
                          pres);

    int flush_result = fflush(f);
    int close_result = fclose(f);

    if (written < 0 || flush_result != 0 || close_result != 0) {
        return ESP_FAIL;
    }

    return ESP_OK;
}

/**
 * @brief Bọc gói quá trình ghi thông số cảm biến.
 * 
 * @param sample Cấu trúc chứa thông tin tổng hợp của môi trường.
 * 
 * @note Kiểm tra xem mẫu thử có hợp lệ không và thẻ SD có kết nối chưa. Xử lý chuỗi thời gian 
 * chưa được đồng bộ nếu cần và gọi w_sd. Nếu gọi thất bại, tự động kích hoạt hàm 
 * logger_unmount_sd_card để giải phóng tài nguyên lỗi.
 * @return Mã lỗi theo chuẩn ESP_ERR_*.
 */
esp_err_t logger_write_sample(const sensor_sample_t *sample)
{
    if (sample == NULL || !sample->valid) {
        return ESP_ERR_INVALID_ARG;
    }

    if (!sd_ready) {
        return ESP_ERR_INVALID_STATE;
    }

    const char *timestamp = sample->timestamp[0] != '\0' ? sample->timestamp : "UNSYNC";

    esp_err_t err = w_sd(timestamp,
                         sample->pm25,
                         sample->pm10,
                         sample->temperature,
                         sample->humidity,
                         sample->pressure);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "SD write failed, marking card as disconnected");
        logger_unmount_sd_card();
        return err;
    }

    return err;
}

/**
 * @brief Task xử lý chính vòng lặp ghi thẻ nhớ.
 * 
 * @param pvParameters Tham số truyền từ lúc khởi tạo Task.
 * 
 * @note Cơ chế là chạy một vòng lặp vô hạn, lấy dữ liệu cảm biến ra khỏi queue 
 * (params->sensor_queue). Khi lấy thành công, nếu thẻ SD đang lỗi kết nối, task sẽ cố 
 * gắng thử gọi init_sd_card lại theo chu kỳ định sẵn (SD_RETRY_INTERVAL_MS). Sau đó lưu dữ liệu đó.
 */
void logger_task(void *pvParameters)
{
    logger_task_params_t *params = (logger_task_params_t *)pvParameters;
    sensor_sample_t sample;
    TickType_t last_mount_attempt = 0;

    init_sd_card();

    if (params == NULL || params->sensor_queue == NULL) {
        ESP_LOGE(TAG, "logger_task missing sensor queue");
        vTaskDelete(NULL);
        return;
    }

    while (1) {
        if (xQueueReceive(params->sensor_queue, &sample, portMAX_DELAY) == pdTRUE) {
            if (!sd_ready) {
                TickType_t now = xTaskGetTickCount();
                if ((now - last_mount_attempt) >= pdMS_TO_TICKS(SD_RETRY_INTERVAL_MS)) {
                    last_mount_attempt = now;
                    init_sd_card();
                }

                if (!sd_ready) {
                    continue;
                }
            }

            esp_err_t err = logger_write_sample(&sample);
            if (err != ESP_OK) {
                ESP_LOGW(TAG, "logger_write_sample failed: %s", esp_err_to_name(err));
            } else {
                ESP_LOGI(TAG, "Sample logged to SD at %s", sample.timestamp);
            }
        }
    }
}
