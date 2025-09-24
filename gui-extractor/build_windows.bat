@echo off
echo Building PDF Extractor GUI - Windows GUI Version (no console)
echo ==============================================================

REM Clean previous builds
if exist build rmdir /s /q build
if exist Makefile del Makefile*

REM Run qmake for Windows GUI build (default)
C:\Qt\6.10.0\llvm-mingw_64\bin\qmake.exe CONFIG+=release

REM Build
mingw32-make clean
mingw32-make -j4

if %errorlevel% equ 0 (
    echo.
    echo Build successful!
    echo Executable: release\pdfextractor_gui.exe

    REM Ensure all Qt dependencies are present
    echo Deploying Qt dependencies...
    C:\Qt\6.10.0\llvm-mingw_64\bin\windeployqt.exe release\pdfextractor_gui.exe

    echo Windows GUI build complete!
    echo This version will run without a console window.
) else (
    echo Build failed!
    exit /b 1
)