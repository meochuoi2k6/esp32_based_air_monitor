#pragma once

#include <stdbool.h>

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "sensor_task.h"

/**
 * @brief logger_task_params_t: Cấu trúc truyền tham số cho task ghi log.
 * 
 * Các trường dữ liệu:
 * @brief- `sensor_queue`: Hàng đợi nhận dữ liệu cảm biến để ghi log
 * 
 * @note Chứa hàng đợi (queue) để nhận dữ liệu từ cảm biến.
 */
typedef struct logger_task_params_t {
    QueueHandle_t sensor_queue; /*!< Hàng đợi nhận dữ liệu cảm biến để ghi log */
} logger_task_params_t;

/**
 * @brief Kiểm tra trạng thái sẵn sàng của thẻ nhớ SD.
 * 
 * @note Trả về cờ trạng thái báo hiệu thẻ nhớ đã được kết nối và mount thành công hay chưa.
 * @return true nếu thẻ SD đã sẵn sàng để ghi, false nếu chưa.
 */
bool logger_is_sd_ready(void);

/**
 * @brief Khởi tạo giao tiếp và mount thẻ nhớ SD.
 * 
 * @note Khởi tạo bus SPI nếu chưa được khởi tạo, sau đó thực hiện cấu hình slot cho thẻ SD và 
 * mount hệ thống file FAT. Nếu thẻ đã sẵn sàng thì không thực hiện lại.
 * @return true nếu khởi tạo thành công, false nếu có lỗi xảy ra.
 */
bool init_sd_card(void);

/**
 * @brief Ghi một bản ghi mẫu dữ liệu vào file log trên thẻ nhớ.
 * 
 * @param sample Con trỏ tới cấu trúc dữ liệu cảm biến cần ghi.
 * 
 * @note Kiểm tra trạng thái dữ liệu và kết nối thẻ SD. Nếu hợp lệ, tiến hành ghi các giá trị 
 * PM2.5, PM10, nhiệt độ, độ ẩm, áp suất cùng mốc thời gian vào file định dạng CSV. Nếu ghi lỗi, 
 * đánh dấu thẻ nhớ bị ngắt kết nối.
 * @return ESP_OK nếu ghi thành công, hoặc các mã lỗi ESP_ERR_* nếu có sự cố.
 */
esp_err_t logger_write_sample(const sensor_sample_t *sample);

/**
 * @brief Task chạy ngầm xử lý việc ghi log liên tục vào thẻ SD.
 * 
 * @param pvParameters Con trỏ tham số truyền vào task (kiểu logger_task_params_t).
 * 
 * @note Task liên tục chờ dữ liệu từ hàng đợi (queue). Khi có dữ liệu, kiểm tra thẻ SD; nếu chưa 
 * kết nối sẽ thử kết nối lại định kỳ. Khi có thẻ nhớ và nhận được dữ liệu hợp lệ, task sẽ lưu 
 * mẫu dữ liệu đó vào thẻ.
 */
void logger_task(void *pvParameters);
