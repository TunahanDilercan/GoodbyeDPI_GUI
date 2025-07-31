@echo off
echo =============================================
echo GoodbyeDPI Driver Fix Script
echo =============================================
echo.
echo Cleaning up any existing WinDivert services...

sc stop WinDivert 2>nul
sc delete WinDivert 2>nul
sc stop windivert14 2>nul  
sc delete windivert14 2>nul

echo.
echo Checking for leftover driver files...
del /q "%WINDIR%\System32\drivers\WinDivert*.sys" 2>nul
del /q "%WINDIR%\SysWOW64\WinDivert*.dll" 2>nul
del /q "%WINDIR%\System32\WinDivert*.dll" 2>nul

echo.
echo Cleanup completed. Now try running GoodbyeDPI again.
echo.
echo If it still doesn't work, you may need to:
echo 1. Enable Windows Test Mode (run enable_testmode.bat)
echo 2. Or disable driver signature enforcement temporarily
echo.
pause
