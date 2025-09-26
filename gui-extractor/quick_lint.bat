@echo off
REM Quick linting for common Qt/C++ issues

echo ====================================
echo Quick Qt/C++ Lint Check
echo ====================================
echo.

REM Run cppcheck with Qt-aware configuration
echo Running Cppcheck analysis...
echo ----------------------------
cppcheck --enable=warning,style,performance,portability ^
         --suppress=missingIncludeSystem ^
         --suppress=unmatchedSuppression ^
         --suppress=unknownMacro ^
         --inline-suppr ^
         --std=c++17 ^
         --template="{file}:{line}: [{severity}] {message}" ^
         --quiet ^
         main.cpp queryrunner.cpp promptquery.cpp modellistfetcher.cpp 2>&1 | findstr /V "information:"

echo.
echo Static Pattern Checks...
echo ------------------------

REM Check for potential memory leaks
echo Memory Management:
findstr /n "\<new\>" *.cpp | findstr /V "QNew\|newQuery\|new.*parent" | findstr /V "//" >nul 2>&1
if %errorlevel% equ 0 (
    echo   [!] Found 'new' without parent - check for memory leaks:
    findstr /n "\<new\>" *.cpp | findstr /V "QNew\|newQuery\|new.*parent" | findstr /V "//"
)

REM Check for old-style connect
echo.
echo Signal/Slot Connections:
findstr /n "SIGNAL\|SLOT" *.cpp >nul 2>&1
if %errorlevel% equ 0 (
    echo   [!] Found old-style connects - consider new syntax:
    findstr /n "SIGNAL\|SLOT" *.cpp
)

REM Check for missing override
echo.
echo Virtual Functions:
findstr /n "virtual.*;" *.h | findstr /V "override\|= 0" >nul 2>&1
if %errorlevel% equ 0 (
    echo   [!] Virtual functions missing 'override':
    findstr /n "virtual.*;" *.h | findstr /V "override\|= 0"
)

echo.
echo ====================================
echo Quick lint complete!
echo ====================================