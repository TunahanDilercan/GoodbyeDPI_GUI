@echo off
echo Testing GoodbyeDPI v2.3.9 - 64bit...
cd /d "d:\FilesHan\kod\GUI\last\GoodbyeDPI_GUI\src\64_exe"
echo Starting program...
goodbyedpi_new.exe -4 --dns-addr 77.88.8.8 --dns-port 1253 --dnsv6-addr 2a02:6b8::feed:0ff --dnsv6-port 1253
echo Exit code: %ERRORLEVEL%
echo Program ended.
echo.
echo === Log file contents ===
if exist goodbyedpi_log.txt (
    type goodbyedpi_log.txt
) else (
    echo No log file found
)
echo.
echo === Error file contents ===
if exist goodbyedpi_error.log (
    type goodbyedpi_error.log
) else (
    echo No error file found
)
echo.
pause
