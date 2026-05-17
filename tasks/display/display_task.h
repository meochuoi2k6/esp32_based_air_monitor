#pragma once

/**
 * @file display_task.h
 * @brief Định nghĩa các hàm và cấu trúc phục vụ việc hiển thị dữ liệu lên màn hình OLED.
 * 
 * @note File này cung cấp các API để khởi tạo màn hình OLED (SSD1306), vẽ các ký tự, chuỗi, 
 * và bitmap (icon), cũng như cập nhật toàn bộ giao diện dựa trên dữ liệu cảm biến và trạng 
 * thái hệ thống (WiFi, SD Card).
 */

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "sensor_task.h"

/**
 * @brief display_params_t: Cấu trúc tham số truyền vào cho task hiển thị (display_task).
 * 
 * Các trường dữ liệu:
 * @brief- `sensor_queue`: Hàng đợi nhận dữ liệu cảm biến
 * 
 * @note Chứa hàng đợi (`sensor_queue`) để task nhận dữ liệu mẫu (sample) từ cảm biến.
 */
typedef struct display_params_t {
    QueueHandle_t sensor_queue; /*!< Hàng đợi nhận dữ liệu cảm biến */
} display_params_t;

/**
 * @brief Khởi tạo màn hình OLED.
 * 
 * @note Cấu hình I2C và gọi các API của thư viện SSD1306 để khởi chạy phần cứng màn hình, 
 * thiết lập kích thước (128x64) và xóa bộ đệm ban đầu.
 */
void oled_init();

/**
 * @brief Vẽ một ký tự lên màn hình tại tọa độ (x, y).
 * 
 * @param x Tọa độ X.
 * @param y Tọa độ Y.
 * @param c Ký tự cần vẽ.
 * 
 * @note Ký tự được ánh xạ từ mảng font cơ bản (8x8 pixel).
 */
void draw_char(int x, int y, char c);

/**
 * @brief Vẽ một chuỗi ký tự lên màn hình tại tọa độ (x, y).
 * 
 * @param x Tọa độ X.
 * @param y Tọa độ Y.
 * @param str Con trỏ trỏ tới chuỗi cần vẽ (null-terminated).
 * 
 * @note Hàm sẽ liên tục gọi `draw_char` và dịch tọa độ X, nếu vượt quá chiều rộng màn hình 
 * thì sẽ tự ngắt.
 */
void draw_string(int x, int y, const char *str);

/**
 * @brief Vẽ một ảnh bitmap (icon) lên màn hình tại tọa độ (x, y).
 * 
 * @param x Tọa độ X.
 * @param y Tọa độ Y.
 * @param bitmap Con trỏ trỏ tới mảng dữ liệu ảnh (ví dụ icon 16x16).
 * 
 * @note Hàm giải mã mảng bitmap để set từng pixel trắng/đen lên bộ đệm màn hình OLED.
 */
void draw_bitmap(int x, int y, const uint8_t *bitmap);

/**
 * @brief Cập nhật và hiển thị toàn bộ giao diện màn hình chính.
 * 
 * @param sample Cấu trúc chứa dữ liệu cảm biến mới nhất (PM2.5, PM10, Nhiệt độ, Độ ẩm, Áp suất).
 * @param isWifi Trạng thái WiFi (true: đã kết nối, false: chưa kết nối) để hiển thị icon tương ứng.
 * @param isSd Trạng thái thẻ nhớ SD (true: sẵn sàng, false: lỗi) để hiển thị icon tương ứng.
 * 
 * @note Hàm thực hiện clear buffer, vẽ lại các icon trạng thái, in các dòng thông số đo lường 
 * và cuối cùng là flush buffer xuống phần cứng I2C.
 * @return ESP_OK nếu hiển thị thành công, ESP_FAIL nếu có lỗi giao tiếp.
 */
esp_err_t oled_show(sensor_sample_t sample, bool isWifi, bool isSd);

/**
 * @brief Task chạy ngầm xử lý việc hiển thị dữ liệu liên tục lên OLED.
 * 
 * @param pvParameters Tham số truyền vào cho task, ép kiểu về `display_params_t*`.
 * 
 * @note Task sẽ khởi tạo OLED trước, sau đó đi vào vòng lặp vô tận, chặn (block) chờ 
 * dữ liệu từ `sensor_queue`. Mỗi khi có dữ liệu mới, task sẽ kết hợp với trạng thái 
 * WiFi/SD và gọi `oled_show` để cập nhật màn hình.
 */
void display_task(void *pvParameters);

