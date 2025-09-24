# PDF Extractor GUI - Error Recovery and Abort Control Solution

## Problem Statement

The application enters an unusable state when a prompt fails during processing. The QueryRunner remains stuck in a non-Idle state (e.g., `GeneratingSummary`), preventing users from starting new analyses. Users must restart the application to continue working.

## Core Issues

1. **State Not Reset on Errors**: When PromptQuery emits `errorOccurred`, QueryRunner forwards the signal but doesn't reset `m_currentStage` to Idle
2. **Processing Check Blocks Restart**: `processPDF()` and `processText()` check `if (m_currentStage != Idle)` and reject new requests
3. **Timeout Noise**: Network timeouts show disruptive message boxes instead of just logging
4. **No Cancel Option**: Users cannot abort long-running operations

## Solution Architecture

### 1. State Management Improvements

#### Add Reset and Abort Methods to QueryRunner

```cpp
// queryrunner.h
public:
    void reset();    // Resets to Idle state for error recovery
    void abort();    // User-initiated graceful cancellation

signals:
    void abortRequested();  // Signal to cancel operations

// queryrunner.cpp
void QueryRunner::reset() {
    m_currentStage = Idle;
    emit stageChanged(m_currentStage);
    emit progressMessage("Ready for new analysis");
}

void QueryRunner::abort() {
    emit progressMessage("Aborting current operation...");
    emit abortRequested();  // Tell queries to stop
    reset();
}
```

### 2. Centralized Error Handling

#### Differentiate Error Types

```cpp
// queryrunner.h
private slots:
    void handleQueryError(const QString& error);

private:
    QString getStageString(ProcessingStage stage) const;

// queryrunner.cpp
void QueryRunner::handleQueryError(const QString& error) {
    QString contextError = QString("[%1] %2")
        .arg(getStageString(m_currentStage))
        .arg(error);

    // Check if it's a timeout - these are expected for large docs
    bool isTimeout = error.contains("timeout", Qt::CaseInsensitive) ||
                     error.contains("timed out", Qt::CaseInsensitive);

    if (isTimeout) {
        // Only log timeouts, don't show message box
        emit progressMessage("WARNING: Request timed out - this is normal for large documents");
        emit progressMessage("You can retry with a shorter document or adjust timeout in settings");
    } else {
        // Other errors get full treatment
        emit errorOccurred(contextError);
    }

    // Always reset state so user can retry
    reset();
}
```

### 3. Abort Button Implementation

#### UI Changes (main.cpp)

```cpp
// Add member variable
QPushButton* m_abortButton;

// In constructor, after creating settings button
m_abortButton = new QPushButton("⬛");  // Stop sign icon
m_abortButton->setToolTip("Stop Processing");
m_abortButton->setFixedSize(30, 30);
m_abortButton->setEnabled(false);  // Initially disabled
headerLayout->addWidget(m_abortButton);
headerLayout->addWidget(m_settingsButton);

// In connectSignals()
connect(m_abortButton, &QPushButton::clicked, [this]() {
    if (m_queryRunner->isProcessing()) {
        m_queryRunner->abort();
        updateStatus("Processing cancelled");
        setUIEnabled(true);
        stopSpinner();
    }
});

// Update handleStageChanged to manage abort button
void handleStageChanged(QueryRunner::ProcessingStage stage) {
    // Enable abort button when processing
    m_abortButton->setEnabled(stage != QueryRunner::Idle &&
                              stage != QueryRunner::Complete);
    // ... rest of existing code
}
```

### 4. Query Abort Support

#### Make PromptQuery Abortable

```cpp
// promptquery.h
public:
    void abort();  // Cancel current request

// promptquery.cpp
void PromptQuery::abort() {
    if (m_currentReply && m_currentReply->isRunning()) {
        m_currentReply->abort();
        m_currentReply->deleteLater();
        m_currentReply = nullptr;
    }
    if (m_timeoutTimer->isActive()) {
        m_timeoutTimer->stop();
    }
}
```

#### Connect Abort Signal in QueryRunner

```cpp
// In QueryRunner constructor
connect(this, &QueryRunner::abortRequested, m_summaryQuery, &PromptQuery::abort);
connect(this, &QueryRunner::abortRequested, m_keywordsQuery, &PromptQuery::abort);
connect(this, &QueryRunner::abortRequested, m_refineQuery, &PromptQuery::abort);
connect(this, &QueryRunner::abortRequested, m_refinedKeywordsQuery, &PromptQuery::abort);
```

### 5. Improved Error Messages in UI

#### Update handleError in main.cpp

```cpp
void handleError(const QString& error) {
    log("ERROR: " + error);

    // Don't show message box for timeouts
    if (!error.contains("timeout", Qt::CaseInsensitive)) {
        QMessageBox::critical(this, "Error", error);
    }

    setUIEnabled(true);
    stopSpinner();
    updateStatus("Ready - previous operation had errors");

    // Abort button should be disabled after error
    m_abortButton->setEnabled(false);
}
```

### 6. Allow Recovery from Stuck States

#### Modify processPDF and processText

```cpp
void QueryRunner::processPDF(const QString& filePath) {
    if (m_currentStage != Idle) {
        // Auto-recover instead of blocking
        emit progressMessage("Note: Resetting from previous incomplete operation");
        reset();
    }
    // ... continue with normal processing
}

void QueryRunner::processText(const QString& text) {
    if (m_currentStage != Idle) {
        emit progressMessage("Note: Resetting from previous incomplete operation");
        reset();
    }
    // ... continue with normal processing
}
```

## Key Design Principles

1. **Graceful Recovery**: Always reset to Idle on errors
2. **User Control**: Abort button for cancelling operations
3. **Quiet Timeouts**: Expected timeouts only log, don't disrupt
4. **Preserved Results**: Partial results remain visible after errors
5. **Clear State**: Abort button shows when operation can be cancelled
6. **Auto-Recovery**: Starting new analysis auto-resets if needed

## Testing Scenarios

1. **Timeout Recovery**: Long document → Timeout → Can immediately retry
2. **Abort Mid-Process**: Start analysis → Click abort → Can start new analysis
3. **Error Recovery**: Network error → Message shown → Can retry
4. **Partial Results**: Complete 2 stages → Error in stage 3 → Results 1&2 visible
5. **Settings Change**: Error → Change settings → Retry works

## Benefits

- ✅ Application never gets stuck in unusable state
- ✅ Timeouts handled gracefully without popups
- ✅ Users can cancel long operations
- ✅ Partial results preserved
- ✅ Clear visual feedback via abort button state
- ✅ Minimal code changes with maximum reliability