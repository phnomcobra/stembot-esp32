#!/usr/bin/env bash
# Run the compiled firmware inside a QEMU ESP32 emulator.
#
# Requires esp32-qemu from Espressif:
#   https://github.com/espressif/qemu
#
# With ESP-IDF >= 5.0 the emulator is invoked automatically via:
#   idf.py qemu monitor
set -euo pipefail

# shellcheck source=scripts/_common.sh
source "$(dirname "$0")/_common.sh"
_ensure_idf_env

cd "$(dirname "$0")/.."

if [ ! -f "build/stembot.bin" ]; then
    echo "Firmware binary not found — building first..."
    idf.py build
fi

echo "=== Starting QEMU emulation (Ctrl+] to quit) ==="
exec idf.py qemu monitor
