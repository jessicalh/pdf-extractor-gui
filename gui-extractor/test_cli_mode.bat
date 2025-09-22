@echo off
echo Testing CLI mode with verbose output
echo =====================================
echo.

echo Extracting text from paper1_ai.pdf...
release\pdfextractor_gui.exe ..\paper1_ai.pdf test_output.txt -p 1-3 --verbose

echo.
echo Checking output file...
if exist test_output.txt (
    echo [OK] Text file created
    echo First 500 characters:
    powershell -Command "Get-Content test_output.txt -TotalCount 10"
) else (
    echo [ERROR] No output file created
)

echo.
pause