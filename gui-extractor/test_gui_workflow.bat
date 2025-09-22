@echo off
echo ============================================
echo PDF Extractor GUI - Two-Step Workflow Test
echo ============================================
echo.
echo The GUI is now running with the following features:
echo.
echo 1. TWO-STEP PROCESS:
echo    - Extract Text: Extracts text from PDF only
echo    - Analyze Text: Generates AI summaries/keywords
echo.
echo 2. FLEXIBLE INPUT:
echo    - Select PDF and click "Extract Text"
echo    - OR paste text directly from web/OCR
echo    - Then click "Analyze Text"
echo.
echo 3. SETTINGS DIALOG:
echo    - Click "Settings..." button
echo    - All TOML values are loaded
echo    - Modify prompts, temperature, tokens, etc.
echo.
echo 4. TEST WORKFLOW:
echo    Step 1: Browse and select a PDF (e.g., paper1_ai.pdf)
echo    Step 2: Click "Extract Text" (green button)
echo    Step 3: Review/edit extracted text if needed
echo    Step 4: Click "Analyze Text" (orange button)
echo    Step 5: View results in Summary and Keywords tabs
echo.
echo The GUI should already be running.
echo If not, launching it now...
echo.
if not exist release\pdfextractor_gui.exe (
    echo ERROR: GUI executable not found!
    pause
    exit /b 1
)

tasklist /FI "IMAGENAME eq pdfextractor_gui.exe" 2>NUL | find /I /N "pdfextractor_gui.exe">NUL
if "%ERRORLEVEL%"=="1" (
    echo Starting GUI...
    start release\pdfextractor_gui.exe --gui
) else (
    echo GUI is already running!
)

echo.
echo Press any key to exit...
pause >nul