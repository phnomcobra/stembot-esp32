#!/usr/bin/env bash
# Run the compiled firmware inside a QEMU ESP32 emulator.
#
# Requires esp32-qemu from Espressif:
#   https://github.com/espressif/qemu
#
# With ESP-IDF >= 5.0 the emulator is invoked automatically via:
#   idf.py qemu monitor
#
# Networking
# ──────────
# The firmware is compiled with CONFIG_ETH_USE_OPENETH=y (applied from
# sdkconfig.defaults.qemu) so it uses the OpenCores Ethernet driver emulated
# by QEMU.  QEMU receives  -nic user,model=open_eth,hostfwd=…  so that:
#   • The VM can reach any agent running on the host's network (SLIRP NAT).
#   • The host can reach the agent's HTTP listener on localhost:8080.
# Build output goes to  build-qemu/  to keep it separate from the hardware
# build in  build/.
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

# QEMU-specific build: separate output directory + QEMU sdkconfig overlay.
QEMU_BUILD_DIR="build-qemu"
QEMU_SDKCONFIG_DEFAULTS="sdkconfig.defaults;sdkconfig.defaults.qemu"

# NIC: SLIRP user-mode networking with the OpenCores Ethernet model.
# hostfwd rules let the host initiate connections to the agent's HTTP listener.
QEMU_NIC="user,model=open_eth,hostfwd=tcp::8081-:8081"

# ICMP (ping) forwarding via SLIRP requires the host kernel to allow
# unprivileged processes to open raw ICMP sockets.  On Linux this is
# controlled by net.ipv4.ping_group_range.  The default ("1 0") blocks
# all unprivileged ICMP, so QEMU cannot proxy pings to external hosts.
# Set the range to cover all group IDs for the duration of this session.
if [[ "${_OS}" == "linux" ]]; then
    _ping_range="$(cat /proc/sys/net/ipv4/ping_group_range 2>/dev/null || echo "1 0")"
    if [[ "${_ping_range}" == "1"$'\t'"0" || "${_ping_range}" == "1 0" ]]; then
        echo "=== Enabling unprivileged ICMP for QEMU SLIRP (requires sudo) ==="
        sudo sysctl -w net.ipv4.ping_group_range="0 2147483647"
    fi
fi

echo "=== Building QEMU firmware (build dir: ${QEMU_BUILD_DIR}/) ==="
idf.py \
    -B "${QEMU_BUILD_DIR}" \
    -DSDKCONFIG_DEFAULTS="${QEMU_SDKCONFIG_DEFAULTS}" \
    build

echo ""
echo "=== Starting QEMU emulation (Ctrl+] to quit) ==="
echo "    Agent HTTP reachable at http://localhost:8081"
echo ""
exec idf.py \
    -B "${QEMU_BUILD_DIR}" \
    qemu \
    --qemu-extra-args "-nic ${QEMU_NIC}" \
    monitor

