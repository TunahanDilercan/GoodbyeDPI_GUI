@echo off
echo ========================================
echo   GoodbyeDPI - Driver Signature Fix
echo ========================================
echo.
echo Bu script Windows driver signature enforcement'i devre disi birakir.
echo Bu islem yonetici yetkileri gerektirir ve yeniden baslatma gereklidir.
echo.

:: Check if running as administrator
net session >nul 2>&1
if %errorlevel% neq 0 (
    echo HATA: Bu script yonetici olarak calistirilmalidir!
    echo Sag tik -> "Yonetici olarak calistir" secin.
    echo.
    pause
    exit /b 1
)

echo 1. Test Signing modunu etkinlestiriliyor...
bcdedit /set testsigning on
if %errorlevel% neq 0 (
    echo HATA: Test signing etkinlestirilemedi!
    pause
    exit /b 1
)

echo.
echo 2. Driver signature enforcement devre disi birakiliyor...
bcdedit /set nointegritychecks on
if %errorlevel% neq 0 (
    echo HATA: Integrity checks devre disi birakilamadi!
    pause
    exit /b 1
)

echo.
echo 3. Load Options ayarlaniyor...
bcdedit /set loadoptions DISABLE_INTEGRITY_CHECKS
if %errorlevel% neq 0 (
    echo UYARI: Load options ayarlanamadi (bu normal olabilir)
)

echo.
echo ========================================
echo   BASARILI! Ayarlar tamamlandi.
echo ========================================
echo.
echo ONEMLI: Bilgisayarinizi YENIDEN BASLATMANIZ gerekiyor.
echo.
echo Simdi yeniden baslatmak istiyor musunuz? [Y/N]
set /p choice="Seciminiz: "

if /i "%choice%"=="Y" (
    echo.
    echo Bilgisayar 10 saniye sonra yeniden baslatilacak...
    shutdown /r /t 10
    echo Yeniden baslatmayi iptal etmek icin: shutdown /a
) else (
    echo.
    echo Manuel olarak yeniden baslatmayi unutmayin!
    echo Yeniden baslatmadan sonra GoodbyeDPI calismaya baslayacak.
)

echo.
pause
