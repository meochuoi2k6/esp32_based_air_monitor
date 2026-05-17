#pragma once

/**
 * @file wifi_task.h
 * @brief Khai báo các API phục vụ cấu hình, khởi tạo và duy trì kết nối WiFi.
 * 
 * @note File cung cấp các hàm khởi tạo NVS và WiFi driver, bắt đầu quá trình kết nối 
 * làm máy trạm (Station), kết nối thủ công bằng ssid/password, và kiểm tra trạng thái WiFi.
 */

#include <stdbool.h>

/**
 * @brief Khởi tạo phần cứng WiFi và tiến hành kết nối theo cấu hình có sẵn.
 * 
 * @note Hàm thực hiện khởi tạo NVS (cần thiết cho WiFi của ESP32), thiết lập Netif, 
 * vòng lặp sự kiện, đăng ký event handler và bắt đầu gọi `esp_wifi_connect`.
 * @return true nếu gọi lệnh connect thành công (chưa chắc chắn có IP), false nếu có lỗi.
 */
bool wifi_init();

/**
 * @brief Lấy trạng thái hiện tại của kết nối WiFi.
 * 
 * @note Kiểm tra cờ isWifi. Cờ này được cập nhật trong wifi_event_handler khi có 
 * sự kiện IP_EVENT_STA_GOT_IP hoặc WIFI_EVENT_STA_DISCONNECTED.
 * @return true nếu đã kết nối và có IP hợp lệ, false nếu bị mất mạng hoặc đang kết nối.
 */
bool wifi_is_connected(void);

/**
 * @brief Kết nối tới mạng WiFi cụ thể bằng SSID và Password.
 * 
 * @param ssid Tên mạng WiFi.
 * @param pass Mật khẩu WiFi.
 * 
 * @note Hàm sẽ set cấu hình mới cho giao diện STA, ghi đè lên cấu hình cũ, 
 * và gọi lệnh connect lại để bắt đầu quá trình xác thực.
 */
void wifi_connect(const char *ssid, const char *pass);

/**
 * @brief Task để khởi tạo dịch vụ WiFi.
 * 
 * @param pvParameters Tham số truyền cho task (không dùng).
 * 
 * @note Task này chỉ gọi `wifi_init()` rồi sau đó tự xóa bản thân (vTaskDelete), 
 * giúp đưa việc khởi tạo WiFi chạy ẩn mà không block main task.
 */
void wifi_task(void *pvParameters);
