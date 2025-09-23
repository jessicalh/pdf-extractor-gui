# PDF Extractor GUI - Comprehensive Project Check-in

## Overview

We've built a sophisticated Qt-based application that bridges traditional PDF text extraction with modern AI language model capabilities. The application processes documents through a carefully orchestrated pipeline of AI queries, each building on the previous results to produce increasingly refined output.

## What We've Accomplished

### Core Architecture Improvements
1. **Migrated from TOML to SQLite** - Removed all legacy TOML configuration code and implemented a robust SQLite-based settings system with per-prompt configuration storage
2. **Fixed initialization order** - Database is now properly initialized before QueryRunner creation, preventing "Failed to build prompt" errors
3. **Updated network defaults** - Changed from WSL bridge IP (172.20.10.3) to localhost (127.0.0.1) for native Windows operation
4. **Set 10-minute timeouts** - All AI queries now have 600-second timeouts to handle slow LLM responses

### AI Processing Pipeline
1. **Stage 1: Summary Extraction** - Generates academic research summaries
2. **Stage 2: Keyword Extraction** - Extracts domain-specific scientific terms
3. **Stage 3: Prompt Refinement** - Uses AI to improve the keyword extraction prompt
4. **Stage 4: Refined Keywords** - Re-extracts keywords using the improved prompt

### Critical Fixes
- **Text placeholder handling** - Refined keywords query now ensures {text} is present in prompts
- **Proper completion logic** - Process correctly ends if summary or refinement fails
- **JSON reasoning extraction** - Properly extracts reasoning from gpt-oss JSON response structure
- **Enhanced logging** - Comprehensive logging to both UI and lastrun.log file

## Qt Signals and Slots Event Flow

### Architecture Overview

The application uses Qt's signals and slots mechanism to create a loosely coupled, event-driven architecture. Here's how the components interact:

```
PDFExtractorGUI (Main Window)
    ├── UI Controls (buttons, text edits, tabs)
    └── QueryRunner (Pipeline Orchestrator)
            ├── SummaryQuery
            ├── KeywordsQuery
            ├── RefineKeywordsQuery
            └── KeywordsWithRefinementQuery
```

### Complete Event Sequence for Document Processing

#### 1. USER INITIATES PROCESSING

**Trigger**: User clicks "Analyze" button (PDF or Text tab)

**Signal Flow**:
```cpp
QPushButton::clicked → PDFExtractorGUI::analyzePDF() or analyzeText()
```

**Actions**:
- `clearResults()` is called - clears all output controls
- `m_queryRunner->processPDF(path)` or `m_queryRunner->processText(text)` is called

#### 2. QUERYRUNNER BEGINS PIPELINE

**Initial Processing**:
```cpp
QueryRunner::processPDF/processText()
    ├── Sets m_currentStage = ExtractingText
    ├── emit stageChanged(ExtractingText) → PDFExtractorGUI::handleStageChanged()
    ├── emit progressMessage("Opening PDF...") → PDFExtractorGUI::log()
    ├── Extracts/cleans text
    ├── emit textExtracted(text) → Lambda fills m_extractedTextEdit
    └── Calls startPipeline(text)
```

**Pipeline Initialization**:
```cpp
QueryRunner::startPipeline()
    ├── Creates/clears lastrun.log
    ├── Calls cleanupText() - removes copyright, normalizes
    └── Calls runSummaryExtraction()
```

#### 3. STAGE 1 - SUMMARY EXTRACTION

**Signal Chain**:
```cpp
QueryRunner::runSummaryExtraction()
    ├── Sets m_currentStage = GeneratingSummary
    ├── emit stageChanged() → PDFExtractorGUI updates status
    ├── emit progressMessage("=== STAGE 1...") → PDFExtractorGUI::log()
    ├── Configures SummaryQuery with settings
    └── Calls m_summaryQuery->execute(text)

SummaryQuery::execute()
    ├── emit progressUpdate() → QueryRunner::progressMessage → PDFExtractorGUI::log()
    ├── Posts to LM Studio via QNetworkAccessManager
    └── QNetworkReply::finished → PromptQuery::handleNetworkReply()

PromptQuery::handleNetworkReply()
    ├── Parses JSON response
    ├── Extracts content and reasoning fields
    ├── emit progressUpdate() for reasoning → (chain to UI)
    └── Calls processResponse(content)

SummaryQuery::processResponse()
    └── emit resultReady(summary) → QueryRunner::handleSummaryResult()

QueryRunner::handleSummaryResult()
    ├── Stores m_summary
    ├── emit summaryGenerated(text) → Lambda fills m_summaryTextEdit
    ├── Checks if empty/failed → ends process if so
    └── Calls advanceToNextStage()
```

