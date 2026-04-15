@echo off
set WORKSPACE_ROOT=..\..\..
echo [BUILD] Activating Environment...
call %WORKSPACE_ROOT%\.venv\Scripts\activate.bat
echo [BUILD] Building...
west build -b esp32c3_042_oled --pristine
pause
