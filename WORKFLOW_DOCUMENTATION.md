# PDF Extractor GUI - Complete Workflow Documentation

## Overview
This document describes the complete workflow of the PDF Extractor GUI application as implemented in the codebase, matching the agreed-upon specification.

## Workflow Process

### 1. Input Stage
The application accepts two types of input:

#### PDF Input Path
1. User selects PDF file via Browse button or types path
2. User clicks "Analyze" button in PDF File tab
3. `analyzePDF()` is called which:
   - Validates PDF path is not empty
   - Clears all output controls via `clearResults()`
   - Calls `m_queryRunner->processPDF(pdfPath)`

#### Text Input Path
1. User pastes text into "Paste Text" tab
2. User clicks "Analyze" button in Paste Text tab
3. `analyzeText()` is called which:
   - Validates text is not empty
   - Clears all output controls via `clearResults()`
   - Calls `m_queryRunner->processText(text)`

### 2. Text Extraction/Cleaning Stage

#### For PDF Input
1. QueryRunner sets stage to `ExtractingText`
2. `extractTextFromPDF()` loads PDF and extracts text page by page
3. Progress messages sent: "Extracting page X of Y (Z%)..."
4. If extraction fails: error to status bar, process ends
5. Extracted text sent to `textExtracted` signal → fills "Extracted Text" tab

#### For Text Input
1. QueryRunner sets stage to `ExtractingText`
2. Text is passed directly as extracted text
3. Progress message sent: "Processing pasted text..."
4. Text sent to `textExtracted` signal → fills "Extracted Text" tab

#### Text Cleanup (Both Paths)
1. `cleanupText()` is called which:
   - **Always removes copyright notices** via `removeCopyrightNotices()`
   - Removes excessive whitespace
   - For pasted text: removes zero-width characters, normalizes quotes
   - Truncates to 50,000 characters if needed
2. If cleaned text is empty: error to status bar, process ends

### 3. AI Pipeline Execution

The pipeline runs sequentially through `advanceToNextStage()`:

#### Stage 1: Summary Generation
1. Stage set to `GeneratingSummary`
2. `runSummaryExtraction()` configures SummaryQuery with:
   - Connection settings (URL, model)
   - Summary settings (temperature, context, timeout)
   - Summary preprompt and prompt from database
3. Progress: "Preparing Summary Extraction request..." → "Sending request to LM Studio..."
4. **Full prompt logged to Run Log** (preprompt + prompt with {text} replaced)
5. Network response logged with full details
6. Result handling:
   - If "Not Evaluated": error to status bar, **process ends**
   - If empty: error to status bar, **process ends**
   - Success: fills "Summary Result" tab, advances to next stage

#### Stage 2: Keywords Extraction
1. Stage set to `ExtractingKeywords`
2. `runKeywordExtraction()` configures KeywordsQuery with:
   - Connection settings (URL, model)
   - Keywords settings (temperature, context, timeout)
   - Keywords preprompt and prompt from database
3. Progress: "Preparing Keyword Extraction request..." → "Sending request to LM Studio..."
4. **Full prompt logged to Run Log**
5. Network response logged with full details
6. Result handling:
   - If "Not Evaluated": error logged, **still proceeds to refinement**
   - If empty: **still proceeds to refinement**
   - Success: fills "Keywords Result" tab, advances to next stage

#### Stage 3: Prompt Refinement
1. Stage set to `RefiningPrompt`
2. `runPromptRefinement()` configures RefineKeywordsQuery with:
   - Connection settings (URL, model)
   - Refinement settings (temperature, context, timeout)
   - Keyword refinement preprompt and preprompt refinement prompt
   - Original keywords and original prompt
3. Progress: "Preparing Prompt Refinement request..." → "Sending request to LM Studio..."
4. **Full prompt logged to Run Log** with {text}, {keywords}, {original_prompt} replaced
5. Network response logged with full details
6. Result handling:
   - If "Not Evaluated": uses original prompt, **process ends**
   - If empty: uses original prompt, **process ends**
   - Success: stores refined prompt internally, advances to next stage

#### Stage 4: Refined Keywords Extraction
1. Stage set to `ExtractingRefinedKeywords`
2. `runRefinedKeywordExtraction()` configures KeywordsWithRefinementQuery with:
   - Connection settings (URL, model)
   - Keywords settings (temperature, context, timeout)
   - Keywords preprompt
   - **Refined prompt from Stage 3**
3. Progress: "Preparing Refined Keyword Extraction request..." → "Sending request to LM Studio..."
4. **Full prompt logged to Run Log** using refined prompt
5. Network response logged with full details
6. Result handling:
   - Updates "Keywords Result" tab with refined keywords
   - Sets stage to `Complete`
   - Emits `processingComplete` signal
   - Returns to `Idle` state

### 4. UI Updates During Processing

#### Status Bar
- Shows current stage via `handleStageChanged()`
- Displays spinner animation during processing
- Shows error messages when failures occur
- Returns to "Ready" when complete or on error

#### Run Log Tab
- Timestamp prefixed messages via `log()`
- Shows all progress updates from QueryRunner
- **Displays full prompts** for each AI query
- **Shows network responses** with maximum verbosity
- Errors displayed with "ERROR:" prefix

#### Result Tabs
- "Extracted Text": Filled after text extraction/cleaning
- "Summary Result": Filled after summary generation
- "Keywords Result": Filled after initial keywords, **updated** after refined keywords
- Internal prompt refinement (not shown in UI)

### 5. Error Handling

#### Process-Ending Errors
- PDF extraction failure
- Empty text after cleanup
- Summary returns "Not Evaluated" or empty
- Refinement returns "Not Evaluated" or empty

#### Non-Fatal Errors
- Keywords returns "Not Evaluated" or empty (proceeds to refinement anyway)

#### Error Display
- All errors shown on status bar
- All errors logged to Run Log with "ERROR:" prefix
- Critical errors show QMessageBox dialog
- Process returns to Idle state on any error

### 6. Settings Management

All prompts and parameters loaded from SQLite database:
- Connection: URL, model name, overall timeout
- Summary: temperature, context length, timeout, preprompt, prompt
- Keywords: temperature, context length, timeout, preprompt, prompt
- Refinement: temperature, context length, timeout, preprompts, refinement prompt

Settings reloaded when Settings dialog is saved.

## Key Implementation Details

1. **Copyright Always Removed**: The `removeCopyrightNotices()` function is always called regardless of input type
2. **Sequential Pipeline**: Each stage must complete before the next begins via `advanceToNextStage()`
3. **Verbose Logging**: Full prompts and network responses logged to Run Log
4. **Single QueryRunner**: Manages entire pipeline for both PDF and text input
5. **Atomic Clear**: All output controls cleared at start of each analysis
6. **No UI Modifications**: Workflow uses existing UI without changes