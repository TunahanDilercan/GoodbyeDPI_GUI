@echo off
echo ==========================================
echo         Test Modu Etkinleştirici
echo ==========================================
echo.

REM Admin kontrolü
net session >nul 2>&1
if %errorLevel__ == 0 (
    echo [✓] Yönetici yetkileri tespit edildi
) else (
    echo [!] Bu dosyayı "Yönetici olarak çalıştır" ile açın!
    pause
    exit /b 1
)

echo [i] Windows Test Modu etkinleştiriliyor...
echo [i] Bu işlem WinDivert driver'ının çalışması için gereklidir
echo.

bcdedit /set testsigning on

if %errorLevel__ == 0 (
    echo [✓] Test modu başarıyla etkinleştirildi!
    echo.
    echo [!] Değişiklikler için sistemi YENIDEN BAŞLATMANIZ gerekiyor
    echo.
    echo Yeniden başlatmak istiyor musunuz? (E/H)
    choice /c EH /n /m "E=Evet, H=Hayır: "
    if errorlevel 2 goto :end
    if errorlevel 1 goto :restart
) else (
    echo [X] Test modu etkinleştirilemedi!
    echo [!] Manuel olarak deneyin: bcdedit /set testsigning on
    goto :end
)

:restart
echo.
echo [i] Sistem 10 saniye içinde yeniden başlatılacak...
echo [i] Dosyalarınızı kaydettiğinizden emin olun!
echo.
timeout /t 10
shutdown /r /f /t 0
goto :end

:end
echo.
echo Test modunu devre dışı bırakmak için:
echo bcdedit /set testsigning off
echo.
pause
