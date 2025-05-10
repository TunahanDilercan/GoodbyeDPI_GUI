@ECHO OFF
REM Sessiz çalışma için kullanıcı mesajları kaldırıldı
ECHO Stopping and removing GoodbyeDPI services...
sc query "GoodbyeDPI" >nul 2>&1
IF %ERRORLEVEL% EQU 0 (
    sc stop "GoodbyeDPI" >nul 2>&1
    sc delete "GoodbyeDPI" >nul 2>&1
    ECHO GoodbyeDPI service removed.
) ELSE (
    ECHO GoodbyeDPI service not found.
)

sc query "WinDivert1.4" >nul 2>&1
IF %ERRORLEVEL% EQU 0 (
    sc stop "WinDivert1.4" >nul 2>&1
    sc delete "WinDivert1.4" >nul 2>&1
    ECHO WinDivert1.4 service removed.
) ELSE (
    ECHO WinDivert1.4 service not found.
)
