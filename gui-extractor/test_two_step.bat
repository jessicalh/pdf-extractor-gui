@echo off
echo Testing Two-Step PDF Extraction and Analysis
echo ==============================================
echo.
echo This GUI now has a two-step process:
echo 1. Extract Text - Extracts text from PDF or allows pasting
echo 2. Analyze Text - Generates summaries and keywords
echo.
echo The extracted text tab is now editable, so you can:
echo - Extract from PDF using "Extract Text" button
echo - Or paste text from web/OCR/other sources
echo - Then click "Analyze Text" to generate AI summaries/keywords
echo.
echo Launching GUI...
start release\pdfextractor_gui.exe --gui
echo.
echo Test scenarios:
echo 1. Select a PDF and click "Extract Text"
echo 2. After extraction, click "Analyze Text"
echo 3. Or clear the text box, paste any text, then "Analyze Text"
echo.
pause