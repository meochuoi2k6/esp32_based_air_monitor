/**
 * @file wifi_scan_task.c
 * @brief Triển khai chức năng quét tìm các mạng WiFi lân cận.
 * 
 * @note File này chịu trách nhiệm sử dụng API esp_wifi_scan để tiến hành thu thập 
 * AP records (max 10), lọc các AP dải 2.4GHz, đóng gói thông tin SSID và RSSI vào 
 * một bộ đệm lưu trữ dưới định dạng JSON. Cung cấp API truy xuất kết quả.
 */

#include "wifi_scan_task.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_timer.h"

#include <stdio.h>
#include <string.h>

#define TAG "WIFI_SCAN"

#define MAX_AP 10
#define JSON_BUF_SIZE 512
#define SCAN_INTERVAL_US (5 * 1000000)

static bool scan_requested = false;
static bool scanning = false;

static char cached_json[JSON_BUF_SIZE];
static size_t cached_len = 0;

static int64_t last_scan_time = 0;

// ================= SCAN =================
/**
 * @brief Thực thi việc quét WiFi thực tế.
 * 
 * @note Cấu hình quét WiFi mặc định, gọi `esp_wifi_scan_start`, lấy danh sách qua 
 * `esp_wifi_scan_get_ap_records`. Lặp qua kết quả để chỉ giữ các mạng chạy 2.4GHz 
 * (primary <= 14) và gộp vào biến cached_json bằng hàm snprintf với định dạng mảng JSON.
 */
static void do_wifi_scan(void)
{
    uint16_t number = MAX_AP;
    wifi_ap_record_t ap_records[MAX_AP];

    wifi_scan_config_t config = {
        .ssid = NULL,
        .channel = 0,
        .show_hidden = false
    };

    ESP_LOGI(TAG, "Scanning...");

    esp_wifi_scan_start(&config, true);
    esp_wifi_scan_get_ap_records(&number, ap_records);

    int len = 0;
    len += snprintf(cached_json + len, JSON_BUF_SIZE - len, "[");

    int count = 0;

    for (int i = 0; i < number; i++) {

        if (ap_records[i].primary > 14) continue; // chỉ 2.4GHz

        if (count > 0) {
            len += snprintf(cached_json + len, JSON_BUF_SIZE - len, ",");
        }

        len += snprintf(cached_json + len, JSON_BUF_SIZE - len,
                        "{\"ssid\":\"%s\",\"rssi\":%d}",
                        (char*)ap_records[i].ssid,
                        ap_records[i].rssi);

        count++;
    }

    len += snprintf(cached_json + len, JSON_BUF_SIZE - len, "]");

    cached_len = len;

    ESP_LOGI(TAG, "Scan done (%d AP)", count);
}

// ================= TASK =================
/**
 * @brief Task xử lý ngầm việc quét WiFi.
 * 
 * @param pvParameters Tham số đầu vào task (không dùng).
 * 
 * @note Vòng lặp liên tục mỗi 100ms kiểm tra cờ scan_requested. Nếu được yêu cầu, 
 * có thực hiện chống rung (debounce) 5 giây (SCAN_INTERVAL_US) để tránh gọi quét liên tục. 
 * Tiến hành gọi `do_wifi_scan()` và cập nhật mốc thời gian `last_scan_time`.
 */
void wifi_scan_task(void *pvParameters)
{
    while (1)
    {
        if (scan_requested)
        {
            int64_t now = esp_timer_get_time();

            if (now - last_scan_time < SCAN_INTERVAL_US) {
                ESP_LOGW(TAG, "Skip (debounce)");
                scan_requested = false;
                continue;
            }

            if (scanning) {
                ESP_LOGW(TAG, "Already scanning");
                scan_requested = false;
                continue;
            }

            scanning = true;

            do_wifi_scan();

            last_scan_time = now;
            scanning = false;
            scan_requested = false;
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

// ================= API =================
/**
 * @brief Kích hoạt cờ yêu cầu quét WiFi từ các module khác.
 * 
 * @note Thiết lập `scan_requested = true`.
 */
void wifi_scan_request(void)
{
    scan_requested = true;
}

/**
 * @brief Lấy con trỏ tới chuỗi kết quả quét dưới dạng JSON.
 * 
 * @note Chuỗi này chứa thông tin các mạng quét được sau khi `do_wifi_scan()` thực thi.
 * @return Con trỏ chuỗi mảng JSON chứa SSID và RSSI.
 */
const char* wifi_scan_get_json(void)
{
    return cached_json;
}

/**
 * @brief Lấy độ dài chuỗi JSON đã lưu.
 * 
 * @note Trả về giá trị của `cached_len`.
 * @return Kích thước tính bằng byte của chuỗi JSON.
 */
size_t wifi_scan_get_len(void)
{
    return cached_len;
}
