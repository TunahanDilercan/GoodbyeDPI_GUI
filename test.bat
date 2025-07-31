@echo off
echo =============================================
echo GoodbyeDPI GUI Test Script
echo =============================================

echo Testing 32-bit version...
echo.
cd src\32_exe
if exist goodbyedpi.exe (
    echo 32-bit executable found: goodbyedpi.exe (%~z1 bytes)
    echo File info:
    dir goodbyedpi.exe
) else (
    echo ERROR: 32-bit executable not found!
)

echo.
echo Testing 64-bit version...
cd ..\64_exe
if exist goodbyedpi.exe (
    echo 64-bit executable found: goodbyedpi.exe (%~z1 bytes)
    echo File info:
    dir goodbyedpi.exe
) else (
    echo ERROR: 64-bit executable not found!
)

cd ..\..

echo.
echo =============================================
echo Test completed!
echo.
echo To run the applications:
echo 32-bit: src\32_exe\goodbyedpi.exe
echo 64-bit: src\64_exe\goodbyedpi.exe
echo.
echo Note: Run as Administrator for proper operation!
echo =============================================
pause
