@echo off
set WORKSPACE_ROOT=..\..\..
call %WORKSPACE_ROOT%\.venv\Scripts\activate.bat

echo [ALL] Starting Automatic Flow...
echo [1/3] Building...
west build -b esp32c3_042_oled --pristine
if %ERRORLEVEL% NEQ 0 ( pause & exit /b )

echo [2/3] Flashing...
west flash
if %ERRORLEVEL% NEQ 0 ( pause & exit /b )

echo [3/3] Opening Monitor (AUTO)...
west espressif monitor
