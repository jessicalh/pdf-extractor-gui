# Current State and Remaining Bug - PDF Extractor GUI
## Date: 2025-09-26

## What's Working
1. **PDF → PDF**: Multiple PDF extractions in a row work perfectly
2. **Zotero → Zotero**: Multiple Zotero extractions in a row work perfectly
3. **PDF → Zotero**: PDF followed by Zotero works
4. **First run always works**: Whether PDF or Zotero

## What's Broken
**Zotero → PDF sequence fails**: After a successful Zotero extraction, the next PDF extraction fails with "Not Evaluated" from the LLM.

## What We Know For Certain
1. **It's NOT the LLM being inconsistent** - The pattern is 100% reproducible
2. **It's NOT the PDF extraction** - The extracted text appears correctly in the UI
3. **It's NOT simple state persistence** - We fixed QueryRunner state leaks
4. **It IS something we're sending** - The LLM correctly identifies the content as "garbled"
5. **The same PDF file works** - When run first or after another PDF
6. **The text length is identical** - ~75,900 characters sent in both cases

## What We've Already Fixed
### Session 1: Fixed state leaks in QueryRunner::reset()
```cpp
void QueryRunner::reset() {
    m_currentStage = Idle;
    emit stageChanged(m_currentStage);

    // Clear ALL persistent state from previous runs
    m_extractedText.clear();
    m_cleanedText.clear();
    m_summary.clear();
    m_originalKeywords.clear();
    m_suggestedPrompt.clear();
    m_refinedKeywords.clear();

    // CRITICAL: Also clear state in the reused query objects!
    m_keywordsQuery->setSummaryResult("");
    m_refineQuery->setOriginalKeywords("");
    m_refineQuery->setOriginalPrompt("");
    m_refinedKeywordsQuery->setSummaryResult("");

    emit progressMessage("Ready for new analysis");
}
```

This fixed SOME issues but the Zotero→PDF bug persists.

## Key Observations
1. **The bug is directional**: Zotero corrupts the next PDF, but PDF doesn't corrupt the next Zotero
2. **The corruption is invisible**: The text looks fine in logs but LLM sees it as garbled
3. **It's not accumulation**: Zotero→Zotero works, so it's not adding to itself
4. **Reset doesn't help**: Our reset() clears member variables but misses something

## Investigation Priorities for Next Session

### 1. Network/HTTP State - PRIME SUSPECT
**Finding**: Both Zotero and PromptQuery use separate QNetworkAccessManager instances, but Qt may share global network state between them.

Potential contamination:
- **HTTP cache**: Zotero might cache responses that affect next request
- **Connection pool**: Reused TCP connections with wrong state
- **Cookies**: Zotero API might set cookies that persist
- **Proxy settings**: Global proxy configuration changes
- **SSL/TLS state**: Certificate or session caching

Fix to try:
```cpp
// In PromptQuery, before each request:
m_networkManager->clearAccessCache();
m_networkManager->clearConnectionCache();
// Or create a fresh QNetworkAccessManager for each request
```

### 2. Text Encoding Issues
- Is Zotero's text using different encoding that affects the JSON?
- Are there hidden characters from Zotero's download?
- Check for UTF-8 vs other encodings

### 3. File System State
- Is Zotero's temp PDF file interfering?
- Check if file handles are being reused
- Look for working directory changes

### 4. Qt Object State
- QNetworkAccessManager might need recreation not just reuse
- Other Qt objects that persist between runs
- Check for Qt settings or registry changes

### 5. Memory/Buffer Issues
- Buffers not being cleared
- QString internal state
- JSON serialization differences

## Debug Points to Add Next Session
1. **Compare raw bytes** sent to network between working and failing cases
2. **Hex dump** the JSON to see hidden characters
3. **Check QNetworkAccessManager state** before and after Zotero
4. **Monitor file handles** during Zotero operation
5. **Track all Qt object creation/deletion**

## Workaround for Users
**Avoid the Zotero→PDF sequence**. Instead use:
- Multiple PDFs in a row ✅
- Multiple Zoteros in a row ✅
- PDF then Zotero ✅
- Just not Zotero then PDF ❌

## Build Info
Current build with partial fix:
```
make release-console
./build/release-console/pdfextractor_gui_console.exe
```

## Files Modified in This Session
- queryrunner.cpp - Added state clearing to reset()
- main.cpp - Added debug logging
- promptquery.cpp - Added JSON dump debugging

## Next Steps
The bug is well-characterized and reproducible. The next session should focus on finding what Zotero changes that persists beyond our reset() and specifically breaks the next PDF run's network communication.