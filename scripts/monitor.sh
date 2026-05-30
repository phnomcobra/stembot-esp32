#!/usr/bin/env bash
# Open an IDF serial monitor on the attached ESP32.
#
# Override the default port by setting STEMBOT_PORT:
#   STEMBOT_PORT=/dev/ttyACM0 ./scripts/monitor.sh
#
# Common port names:
#   macOS  : /dev/tty.usbserial-* | /dev/tty.SLAB_USBtoUART | /dev/tty.usbmodem*
#   Ubuntu : /dev/ttyUSB0 | /dev/ttyACM0
set -euo pipefail

# shellcheck source=scripts/_common.sh
source "$(dirname "$0")/_common.sh"
_ensure_idf_env

PORT="${STEMBOT_PORT:-$(_default_port)}"
cd "$(dirname "$0")/.."

echo "=== Serial monitor on $PORT (Ctrl+] to exit) ==="
idf.py -p "$PORT" monitor
