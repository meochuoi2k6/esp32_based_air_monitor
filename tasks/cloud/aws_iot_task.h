#pragma once

/**
 * @file aws_iot_task.h
 * @brief Định nghĩa các thành phần liên quan đến tác vụ giao tiếp với AWS IoT Core.
 * 
 * @note File này chứa định nghĩa cấu trúc dữ liệu cho task AWS IoT và khai báo các hàm 
 * cấu hình, quản lý kết nối MQTT, cũng như task thực thi quá trình đẩy dữ liệu cảm biến 
 * lên cloud.
 */

#include <stdbool.h>


#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "sensor_task.h"
#include "esp_err.h"
#include "mqtt_client.h"

/**
 * @brief aws_iot_task_params_t: Cấu trúc lưu trữ tham số truyền cho task AWS IoT.
 * 
 * Các trường dữ liệu:
 * @brief- `cloud_queue`: Hàng đợi chứa mẫu dữ liệu để gửi lên cloud
 * 
 * @note Chứa hàng đợi (`cloud_queue`) để task MQTT có thể lấy dữ liệu cần đẩy lên cloud.
 */
typedef struct aws_iot_task_params_t {
    QueueHandle_t cloud_queue; /*!< Hàng đợi chứa mẫu dữ liệu để gửi lên cloud */
} aws_iot_task_params_t;

/**
 * @brief Khởi tạo và thiết lập các thông số cơ bản cho client MQTT của AWS IoT.
 * 
 * @param config Con trỏ trỏ tới cấu trúc cấu hình MQTT client của ESP-IDF.
 * 
 * @note Hàm này sẽ điền thông tin về URI, Port, và gán các con trỏ trỏ tới chứng chỉ 
 * CA, chứng chỉ thiết bị, và khóa riêng tư (đã nhúng vào nhị phân) vào biến config. 
 * @return ESP_OK nếu gán thông số thành công, hoặc lỗi ESP_ERR_INVALID_ARG nếu config bị null.
 */
esp_err_t esp_configure_mqtt_client(esp_mqtt_client_config_t *config); //no static here

/* Các biến chứa chứng chỉ đã được build nhúng vào ROM */
extern const uint8_t AmazonRootCA1_pem_start[] asm("_binary_AmazonRootCA1_pem_start");
extern const uint8_t device_pem_crt_start[] asm("_binary_device_pem_crt_start");
extern const uint8_t private_pem_key_start[] asm("_binary_private_pem_key_start");

/**
 * @brief Kiểm tra trạng thái kết nối tới AWS IoT Cloud.
 * 
 * @note Trả về true nếu MQTT Client đã kết nối hoàn tất tới Broker, ngược lại trả về false.
 * @return true/false tương ứng với trạng thái kết nối MQTT.
 */
bool aws_iot_is_connected(void);

/**
 * @brief Task chạy ngầm xử lý việc kết nối và đẩy dữ liệu lên AWS IoT.
 * 
 * @param pvParameters Tham số truyền vào cho task, ép kiểu về `aws_iot_task_params_t*`.
 * 
 * @note Cơ chế là đợi WiFi kết nối và thời gian được đồng bộ xong thì mới bắt đầu cấu hình 
 * và khởi động MQTT client. Khi đã có kết nối MQTT thành công, vòng lặp chính sẽ liên tục đọc 
 * queue để lấy mẫu cảm biến, chuyển sang định dạng chuỗi JSON và publish lên topic Telemetry 
 * của AWS IoT.
 */
void aws_iot_task(void *pvParameters);
