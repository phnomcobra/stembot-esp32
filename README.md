# stembot-esp32

ESP32 firmware for a stembot edge node. Protocol-compatible with
[stembot-python](../stembot-python) and [stembot-rust](../stembot-rust), targeting
resource-constrained deployment on the ESP32 microcontroller.

## Overview

This project implements a stembot agent that runs natively on an ESP32. It is
intended to operate as a leaf node in a stembot topology — polling a single upstream
peer, processing control messages, and reporting back over the network.

## Differences from Other Stembot Implementations

The ESP32 agent is intentionally minimal compared to the Python and Rust
implementations:

- **No web server** — the agent does not expose an HTTP interface.
- **Single peer** — polls against one configured peer only; does not route or forward
  messages to other nodes.
- **No routing table** — uses the single peer as a default route.
- **Serial processing** — polls and processes one network message at a time.
- **No DAO / kvstore** — no data object abstraction layer or key-value storage.
- **Serial logging only** — all log output goes to the serial port via `ESP_LOG*`.
- **Core control forms** — initially handles `GetPeers`, `GetConfig`, and `Benchmark`
  forms via ticket request processing.
- **Ticket model** — processes ticket requests, produces ticket responses, and
  optionally produces ticket traces. Does not cache tickets or traces.
- **Wireless networking** — manages its own Wi-Fi connection to reach the upstream peer.

## Development

All workflows are driven by shell scripts in `scripts/`. See [AGENTS.md](AGENTS.md)
for a full reference.

### Quick Start

```bash
# First-time setup
./scripts/setup.sh --install

# Build and run in QEMU emulation
./scripts/build.sh
./scripts/emulate.sh        # Ctrl+] to quit

# Flash to hardware
./scripts/flash.sh
./scripts/monitor.sh        # Ctrl+] to exit

# Run unit tests
./scripts/test.sh

# Lint (check then fix)
./scripts/lint.sh --fix
./scripts/lint.sh
```

## Requirements

- ESP-IDF (installed at `~/esp/esp-idf`)
- CMake, Ninja
- Conan 2 (via pipx)
- clang-format, clang-tidy
- qemu-system-xtensa (for emulation)

Run `./scripts/setup.sh` to verify all prerequisites, or `./scripts/setup.sh --install`
to install any that are missing.