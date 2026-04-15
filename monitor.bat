@echo off
set WORKSPACE_ROOT=..\..\..
call %WORKSPACE_ROOT%\.venv\Scripts\activate.bat

echo [MONITOR] Starting Automatic Monitor...
echo Press any key to enter MANUAL mode or wait for AUTO...

set "PORT_NAME="
choice /c YN /t 3 /d Y /n /m "Auto-detecting... "
if %errorlevel% equ 2 (
    echo Available ports:
    python -m serial.tools.list_ports
    set /p PORT_NAME="Enter COM port (e.g. 30): "
)

if "%PORT_NAME%"=="" (
    west espressif monitor
) else (
    REM Check if numeric and prepend COM
    echo %PORT_NAME%| findstr /r "^[0-9][0-9]*$" >nul
    if %errorlevel% equ 0 set PORT_NAME=COM%PORT_NAME%
    python -m serial.tools.miniterm %PORT_NAME% 115200 --raw
)
pause
