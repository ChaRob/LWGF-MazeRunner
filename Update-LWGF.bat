@echo off
cd /d "%~dp0"

powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0Update-LWGF.ps1"

if errorlevel 1 (
    echo.
    echo LWGF update failed.
    pause
    exit /b 1
)

echo.
echo LWGF update completed.
pause