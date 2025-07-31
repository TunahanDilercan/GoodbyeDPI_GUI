@echo off
echo =============================================
echo GoodbyeDPI GUI Build Script
echo =============================================

cd src

echo Building 32-bit version...
make clean
make
if errorlevel 1 (
    echo ERROR: 32-bit build failed!
    pause
    exit /b 1
)
if not exist "32_exe\goodbyedpi.exe" (
    echo ERROR: 32-bit executable not found!
    pause
    exit /b 1
)

echo.
echo Building 64-bit version...
make clean
make BIT64=1
if errorlevel 1 (
    echo ERROR: 64-bit build failed!
    pause
    exit /b 1
)
if not exist "64_exe\goodbyedpi.exe" (
    echo ERROR: 64-bit executable not found!
    pause
    exit /b 1
)

echo.
echo =============================================
echo Build completed successfully!
echo 32-bit executable: src\32_exe\goodbyedpi.exe
echo 64-bit executable: src\64_exe\goodbyedpi.exe
echo =============================================

:: WinDivert dosyalarını kopyala
echo Copying WinDivert files...
copy /Y ..\binary\x86\*.dll .\32_exe\
copy /Y ..\binary\x86\*.sys .\32_exe\
copy /Y ..\binary\amd64\*.dll .\64_exe\
copy /Y ..\binary\amd64\*.sys .\64_exe\

echo.
echo All builds completed!
pause
