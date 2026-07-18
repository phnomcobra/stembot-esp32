# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [2.0.0] - 2026-07-18

### Added

- `processor` component: FreeRTOS task that polls the peer for network messages,
  dispatches ticket requests and ping messages, and returns encrypted responses.
- `AgentClient`: AES-256-EAX encrypted HTTP client for inter-agent communication,
  implementing the same wire format as `stembot-python` and `stembot-rust`.
- `configure` TUI: serial REPL (`esp_console`) for setting `agtuuid`, `peerUrl`,
  Wi-Fi credentials, and encryption passphrase; `save`, `reboot`, `wifi_connect`,
  `ping`, and `net_info` commands.
- `control_form` namespace: `GetConfig` and `Benchmark` control form types with
  JSON serialisation/deserialisation.
- `network_message` namespace: `NetworkTicket`, `NetworkMessagesRequest/Response`,
  `Ping`, `Acknowledgement`, and `TicketTraceResponse` message types.
- `Config`: NVS-backed configuration model with `debug`, `polling`, `poll_ticks`,
  `peerUrl`, `wifiSSID`, `wifiPassword`, and AES-256 key fields.
- `KVStore`: thin C++ wrapper around the ESP-IDF NVS API.

### Fixed

- `Config` key round-trip: constructor now reads the key with `getBytes` (blob)
  to match the `putBytes` used in `save()`, preventing silent fallback to the
  default key after a reboot.

### Changed

- `flash.sh` now exits with an error if `CONFIG_ETH_USE_OPENETH=y` is detected
  in `sdkconfig`, preventing accidental flashing of QEMU-only builds to hardware.

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

[Unreleased]: https://github.com/phnomcobra/stembot-esp32/compare/v2.0.0...HEAD
[2.0.0]: https://github.com/phnomcobra/stembot-esp32/compare/v0.1.0...v2.0.0
[0.1.0]: https://github.com/phnomcobra/stembot-esp32/releases/tag/v0.1.0
