@echo off
echo ================================================================================
echo                     PDF EXTRACTOR GUI - TEST SUITE
echo ================================================================================
echo.

echo [TEST 1] Command Line Mode - Basic extraction
echo --------------------------------------------------------------------------------
release\pdfextractor_gui.exe paper2_bert.pdf cli_test1.txt -p 1-2
echo.
timeout /t 2 >nul

echo [TEST 2] Command Line Mode - With AI features
echo --------------------------------------------------------------------------------
release\pdfextractor_gui.exe paper2_bert.pdf cli_test2.txt -p 1-3 --config lmstudio_config.toml --summary cli_test2_summary.txt --keywords cli_test2_keywords.txt
echo.
timeout /t 2 >nul

echo [TEST 3] Command Line Mode - Verbose with all features
echo --------------------------------------------------------------------------------
release\pdfextractor_gui.exe paper1_ai.pdf cli_test3.txt -p 1-3 --config lmstudio_config.toml --summary cli_test3_summary.txt --keywords cli_test3_keywords.txt --verbose
echo.
timeout /t 2 >nul

echo [TEST 4] GUI Mode - Launch interactive window
echo --------------------------------------------------------------------------------
echo Launching GUI window... (close window when done testing)
start release\pdfextractor_gui.exe --gui
echo.

echo ================================================================================
echo                              TEST SUITE COMPLETE
echo ================================================================================
echo.
echo Command line outputs created:
echo   - cli_test1.txt (basic extraction)
echo   - cli_test2.txt, cli_test2_summary.txt, cli_test2_keywords.txt (with AI)
echo   - cli_test3.txt, cli_test3_summary.txt, cli_test3_keywords.txt (verbose)
echo.
echo GUI window launched for interactive testing.
echo ================================================================================
pause