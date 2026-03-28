# 🌫️ ESP32 Air Monitor

## 📌 Overview
This project is an air quality monitoring system based on ESP32. It collects environmental data, displays it on an OLED screen, and logs data to an SD card.

## ⚙️ Hardware Components
- ESP32  
- BME680 (temperature, humidity, gas sensor)  
- PMS7001 (PM1.0, PM2.5, PM10 dust sensor)  
- OLED SSD1306 (display)  
- SD Card Module (data logging)  

## 🔌 Wiring

### I2C (OLED + BME680)
- SDA → GPIO21  
- SCL → GPIO22  

### UART (PMS7001)
- RX → GPIO16  
- TX → GPIO17  

### SPI (SD Card)
- SCK → GPIO18  
- MISO → GPIO19  
- MOSI → GPIO23  
- CS → GPIO5  

## 📊 Features
- Read environmental data from BME680  
- Measure air quality (PM1.0, PM2.5, PM10) using PMS7001  
- Display real-time data on OLED  
- Log data to SD card in CSV format  

## 🚀 Getting Started

### 1. Clone repository
git clone https://github.com/meochuoi2k6/esp32_based_air_monitor
cd air-monitor  

### 2. Set up ESP-IDF
Make sure ESP-IDF is installed and configured:  
. $IDF_PATH/export.sh  

### 3. Build project
idf.py build  

### 4. Flash to ESP32
idf.py flash  

### 5. Monitor output
idf.py monitor  

## 📁 Project Structure
air_monitor/  
├── main/  
├── components/  
├── build/  
├── CMakeLists.txt  
├── README.md  
└── sdkconfig  

## 📈 Data Logging Format (CSV)
Example:  
time,temperature,humidity,pm2.5  
12:00,30.2,65,12  

## ⚠️ Notes
- Do NOT upload `esp-idf/` to the repository  
- Use a stable 5V supply for PMS7001  
- Ensure correct wiring for I2C, UART, and SPI  