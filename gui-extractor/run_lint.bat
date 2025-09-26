@echo off
REM Lint script for PDF Extractor GUI
REM Runs clang-tidy with Qt-specific checks

echo ========================================
echo Running Clang-Tidy Qt Linting
echo ========================================

set QT_PATH=C:\Qt\6.10.0\llvm-mingw_64
set QT_INCLUDES=-I%QT_PATH%\include -I%QT_PATH%\include\QtCore -I%QT_PATH%\include\QtWidgets -I%QT_PATH%\include\QtGui -I%QT_PATH%\include\QtNetwork -I%QT_PATH%\include\QtSql -I%QT_PATH%\include\QtPdf

REM Check if clang-tidy exists
where clang-tidy >nul 2>nul
if %errorlevel% neq 0 (
    echo ERROR: clang-tidy not found in PATH
    echo Please install LLVM/Clang tools
    echo Download from: https://releases.llvm.org/
    exit /b 1
)

echo.
echo Checking main.cpp...
clang-tidy main.cpp -- %QT_INCLUDES% -std=c++17 -DUNICODE -DQT_WIDGETS_LIB -DQT_GUI_LIB -DQT_CORE_LIB

echo.
echo Checking queryrunner.cpp...
clang-tidy queryrunner.cpp -- %QT_INCLUDES% -std=c++17 -DUNICODE -DQT_WIDGETS_LIB -DQT_GUI_LIB -DQT_CORE_LIB

echo.
echo Checking promptquery.cpp...
clang-tidy promptquery.cpp -- %QT_INCLUDES% -std=c++17 -DUNICODE -DQT_WIDGETS_LIB -DQT_GUI_LIB -DQT_CORE_LIB

echo.
echo Checking headers...
clang-tidy queryrunner.h promptquery.h -- %QT_INCLUDES% -std=c++17 -DUNICODE -DQT_WIDGETS_LIB -DQT_GUI_LIB -DQT_CORE_LIB

echo.
echo ========================================
echo Linting complete!
echo ========================================