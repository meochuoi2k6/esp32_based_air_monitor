#pragma once

/**
 * @file aws_iot_config.h
 * @brief Cấu hình các thông số cần thiết để kết nối tới AWS IoT Core.
 * 
 * @note File này chứa các macro định nghĩa điểm cuối (endpoint), Client ID, và Tên thiết bị (Thing Name) 
 * phục vụ cho việc thiết lập MQTT. Ngoài ra, nó cũng định nghĩa các topic sẽ được publish (telemetry) 
 * và subscribe (command). Các chứng chỉ và khóa phải được chuẩn bị sẵn và nhúng vào firmware.
 */

/*
 * Fill these values with your AWS IoT Core resources before enabling the
 * runtime task in app_main().
 */

/** 
 * @brief AWS IoT Core Endpoint.
 * @note Đây là địa chỉ endpoint được AWS cung cấp cho tài khoản và region của bạn. 
 */
#define AWS_IOT_ENDPOINT   "abg1djzmyzl0n-ats.iot.ap-southeast-2.amazonaws.com"

/** 
 * @brief Tên Client ID sử dụng khi kết nối MQTT.
 * @note Phải là duy nhất để tránh bị ngắt kết nối khi có thiết bị khác dùng chung Client ID. 
 */
#define AWS_IOT_CLIENT_ID  "AIR_MONITOR"

/** 
 * @brief Tên Thing (Thiết bị) đăng ký trên AWS IoT.
 * @note Dùng để nhận diện thiết bị và quản lý Device Shadow hoặc các chính sách bảo mật. 
 */
#define AWS_IOT_THING_NAME "AIR_MONITOR"

/** 
 * @brief MQTT Topic dùng để publish dữ liệu cảm biến (Telemetry).
 * @note Dữ liệu được gửi lên sẽ theo định dạng JSON chứa nhiệt độ, độ ẩm, chất lượng không khí. 
 */
#define AWS_IOT_TELEMETRY_TOPIC "airmonitor/"AWS_IOT_THING_NAME"/telemetry"

/** 
 * @brief MQTT Topic dùng để subscribe các lệnh điều khiển từ Cloud (Command).
 * @note Thiết bị sẽ lắng nghe topic này để thực thi các lệnh từ server (ví dụ: cập nhật OTA, đổi cấu hình). 
 */
#define AWS_IOT_COMMAND_TOPIC   "airmonitor/"AWS_IOT_THING_NAME"/cmd"

/*
 * Expected certificate layout if you later embed them into the firmware:
 *
 * certs/
 *   AmazonRootCA1.pem
 *   device.pem.crt
 *   private.pem.key
 */

