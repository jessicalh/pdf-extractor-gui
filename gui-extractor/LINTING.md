# Qt Project Linting Guide

## Available Linting Tools

### 1. Quick Lint (Included) ✅
Run basic checks immediately:
```batch
quick_lint.bat
```
Checks for:
- Variable shadowing
- Memory management issues
- Old-style Qt connects
- Missing override keywords

### 2. Cppcheck (Installed) ✅
General C++ static analysis:
```batch
cppcheck --enable=all main.cpp queryrunner.cpp promptquery.cpp
```

### 3. Clang-Tidy (Recommended)
Modern C++ linter with Qt support:
```batch
clang-tidy main.cpp --config-file=.clang-tidy -- -I C:\Qt\6.10.0\llvm-mingw_64\include
```
Install: https://releases.llvm.org/

### 4. Clazy (Best for Qt)
Qt-specific static analyzer:
```batch
clazy-standalone main.cpp -I C:\Qt\6.10.0\llvm-mingw_64\include
```
Install: https://github.com/KDE/clazy

## Common Issues Found

### Variable Shadowing
**Issue**: Local variables named same as signals/functions
```cpp
// Bad - shadows the signal name
emit progressUpdate("message");
QString progressUpdate = "text";  // Shadow!

// Good - use different name
emit progressUpdate("message");
QString updateMessage = "text";
```

### Missing Override
**Issue**: Virtual functions without override keyword
```cpp
// Bad
virtual ~PromptQuery();

// Good
virtual ~PromptQuery() override;
// Or for destructors
~PromptQuery() override;
```

### Memory Management
**Issue**: Using new without parent
```cpp
// Bad - potential memory leak
auto query = new SummaryQuery();

// Good - Qt parent ownership
auto query = new SummaryQuery(this);
```

## Integration with Qt Creator

1. **Enable Clang Code Model**:
   - Tools → Options → C++ → Code Model
   - Check "Use Clang Code Model"

2. **Configure Clang-Tidy**:
   - Tools → Options → Analyzer
   - Clang-Tidy and Clazy → Select checks

3. **Run Analysis**:
   - Analyze → Clang-Tidy and Clazy

## Recommended Workflow

1. **Before Commit**: Run `quick_lint.bat`
2. **Weekly**: Run full `lint_all.bat`
3. **CI/CD**: Integrate cppcheck in build pipeline

## Suppressing False Positives

### Cppcheck
```cpp
// cppcheck-suppress unusedFunction
void specialFunction() { }
```

### Clang-Tidy
```cpp
// NOLINTNEXTLINE(modernize-use-nullptr)
char* ptr = 0;
```

### Project-wide
Add to `.clang-tidy` or `cppcheck-suppressions.txt`

## Current Status

✅ **Clean**: Memory management, Qt connections
⚠️ **Warnings**: Variable shadowing in promptquery.cpp
✅ **No Critical Issues**