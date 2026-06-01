# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [0.1.0] - 2026-05-31

### Added

- Initial project scaffold: ESP32 firmware targeting ESP-IDF with CMake and Conan.
- `helloworld` component with host-native unit test (Catch2).
- `app_main` entry point in `main/` using `ESP_LOGI` for serial output.
- Developer scripts: `setup.sh`, `build.sh`, `emulate.sh`, `flash.sh`,
  `monitor.sh`, `stop.sh`, `lint.sh`, `test.sh`.
- QEMU emulation support via `idf.py qemu monitor`.
- clang-format and clang-tidy static analysis via `lint.sh`.
- `.gitignore` excluding build artefacts and auto-generated Conan files.
- `AGENTS.md` with script reference and conventions for AI coding agents.
- `README.md` with project overview and development quick-start.

[Unreleased]: https://github.com/phnomcobra/stembot-esp32/compare/v0.1.0...HEAD
[0.1.0]: https://github.com/phnomcobra/stembot-esp32/releases/tag/v0.1.0
