#!/bin/bash
set -e
# Fully Automatic Flow
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
WORKSPACE_ROOT="${SCRIPT_DIR}/../../.."

source "${WORKSPACE_ROOT}/.venv/bin/activate"

echo "[ALL] Building..."
west build -b esp32c3_042_oled --pristine

echo "[ALL] Flashing..."
west flash

echo "[ALL] Opening Monitor..."
# Call the monitor script which has persistence and fallback logic
bash "${SCRIPT_DIR}/monitor.sh"
