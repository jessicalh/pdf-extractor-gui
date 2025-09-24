@echo off
echo Building PDF Extractor GUI - Console Version (for debugging)
echo ===========================================================

REM Clean previous builds
if exist test_release rmdir /s /q test_release
if exist build rmdir /s /q build
if exist Makefile del Makefile*

REM Run qmake with console_build CONFIG
C:\Qt\6.10.0\llvm-mingw_64\bin\qmake.exe CONFIG+=console_build CONFIG+=release

REM Build
mingw32-make clean
mingw32-make -j4

if %errorlevel% equ 0 (
    echo.
    echo Build successful!
    echo Executable: test_release\pdfextractor_gui_console.exe

    REM Copy required DLLs and resources
    echo Copying dependencies...
    if not exist test_release mkdir test_release
    xcopy /y release\*.dll test_release\ >nul 2>&1
    xcopy /y /e release\platforms test_release\platforms\ >nul 2>&1
    xcopy /y /e release\styles test_release\styles\ >nul 2>&1
    xcopy /y /e release\imageformats test_release\imageformats\ >nul 2>&1
    xcopy /y /e release\sqldrivers test_release\sqldrivers\ >nul 2>&1

    echo Console build complete!
) else (
    echo Build failed!
    exit /b 1
)