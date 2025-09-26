@echo off
REM Comprehensive linting script for Qt project
REM Runs multiple linters to catch different issues

setlocal enabledelayedexpansion

echo ================================================
echo Qt Project Linting Suite
echo ================================================
echo.

REM Set paths
set QT_PATH=C:\Qt\6.10.0\llvm-mingw_64
set PROJECT_PATH=%cd%

REM Check for available linters
set FOUND_LINTERS=0

REM 1. Check for clazy (Qt-specific linter)
where clazy-standalone >nul 2>nul
if %errorlevel% equ 0 (
    echo [✓] Clazy found - Running Qt-specific checks...
    set FOUND_LINTERS=1
    echo ----------------------------------------
    clazy-standalone main.cpp queryrunner.cpp promptquery.cpp ^
        -I%QT_PATH%\include ^
        -I%QT_PATH%\include\QtCore ^
        -I%QT_PATH%\include\QtWidgets ^
        -I%QT_PATH%\include\QtGui ^
        -I%QT_PATH%\include\QtNetwork ^
        -I%QT_PATH%\include\QtSql ^
        -I%QT_PATH%\include\QtPdf ^
        --checks=level0,level1,no-qstring-allocations,container-anti-pattern ^
        2>&1 | findstr /V "Clazy" | findstr /V "^$"
    echo.
) else (
    echo [!] Clazy not found (install from: https://github.com/KDE/clazy)
)

REM 2. Check for clang-tidy
where clang-tidy >nul 2>nul
if %errorlevel% equ 0 (
    echo [✓] Clang-tidy found - Running modern C++ checks...
    set FOUND_LINTERS=1
    echo ----------------------------------------

    REM Create compilation database if needed
    if not exist compile_commands.json (
        echo {"directory":"%cd%","command":"clang++ -c main.cpp","file":"main.cpp"} > compile_commands.json
    )

    clang-tidy main.cpp queryrunner.cpp promptquery.cpp ^
        --config-file=.clang-tidy ^
        -- -I%QT_PATH%\include ^
        -I%QT_PATH%\include\QtCore ^
        -I%QT_PATH%\include\QtWidgets ^
        -I%QT_PATH%\include\QtGui ^
        -I%QT_PATH%\include\QtNetwork ^
        -I%QT_PATH%\include\QtSql ^
        -I%QT_PATH%\include\QtPdf ^
        -std=c++17 -DUNICODE 2>&1 | findstr /V "^$"
    echo.
) else (
    echo [!] Clang-tidy not found (install LLVM from: https://releases.llvm.org/)
)

REM 3. Check for cppcheck
where cppcheck >nul 2>nul
if %errorlevel% equ 0 (
    echo [✓] Cppcheck found - Running static analysis...
    set FOUND_LINTERS=1
    echo ----------------------------------------
    cppcheck --enable=all ^
        --suppress=missingIncludeSystem ^
        --suppress=unmatchedSuppression ^
        --inline-suppr ^
        -I%QT_PATH%\include ^
        --std=c++17 ^
        --template="{file}:{line}: {severity}: {message}" ^
        main.cpp queryrunner.cpp promptquery.cpp modellistfetcher.cpp 2>&1
    echo.
) else (
    echo [!] Cppcheck not found (install from: http://cppcheck.sourceforge.net/)
)

REM 4. Basic Qt pattern checks with findstr
echo [✓] Running basic Qt pattern checks...
echo ----------------------------------------

REM Check for common Qt issues
echo Checking for potential issues:

REM Check for missing Q_OBJECT in QObject-derived classes
findstr /n "class.*:.*public QObject" *.h >nul
if %errorlevel% equ 0 (
    echo - Checking Q_OBJECT macro in QObject classes...
    for %%f in (*.h) do (
        findstr "class.*:.*public QObject" %%f >nul
        if !errorlevel! equ 0 (
            findstr "Q_OBJECT" %%f >nul
            if !errorlevel! neq 0 (
                echo   WARNING: %%f may be missing Q_OBJECT macro
            )
        )
    )
)

REM Check for new/delete instead of Qt parent ownership
echo - Checking for manual memory management...
findstr /n "\<delete\>" *.cpp | findstr /V "deleteLater" >temp_deletes.txt
for /f "tokens=*" %%i in (temp_deletes.txt) do echo   NOTE: %%i
del temp_deletes.txt >nul 2>nul

REM Check for connect statements without proper syntax
echo - Checking signal/slot connections...
findstr /n "connect(" *.cpp | findstr /V "&" >temp_connects.txt
for /f "tokens=*" %%i in (temp_connects.txt) do echo   CHECK: %%i (ensure new connect syntax)
del temp_connects.txt >nul 2>nul

echo.

REM Summary
if %FOUND_LINTERS% equ 0 (
    echo ================================================
    echo WARNING: No advanced linters found!
    echo.
    echo Recommended tools to install:
    echo   1. Clazy - Qt-specific static analyzer
    echo      https://github.com/KDE/clazy
    echo   2. Clang-tidy - Modern C++ linter
    echo      https://releases.llvm.org/
    echo   3. Cppcheck - General static analysis
    echo      http://cppcheck.sourceforge.net/
    echo ================================================
) else (
    echo ================================================
    echo Linting complete!
    echo Review any warnings or issues above.
    echo ================================================
)

endlocal