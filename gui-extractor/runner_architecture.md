# QueryRunner Architecture

## Overview
A single runner class that manages the entire processing pipeline from input (PDF or text) through all four LLM queries.

## Design Rationale

### Why One Runner Class?
- **Only 2 input types**: PDF file and pasted text - not worth separate classes
- **Shared pipeline**: Both inputs follow the exact same processing steps after text extraction
- **State management**: Single place to track progress through the pipeline
- **Settings coordination**: All queries share connection settings from database

### Input Handling

```cpp
enum InputType {
    PDFFile,      // Extract text from PDF
    PastedText    // Use provided text directly
};
```

Both types undergo:
1. Text extraction/validation
2. Copyright stripping (always done for LLM)
3. Cleanup (excessive whitespace, special characters)
4. Length limiting (50K chars max)

### Processing Pipeline

```
Input (PDF/Text)
    ↓
Extract/Clean Text
    ↓
Generate Summary ────────────┐
    ↓                        │
Extract Keywords             │ All results
    ↓                        │ available
Refine Prompt               │ individually
    ↓                        │
Extract Refined Keywords ────┘
    ↓
Complete
```

## Key Features

### State Machine
```cpp
enum ProcessingStage {
    Idle,                      // Ready for input
    ExtractingText,           // PDF parsing or text cleanup
    GeneratingSummary,        // Query 1
    ExtractingKeywords,       // Query 2
    RefiningPrompt,           // Query 3
    ExtractingRefinedKeywords,// Query 4
    Complete                  // All done
};
```

### Signal Architecture
Two types of signals:
1. **Progress signals**: Real-time updates as processing occurs
2. **Result signals**: Emitted when each stage completes

```cpp
// Progress tracking
void stageChanged(ProcessingStage stage);
void progressMessage(const QString& message);

// Individual results (available as soon as ready)
void summaryGenerated(const QString& summary);
void keywordsExtracted(const QString& keywords);
void promptRefined(const QString& suggestedPrompt);
void refinedKeywordsExtracted(const QString& keywords);
```

### Copyright Stripping
Always applied regardless of input type:
- Copyright lines
- © symbols
- "All Rights Reserved"
- Common license headers

### Text Cleanup Strategy

**PDF Files**:
- Basic whitespace normalization
- Page break handling

**Pasted Text**:
- Remove zero-width characters
- Normalize smart quotes
- Handle copy/paste artifacts
- Support for mixed content (could include formatted text, tables, etc.)

## Usage Example

```cpp
// In main GUI
auto runner = new QueryRunner(this);

// Connect to UI updates
connect(runner, &QueryRunner::stageChanged, [=](ProcessingStage stage) {
    updateStatusBar(stageToString(stage));
});

connect(runner, &QueryRunner::summaryGenerated, [=](QString summary) {
    m_summaryTextEdit->setPlainText(summary);
});

connect(runner, &QueryRunner::keywordsExtracted, [=](QString keywords) {
    m_keywordsOriginalEdit->setPlainText(keywords);
});

connect(runner, &QueryRunner::promptRefined, [=](QString prompt) {
    m_keywordsSuggestionsEdit->setPlainText(prompt);
});

connect(runner, &QueryRunner::refinedKeywordsExtracted, [=](QString keywords) {
    m_keywordsRefinedEdit->setPlainText(keywords);
});

// Process based on input type
if (isPDF) {
    runner->processPDF(filePath);
} else {
    runner->processText(pastedText);
}
```

## Advantages of This Architecture

1. **Simplicity**: One class manages the entire flow
2. **Consistency**: Both input types handled identically after extraction
3. **Maintainability**: All pipeline logic in one place
4. **Testability**: Can test with either input type easily
5. **Progress tracking**: Clear stages make UI updates simple
6. **Error recovery**: Each stage can fail independently

## Integration with Prompt Classes

The QueryRunner owns instances of all four prompt query classes:
- `SummaryQuery`
- `KeywordsQuery`
- `RefineKeywordsQuery`
- `KeywordsWithRefinementQuery`

It configures each with database settings before execution and handles their responses to advance through the pipeline.

## Settings Management

All settings loaded from SQLite database once on initialization:
- Connection (URL, model, timeout)
- Per-query settings (temperature, context, timeout, prompts)

Cached in struct for efficient reuse across all queries.