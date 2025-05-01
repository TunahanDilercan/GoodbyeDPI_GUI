@ECHO OFF
REM Sessiz çalışma için kullanıcı mesajları kaldırıldı
sc stop "GoodbyeDPI" >nul 2>&1
sc delete "GoodbyeDPI" >nul 2>&1
sc stop "WinDivert1.4" >nul 2>&1
sc delete "WinDivert1.4" >nul 2>&1
