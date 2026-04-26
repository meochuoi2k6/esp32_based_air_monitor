#pragma once

/*
 * Fill these values with your AWS IoT Core resources before enabling the
 * runtime task in app_main().
 */
#define AWS_IOT_ENDPOINT   "abg1djzmyzl0n-ats.iot.ap-southeast-2.amazonaws.com"
#define AWS_IOT_CLIENT_ID  "AIR_MONITOR"
#define AWS_IOT_THING_NAME "AIR_MONITOR"

#define AWS_IOT_TELEMETRY_TOPIC "airmonitor/"AWS_IOT_THING_NAME"/telemetry"
#define AWS_IOT_COMMAND_TOPIC   "airmonitor/"AWS_IOT_THING_NAME"/cmd"

/*
 * Expected certificate layout if you later embed them into the firmware:
 *
 * certs/
 *   AmazonRootCA1.pem
 *   device.pem.crt
 *   private.pem.key
 */
