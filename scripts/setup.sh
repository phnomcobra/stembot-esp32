#!/usr/bin/env bash
# ===========================================================
# setup.sh — Verify, install, or upgrade project prerequisites
#
# Usage:
#   ./scripts/setup.sh            # check only (exit 1 if anything missing)
#   ./scripts/setup.sh --install  # install missing tools, leave existing alone
#   ./scripts/setup.sh --update   # upgrade all tools and install any missing
# ===========================================================
set -euo pipefail

# shellcheck source=scripts/_common.sh
source "$(dirname "$0")/_common.sh"

INSTALL_MODE=0
UPDATE_MODE=0

while [[ $# -gt 0 ]]; do
    case "$1" in
        --install) INSTALL_MODE=1; shift ;;
        --update)  UPDATE_MODE=1;  shift ;;
        -h|--help) sed -n '2,8p' "$0" | sed 's/^# \?//'; exit 0 ;;
        *) echo "Unknown argument: $1"; echo "Usage: $0 [--install|--update]"; exit 1 ;;
    esac
done

MISSING=0

# ── Package-manager helpers ─────────────────────────────────

_pkg_update_index() {
    case "$_OS" in
        macos) brew update ;;
        linux) sudo apt-get update ;;
    esac
}

_pkg_install() {
    local brew_pkg="$1" apt_pkg="${2:-$1}"
    printf "    Installing %s...\n" "$brew_pkg"
    case "$_OS" in
        macos) echo y | HOMEBREW_CURL_RETRIES=3 brew install "$brew_pkg" ;;
        linux) sudo apt-get install -y "$apt_pkg" ;;
        *)     echo "    Unsupported OS — install $apt_pkg manually"; return 1 ;;
    esac
}

_pkg_upgrade() {
    local brew_pkg="$1" apt_pkg="${2:-$1}"
    printf "    Upgrading %s...\n" "$brew_pkg"
    case "$_OS" in
        macos) echo y | HOMEBREW_CURL_RETRIES=3 brew upgrade "$brew_pkg" || true ;;  # exits 1 when already up-to-date
        linux) sudo apt-get install --only-upgrade -y "$apt_pkg" ;;
        *)     echo "    Unsupported OS — upgrade $apt_pkg manually"; return 1 ;;
    esac
}

# ── Per-tool handlers ───────────────────────────────────────

# handle_tool <cmd> <brew-formula> [<apt-package>]
handle_tool() {
    local cmd="$1" brew_pkg="$2" apt_pkg="${3:-$2}"
    if command -v "$cmd" &>/dev/null; then
        printf "  [OK]      %-14s %s\n" "$cmd" "$(command -v "$cmd")"
        if [[ "$UPDATE_MODE" -eq 1 ]]; then _pkg_upgrade "$brew_pkg" "$apt_pkg"; fi
    else
        printf "  [MISSING] %-14s — %s\n" "$cmd" "$(_install_hint "$brew_pkg" "$apt_pkg")"
        if [[ "$INSTALL_MODE" -eq 1 || "$UPDATE_MODE" -eq 1 ]]; then
            _pkg_install "$brew_pkg" "$apt_pkg"
        else
            MISSING=1
        fi
    fi
}

handle_brew() {
    # macOS only: ensure Homebrew itself is present before any brew calls.
    [[ "$_OS" != "macos" ]] && return 0
    if command -v brew &>/dev/null; then
        printf "  [OK]      %-14s %s\n" "brew" "$(command -v brew)"
    else
        printf "  [MISSING] %-14s\n" "brew"
        if [[ "$INSTALL_MODE" -eq 1 || "$UPDATE_MODE" -eq 1 ]]; then
            echo "    Installing Homebrew..."
            /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
        else
            echo "              https://brew.sh"
            MISSING=1
        fi
    fi
}

handle_conan() {
    if command -v conan &>/dev/null; then
        printf "  [OK]      %-14s %s\n" "conan" "$(command -v conan)"
        if [[ "$UPDATE_MODE" -eq 1 ]]; then
            printf "    Upgrading conan...\n"
            pipx upgrade conan
        fi
    else
        printf "  [MISSING] %-14s — pipx install conan\n" "conan"
        if [[ "$INSTALL_MODE" -eq 1 || "$UPDATE_MODE" -eq 1 ]]; then
            printf "    Installing conan...\n"
            # Ensure pipx is available first
            if ! command -v pipx &>/dev/null; then
                _pkg_install pipx pipx
            fi
            pipx install conan
            # Make sure pipx-managed bins are on PATH for the rest of this session
            export PATH="$HOME/.local/bin:$PATH"
        else
            MISSING=1
        fi
    fi
}

