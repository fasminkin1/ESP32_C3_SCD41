#!/bin/bash
# Standalone SERIAL MONITOR
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
WORKSPACE_ROOT="${SCRIPT_DIR}/../../.."

echo "[MONITOR] Activating Environment..."
source "${WORKSPACE_ROOT}/.venv/bin/activate"

echo "[MONITOR] Searching for available ports..."
echo "---------------------------------------------------"
# Corrected glob to find USB and ACM devices
ls /dev/ttyUSB* /dev/ttyACM* 2>/dev/null || echo "No USB/ACM ports found (check permissions or connection)."
echo "---------------------------------------------------"

# Try to use west monitor if possible as it's more integrated
echo "[MONITOR] Starting West Monitor (Ctrl+T Ctrl+X to exit)..."
if west espressif monitor; then
    echo "[MONITOR] Monitor finished."
else
    echo "[MONITOR] West monitor failed or cancelled. Trying fallback..."
    # Fallback to miniterm if west monitor fails
    PORT_NAME=$(ls /dev/tty[U|A]* 2>/dev/null | head -n 1)
    if [ -n "$PORT_NAME" ]; then
        echo "[MONITOR] Using fallback on $PORT_NAME..."
        python3 -m serial.tools.miniterm $PORT_NAME 115200 --raw
    else
        echo "[ERROR] No serial ports found for fallback."
    fi
fi

echo "---------------------------------------------------"
read -p "Monitoring ended. Press ENTER to close window..."
