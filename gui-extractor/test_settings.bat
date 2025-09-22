@echo off
echo Testing PDF Extractor GUI with Settings Dialog
echo ================================================
echo.

echo Test 1: Command line mode with basic extraction
release\pdfextractor_gui.exe ..\test1_lorem.pdf test_output.txt -p 1
if %errorlevel% neq 0 (
    echo [ERROR] Basic extraction failed
) else (
    echo [OK] Basic extraction completed
    type test_output.txt | findstr /c:"Lorem ipsum" >nul
    if %errorlevel% equ 0 (
        echo [OK] Text extracted successfully
    ) else (
        echo [ERROR] No text found in output
    )
)
echo.

echo Test 2: Launch GUI mode
echo Press Settings button to test the new dialog
echo Close the GUI when done testing
start /wait release\pdfextractor_gui.exe --gui

echo.
echo Testing complete!
pause