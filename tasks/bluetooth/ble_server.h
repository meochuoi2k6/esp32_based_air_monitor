#pragma once

/**
 * @file ble_server.h
 * @brief Khai báo các API khởi tạo và giao tiếp Bluetooth Low Energy (BLE).
 * 
 * @note File này định nghĩa các hàm dùng để thiết lập ESP32 hoạt động như một BLE Server. 
 * Thông qua đó, thiết bị có thể phát sóng (advertise), cho phép ứng dụng di động kết nối 
 * và truyền/nhận lệnh (đặc biệt là thông tin cấu hình WiFi).
 */

/**
 * @brief Khởi tạo giao thức Bluetooth Low Energy (BLE).
 * 
 * @note Hàm này cấu hình và kích hoạt controller Bluetooth của ESP ở chế độ BLE, giải phóng bộ 
 * nhớ của Classic BT để tiết kiệm tài nguyên. Sau đó, nó khởi tạo Bluedroid, đăng ký callback xử 
 * lý sự kiện GATTS và đăng ký ứng dụng BLE GATTS cơ bản.
 */
void ble_init(void);

/**
 * @brief Gửi thông báo (notify/indicate) qua BLE cho thiết bị client đã kết nối.
 * 
 * @param data Chuỗi dữ liệu (dạng string) cần gửi đi.
 * 
 * @note Cơ chế là sử dụng hàm `esp_ble_gatts_send_indicate` để truyền đi chuỗi byte tương ứng 
 * của dữ liệu, gửi qua interface (gatts_if) và ID kết nối (conn_id) đã được lưu khi có thiết bị 
 * kết nối thành công.
 */
void ble_send_notify(const char *data);
