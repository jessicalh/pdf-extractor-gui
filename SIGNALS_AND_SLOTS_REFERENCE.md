# Qt Signals and Slots Reference - PDF Extractor GUI

## Quick Reference for Debugging

When something isn't working, find the relevant section below to trace the signal flow.

## Object Hierarchy

```
PDFExtractorGUI (main.cpp)
├── m_queryRunner (QueryRunner*)
│   ├── m_summaryQuery (SummaryQuery*)
│   ├── m_keywordsQuery (KeywordsQuery*)
│   ├── m_refineQuery (RefineKeywordsQuery*)
│   └── m_refinedKeywordsQuery (KeywordsWithRefinementQuery*)
├── m_pdfAnalyzeButton (QPushButton*)
├── m_textAnalyzeButton (QPushButton*)
├── m_extractedTextEdit (QPlainTextEdit*)
├── m_summaryTextEdit (QTextEdit*)
├── m_keywordsTextEdit (QTextEdit*)
├── m_promptSuggestionsEdit (QTextEdit*)
├── m_refinedKeywordsEdit (QTextEdit*)
└── m_logTextEdit (QPlainTextEdit*)
```

## Button Click → Processing Start

### PDF Analyze Button
**File**: main.cpp, line 858
```cpp
connect(m_pdfAnalyzeButton, &QPushButton::clicked,
        this, &PDFExtractorGUI::analyzePDF);
```
→ Calls `PDFExtractorGUI::analyzePDF()` (line 910)
→ Which calls `m_queryRunner->processPDF(pdfPath)` (line 931)

### Text Analyze Button
**File**: main.cpp, line 859
```cpp
connect(m_textAnalyzeButton, &QPushButton::clicked,
        this, &PDFExtractorGUI::analyzeText);
```
→ Calls `PDFExtractorGUI::analyzeText()` (line 934)
→ Which calls `m_queryRunner->processText(text)` (line 955)

## QueryRunner Signal Connections

### Status and Progress Updates
**File**: main.cpp, lines 862-864
```cpp
connect(m_queryRunner, &QueryRunner::stageChanged,
        this, &PDFExtractorGUI::handleStageChanged);

connect(m_queryRunner, &QueryRunner::progressMessage,
        this, &PDFExtractorGUI::log);

connect(m_queryRunner, &QueryRunner::errorOccurred,
        this, &PDFExtractorGUI::handleError);
```

### Result Display Connections
**File**: main.cpp, lines 867-889

**Text Extracted**:
```cpp
connect(m_queryRunner, &QueryRunner::textExtracted,
        [this](const QString& text) {
            m_extractedTextEdit->setPlainText(text);
            m_resultsTabWidget->setCurrentWidget(m_extractedTextEdit);
        });
```

**Summary Generated**:
```cpp
connect(m_queryRunner, &QueryRunner::summaryGenerated,
        [this](const QString& summary) {
            m_summaryTextEdit->setPlainText(summary);
        });
```

**Keywords Extracted**:
```cpp
connect(m_queryRunner, &QueryRunner::keywordsExtracted,
        [this](const QString& keywords) {
            m_keywordsTextEdit->setPlainText(keywords);
        });
```

**Prompt Refined**:
```cpp
connect(m_queryRunner, &QueryRunner::promptRefined,
        [this](const QString& prompt) {
            m_promptSuggestionsEdit->setPlainText(prompt);
        });
```

**Refined Keywords Extracted**:
```cpp
connect(m_queryRunner, &QueryRunner::refinedKeywordsExtracted,
        [this](const QString& keywords) {
            m_refinedKeywordsEdit->setPlainText(keywords);
        });
```

## Internal QueryRunner → Query Connections

### Summary Query
**File**: queryrunner.cpp, lines 18-23
```cpp
connect(m_summaryQuery, &PromptQuery::resultReady,
        this, &QueryRunner::handleSummaryResult);
connect(m_summaryQuery, &PromptQuery::errorOccurred,
        this, &QueryRunner::errorOccurred);
connect(m_summaryQuery, &PromptQuery::progressUpdate,
        this, &QueryRunner::progressMessage);
```

### Keywords Query
**File**: queryrunner.cpp, lines 25-30
```cpp
connect(m_keywordsQuery, &PromptQuery::resultReady,
        this, &QueryRunner::handleKeywordsResult);
// errorOccurred and progressUpdate similar pattern
```

### Refinement Query
**File**: queryrunner.cpp, lines 32-37
```cpp
connect(m_refineQuery, &PromptQuery::resultReady,
        this, &QueryRunner::handleRefinementResult);
// errorOccurred and progressUpdate similar pattern
```

