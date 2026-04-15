@echo off
set WORKSPACE_ROOT=..\..\..
echo [FLASH] Activating Environment...
call %WORKSPACE_ROOT%\.venv\Scripts\activate.bat
echo [FLASH] Flashing...
west flash
pause
