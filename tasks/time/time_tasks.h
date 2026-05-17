#pragma once

/**
 * @file time_tasks.h
 * @brief Khai báo các hàm đồng bộ và lấy thời gian thực.
 * 
 * @note File này cung cấp API để cấu hình SNTP, đồng bộ thời gian từ internet, kiểm tra trạng 
 * thái đồng bộ, và lấy thời gian hiện tại dưới dạng chuỗi format.
 */

#include <stdbool.h>
#include "esp_err.h"

/**
 * @brief Kiểm tra xem thời gian đã được đồng bộ với máy chủ SNTP chưa.
 * 
 * @note Trả về giá trị của biến cờ s_time_synced.
 * @return true nếu đã đồng bộ thành công, false nếu chưa.
 */
bool time_is_synced(void);

/**
 * @brief Khởi tạo giao thức SNTP và yêu cầu đồng bộ.
 * 
 * @note Thiết lập múi giờ (TZ) thành "ICT-7" (chuẩn giờ Đông Dương) và khởi chạy 
 * dịch vụ SNTP theo chế độ POLL với server "pool.ntp.org".
 * @return ESP_OK nếu cấu hình bắt đầu đồng bộ thành công.
 */
esp_err_t init_time(void);

/**
 * @brief Chờ đợi hệ thống đồng bộ được thời gian hợp lệ.
 * 
 * @param max_wait_ms Thời gian tối đa (ms) để đợi đồng bộ.
 * 
 * @note Hàm này sử dụng vòng lặp kiểm tra năm hiện tại từ hàm localtime. Nếu năm 
 * nhận được nhỏ hơn 2020, sẽ tiếp tục chờ cho đến khi vượt quá max_wait_ms.
 * @return true nếu đồng bộ thành công trước khi hết thời gian, false nếu timeout.
 */
bool wait_for_time_sync(int max_wait_ms);

/**
 * @brief Lấy thời gian hiện tại dưới dạng chuỗi.
 * 
 * @param buffer Con trỏ mảng ký tự chứa kết quả.
 * @param max_len Kích thước mảng ký tự.
 * 
 * @note Sử dụng hàm strftime để định dạng thời gian thành "YYYY-MM-DD HH:MM:SS".
 */
void get_time_str(char *buffer, int max_len);

/**
 * @brief Task xử lý đồng bộ thời gian lúc khởi động.
 * 
 * @param pvParameters Tham số truyền cho task (không dùng).
 * 
 * @note Task chờ thiết bị kết nối WiFi. Ngay khi có WiFi, task khởi chạy SNTP, 
 * chờ đồng bộ thành công (hoặc timeout), cập nhật cờ s_time_synced và sau đó tự 
 * hủy chính task để giải phóng RAM.
 */
void time_task(void *pvParameters);
