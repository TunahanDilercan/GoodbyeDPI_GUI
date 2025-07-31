@echo off
echo =============================================
echo GoodbyeDPI Driver Test Mode Enabler
echo =============================================
echo.
echo This script will enable Windows Test Mode to allow
echo unsigned drivers like WinDivert to load.
echo.
echo CAUTION: This will require a system restart!
echo.
pause

echo Enabling test mode...
bcdedit /set testsigning on

if %errorlevel% equ 0 (
    echo.
    echo =============================================
    echo Test mode enabled successfully!
    echo.
    echo Please RESTART your computer now.
    echo After restart, GoodbyeDPI should work properly.
    echo.
    echo To disable test mode later, run:
    echo bcdedit /set testsigning off
    echo =============================================
) else (
    echo.
    echo ERROR: Failed to enable test mode!
    echo Make sure you are running as Administrator.
)

pause
