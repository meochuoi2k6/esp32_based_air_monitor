/**
 * @file time_tasks.c
 * @brief Triển khai các hàm đồng bộ và lấy thời gian thực.
 * 
 * @note File này thực hiện việc cấu hình SNTP (Simple Network Time Protocol), thiết lập 
 * múi giờ và duy trì một task để chờ WiFi kết nối sau đó thực hiện đồng bộ thời gian từ 
 * máy chủ NTP.
 */

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

/**
 * @brief Kiểm tra trạng thái đồng bộ thời gian.
 * 
 * @note Hàm trả về biến cục bộ s_time_synced báo hiệu thời gian đã đồng bộ hay chưa.
 * @return true nếu đã đồng bộ xong, false nếu đang chờ hoặc thất bại.
 */
bool time_is_synced(void)
{
    return s_time_synced;
}

/**
 * @brief Thiết lập cấu hình và khởi chạy SNTP.
 * 
 * @note Cấu hình môi trường TZ (múi giờ ICT-7), gọi tzset(). Sau đó thiết lập SNTP client ở 
 * chế độ POLL, lấy nguồn thời gian từ "pool.ntp.org", và init dịch vụ sntp.
 * @return Trả về ESP_OK.
 */
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

/**
 * @brief Đợi lấy thời gian đúng từ máy chủ NTP.
 * 
 * @param max_wait_ms Thời gian tối đa (ms) chịu đựng việc đợi.
 * 
 * @note Kiểm tra liên tục (mỗi 2s) xem năm đã cập nhật thành một năm hợp lệ (>= 2020) 
 * hay chưa. Vòng lặp dừng lại khi thỏa mãn điều kiện thời gian đúng hoặc vượt mức timeout.
 * @return true nếu đã đồng bộ được thời gian hợp lý, false nếu quá thời gian chờ (timeout).
 */
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

/**
 * @brief Đọc và chuyển thời gian thực thành dạng chuỗi.
 * 
 * @param buffer Bộ đệm chứa chuỗi kết quả.
 * @param max_len Kích thước lớn nhất của bộ đệm.
 * 
 * @note Dùng thư viện chuẩn time để lấy `time_t`, dùng `localtime_r` đổi sang `struct tm` 
 * (thread-safe), và dùng `strftime` để in ra chuỗi dạng "YYYY-MM-DD HH:MM:SS".
 */
void get_time_str(char *buffer, int max_len)
{
    time_t now;
    struct tm timeinfo;

    time(&now);
    localtime_r(&now, &timeinfo);

    strftime(buffer, max_len, "%Y-%m-%d %H:%M:%S", &timeinfo);
}

/**
 * @brief Task xử lý đồng bộ thời gian từ mạng.
 * 
 * @param pvParameters Các thông số (không sử dụng).
 * 
 * @note Task liên tục kiểm tra nếu WiFi có kết nối và thời gian chưa đồng bộ. 
 * Nếu có WiFi, bắt đầu `init_time()` và chờ đến khi `wait_for_time_sync()` chạy 
 * xong. Sau đó lấy chuỗi thời gian ban đầu, bật cờ `s_time_synced` lên, rồi tự huỷ.
 */
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
