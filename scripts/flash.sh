#!/usr/bin/env bash
# Flash compiled firmware to an attached ESP32 over serial.
#
# Override the default port by setting STEMBOT_PORT:
#   STEMBOT_PORT=/dev/ttyUSB0 ./scripts/flash.sh
#
# Common port names:
#   macOS  : /dev/tty.usbserial-* | /dev/tty.SLAB_USBtoUART | /dev/tty.usbmodem*
#   Ubuntu : /dev/ttyUSB0 | /dev/ttyACM0
#            (permission denied? run: sudo usermod -aG dialout $USER  then re-login)
set -euo pipefail

# shellcheck source=scripts/_common.sh
source "$(dirname "$0")/_common.sh"
_ensure_idf_env

PORT="${STEMBOT_PORT:-$(_default_port)}"
cd "$(dirname "$0")/.."

if grep -q '^CONFIG_ETH_USE_OPENETH=y' sdkconfig 2>/dev/null; then
    echo "ERROR: CONFIG_ETH_USE_OPENETH=y is set in sdkconfig." >&2
    echo "       This option is for QEMU emulation only and will cause exceptions on real hardware." >&2
    echo "       Disable it (set CONFIG_ETH_USE_OPENETH=n) and rebuild before flashing." >&2
    exit 1
fi

echo "=== Flashing to $PORT ==="
idf.py -p "$PORT" flash
