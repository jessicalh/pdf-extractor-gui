@echo off
REM ===================================================
REM PDF Extractor GUI - Windows Build Environment Setup
REM ===================================================
REM
REM This sets up environment variables for the build system.
REM Run this before using make if Qt is not in the default location.
REM
REM Usage: setup_env.bat
REM Then:  make release-console

echo Setting up PDF Extractor GUI build environment...

REM Qt Installation Settings
REM Modify these if Qt is installed in a different location
set QT_VERSION=6.10.0
set QT_COMPILER=llvm-mingw_64
set QT_BASE=C:\Qt\%QT_VERSION%\%QT_COMPILER%

REM Build tool paths (usually don't need to change)
set QMAKE=%QT_BASE%\bin\qmake.exe
set MAKE=mingw32-make
set DEPLOY_TOOL=%QT_BASE%\bin\windeployqt.exe
set DEPLOY_FLAGS=--no-translations

REM Verify Qt installation
if not exist "%QMAKE%" (
    echo ERROR: Qt not found at %QT_BASE%
    echo Please edit setup_env.bat to set the correct Qt path
    exit /b 1
)

echo.
echo Environment configured:
echo   Qt Version:  %QT_VERSION%
echo   Qt Compiler: %QT_COMPILER%
echo   Qt Base:     %QT_BASE%
echo   QMake:       %QMAKE%
echo.
echo You can now run:
echo   make               - Build release Windows GUI
echo   make release-console - Build with console for debugging
echo   make clean         - Clean all build artifacts
echo   make help          - Show all targets
echo.