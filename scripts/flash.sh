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

echo "=== Flashing to $PORT ==="
idf.py -p "$PORT" flash
