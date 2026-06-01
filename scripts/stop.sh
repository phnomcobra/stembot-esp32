#!/usr/bin/env bash
# stop.sh — Stop any running QEMU emulation or IDF monitor processes.
set -euo pipefail

stopped=0

if pkill -f "qemu-system-xtensa" 2>/dev/null; then
    echo "Stopped: qemu-system-xtensa"
    stopped=1
fi

if pkill -f "idf_monitor" 2>/dev/null; then
    echo "Stopped: idf_monitor"
    stopped=1
fi

if [[ "$stopped" -eq 0 ]]; then
    echo "No monitor or emulation processes found."
fi
