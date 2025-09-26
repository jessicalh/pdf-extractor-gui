# Unified PDF Processing Architecture
## Date: 2025-09-26

## Summary
All PDF processing has been consolidated into a single, safe path through `QueryRunner::processPDF()`. This ensures:
- Consistent safety checks for ALL PDFs (local files and Zotero downloads)
- No duplicate code paths
- Complete separation of concerns (Zotero provides only PDF path, no metadata leakage to LLM)
- Comprehensive exception handling at every level

## Architecture Overview

### Single Entry Point
```
User Action → main.cpp::analyzePDF(path) → QueryRunner::processPDF(path)
```

All PDFs, regardless of source, flow through this single path:
1. **PDF File Tab**: User selects file → `analyzePDF()` → `QueryRunner::processPDF()`
2. **Zotero Tab**: Downloads PDF → returns path → `analyzePDF(path)` → `QueryRunner::processPDF(path)`

### Safety Checks in QueryRunner::processPDF()
Every PDF goes through these checks in order:
1. Empty path validation
2. File existence check
3. Read permission verification
4. File size limit (500MB max via SafePdfLoader)
5. PDF header validation (ensures it's actually a PDF)
6. Exception handling wrapper around all operations

### Text Extraction Flow
```cpp
QueryRunner::processPDF(path)
    ↓
QueryRunner::extractTextFromPDF(path)
    ↓
SafePdfLoader::loadPdf(doc, path, error, 60000)  // 60s timeout
    ↓
SafePdfLoader::extractTextSafely(doc, error)    // Memory limits
    ↓
Returns extracted text (no metadata)
```

## Key Design Decisions

### 1. No Metadata Leakage
- Zotero component ONLY provides the PDF file path
- No paper title, authors, or collection info reaches the LLM
- All metadata stays in the Zotero UI for user reference only

### 2. DRY Principle Enforcement
- Removed duplicate `analyzeZoteroPdf()` function
- Commented out unused `inputmethod.cpp/h` files
- Single PDF processing implementation for maintainability

### 3. Comprehensive Exception Handling
- Try-catch blocks at every level
- RAII pattern with smart pointers for automatic cleanup
- QScopeGuard for network reply cleanup
- Timeout protection at multiple stages

## Files Changed

### Removed/Commented Out
- `inputmethod.cpp` and `inputmethod.h` - Completely unused, commented out in .pro file

### Modified for Unification
- **main.cpp**:
  - Single `analyzePDF()` function handles all sources
  - Removed `analyzeZoteroPdf()` duplicate

- **queryrunner.cpp**:
  - `processPDF()` contains ALL safety checks
  - `extractTextFromPDF()` uses SafePdfLoader exclusively

- **zoteroinput.cpp**:
  - Only provides PDF path via `getPdfPath()`
  - Does not pass any metadata to processing pipeline

- **pdfextractor_gui.pro**:
  - Commented out unused inputmethod files

## Safety Features

### Memory Protection
- Max 1MB per PDF page
- Max 10MB total text
- Max 1000 pages
- Smart pointers for automatic cleanup

### Timeout Protection
- 10s for PDF validation
- 30s for PDF loading
- 60s for text extraction

### Error Recovery
- All error paths properly reset UI state
- Network replies cleaned up on all paths
- Temporary files deleted on failure

## Testing Validation
Build completed successfully:
```bash
make release-console
# Output: build/release-console/pdfextractor_gui_console.exe
```

## Benefits of Unified Architecture

1. **Maintainability**: Single implementation to update/fix
2. **Consistency**: All PDFs get same safety treatment
3. **Security**: No metadata leakage possible
4. **Reliability**: Comprehensive exception handling
5. **Performance**: Reused SafePdfLoader optimizations

## Next Steps
The architecture is now clean and unified. Any future PDF sources should:
1. Obtain the PDF file path
2. Call `analyzePDF(path)`
3. Let the unified pipeline handle everything else

No special handling or duplicate code paths needed.