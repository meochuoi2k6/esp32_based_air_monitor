# Changelog

All notable changes to this project will be documented in this file.

## [0.2.0] - 2026-04-06

### Changed
- Updated sensor sampling flow to average BME680 values across valid sampling cycles.
- Updated OLED display and SD logging to use averaged environmental values consistently.
- Redrew the OLED Wi-Fi and SD card status icons for clearer 16x16 monochrome rendering.

### Fixed
- Added a timeout to SNTP time synchronization to prevent the device from blocking forever when Wi-Fi or time sync is unavailable.
- Prevented invalid PMS7001 UART frames from skewing BME680 average calculations.
- Aligned displayed values, printed values, and logged values so they now represent the same averaged dataset.

### Files Updated
- `main/air_monitor.c`
- `components/icons/icons.c`