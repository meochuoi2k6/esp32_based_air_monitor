#pragma once

/**
 * @file sensor_task.h
 * @brief Định nghĩa cấu trúc dữ liệu và API phục vụ việc đọc cảm biến (PMS7003, BME680).
 * 
 * @note File này chịu trách nhiệm khai báo các cấu trúc mẫu dữ liệu (`sensor_sample_t`), cấu 
 * trúc tham số của task, và các hàm hỗ trợ khởi tạo, đọc/tính trung bình, cũng như hàm task 
 * chính để phân phối dữ liệu cho các module khác.
 */

#include <stdbool.h>

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

/**
 * @brief Kích thước bộ đệm chuỗi thời gian (timestamp).
 * @note Độ dài tối thiểu 20 để chứa chuỗi định dạng "YYYY-MM-DD HH:MM:SS".
 */
#define SENSOR_SAMPLE_TIMESTAMP_LEN 20

/**
 * @brief sensor_sample_t: Cấu trúc lưu trữ một bộ dữ liệu (sample) từ các cảm biến.
 * 
 * Các trường dữ liệu:
 * @brief- `pm25`: Chỉ số bụi mịn PM2.5 (ug/m3)
 * @brief- `pm10`: Chỉ số bụi mịn PM10 (ug/m3)
 * @brief- `temperature`: Nhiệt độ (độ C)
 * @brief- `humidity`: Độ ẩm tương đối (%)
 * @brief- `pressure`: Áp suất không khí (hPa)
 * @brief- `timestamp`: Chuỗi thời gian lúc lấy mẫu
 * @brief- `valid`: Cờ báo hiệu dữ liệu hợp lệ hay không
 * 
 * @note Dữ liệu bụi mịn (PM2.5, PM10) lấy từ cảm biến PMS7003 (UART). Dữ liệu môi trường 
 * (temperature, humidity, pressure) lấy từ BME680 (I2C). Các trường `timestamp` và `valid` 
 * đánh dấu tính hợp lệ của mẫu.
 */
typedef struct sensor_sample_t {
    int pm25;           /*!< Chỉ số bụi mịn PM2.5 (ug/m3) */
    int pm10;           /*!< Chỉ số bụi mịn PM10 (ug/m3) */
    float temperature;  /*!< Nhiệt độ (độ C) */
    float humidity;     /*!< Độ ẩm tương đối (%) */
    float pressure;     /*!< Áp suất không khí (hPa) */
    char timestamp[SENSOR_SAMPLE_TIMESTAMP_LEN]; /*!< Chuỗi thời gian lúc lấy mẫu */
    bool valid;         /*!< Cờ báo hiệu dữ liệu hợp lệ hay không */
} sensor_sample_t;

/**
 * @brief sensor_task_params_t: Cấu trúc tham số khởi tạo task cảm biến.
 * 
 * @note Chứa danh sách các hàng đợi (queue) để đẩy mẫu dữ liệu sau khi đọc xong tới các task 
 * xử lý khác (như hiển thị, lưu thẻ nhớ, hoặc gửi lên Cloud).
 * 
 * Các trường dữ liệu:
 * - display_queue: Hàng đợi gửi dữ liệu cho display_task
 * - logger_queue: Hàng đợi gửi dữ liệu cho logger_task
 * - cloud_queue: Hàng đợi gửi dữ liệu cho aws_iot_task
 */
typedef struct sensor_task_params_t {
    QueueHandle_t display_queue; /*!< Hàng đợi gửi dữ liệu cho display_task */
    QueueHandle_t logger_queue;  /*!< Hàng đợi gửi dữ liệu cho logger_task */
    QueueHandle_t cloud_queue;   /*!< Hàng đợi gửi dữ liệu cho aws_iot_task */
} sensor_task_params_t;

/**
 * @brief Khởi tạo phần cứng và thư viện cần thiết cho các cảm biến.
 * 
 * @note Hàm thực hiện cấu hình giao tiếp UART cho PMS7003 và I2C cho BME680, 
 * thiết lập các thông số cơ bản trước khi bắt đầu đọc dữ liệu.
 * @return ESP_OK nếu thành công, hoặc các mã lỗi ESP_ERR_... nếu thất bại.
 */
esp_err_t sensor_task_init(void);

/**
 * @brief Đọc liên tiếp nhiều lần và lấy giá trị trung bình từ các cảm biến.
 * 
 * @param out_sample Con trỏ nhận dữ liệu sau khi đã tính trung bình.
 * 
 * @note Quá trình đọc sẽ lấy một số lượng mẫu định trước (VD: 5 mẫu) có delay giữa các 
 * lần đọc, sau đó chia trung bình để lọc nhiễu, rồi gán timestamp hiện tại.
 * @return ESP_OK nếu quá trình hoàn tất thành công.
 */
esp_err_t sensor_task_read_average(sensor_sample_t *out_sample);

/**
 * @brief Lấy bản sao của mẫu dữ liệu gần nhất đã được đọc thành công.
 * 
 * @param out_sample Con trỏ nhận bản sao của mẫu dữ liệu.
 * 
 * @note Hữu ích khi một hàm/module ngoài cần lấy nhanh giá trị hiện thời mà không muốn 
 * thông qua queue hay phải chờ quá trình đọc thực tế.
 * @return ESP_OK nếu có mẫu hợp lệ, ESP_ERR_INVALID_STATE nếu chưa có dữ liệu.
 */
esp_err_t sensor_task_get_latest(sensor_sample_t *out_sample);

/**
 * @brief Task ngầm liên tục đọc dữ liệu cảm biến và đẩy vào các queue.
 * 
 * @param pvParameters Tham số truyền vào cho task, ép kiểu về `sensor_task_params_t*`.
 * 
 * @note Vòng lặp chính của task sẽ gọi `sensor_task_read_average`, in log, và nếu dữ liệu 
 * hợp lệ thì xQueueSend tới display, logger, cloud. Sau đó delay 1 khoảng (VD: 1000ms).
 */
void sensor_task(void *pvParameters);

