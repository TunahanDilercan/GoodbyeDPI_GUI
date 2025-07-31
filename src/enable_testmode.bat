@echo off
echo Test modunu etkinlestiriyor...
echo Bu islem yonetici yetkileri gerektirir.
echo.

:: Test Signing mode'u ac
bcdedit /set testsigning on

if %errorlevel% equ 0 (
    echo.
    echo ✅ Test modu basariyla etkinlestirildi!
    echo.
    echo ONEMLI: Bilgisayarinizi yeniden baslatmaniz gerekiyor.
    echo Yeniden baslatmak istiyor musunuz? [Y/N]
    set /p choice=
    if /i "%choice%"=="Y" (
        echo Bilgisayar yeniden baslatiliyor...
        shutdown /r /t 5
    ) else (
        echo Manuel olarak yeniden baslatmayi unutmayin!
    )
) else (
    echo.
    echo ❌ HATA: Test modu etkinlestirilemedi!
    echo Bu komutu "Yonetici olarak calistir" ile calistirmaniz gerekiyor.
    echo.
    echo Saglik tik yapip "Yonetici olarak calistir" secin.
)
echo.
pause