### Refined Keywords Query
**File**: queryrunner.cpp, lines 39-44
```cpp
connect(m_refinedKeywordsQuery, &PromptQuery::resultReady,
        this, &QueryRunner::handleRefinedKeywordsResult);
// errorOccurred and progressUpdate similar pattern
```

## Network Reply Handling

### QNetworkReply → PromptQuery
**File**: promptquery.cpp, lines 113-114
```cpp
connect(m_currentReply, &QNetworkReply::finished,
        this, &PromptQuery::handleNetworkReply);
```

### Timeout Timer
**File**: promptquery.cpp (in constructor)
```cpp
connect(m_timeoutTimer, &QTimer::timeout,
        this, &PromptQuery::handleTimeout);
```

## Signal Emission Points

### QueryRunner Signals

**textExtracted**: queryrunner.cpp, line 75
```cpp
emit textExtracted(m_extractedText);
```

**summaryGenerated**: queryrunner.cpp, line 265
```cpp
emit summaryGenerated(m_summary);
```

**keywordsExtracted**: queryrunner.cpp, line 284
```cpp
emit keywordsExtracted(m_originalKeywords);
```

**promptRefined**: queryrunner.cpp, line 278
```cpp
emit promptRefined(m_suggestedPrompt);
```

**refinedKeywordsExtracted**: queryrunner.cpp, line 301
```cpp
emit refinedKeywordsExtracted(m_refinedKeywords);
```

**processingComplete**: queryrunner.cpp, line 318
```cpp
emit processingComplete();
```

### PromptQuery Base Class Signals

**progressUpdate**: Throughout promptquery.cpp
- Line 86: "=== REQUEST SENT ==="
- Line 178: "=== RESPONSE RECEIVED ==="
- Line 211: "Processing response..."

**errorOccurred**: promptquery.cpp
- Line 56: "Failed to build prompt"
- Line 100: Network error
- Line 144: "No response from model"
- Line 217: "No content in response to process"

**resultReady**: Emitted by subclasses
- SummaryQuery: line 238
- KeywordsQuery: line 284
- RefineKeywordsQuery: lines 335, 340
- (KeywordsWithRefinementQuery inherits from KeywordsQuery)

## Complete Flow for PDF Processing

1. **User clicks analyze** → `m_pdfAnalyzeButton::clicked`
2. **Slot called** → `PDFExtractorGUI::analyzePDF()`
3. **Clear results** → `clearResults()` called (line 928)
4. **Start pipeline** → `m_queryRunner->processPDF(path)`
5. **Extract text** → `QueryRunner::extractTextFromPDF()`
6. **Signal text** → `emit textExtracted(text)`
7. **Update UI** → Lambda fills `m_extractedTextEdit`
8. **Run summary** → `runSummaryExtraction()`
9. **Query executes** → `m_summaryQuery->execute()`
10. **Network request** → `QNetworkAccessManager::post()`
11. **Reply received** → `QNetworkReply::finished`
12. **Handle reply** → `PromptQuery::handleNetworkReply()`
13. **Process response** → `SummaryQuery::processResponse()`
14. **Emit result** → `emit resultReady(summary)`
15. **Handle result** → `QueryRunner::handleSummaryResult()`
16. **Update UI** → `emit summaryGenerated()` → Lambda fills `m_summaryTextEdit`
17. **Advance stage** → `advanceToNextStage()`
18. **Repeat** for keywords, refinement, refined keywords

## Debugging Tips

### If UI not updating:
1. Check the lambda connections in `PDFExtractorGUI::connectSignals()`
2. Verify the QueryRunner is emitting the expected signal
3. Check `handleStageChanged()` for status bar updates

### If stage not advancing:
1. Check `QueryRunner::advanceToNextStage()` switch statement
2. Verify the handler is calling `advanceToNextStage()`
3. Look for early returns in handlers (empty/failed results)

### If network timeout:
1. Default timeout is 600000ms (10 minutes)
2. Set in database initialization (main.cpp, lines 625, 639, 655)
3. Timer started in `PromptQuery::execute()` line 116

### If "Failed to build prompt":
1. Check database initialization order (main.cpp, line 486-489)
2. Verify settings loaded in `QueryRunner::loadSettingsFromDatabase()`
3. Check prompt has `{text}` placeholder

### Common Signal Paths to Check:

**No summary appearing**:
`SummaryQuery::processResponse()` → `emit resultReady()` → `QueryRunner::handleSummaryResult()` → `emit summaryGenerated()` → Lambda in main.cpp line 872

**Keywords not updating**:
`KeywordsQuery::processResponse()` → `emit resultReady()` → `QueryRunner::handleKeywordsResult()` → `emit keywordsExtracted()` → Lambda in main.cpp line 876

**Refined keywords missing**:
Check if `m_suggestedPrompt` is empty in `runRefinedKeywordExtraction()` (queryrunner.cpp line 244)