# Changelog

All notable changes to this project will be documented in this file.

## [Unreleased]

### Added
- Added initial RTOS task folders for `display`, `sensors`, `storage`, `time`, and `wifi`.
- Added `sensor_sample_t` as a shared runtime data model for sensor output.
- Added a first `sensor_task` module API with initialization, averaged sampling, latest-sample access, and FreeRTOS task entry points.
- Added queue-based data flow from `sensor_task` to `display_task` and `logger_task`.
- Added `display_task` for OLED rendering with Wi-Fi and SD card status indicators.
- Added `logger_task` for timestamped CSV logging to SD card.
- Added `wifi_task` and `time_task` modules to separate Wi-Fi connection handling from SNTP time synchronization.
- Added component `CMakeLists.txt` files for the new task modules.

### Changed
- Started refactoring the application from a single-file flow toward task-oriented modules under `tasks/`.
- Moved UART and BME680 sensor handling logic into `tasks/sensors/sensor_task.c`.
- Converted `main/air_monitor.c` into a FreeRTOS task bootstrap that creates sample queues and starts Wi-Fi, time, sensor, display, and logger tasks.
- Converted OLED update logic into a queue-driven `display_task` design.
- Moved `i2cdev_init()` to `app_main()` so shared I2C setup is initialized once before sensor and display tasks start.
- Disabled the BME680 gas heater because the project currently uses temperature, humidity, and pressure only; this reduces sensor self-heating and temperature offset.

### Fixed
- Fixed missing type includes in display, logger, icon, and font headers that caused compile-time parser errors.
- Improved SD logger recovery when the SD card is removed and reinserted by unmounting on write failure and retrying mount periodically.

### Notes
- The RTOS task split is now wired into `main`, but hardware validation is still required on the ESP32 target.
- `idf.py build` could not be verified in the local Codex shell because the ESP-IDF Python virtual environment is missing on this machine.

## [0.2.0] - 2026-04-06

### Added
- Added `docs/air_monitor_state_machine.drawio` for documenting the current application flow.
- Updated project documentation in `README.md` to reflect averaging, SNTP timeout handling, OLED status icons, and deep sleep behavior.

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
- `docs/air_monitor_state_machine.drawio`
- `README.md`
