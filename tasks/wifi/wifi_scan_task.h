#pragma once

/**
 * @file wifi_scan_task.h
 * @brief Định nghĩa các API phục vụ quét và tìm các mạng WiFi xung quanh.
 * 
 * @note File khai báo một số hàm cho phép các module khác (như BLE) gửi yêu cầu quét 
 * WiFi, đồng thời cung cấp hàm lấy kết quả quét về dưới dạng chuỗi JSON chuẩn.
 */

#include <stddef.h>

/**
 * @brief Task ngầm liên tục kiểm tra cờ yêu cầu và quét WiFi.
 * 
 * @param pvParameters Tham số truyền cho task (không sử dụng).
 * 
 * @note Task sẽ bị chặn (block) phần lớn thời gian bằng delay. Khi cờ scan_requested 
 * bật lên, task tiến hành gọi hàm quét nội bộ. Sau khi lấy kết quả và build file JSON 
 * xong, cờ được tắt đi.
 */
void wifi_scan_task(void *pvParameters);

/**
 * @brief Gửi yêu cầu bắt đầu quét WiFi.
 * 
 * @note Hàm đơn giản chỉ đánh cờ `scan_requested = true`. Sau đó, wifi_scan_task 
 * trong chu kỳ kiểm tra tiếp theo sẽ đọc cờ này và thực thi quét.
 */
void wifi_scan_request(void);

/**
 * @brief Lấy chuỗi JSON kết quả mạng WiFi đã quét.
 * 
 * @note Trả về con trỏ tới bộ đệm tĩnh cục bộ đang chứa chuỗi mảng JSON dạng 
 * `[{"ssid":"A","rssi":-50},...]` để bên yêu cầu có thể đọc ra dễ dàng.
 * @return Con trỏ chuỗi chứa JSON quét mạng.
 */
const char* wifi_scan_get_json(void);

/**
 * @brief Lấy độ dài chuỗi JSON kết quả.
 * 
 * @note Trả về kích thước chuỗi đã build của mảng JSON (không bao gồm ký tự kết thúc '\0').
 * @return Kích thước mảng JSON dạng số nguyên size_t.
 */
size_t wifi_scan_get_len(void);