#### 4. STAGE 2 - KEYWORD EXTRACTION

**Signal Chain** (similar pattern):
```cpp
QueryRunner::runKeywordExtraction()
    └── ... → KeywordsQuery::execute() → ... → resultReady()
        └── QueryRunner::handleKeywordsResult()
            ├── emit keywordsExtracted(keywords) → Lambda fills m_keywordsTextEdit
            └── advanceToNextStage() [proceeds even if empty]
```

#### 5. STAGE 3 - PROMPT REFINEMENT

**Signal Chain**:
```cpp
QueryRunner::runPromptRefinement()
    └── ... → RefineKeywordsQuery::execute() → ... → resultReady()
        └── QueryRunner::handleRefinementResult()
            ├── emit promptRefined(prompt) → Lambda fills m_promptSuggestionsEdit
            ├── Checks if empty/"Not Evaluated" → completes if so
            └── advanceToNextStage() if successful
```

#### 6. STAGE 4 - REFINED KEYWORD EXTRACTION

**Signal Chain**:
```cpp
QueryRunner::runRefinedKeywordExtraction()
    ├── Ensures m_suggestedPrompt has {text} placeholder
    └── ... → KeywordsWithRefinementQuery::execute() → ... → resultReady()
        └── QueryRunner::handleRefinedKeywordsResult()
            ├── emit refinedKeywordsExtracted(keywords) → Lambda fills m_refinedKeywordsEdit
            └── advanceToNextStage() → Sets Complete, emits processingComplete()
```

#### 7. COMPLETION

**Final Signals**:
```cpp
QueryRunner (when stage = ExtractingRefinedKeywords):
    ├── Sets m_currentStage = Complete
    ├── emit stageChanged(Complete) → PDFExtractorGUI updates status
    ├── emit processingComplete() → Lambda re-enables UI
    ├── emit progressMessage("All processing complete")
    └── Sets m_currentStage = Idle
```

### Error Handling Signals

**Network/Processing Errors**:
```cpp
PromptQuery::errorOccurred(QString)
    → QueryRunner::errorOccurred(QString) [forwarded]
        → PDFExtractorGUI::handleError()
            ├── Logs to m_logTextEdit
            └── Shows QMessageBox::critical()
```

**Timeout Handling**:
```cpp
QTimer::timeout() → PromptQuery::handleTimeout()
    ├── Aborts network request
    └── emit errorOccurred("Request timeout")
```

### Key Design Patterns

1. **Observer Pattern** - Signals/slots provide loose coupling between components
2. **Chain of Responsibility** - Each query processes and passes to the next
3. **Strategy Pattern** - Different PromptQuery subclasses for different AI tasks
4. **State Machine** - ProcessingStage enum tracks pipeline progress

### Critical Signal Connections

**In QueryRunner constructor**:
```cpp
// Each query type has 3 connections:
connect(m_summaryQuery, &PromptQuery::resultReady, this, &QueryRunner::handleSummaryResult);
connect(m_summaryQuery, &PromptQuery::errorOccurred, this, &QueryRunner::errorOccurred);
connect(m_summaryQuery, &PromptQuery::progressUpdate, this, &QueryRunner::progressMessage);
// (repeated for all 4 query types)
```

**In PDFExtractorGUI::connectSignals()**:
```cpp
// QueryRunner state signals
connect(m_queryRunner, &QueryRunner::stageChanged, this, &handleStageChanged);
connect(m_queryRunner, &QueryRunner::progressMessage, this, &log);
connect(m_queryRunner, &QueryRunner::errorOccurred, this, &handleError);

// Result signals (using lambdas for UI updates)
connect(m_queryRunner, &QueryRunner::textExtracted, [this](QString text) {
    m_extractedTextEdit->setPlainText(text);
});
// (similar for summaryGenerated, keywordsExtracted, etc.)
```

## Data Flow Summary

1. **Input** → PDF file or pasted text
2. **Text Extraction** → Copyright removal, normalization
3. **Summary Query** → Academic research overview
4. **Keywords Query** → Initial domain-specific terms
5. **Refinement Query** → Improved extraction prompt (may end here)
6. **Refined Keywords Query** → Enhanced keyword extraction
7. **Output** → All results displayed in respective tabs + lastrun.log

## Current State

The application is fully functional with:
- Robust error handling at each stage
- Comprehensive logging (UI + file)
- Proper timeout management
- Graceful degradation (continues with partial results where appropriate)
- Clean separation of concerns via Qt's signal/slot mechanism

The event-driven architecture ensures the UI remains responsive during long-running AI queries, while the QueryRunner orchestrates the complex multi-stage pipeline without tight coupling to the UI layer.