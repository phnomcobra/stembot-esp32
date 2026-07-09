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

# Locate a working Espressif QEMU binary under ~/.espressif/tools/ and prepend
# its directory to PATH so that idf.py picks it up via shutil.which.
#
# This is needed on Intel Macs because Espressif's esp-develop-9.2.2-20260417
# release contains arm64 binaries in both macOS tarballs (packaging bug).
# setup.sh --install downloads a working x86_64 fallback from the previous
# release; emulate.sh finds it here regardless of the exact subdirectory used.
_espressif_qemu_dir=""
while IFS= read -r _bin; do
    if "$_bin" --version &>/dev/null 2>&1; then
        _espressif_qemu_dir="$(dirname "$_bin")"
        break
    fi
done < <(find "$HOME/.espressif/tools" -name "qemu-system-xtensa" -type f 2>/dev/null)

if [ -n "$_espressif_qemu_dir" ]; then
    export PATH="$_espressif_qemu_dir:$PATH"
fi

# Verify qemu-system-xtensa is available before attempting emulation.
if ! command -v qemu-system-xtensa &>/dev/null; then
    echo "ERROR: qemu-system-xtensa is not installed or is not supported on this platform."
    echo "Run: ./scripts/setup.sh --install"
    echo "See: https://github.com/espressif/qemu/releases"
    exit 1
fi

if [ ! -f "build/stembot.bin" ]; then
    echo "Firmware binary not found — building first..."
    idf.py build
fi

echo "=== Starting QEMU emulation (Ctrl+] to quit) ==="
exec idf.py qemu monitor
