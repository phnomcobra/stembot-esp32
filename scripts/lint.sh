#!/usr/bin/env bash
# ===========================================================
# lint.sh — Run clang-format and clang-tidy across all sources
#
# Usage:
#   ./scripts/lint.sh          # check only (exit 1 on violations)
#   ./scripts/lint.sh --fix    # auto-format files in-place
# ===========================================================
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
FIX_MODE=0

while [[ $# -gt 0 ]]; do
    case "$1" in
        --fix) FIX_MODE=1; shift ;;
        *) echo "Unknown argument: $1"; echo "Usage: $0 [--fix]"; exit 1 ;;
    esac
done

# Collect all project sources, excluding build artefacts
SOURCES=$(find "$REPO_ROOT/components" "$REPO_ROOT/main" "$REPO_ROOT/tests" \
    -not -path '*/build/*' \
    \( -name '*.cpp' -o -name '*.h' \) | sort)

# ── 1. clang-format ────────────────────────────────────────
echo "=== [1/2] clang-format ==="

# shellcheck source=scripts/_common.sh
source "$(dirname "$0")/_common.sh"
_ensure_pip_bin

if ! command -v clang-format &>/dev/null; then
    echo "ERROR: clang-format not found."
    echo "  $(_llvm_install_hint)"
    exit 1
fi

if [[ "$FIX_MODE" -eq 1 ]]; then
    echo "$SOURCES" | xargs clang-format -i
    echo "Files formatted in-place."
else
    FORMAT_FAIL=0
    while IFS= read -r file; do
        if ! clang-format --dry-run --Werror "$file" 2>/dev/null; then
            echo "  NOT FORMATTED: ${file#"$REPO_ROOT/"}"
            FORMAT_FAIL=1
        fi
    done <<< "$SOURCES"

    if [[ "$FORMAT_FAIL" -eq 1 ]]; then
        echo ""
        echo "Formatting violations found. Run './scripts/lint.sh --fix' to auto-correct."
        exit 1
    fi
    echo "All files pass formatting checks."
fi

# ── 2. clang-tidy (via CMake native-lint preset) ───────────
echo ""
echo "=== [2/2] clang-tidy ==="

if ! command -v clang-tidy &>/dev/null; then
    echo "ERROR: clang-tidy not found."
    echo "  $(_llvm_install_hint)"
    exit 1
fi

cd "$REPO_ROOT/tests"

echo "Installing Conan dependencies for lint build..."
# Remove the auto-generated user presets file so Conan regenerates it fresh
# for only this output folder, avoiding duplicate preset name conflicts.
rm -f CMakeUserPresets.json
conan install . --output-folder=build/lint --build=missing -s build_type=Debug

echo "Configuring CMake (lint preset)..."
cmake --preset native-lint

echo "Building with clang-tidy..."
cmake --build --preset native-lint

echo "Static analysis passed."
