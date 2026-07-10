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
    # Detect a broken pipx symlink — this happens when VS Code (snap) updates to
    # a new revision and deletes the old venv that ~/.local/bin/conan pointed to.
    local conan_link="$HOME/.local/bin/conan"
    local _broken_symlink=0
    if [[ -L "$conan_link" && ! -e "$conan_link" ]]; then
        printf "  [WARN]    %-14s — broken symlink at %s (VS Code snap update?); will reinstall\n" "conan" "$conan_link"
        _broken_symlink=1
    fi

    if [[ "$_broken_symlink" -eq 0 ]] && command -v conan &>/dev/null; then
        printf "  [OK]      %-14s %s\n" "conan" "$(command -v conan)"
        if [[ "$UPDATE_MODE" -eq 1 ]]; then
            printf "    Upgrading conan...\n"
            pipx upgrade conan
        fi
    else
        if [[ "$_broken_symlink" -eq 0 ]]; then
            printf "  [MISSING] %-14s — pipx install conan\n" "conan"
        fi
        if [[ "$INSTALL_MODE" -eq 1 || "$UPDATE_MODE" -eq 1 || "$_broken_symlink" -eq 1 ]]; then
            printf "    Installing conan...\n"
            # Ensure pipx is available first
            if ! command -v pipx &>/dev/null; then
                _pkg_install pipx pipx
            fi
            # Use --force so pipx regenerates the venv and symlink even if a
            # stale venv entry already exists (e.g., after a snap revision bump).
            pipx install --force conan
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
        elif [[ "$INSTALL_MODE" -eq 1 ]]; then
            printf "    Ensuring ESP-IDF submodules are initialised...\n"
            git -C "$idf_dir" submodule update --init --recursive
            if [ ! -d "$python_env_dir" ] || [ -z "$(ls -A "$python_env_dir" 2>/dev/null)" ]; then
                printf "    ESP-IDF tools not installed — running install.sh all...\n"
                "$idf_dir/install.sh" all
            fi
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

    # Check PATH first; if not there, ask idf_tools.py for the extended tools
    # PATH (the same mechanism idf.py uses) and re-check, then fall back to a
    # recursive find under ~/.espressif/tools/.
    local qemu_bin=""
    if command -v qemu-system-xtensa &>/dev/null; then
        qemu_bin="$(command -v qemu-system-xtensa)"
    elif [ -f "$idf_tools" ]; then
        local _idf_export
        _idf_export="$(python3 "$idf_tools" export --format sh 2>/dev/null || true)"
        if [ -n "$_idf_export" ]; then
            local _saved_path="$PATH"
            eval "$_idf_export" 2>/dev/null || true
            if command -v qemu-system-xtensa &>/dev/null; then
                qemu_bin="$(command -v qemu-system-xtensa)"
            fi
            PATH="$_saved_path"
        fi
    fi
    if [ -z "$qemu_bin" ]; then
        qemu_bin="$(find "$HOME/.espressif/tools" -name "qemu-system-xtensa" -type f 2>/dev/null | head -1)"
    fi

    # Verify the binary actually executes — a stale binary from a previous
    # machine (e.g., x86_64 binary brought to an Apple Silicon machine) would
    # be found by 'find' but fail to run.  Clear it so the install path fires.
    if [ -n "$qemu_bin" ] && ! "$qemu_bin" --version &>/dev/null; then
        printf "  [WARN]    %-14s — found at %s but fails to run; will reinstall\n" "qemu-xtensa" "$qemu_bin"
        rm -rf "$(dirname "$(dirname "$qemu_bin")")" 2>/dev/null || true
        qemu_bin=""
    fi

    if [ -n "$qemu_bin" ]; then
        printf "  [OK]      %-14s %s\n" "qemu-xtensa" "$qemu_bin"
        if [[ "$UPDATE_MODE" -eq 1 ]] && [ -f "$idf_tools" ]; then
            printf "    Upgrading qemu-xtensa via idf_tools.py...\n"
            rm -f "$HOME/.espressif/dist/qemu-xtensa-"*
            python3 "$idf_tools" install qemu-xtensa || \
                echo "    WARNING: qemu-xtensa upgrade failed (the pre-built binary may not support this CPU). Emulation will be unavailable."
        fi
        return 0
    fi

    printf "  [WARN]    %-14s — not found (emulation unavailable); install with: python3 %s install qemu-xtensa\n" "qemu-xtensa" "\$IDF_PATH/tools/idf_tools.py"
    if [[ "$INSTALL_MODE" -eq 1 || "$UPDATE_MODE" -eq 1 ]]; then
        # libSDL2 is a runtime dependency of the pre-built QEMU binary.
        printf "    Installing libsdl2 (QEMU runtime dependency)...\n"
        _pkg_install sdl2 libsdl2-2.0-0
        if [ -f "$idf_tools" ]; then
            printf "    Installing qemu-xtensa via idf_tools.py...\n"
            rm -f "$HOME/.espressif/dist/qemu-xtensa-"*
            if python3 "$idf_tools" install qemu-xtensa 2>&1; then
                : # success — nothing more to do
            else
                # idf_tools.py removed the version directory after its
                # verification step failed. Common causes on macOS:
                #   1) The "x86_64" tarball actually contains an arm64 binary
                #      (confirmed Espressif packaging bug in 20260417).
                #   2) macOS quarantine blocks execution of the binary.
                # Strategy: detect the arm64-in-x86_64 case and fall back to
                # the previous release (20250817) which ships a genuine x86_64
                # binary, installed into the directory that idf_tools.py expects.
                printf "    idf_tools.py verification failed — checking binary architecture...\n"

                # Determine the version idf_tools expects for qemu-xtensa.
                local _qemu_ver
                _qemu_ver="$(python3 "$idf_tools" list 2>/dev/null \
                    | grep -A1 '^\* qemu-xtensa' | awk '/recommended/{print $2}' \
                    | tr -d '()')"
                # Fallback: parse directly from tools.json if list output differs.
                if [ -z "$_qemu_ver" ] && [ -f "$idf_dir/tools/tools.json" ]; then
                    _qemu_ver="$(python3 - <<'EOF'
