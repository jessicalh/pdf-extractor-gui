# State Leak Bug Fix - Session Summary
## Date: 2025-09-26

## Bug Discovery
Through systematic testing, discovered that PDF extraction failed with "Not Evaluated" ONLY after running Zotero first.

### Pattern Identified:
- ✅ PDF → PDF: Works
- ✅ PDF → Zotero: Works
- ❌ Zotero → PDF: FAILS with "Not Evaluated"

## Root Cause
State from previous runs was contaminating subsequent runs:

1. **QueryRunner member variables persisted**:
   - `m_extractedText`, `m_cleanedText`, `m_summary`, `m_originalKeywords`
   - These were NOT cleared between runs

2. **PromptQuery objects are reused** (created once in constructor):
   - `m_keywordsQuery->m_summaryResult` persisted
   - `m_refineQuery->m_originalKeywords` persisted
   - When PDF ran after Zotero, keywords query included Zotero's summary with PDF's text

3. **LLM saw mismatched content**:
   - Text from current PDF
   - Summary from previous Zotero run
   - LLM correctly identified this as "garbled" and returned "Not Evaluated"

## Fix Applied
Modified `QueryRunner::reset()` to clear ALL state:

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

## Testing Status
- Built successfully: `build/release-console/pdfextractor_gui_console.exe`
- Ready to test the fix
- Should now be able to run Zotero → PDF without failure

## Key Learnings
1. **Reused objects must be reset**: Query objects created once, used many times
2. **State leaks are subtle**: Same text extraction worked, but metadata contaminated
3. **LLM was correct**: It detected the mismatch and properly rejected it

## Next Steps
Test the pattern that previously failed:
1. Run Zotero extraction
2. Run PDF extraction
3. Verify both succeed without contamination