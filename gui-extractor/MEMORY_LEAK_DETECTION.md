# Memory Leak Detection Tools for Qt/C++ on Windows

## Recommended Tools

### 1. **Visual Studio Diagnostic Tools** (Best for Windows)
Built into Visual Studio and works excellently with Qt applications.

**Setup:**
```batch
# Build with debug symbols
qmake CONFIG+=debug
make
```

**Usage:**
1. Open project in Visual Studio
2. Debug → Performance Profiler
3. Select "Memory Usage"
4. Run application and take snapshots
5. Compare snapshots to find leaks

### 2. **Application Verifier + Debugging Tools**
Microsoft's official leak detection toolkit.

**Installation:**
```batch
# Download Windows SDK for debugging tools
# https://developer.microsoft.com/windows/downloads/windows-sdk/
```

**Usage:**
```batch
# Enable heap verification
appverif -enable Heaps -for pdfextractor_gui.exe

# Run with debugger
cdb -g -G pdfextractor_gui.exe

# In debugger, use:
!heap -l    # List all heap allocations
!heap -p -a <address>   # Show allocation call stack
```

### 3. **Dr. Memory** (Cross-platform, Free)
Similar to Valgrind but works on Windows.

**Installation:**
```batch
# Download from https://drmemory.org/
# Add to PATH: C:\DrMemory\bin64
```

**Usage:**
```batch
# Basic memory check
drmemory.exe -- pdfextractor_gui.exe

# Detailed report with symbols
drmemory.exe -show_reachable -report_leak_max 100 -- pdfextractor_gui.exe

# Output goes to DrMemory-pdfextractor_gui.exe.<pid> folder
```

### 4. **Intel Inspector** (Commercial, Most Comprehensive)
Professional-grade memory and threading checker.

**Usage:**
```batch
# Command line analysis
inspxe-cl -collect mi3 -- pdfextractor_gui.exe
inspxe-cl -report problems

# GUI analysis
inspxe-gui
```

### 5. **CRT Debug Heap** (Built into MSVC)
Lightweight, built-in memory leak detection.

**Add to main.cpp:**
```cpp
#ifdef _WIN32
#include <crtdbg.h>
#endif

int main(int argc, char *argv[])
{
    #ifdef _WIN32
    // Enable memory leak detection
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

    // Break on specific allocation number (if you know it)
    // _CrtSetBreakAlloc(1234);
    #endif

    QApplication app(argc, argv);
    // ... rest of application
}
```

**Output appears in Debug Console:**
```
Detected memory leaks!
Dumping objects ->
{1234} normal block at 0x00A91F00, 128 bytes long.
 Data: <                > 00 00 00 00 00 00 00 00
```

### 6. **AddressSanitizer** (Clang/GCC)
Fast memory error detector built into modern compilers.

**Build with ASan:**
```batch
# Using clang (already installed with Qt LLVM-MinGW)
clang++ -fsanitize=address -g -O1 main.cpp -o pdfextractor_gui.exe

# Or in .pro file:
QMAKE_CXXFLAGS += -fsanitize=address
QMAKE_LFLAGS += -fsanitize=address
```

**Run:**
```batch
set ASAN_OPTIONS=verbosity=1:halt_on_error=0
pdfextractor_gui.exe
```

## Qt-Specific Considerations

### Qt Object Tree
Qt's parent-child ownership system automatically handles memory:
```cpp
// Good - parent takes ownership
auto button = new QPushButton("Click", parent);

// Bad - potential leak if not deleted manually
auto button = new QPushButton("Click");
```

### Smart Pointers with Qt
```cpp
// Modern Qt way
auto query = std::make_unique<SummaryQuery>();

// Or use QScopedPointer for Qt objects
QScopedPointer<QFile> file(new QFile("data.txt"));
```

### Common Qt Memory Issues to Check

1. **Disconnected signals from deleted objects**
```cpp
// Always disconnect before delete
if (m_reply) {
    m_reply->disconnect();
    m_reply->deleteLater();
    m_reply = nullptr;
}
```

2. **Event loop leaks**
```cpp
// Use deleteLater() for objects in event processing
reply->deleteLater();  // Good
delete reply;         // Bad - might crash if in signal handler
```

3. **Timer cleanup**
```cpp
// Timers should be stopped and deleted properly
if (m_timer->isActive()) {
    m_timer->stop();
}
delete m_timer;
```

## Quick Check Script

Create `check_memory.bat`:
```batch
@echo off
echo Running memory leak detection...
echo.

REM Method 1: Dr. Memory (if installed)
where drmemory >nul 2>nul
if %errorlevel% equ 0 (
    echo [Dr. Memory Check]
    drmemory.exe -brief -count_leaks -- pdfextractor_gui.exe
    goto end
)

REM Method 2: Build with debug CRT
echo [Building with Debug CRT]
qmake CONFIG+=debug DEFINES+=_CRTDBG_MAP_ALLOC
make clean
make
echo.
echo Run the debug build and check console output for leaks
echo Debug build: build\debug\pdfextractor_gui.exe

:end
echo.
echo Memory check complete!
```

## Recommended Workflow

1. **Development**: Use CRT Debug Heap for quick checks
2. **Testing**: Run Dr. Memory for comprehensive analysis
3. **CI/CD**: Integrate AddressSanitizer in build pipeline
4. **Deep Analysis**: Use Visual Studio Diagnostics or Intel Inspector

## Interpreting Results

### No Issues
```
Dr. Memory: no errors, no leaks
CRT: No memory leaks detected
```

### Memory Leak Found
```
LEAK 120 bytes
  queryrunner.cpp:45: new PromptQuery
  main.cpp:102: createQuery()
```
**Fix**: Ensure proper parent ownership or use smart pointers

### Use After Free
```
ERROR: accessing freed memory
  promptquery.cpp:163: m_reply->readAll()
```
**Fix**: Set pointers to nullptr after delete/deleteLater

## Best Practices

1. **Always use Qt parent-child relationships**
2. **Prefer stack allocation over heap**
3. **Use smart pointers for non-Qt objects**
4. **Call deleteLater() for Qt objects in signals**
5. **Run memory checks before each release**
6. **Set up automated memory testing in CI**