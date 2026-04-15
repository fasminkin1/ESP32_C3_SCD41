#!/bin/bash
set -e
# Only BUILD the project
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
WORKSPACE_ROOT="${SCRIPT_DIR}/../../.."

echo "[BUILD] Activating Environment..."
source "${WORKSPACE_ROOT}/.venv/bin/activate"

echo "[BUILD] Building..."
west build -b esp32c3_042_oled --pristine

echo "[BUILD] Success!"
