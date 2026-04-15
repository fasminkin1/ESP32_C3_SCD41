#!/bin/bash
set -e
# Only FLASH the firmware
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
WORKSPACE_ROOT="${SCRIPT_DIR}/../../.."

echo "[FLASH] Activating Environment..."
source "${WORKSPACE_ROOT}/.venv/bin/activate"

echo "[FLASH] Flashing..."
west flash

echo "[FLASH] Done!"