import json, sys
data = json.load(open(sys.argv[1]))
for t in data.get("tools", []):
    if t.get("name") == "qemu-xtensa":
        for v in t.get("versions", []):
            if v.get("status") == "recommended":
                print(v["name"]); sys.exit(0)
EOF
                    "$idf_dir/tools/tools.json" 2>/dev/null)"
                fi

                local _qemu_dest="$espressif_qemu_dir/${_qemu_ver:-esp_develop_9.2.2_20260417}"

                # Re-extract the downloaded tarball (still in dist/) to inspect it.
                local _qemu_tarball
                _qemu_tarball="$(find "$HOME/.espressif/dist" \
                    -name "qemu-xtensa-*x86_64-apple-darwin.tar.xz" \
                    2>/dev/null | head -1)"

                if [ -n "$_qemu_tarball" ] && [ -f "$_qemu_tarball" ]; then
                    mkdir -p "$_qemu_dest"
                    tar -xJf "$_qemu_tarball" -C "$_qemu_dest" --strip-components=1

                    local _qemu_bin="$_qemu_dest/bin/qemu-system-xtensa"
                    local _actual_arch
                    _actual_arch="$(file "$_qemu_bin" 2>/dev/null)"

                    # The fallback only applies when running on Intel (x86_64) and
                    # the downloaded binary is arm64 — an Espressif packaging bug
                    # specific to certain releases.  On Apple Silicon (arm64) an
                    # arm64 binary is correct and idf_tools.py failure means
                    # something else is wrong.
                    local _host_arch
                    _host_arch="$(uname -m)"  # x86_64 on Intel, arm64 on Apple Silicon

                    if [[ "$_host_arch" == "x86_64" ]] && echo "$_actual_arch" | grep -q "arm64"; then
                        # Confirmed packaging bug: arm64 binary in x86_64 tarball.
                        # Download the last release that shipped a real x86_64 binary.
                        printf "    Detected arm64-in-x86_64 tarball (Espressif packaging bug).\n"
                        printf "    Downloading fallback release esp_develop_9.2.2_20250817...\n"
                        rm -rf "$_qemu_dest"

                        local _fallback_url="https://github.com/espressif/qemu/releases/download/esp-develop-9.2.2-20250817/qemu-xtensa-softmmu-esp_develop_9.2.2_20250817-x86_64-apple-darwin.tar.xz"
                        local _fallback_tar="$HOME/.espressif/dist/qemu-xtensa-fallback-x86_64-apple-darwin.tar.xz"
                        curl -L --progress-bar "$_fallback_url" -o "$_fallback_tar" || true

                        if [ -f "$_fallback_tar" ]; then
                            # Install missing Homebrew runtime deps for the binary.
                            printf "    Installing QEMU runtime dependencies (pixman libgcrypt glib gettext)...\n"
                            echo y | HOMEBREW_CURL_RETRIES=3 brew install pixman libgcrypt glib gettext || true

                            mkdir -p "$_qemu_dest"
                            tar -xJf "$_fallback_tar" -C "$_qemu_dest" --strip-components=1
                            xattr -dr com.apple.quarantine "$_qemu_dest" 2>/dev/null || true

                            if "$_qemu_dest/bin/qemu-system-xtensa" --version &>/dev/null; then
                                printf "    qemu-xtensa (fallback x86_64 build) installed successfully.\n"
                            else
                                rm -rf "$_qemu_dest"
                                echo "    WARNING: fallback qemu-xtensa binary failed to run. Emulation will be unavailable."
                            fi
                        else
                            echo "    WARNING: could not download fallback qemu-xtensa. Emulation will be unavailable."
                        fi
                    else
                        # Binary is the right arch — quarantine was the blocker.
                        xattr -dr com.apple.quarantine "$_qemu_dest" 2>/dev/null || true
                        if "$_qemu_bin" --version &>/dev/null; then
                            printf "    qemu-xtensa installed successfully (quarantine removed).\n"
                        else
                            rm -rf "$_qemu_dest"
                            echo "    WARNING: qemu-xtensa binary failed to run. Emulation will be unavailable."
                        fi
                    fi
                else
                    echo "    WARNING: qemu-xtensa tarball not found in dist cache. Emulation will be unavailable."
                fi
            fi
        else
            echo "    WARNING: ESP-IDF not found at $idf_dir — install ESP-IDF first."
            MISSING=1
        fi
    fi
    # QEMU is optional — its absence does not block other workflows.
}

# ── Main ────────────────────────────────────────────────────

if [[ "$INSTALL_MODE" -eq 1 || "$UPDATE_MODE" -eq 1 ]]; then
    echo ""
    echo "=== Updating package index ==="
    handle_brew  # ensure brew exists on macOS before updating index
    _pkg_update_index

    # On macOS the bundled Python ships without the system trust store, so
    # pip-system-certs / certifi must be kept up-to-date to reach HTTPS hosts.
    # On Linux the OS certificate store is managed by the distro (e.g. via
    # ca-certificates) and pip cannot install packages system-wide without
    # --break-system-packages (PEP 668), so skip this step entirely.
    if [[ "$_OS" == "macos" ]]; then
        echo ""
        echo "=== Updating pip certificates ==="
        python3 -m pip install --upgrade pip-system-certs certifi
    fi
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
