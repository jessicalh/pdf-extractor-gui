# InputMethod Integration Guide

## Overview
This document shows how to integrate the new InputMethod abstraction into the existing main.cpp without major UI changes.

## Key Integration Points

### 1. Add includes
```cpp
#include "inputmethod.h"
```

### 2. Replace individual input UI elements with InputMethod instances

In the MainWindow class, replace:
```cpp
// OLD:
QLineEdit *m_filePathEdit;
QPushButton *m_browseButton;
QTextEdit *m_pasteTextEdit;

// NEW:
QVector<InputMethod*> m_inputMethods;
InputMethod* m_currentInputMethod;
```

### 3. Initialize input methods in constructor

```cpp
// Create input methods
m_inputMethods.append(new PdfFileInputMethod(this, this));
m_inputMethods.append(new PasteTextInputMethod(this, this));
m_inputMethods.append(new ZoteroInputMethod(this, this));

// Add tabs for each input method
for (InputMethod* method : m_inputMethods) {
    m_inputTabWidget->addTab(method->getWidget(), method->getName());
}

// Set default
m_currentInputMethod = m_inputMethods[0];

// Connect tab changes
connect(m_inputTabWidget, &QTabWidget::currentChanged, [this](int index) {
    if (index >= 0 && index < m_inputMethods.size()) {
        m_currentInputMethod = m_inputMethods[index];
    }
});
```

### 4. Unify analyze button handling

Replace separate analyzePdf() and analyzeText() with:

```cpp
void analyze() {
    if (!m_currentInputMethod) {
        updateStatus("No input method selected");
        return;
    }

    // Validate input
    QString error = m_currentInputMethod->validate();
    if (!error.isEmpty()) {
        QMessageBox::warning(this, "Input Error", error);
        return;
    }

    if (m_queryRunner->isProcessing()) {
        updateStatus("Processing already in progress");
        return;
    }

    // Extract text
    QString extractedText = m_currentInputMethod->extractText();
    if (extractedText.isEmpty()) {
        updateStatus("Failed to extract text");
        return;
    }

    // Clean up input method resources
    m_currentInputMethod->cleanup();

    // Display extracted text
    m_extractedTextEdit->setPlainText(extractedText);

    // Start processing
    startProcessing(extractedText);
}
```

### 5. Handle async input methods (Zotero)

For async methods like Zotero:

```cpp
// In constructor, connect async signals
for (InputMethod* method : m_inputMethods) {
    if (method->isAsync()) {
        connect(method, &InputMethod::extractionComplete,
                this, &MainWindow::onExtractionComplete);
        connect(method, &InputMethod::extractionError,
                this, &MainWindow::onExtractionError);
        connect(method, &InputMethod::statusUpdate,
                this, &MainWindow::updateStatus);
    }
}

// Handler methods
void onExtractionComplete(const QString& text) {
    m_extractedTextEdit->setPlainText(text);
    startProcessing(text);
}

void onExtractionError(const QString& error) {
    QMessageBox::warning(this, "Extraction Error", error);
    updateStatus("Extraction failed: " + error);
}
```

### 6. Cleanup in destructor

```cpp
~MainWindow() {
    // Clean up all input methods
    for (InputMethod* method : m_inputMethods) {
        method->cleanup();
        delete method;
    }
}
```

## Benefits of This Approach

1. **Minimal UI Changes**: The UI remains exactly the same from user perspective
2. **Unified Processing**: Single analyze path for all input methods
3. **Easy Extension**: Adding new input methods is straightforward
4. **Resource Safety**: Guaranteed cleanup of resources
5. **Async Support**: Framework supports both sync and async input methods

## Migration Path

1. Keep existing UI elements initially
2. Create InputMethod wrappers that use existing UI
3. Gradually migrate functionality into InputMethod classes
4. Remove old UI code once migration is complete

## Testing Strategy

1. Test each input method independently
2. Verify resource cleanup (no file locks, network connections)
3. Test switching between input methods
4. Test error handling within each method
5. Test async operations for Zotero

## Zotero Integration TODO

The Zotero input method is currently a framework. To complete it:

1. **API Authentication**:
   - Add API key settings
   - OAuth flow for user authentication

2. **Search Implementation**:
   ```cpp
   void searchZotero(const QString& query) {
       QNetworkRequest request(QUrl("https://api.zotero.org/users/{userId}/items"));
       request.setRawHeader("Zotero-API-Version", "3");
       request.setRawHeader("Authorization", "Bearer " + apiKey);

       // Add search parameters
       QUrlQuery params;
       params.addQueryItem("q", query);
       params.addQueryItem("format", "json");

       auto reply = m_networkManager->get(request);
       connect(reply, &QNetworkReply::finished, [this, reply]() {
           // Parse JSON response
           // Populate m_resultsList
       });
   }
   ```

3. **PDF Download**:
   - Get attachment links from item
   - Download to temp directory
   - Extract text using existing PDF extraction

4. **Settings Storage**:
   - Store API credentials in database
   - Remember user preferences
   - Cache recent searches

## Alternative Lighter Integration

If you prefer an even lighter touch:

1. Keep all existing UI code
2. Add InputMethod as a wrapper layer
3. Create adapters that bridge existing UI to InputMethod interface
4. Add Zotero as a third tab using the new system
5. Gradually refactor existing tabs to use InputMethod

This allows incremental migration without breaking existing functionality.