handle_idf() {
    local idf_dir="${IDF_PATH:-$HOME/esp/esp-idf}"
    # Consider IDF present if IDF_PATH is set, idf.py is on PATH, or the
    # default install directory exists (export.sh not yet sourced in this shell).
    if [ -n "${IDF_PATH:-}" ] || command -v idf.py &>/dev/null || [ -f "$idf_dir/export.sh" ]; then
        printf "  [OK]      %-14s %s\n" "idf.py" "$idf_dir"
        # Run install.sh if the Python virtual environment hasn't been created yet,
        # or if we're in update mode.
        local python_env_dir="$HOME/.espressif/python_env"
        if [[ "$UPDATE_MODE" -eq 1 ]]; then
            printf "    Updating ESP-IDF at %s...\n" "$idf_dir"
            git -C "$idf_dir" pull
            git -C "$idf_dir" submodule update --init --recursive
            "$idf_dir/install.sh" all
        elif [[ "$INSTALL_MODE" -eq 1 ]] && { [ ! -d "$python_env_dir" ] || [ -z "$(ls -A "$python_env_dir" 2>/dev/null)" ]; }; then
            printf "    ESP-IDF tools not installed — running install.sh all...\n"
            "$idf_dir/install.sh" all
        fi
    else
        printf "  [MISSING] %-14s\n" "idf.py"
        if [[ "$INSTALL_MODE" -eq 1 || "$UPDATE_MODE" -eq 1 ]]; then
            printf "    Installing ESP-IDF to %s...\n" "$idf_dir"
            mkdir -p "$(dirname "$idf_dir")"
            if [ ! -d "$idf_dir" ]; then
                git clone --recursive https://github.com/espressif/esp-idf.git "$idf_dir"
            fi
            "$idf_dir/install.sh" all
            echo ""
            echo "  ESP-IDF installed. Activate it now:"
            echo "    . \"$idf_dir/export.sh\""
            echo "  To activate on every shell session add the above line to ~/.zshrc or ~/.bashrc"
        else
            printf "  [MISSING] %-14s — https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/\n" "idf.py"
            echo "              After install: . \$HOME/esp/esp-idf/export.sh"
            MISSING=1
        fi
    fi
}

handle_qemu() {
    local idf_dir="${IDF_PATH:-$HOME/esp/esp-idf}"
    local idf_tools="$idf_dir/tools/idf_tools.py"
    local espressif_qemu_dir="$HOME/.espressif/tools/qemu-xtensa"

    # Check PATH first; if not there, search the known espressif tools directory
    # (qemu-xtensa is only added to PATH after sourcing export.sh).
    local qemu_bin=""
    if command -v qemu-system-xtensa &>/dev/null; then
        qemu_bin="$(command -v qemu-system-xtensa)"
    elif [ -d "$espressif_qemu_dir" ]; then
        qemu_bin="$(find "$espressif_qemu_dir" -name "qemu-system-xtensa" -type f 2>/dev/null | head -1)"
    fi

    if [ -n "$qemu_bin" ]; then
        printf "  [OK]      %-14s %s\n" "qemu-xtensa" "$qemu_bin"
        if [[ "$UPDATE_MODE" -eq 1 ]] && [ -f "$idf_tools" ]; then
            printf "    Upgrading qemu-xtensa via idf_tools.py...\n"
            rm -f "$HOME/.espressif/dist/qemu-xtensa-"*
            python3 "$idf_tools" install qemu-xtensa || \
                echo "    WARNING: qemu-xtensa install failed (the pre-built binary may not support this CPU). Emulation will be unavailable."
        fi
        return 0
    fi

    printf "  [MISSING] %-14s — python3 %s install qemu-xtensa\n" "qemu-xtensa" "\$IDF_PATH/tools/idf_tools.py"
    if [[ "$INSTALL_MODE" -eq 1 || "$UPDATE_MODE" -eq 1 ]]; then
        # libSDL2 is a runtime dependency of the pre-built QEMU binary.
        printf "    Installing libsdl2 (QEMU runtime dependency)...\n"
        _pkg_install sdl2 libsdl2-2.0-0
        if [ -f "$idf_tools" ]; then
            printf "    Installing qemu-xtensa via idf_tools.py...\n"
            rm -f "$HOME/.espressif/dist/qemu-xtensa-"*
            python3 "$idf_tools" install qemu-xtensa || \
                echo "    WARNING: qemu-xtensa install failed (the pre-built binary may not support this CPU). Emulation will be unavailable."
        else
            echo "    WARNING: ESP-IDF not found at $idf_dir — install ESP-IDF first."
            MISSING=1
        fi
    else
        MISSING=1
    fi
}

# ── Main ────────────────────────────────────────────────────

if [[ "$INSTALL_MODE" -eq 1 || "$UPDATE_MODE" -eq 1 ]]; then
    echo ""
    echo "=== Updating package index ==="
    handle_brew  # ensure brew exists on macOS before updating index
    _pkg_update_index

    echo ""
    echo "=== Updating pip certificates ==="
    python3 -m pip install --upgrade pip-system-certs certifi
fi

echo ""
echo "=== Checking prerequisites ==="
[[ "$_OS" == "macos" ]] && handle_brew
handle_tool cmake        cmake        cmake
handle_tool ninja        ninja        ninja-build
handle_tool python3      python3      python3
handle_conan
handle_tool git          git          git
handle_tool clang-format llvm         clang-format
handle_tool clang-tidy   llvm         clang-tidy
handle_idf
handle_qemu

if [[ "$MISSING" -eq 1 ]]; then
    echo ""
    echo "One or more prerequisites are missing."
    echo "Run './scripts/setup.sh --install' to install them automatically."
    exit 1
fi

echo ""
echo "=== Initialising Conan default profile ==="
_ensure_pip_bin
conan profile detect --force

echo ""
echo "=== All prerequisites satisfied ==="
echo ""
echo "Quick-start commands:"
echo "  ./scripts/build.sh                               — Build ESP32 firmware"
echo "  STEMBOT_PORT=$(_default_port) ./scripts/flash.sh — Flash to device"
echo "  ./scripts/emulate.sh                             — Run in QEMU"
echo "  ./scripts/test.sh                                — Run host-native tests"
