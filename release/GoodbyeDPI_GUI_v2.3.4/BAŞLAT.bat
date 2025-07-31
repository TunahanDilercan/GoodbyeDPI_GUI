@echo off
echo ==========================================
echo    GoodbyeDPI GUI v2.3.4 - Başlatıcı
echo ==========================================
echo.

REM Admin kontrolü
net session >nul 2>&1
if %errorLevel% == 0 (
    echo [✓] Yönetici yetkileri tespit edildi
    goto :start
) else (
    echo [!] Bu program yönetici yetkileri gerektirir!
    echo [!] Lütfen bu dosyayı "Yönetici olarak çalıştır" seçeneği ile açın
    echo.
    echo     1. Bu dosyaya sağ tıklayın
    echo     2. "Yönetici olarak çalıştır" seçeneğini seçin
    echo.
    pause
    exit /b 1
)

:start
echo [i] GoodbyeDPI GUI başlatılıyor...
echo [i] İlk çalıştırmada test modu otomatik etkinleştirilecek
echo [i] Sistem tepsisinde ikon görünecek
echo.

REM Test modu kontrolü
bcdedit /enum | findstr /i "testsigning" | findstr /i "yes" >nul
if %errorLevel% == 0 (
    echo [✓] Test modu zaten etkin
) else (
    echo [!] Test modu etkinleştiriliyor...
    bcdedit /set testsigning on
    if %errorLevel__ == 0 (
        echo [✓] Test modu etkinleştirildi
        echo [!] Sistem yeniden başlatıldıktan sonra tam olarak aktif olacak
    ) else (
        echo [X] Test modu etkinleştirilemedi
        echo [!] Manuel olarak etkinleştirin: bcdedit /set testsigning on
    )
)

echo.
echo [i] Uygulama başlatılıyor...
start "" "goodbyedpi_gui.exe"

echo [✓] GoodbyeDPI GUI başlatıldı
echo [i] Sistem tepsisindeki ikona sağ tıklayarak script seçebilirsiniz
echo [i] Varsayılan olarak [TR] Alternative 4 seçili gelecek
echo.
echo Kullanım:
echo - Sistem tepsisindeki ikona SAĞ tık
echo - Script menüsünden birini seçin (ör: [TR] Alternative 4)
echo - Script otomatik başlayacak
echo - Stop ile durdurabilirsiniz
echo.
pause
