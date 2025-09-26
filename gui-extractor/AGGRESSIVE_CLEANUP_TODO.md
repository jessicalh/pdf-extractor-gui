# Aggressive Cleanup TODO List - Fixing Zotero Network Contamination Bug
## Date: 2025-09-26

## Problem Summary
After Zotero operations (especially PDF downloads with redirects), subsequent operations fail because the LLM receives corrupted/garbled data, even though our logs show clean JSON. This indicates Qt network buffer/state contamination.

## Root Cause Analysis
- Qt's QNetworkAccessManager maintains internal buffers that don't get properly cleared
- Even with separate QNetworkAccessManager instances, Qt shares global network state
- Known Qt bug where POST request data buffers contaminate subsequent requests
- Zotero's redirect handling may leave residual state

## TODO List - Aggressive Cleanup Strategy

### 1. ✅ Add aggressive cache clearing before each PromptQuery request
**Status**: COMPLETED
**Implementation**: Would have cleared caches, but superseded by fresh manager creation
**File Modified**: promptquery.cpp
**Result**: Replaced with more aggressive fresh manager approach

### 2. ✅ Create fresh QNetworkAccessManager for each request instead of reusing
**Status**: COMPLETED
**Implementation**: Delete and recreate m_networkManager before each request in sendRequest() with full error handling
**File Modified**: promptquery.cpp (line 91-113)
**Result**: Fresh manager with clean cookie jar created for every request

### 3. ✅ Add DoNotBufferUploadData attribute and Content-Length header to requests
**Status**: COMPLETED
**Implementation**: Added both attributes to prevent Qt from buffering the upload
**File Modified**: promptquery.cpp (line 136-138)
**Result**: Request now explicitly controls buffering behavior

### 4. ✅ Fix Zotero redirect handling to properly cleanup before creating new reply
**Status**: COMPLETED
**Implementation**: Clear caches before redirect, add error handling, set DoNotBufferUploadData
**File Modified**: zoteroinput.cpp (line 544-572)
**Result**: Redirect handling now clears caches and handles errors properly

### 5. ✅ Verify all QNetworkReply objects use deleteLater() consistently
**Status**: COMPLETED
**Result**: Verification complete - all paths properly use deleteLater() or qScopeGuard

### 6. ✅ Clear global Qt network state between operations
**Status**: COMPLETED
**Implementation**: Clear caches before posting request
**File Modified**: promptquery.cpp (line 223-226)
**Result**: Caches cleared right before posting

### 7. ✅ Add explicit cookie jar clearing between requests
**Status**: COMPLETED
**Implementation**: Fresh cookie jar created with each new QNetworkAccessManager
**File Modified**: promptquery.cpp (line 108-110)
**Result**: Cookie jar reset on each request

### 8. ✅ Implement connection pool reset between Zotero and other operations
**Status**: COMPLETED
**Implementation**: Covered by fresh QNetworkAccessManager creation
**Result**: Connection pools fully reset with new manager instances

### 9. ✅ Add diagnostics to detect UTF-8 BOM contamination
**Status**: COMPLETED
**Implementation**: Added BOM and non-printable character detection with auto-removal
**File Modified**: promptquery.cpp (line 146-158)
**Result**: Will detect, log, and remove BOM if present

### 10. ✅ Test the aggressive cleanup with Zotero→PDF sequence
**Status**: READY FOR TESTING
**Build Complete**: build/release/pdfextractor_gui.exe
**Test Steps**:
1. Start application
2. Load Zotero collection
3. Analyze a Zotero paper
4. Switch to PDF tab
5. Analyze a PDF
6. Verify PDF analysis works correctly (should not show "Not Evaluated")

### 11. ✅ Test the aggressive cleanup with Zotero→Zotero sequence
**Status**: READY FOR TESTING
**Build Complete**: build/release/pdfextractor_gui.exe
**Test Steps**:
1. Start application
2. Load Zotero collection
3. Analyze first Zotero paper
4. Analyze second Zotero paper
5. Verify second analysis works correctly

## Implementation Notes
- Most aggressive approach: Creating fresh QNetworkAccessManager for each request
- This sacrifices some performance but ensures complete state isolation
- Cache clearing is belt-and-suspenders approach
- BOM detection will help identify if encoding issues are involved

## Files Modified
- gui-extractor/promptquery.cpp
- gui-extractor/zoteroinput.cpp

## Build Command
```bash
make release-console
./build/release-console/pdfextractor_gui_console.exe
```

## Success Criteria
- Zotero→PDF sequence works without "Not Evaluated" error
- Zotero→Zotero sequence works for multiple papers
- No memory leaks from manager recreation
- Performance impact is acceptable