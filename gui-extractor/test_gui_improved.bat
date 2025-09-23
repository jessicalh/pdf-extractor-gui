@echo off
echo Testing Improved GUI Layout...
echo.
echo Starting GUI...

cd release
start pdfextractor_gui.exe

echo.
echo GUI should now be running with:
echo - Top-level tabs for Input and Output
echo - Full window space utilization
echo - Settings button in toolbar
echo - Better organized layout
echo.
echo Press any key to close this window (GUI will remain open)
pause > nul