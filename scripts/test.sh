#!/usr/bin/env bash
# Build and run host-native unit tests using Conan + CMake + Catch2.
set -euo pipefail

# shellcheck source=scripts/_common.sh
source "$(dirname "$0")/_common.sh"
_ensure_pip_bin

TESTS_DIR="$(cd "$(dirname "$0")/.." && pwd)/tests"
cd "$TESTS_DIR"

echo "=== [1/4] Installing Conan dependencies ==="
conan install . --output-folder=build/debug --build=missing -s build_type=Debug

echo ""
echo "=== [2/4] Configuring CMake ==="
cmake --preset native-debug

echo ""
echo "=== [3/4] Building tests ==="
cmake --build --preset native-debug

echo ""
echo "=== [4/4] Running tests ==="
ctest --preset native-debug --output-on-failure
