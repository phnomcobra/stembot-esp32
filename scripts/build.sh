#!/usr/bin/env bash
# Build ESP32 firmware via ESP-IDF.
# The ESP-IDF environment is activated automatically if not already sourced.
set -euo pipefail

# shellcheck source=scripts/_common.sh
source "$(dirname "$0")/_common.sh"
_ensure_idf_env

cd "$(dirname "$0")/.."}

echo "=== Building ESP32 firmware ==="
idf.py build
