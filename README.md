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
- **No DAO** — no data object abstraction layer.
- **Serial logging only** — all log output goes to the serial port via `ESP_LOG*`.
- **Core control forms** — initially handles `GetConfig`, and `Benchmark`
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

## Agent Configuration (TUI)

The firmware includes a serial REPL for configuring the agent. It is accessible
through the emulator console or a live serial monitor.

```bash
# In QEMU (after build):
./scripts/emulate.sh

# On physical hardware (after flash):
./scripts/monitor.sh
```

The prompt `stembot>` appears once the device has booted. Type `help` to list all
available commands.

### Configuration Workflow

```
# 1. Show current settings
stembot> list

# 2. Set individual fields
stembot> set agtuuid  <uuid>
stembot> set peerUrl  http://192.168.1.10:8080/mpi
stembot> set wifiSSID MyNetwork
stembot> set wifiPassword MyPassword
stembot> set passphrase changeme

# 3. Persist settings to NVS flash
stembot> save

# 4. Connect to Wi-Fi (optional — also happens automatically on next boot)
stembot> wifi_connect

# 5. Reboot to apply all settings
stembot> reboot
```

### TUI Command Reference

| Command | Description |
|---|---|
| `list` | Show all current configuration values. |
| `set <field> <value>` | Set a configuration field in memory. Changes are not persisted until `save` is run. Valid fields: `agtuuid`, `peerUrl`, `wifiSSID`, `wifiPassword`, `passphrase`. |
| `save` | Write current in-memory configuration to NVS flash. |
| `reboot` | Reboot the device. |
| `wifi_connect` | Connect to Wi-Fi using the stored `wifiSSID` and `wifiPassword`. |
| `ping <host> [-c <n>]` | Send ICMP echo requests to a hostname or IP address. |
| `net_info` | Show the current IP address, netmask, gateway, and DNS server. |
| `help` | List all registered commands with their descriptions. |

> **Note:** `set` only updates in-memory values. Always run `save` before `reboot`
> to make changes permanent. The encryption passphrase is stored as a derived
> AES-256 key; the raw passphrase is not retained after `save`.

