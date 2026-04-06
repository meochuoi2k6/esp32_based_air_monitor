# ESP32 Air Monitor

## Overview
This project is an ESP32-based air quality monitoring system built with ESP-IDF. It reads particulate matter data from a PMS7001 sensor, environmental data from a BME680 sensor, shows the result on an SSD1306 OLED, stores measurements to an SD card, synchronizes time over Wi-Fi, and then enters deep sleep to save power.

## Hardware
- ESP32
- BME680
- PMS7001
- SSD1306 OLED
- SD card module

## Features
- Reads PM2.5 and PM10 values from PMS7001 over UART
- Reads temperature, humidity, and pressure from BME680 over I2C
- Averages samples before displaying and logging
- Shows device status on OLED, including Wi-Fi and SD icons
- Logs measurements to `/sdcard/log.csv`
- Synchronizes system time with SNTP when Wi-Fi is available
- Uses deep sleep between measurement cycles

## Pin Mapping

### I2C
- SDA -> GPIO21
- SCL -> GPIO22

### UART
- PMS7001 TX -> GPIO16
- PMS7001 RX -> GPIO17

### SPI
- SCK -> GPIO18
- MISO -> GPIO19
- MOSI -> GPIO23
- CS -> GPIO5

## Application Flow
1. Boot and initialize UART, I2C, OLED, and BME680.
2. Start Wi-Fi and try to synchronize time with SNTP.
3. Mount the SD card.
4. Collect valid PMS7001 samples and matching BME680 readings.
5. Average the collected data.
6. Update the OLED and append one CSV row to the SD card.
7. Enter deep sleep for the configured interval.

The state machine diagram for this flow is available in [docs/air_monitor_state_machine.drawio](/D:/ESP/air_monitor/docs/air_monitor_state_machine.drawio).

## Build And Flash

### Prerequisites
- ESP-IDF installed and configured
- Target board connected over USB

### Build
```bash
idf.py build
```

### Flash
```bash
idf.py flash
```

### Monitor
```bash
idf.py monitor
```

## Project Structure
```text
air_monitor/
|-- components/
|   |-- bme680/
|   |-- esp_idf_lib_helpers/
|   |-- i2cdev/
|   |-- icons/
|   `-- ssd1306/
|-- docs/
|   `-- air_monitor_state_machine.drawio
|-- main/
|   `-- air_monitor.c
|-- CMakeLists.txt
|-- README.md
`-- sdkconfig
```

## Notes
- Wi-Fi credentials are currently hardcoded in `main/air_monitor.c`.
- Time synchronization uses a timeout so the device can continue running offline if SNTP is unavailable.
- If SD card mounting fails, the device can still measure and display data.
- Use a stable power supply, especially for PMS7001 and the SD card module.

## Current Limitations
- PMS7001 frame validation currently checks the frame header but does not validate checksum.
- The main application logic is still concentrated in a single file and can be refactored into smaller modules or FreeRTOS tasks later.
