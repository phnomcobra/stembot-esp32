# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [3.0.0] - 2026-11-08

### Added

- `crypto` component (`crypto.hpp`/`crypto.cpp`): `derive_user_key()` (PBKDF2-HMAC-SHA256,
  matching the `stembot-python`/`stembot-rust` v1 user-key KDF), `KeyType`
  (`USER`/`DIST`/`UNKNOWN`), and `user_key_expire_time()`.
- `crypto_task` (`main/crypto.cpp`): FreeRTOS task that periodically exchanges keys with
  the configured peer via a new `exchange_keys` network message and rotates to the best
  distributed key offered by the peer.
- `network_message::ExchangeKeys` / `KeyDistribution`: wire types for the key-distribution
  protocol, plus `NetworkMessageType::ExchangeKeys`.
- `control_form::GetKeyMetadata` / `KeyMetadata`: control form reporting the active key's
  UUID, type, and remaining TTL.
- `Config::set_user_key()`, `Config::keytype`, `Config::keyExpireTime`, and
  `Config::get_key_ttl()` for passphrase-derived and distributed key lifecycle management.
- `state.hpp`: process-wide `Config` and `AgentClient` instances shared by `configure.cpp`,
  `crypto.cpp`, `processor.cpp`, and `main.cpp`.
- `processor` component dispatches `exchange_keys` network messages and
  `get_key_metadata` control forms.

### Changed

- **Breaking:** `Config` no longer falls back to a SHA-256("changeme") default key when
  NVS has no stored key; it now defaults to an all-zero key until a passphrase is set or
  a key is received via exchange. Devices no longer share a default key with
  `stembot-python` out of the box.
- `configure.cpp`, `crypto.cpp`, `processor.cpp`, and `main.cpp` reference the shared
  `config`/`client` instances directly from `state.hpp` instead of each declaring its own
  singleton accessor/alias.
- Consolidated the duplicate `Config::set_passphrase()`/`set_user_key()` methods into a
  single `Config::set_user_key()`.

### Removed

- Unused, empty `executor` namespace (`components/executor/include/executor.hpp`).

## [2.1.0] - 2026-07-26

### Added
- `network_whitelist` and `control_whitelist` fields on `NetworkMessagesRequest` for server-side filtering of polled messages.
- `processor` component uses whitelisted network messages requests.
- `utils` component: added helper functions for string list options.

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

[Unreleased]: https://github.com/phnomcobra/stembot-esp32/compare/v3.0.0...HEAD
[3.0.0]: https://github.com/phnomcobra/stembot-esp32/compare/v2.1.0...v3.0.0
[2.1.0]: https://github.com/phnomcobra/stembot-esp32/compare/v2.0.0...v2.1.0
[2.0.0]: https://github.com/phnomcobra/stembot-esp32/compare/v0.1.0...v2.0.0
[0.1.0]: https://github.com/phnomcobra/stembot-esp32/releases/tag/v0.1.0
