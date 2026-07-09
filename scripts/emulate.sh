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

# Verify qemu-system-xtensa is available before attempting emulation.
if ! command -v qemu-system-xtensa &>/dev/null; then
    # Also search the espressif tools directory (not on PATH until export.sh is sourced).
    _qemu_bin="$(find "$HOME/.espressif/tools/qemu-xtensa" -name "qemu-system-xtensa" -type f 2>/dev/null | head -1)"
    if [ -z "$_qemu_bin" ]; then
        echo "ERROR: qemu-system-xtensa is not installed or is not supported on this platform."
        echo "The pre-built Espressif QEMU binary for this release may not support your CPU."
        echo "See: https://github.com/espressif/qemu/releases"
        exit 1
    fi
fi

if [ ! -f "build/stembot.bin" ]; then
    echo "Firmware binary not found — building first..."
    idf.py build
fi

echo "=== Starting QEMU emulation (Ctrl+] to quit) ==="
exec idf.py qemu monitor
