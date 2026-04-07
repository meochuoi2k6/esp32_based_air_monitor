# Changelog

All notable changes to this project will be documented in this file.

## [Unreleased]

### Added
- Added initial RTOS task folders for `display`, `sensors`, `storage`, `time`, and `wifi`.
- Added `sensor_sample_t` as a shared runtime data model for sensor output.
- Added a first `sensor_task` module API with initialization, averaged sampling, latest-sample access, and FreeRTOS task entry points.
- Added initial `display_task` and `logger_task` source files as part of the ongoing task-based refactor.

### Changed
- Started refactoring the application from a single-file flow toward task-oriented modules under `tasks/`.
- Moved UART and BME680 sensor handling logic into `tasks/sensors/sensor_task.c`.
- Began converting OLED update logic into a queue-driven `display_task` design.

### Notes
- The RTOS refactor is still in progress and is not fully integrated into `main` yet.
- Some newly added task modules are scaffolding-stage and may still require compile and integration fixes before use.

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
