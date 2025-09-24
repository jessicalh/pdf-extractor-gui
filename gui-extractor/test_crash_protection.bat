@echo off
echo Testing PDF Extractor GUI Crash Protection
echo ==========================================
echo.

cd release

echo 1. Testing from release directory (should work)...
echo Starting application...
start /B pdfextractor_gui.exe
timeout /t 3 /nobreak > nul

echo Checking if application is running...
tasklist | findstr /i pdfextractor_gui.exe > nul
if %errorlevel% equ 0 (
    echo SUCCESS: Application started normally from release directory
    taskkill /F /IM pdfextractor_gui.exe > nul 2>&1
) else (
    echo FAILED: Application did not start from release directory
)

echo.
echo 2. Testing from parent directory with full path...
cd ..
start /B "%CD%\release\pdfextractor_gui.exe"
timeout /t 3 /nobreak > nul

tasklist | findstr /i pdfextractor_gui.exe > nul
if %errorlevel% equ 0 (
    echo SUCCESS: Application started with full path
    taskkill /F /IM pdfextractor_gui.exe > nul 2>&1
) else (
    echo FAILED: Application did not start with full path
)

echo.
echo 3. Checking database location...
if exist release\settings.db (
    echo SUCCESS: Database found in release directory
) else (
    echo FAILED: Database not found in release directory
)

echo.
echo 4. Checking logs directory...
if exist release\logs (
    echo SUCCESS: Logs directory exists
) else (
    echo INFO: Logs directory will be created on first crash
)

echo.
echo Test complete!
pause