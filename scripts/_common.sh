#!/usr/bin/env bash
# _common.sh — platform utilities sourced by other scripts in this directory.
# Do NOT execute directly.

# Detect the host operating system.
case "$(uname -s)" in
    Darwin) _OS="macos" ;;
    Linux)  _OS="linux" ;;
    *)      _OS="unknown" ;;
esac

# Print the default serial port for the detected platform.
# Callers should prefer the STEMBOT_PORT environment variable over this value.
_default_port() {
    case "$_OS" in
        macos)  echo "/dev/tty.usbserial-0001" ;;
        *)      echo "/dev/ttyUSB0" ;;
    esac
}

# Print a platform-appropriate package install hint.
# Usage: _install_hint <brew-formula> [<apt-package>]
# If <apt-package> is omitted it falls back to <brew-formula>.
_install_hint() {
    local brew_pkg="$1"
    local apt_pkg="${2:-$1}"
    case "$_OS" in
        macos)  echo "brew install $brew_pkg" ;;
        linux)  echo "sudo apt install $apt_pkg" ;;
        *)      echo "install $apt_pkg" ;;
    esac
}

# Print a platform-appropriate install hint for the LLVM tools
# (clang-format and clang-tidy), which are packaged differently.
_llvm_install_hint() {
    case "$_OS" in
        macos)  echo "brew install llvm  # then: export PATH=\"\$(brew --prefix llvm)/bin:\$PATH\"" ;;
        linux)  echo "sudo apt install clang-format clang-tidy" ;;
        *)      echo "install clang-format and clang-tidy" ;;
    esac
}

# Ensure the ESP-IDF tools are available in PATH.
# If idf.py is not already on PATH, sources $IDF_PATH/export.sh (or
# ~/esp/esp-idf/export.sh) automatically so callers never need to pre-source it.
_ensure_idf_env() {
    if command -v idf.py &>/dev/null; then
        return 0
    fi
    local idf_dir="${IDF_PATH:-$HOME/esp/esp-idf}"
    if [ -f "$idf_dir/export.sh" ]; then
        echo "Activating ESP-IDF environment from $idf_dir ..."
        # shellcheck source=/dev/null
        source "$idf_dir/export.sh"
    else
        echo "ERROR: ESP-IDF not found at $idf_dir"
        echo "Run './scripts/setup.sh --install' or source \$IDF_PATH/export.sh first."
        exit 1
    fi
}

# Ensure the Python user-level bin directory is in PATH.
# Covers both 'pip install --user' and pipx installs, since both land in
# ~/.local/bin on Linux (pipx) or ~/Library/Python/x.y/bin (macOS pip --user).
_ensure_pip_bin() {
    if command -v conan &>/dev/null; then
        return 0
    fi
    # pipx on Linux installs to ~/.local/bin
    local pipx_bin="$HOME/.local/bin"
    if [ -d "$pipx_bin" ]; then
        export PATH="$pipx_bin:$PATH"
        if command -v conan &>/dev/null; then
            return 0
        fi
    fi
    # fallback: pip --user scheme bin
    local user_bin
    user_bin="$(python3 -m site --user-base 2>/dev/null)/bin"
    if [ -d "$user_bin" ]; then
        export PATH="$user_bin:$PATH"
    fi
}
