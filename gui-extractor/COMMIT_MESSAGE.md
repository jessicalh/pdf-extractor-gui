# Fix network contamination bug with aggressive cleanup and UI-as-truth for reruns

## Major Fixes

### 1. Network State Contamination Bug (Critical)
**Problem**: After Zotero operations, subsequent operations (PDF analysis, other Zotero papers) would receive corrupted/garbled responses from LLM, showing "Not Evaluated" errors.

**Root Cause**: Qt's QNetworkAccessManager maintains internal buffers that don't get properly cleared between requests, causing data from Zotero's PDF downloads (especially with redirects) to contaminate subsequent POST requests.

**Solution - Aggressive Cleanup Strategy**:
- Create fresh QNetworkAccessManager for EVERY request (nuclear option)
- Clear all caches and connection pools before each request
- Add explicit cookie jar reset with each new manager
- Set DoNotBufferUploadData attribute on requests
- Add proper error handling and resource cleanup on all paths
- Fix Zotero redirect handling to clear caches before following redirects
- Add BOM contamination detection and removal

**Memory Safety Improvements**:
- Proper cleanup in destructor
- Exception-safe QNetworkAccessManager creation
- No memory leaks even on error paths
- Clear ownership tracking and documentation

### 2. First-Run Regression Fix
**Problem**: After implementing aggressive cleanup, the first Zotero analysis would fail with no extracted text.

**Root Cause**: PromptQuery constructor was creating initial QNetworkAccessManager, but our cleanup deleted it on first use. Since QueryRunner reuses PromptQuery objects, second use had null manager.

**Solution**: Don't create QNetworkAccessManager in constructor; create fresh one for each request.

### 3. UI as Source of Truth for Single-Step Reruns
**Problem**: When stopping a run mid-way and re-running just keywords with updated prompts, QueryRunner's reset() would clear internal text, causing "no text available" error despite text being visible in UI.

**Solution**:
- Added `processKeywordsOnly(extractedText, summaryText)` overload to QueryRunner
- Main.cpp now passes UI text content when doing single-step reruns
- UI is now the authoritative source of truth (user can edit any field)
- QueryRunner accepts external text for single-step operations

## Files Modified
- `promptquery.cpp`: Aggressive cleanup implementation, memory safety
- `promptquery.h`: Updated destructor documentation
- `queryrunner.cpp`: Added UI text parameter support for single-step runs
- `queryrunner.h`: New processKeywordsOnly overload
- `main.cpp`: Pass UI text for keywords-only reruns
- `zoteroinput.cpp`: Fixed redirect handling with cache clearing

## Testing Status
- Basic functionality confirmed working with gpt-oss-120b
- Zotero → PDF sequence works without contamination
- Zotero → Zotero sequence works for multiple papers
- Keywords-only rerun uses UI text correctly
- First Zotero pass works properly
- More comprehensive testing still needed

## Known Improvements
- Performance impact from creating fresh QNetworkAccessManager each time (acceptable tradeoff for reliability)
- Aggressive approach ensures complete state isolation between operations

This fix ensures reliable operation without network state contamination while maintaining UI consistency and user control over the analysis pipeline.