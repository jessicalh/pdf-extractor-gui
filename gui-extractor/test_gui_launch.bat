@echo off
echo Testing GUI Launch...
echo.

cd release
start /B pdfextractor_gui.exe

timeout /t 3 /nobreak > nul

tasklist /FI "IMAGENAME eq pdfextractor_gui.exe" 2>nul | find /I "pdfextractor_gui.exe" > nul
if %ERRORLEVEL% == 0 (
    echo SUCCESS: GUI is running
    echo.
    echo Terminating GUI for cleanup...
    taskkill /F /IM pdfextractor_gui.exe > nul 2>&1
) else (
    echo ERROR: GUI failed to launch
)

cd ..