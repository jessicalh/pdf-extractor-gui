# Exception Handling Improvements for PDF Extractor GUI

## Critical Issues Found

### 1. QPdfDocument Crash Points
- **Location**: `zoteroinput.cpp:530`, `inputmethod.cpp:118`, `queryrunner.cpp:188`
- **Issue**: QPdfDocument::load() can crash on malformed PDFs or OOM conditions
- **Fix**: Wrap in try-catch and validate file size before loading

### 2. Network Reply Management
- **Location**: Throughout `zoteroinput.cpp`
- **Issue**: deleteLater() called without disconnecting signals first
- **Fix**: Always disconnect() before deleteLater()

### 3. Temporary File Operations
- **Location**: `zoteroinput.cpp:339-348`
- **Issue**: No exception handling for disk full/permissions
- **Fix**: Add try-catch blocks around file operations

## Recommended Implementation

### 1. Safe QPdfDocument Loading
```cpp
bool SafeLoadPdf(QPdfDocument* doc, const QString& path) {
    try {
        // Check file size first
        QFileInfo fileInfo(path);
        if (fileInfo.size() > 500 * 1024 * 1024) { // 500MB limit
            emit errorOccurred("PDF file too large (>500MB)");
            return false;
        }

        // Attempt load with timeout
        QTimer::singleShot(30000, [doc]() {
            if (doc->status() == QPdfDocument::Status::Loading) {
                doc->close();
            }
        });

        auto result = doc->load(path);
        return result == QPdfDocument::Error::None;
    } catch (const std::exception& e) {
        qDebug() << "PDF load exception:" << e.what();
        return false;
    } catch (...) {
        qDebug() << "Unknown PDF load exception";
        return false;
    }
}
```

### 2. Safe Network Reply Cleanup
```cpp
void SafeCleanupReply(QNetworkReply*& reply) {
    if (!reply) return;

    // Disconnect all signals first
    reply->disconnect();

    // Schedule deletion
    reply->deleteLater();
    reply = nullptr;
}
```

### 3. Protected Temporary File Creation
```cpp
bool CreateTempPdfFile() {
    try {
        m_tempPdfFile = std::make_unique<QTemporaryFile>(
            QDir::temp().absoluteFilePath("zotero_XXXXXX.pdf"));

        if (!m_tempPdfFile->open()) {
            throw std::runtime_error("Failed to create temp file");
        }

        // Check available disk space
        QStorageInfo storage(QDir::temp());
        if (storage.bytesAvailable() < 100 * 1024 * 1024) { // 100MB min
            throw std::runtime_error("Insufficient disk space");
        }

        m_tempPdfFile->setAutoRemove(false);
        return true;
    } catch (const std::exception& e) {
        logError(QString("Temp file creation failed: %1").arg(e.what()));
        m_tempPdfFile.reset();
        return false;
    }
}
```

### 4. Memory-Safe PDF Validation
```cpp
void ZoteroInputWidget::handlePdfDownloadReply() {
    // ... existing code ...

    if (!pdfData.isEmpty()) {
        // Write to temp file first
        if (m_tempPdfFile && m_tempPdfFile->isOpen()) {
            try {
                m_tempPdfFile->write(pdfData);
                m_tempPdfFile->flush();
                m_tempPdfFile->close();
            } catch (...) {
                showError("Failed to write PDF to disk");
                m_downloadedPdfPath.clear();
                return;
            }
        }

        // Validate PDF in separate scope to ensure cleanup
        {
            std::unique_ptr<QPdfDocument> testDoc;
            try {
                testDoc = std::make_unique<QPdfDocument>();
                if (!SafeLoadPdf(testDoc.get(), m_downloadedPdfPath)) {
                    throw std::runtime_error("Invalid PDF format");
                }
            } catch (const std::exception& e) {
                showError(QString("PDF validation failed: %1").arg(e.what()));
                m_downloadedPdfPath.clear();
                return;
            }
            // testDoc automatically cleaned up here
        }

        // PDF is valid, continue...
    }
}
```

### 5. Guard Against Double-Delete
```cpp
void ZoteroInputWidget::handlePdfDownloadReply() {
    if (!m_currentReply) return;

    // Take ownership and clear member immediately
    QNetworkReply* reply = m_currentReply;
    m_currentReply = nullptr;

    // Ensure cleanup on all paths
    auto cleanup = qScopeGuard([&reply]() {
        if (reply) {
            reply->disconnect();
            reply->deleteLater();
        }
    });

    // ... rest of handling code ...
}
```

### 6. Main Application Exception Barrier
```cpp
void PDFExtractorGUI::analyzeZoteroPdf(const QString& pdfPath) {
    try {
        if (pdfPath.isEmpty()) {
            throw std::invalid_argument("Empty PDF path");
        }

        // Validate file exists and is readable
        QFileInfo fileInfo(pdfPath);
        if (!fileInfo.exists() || !fileInfo.isReadable()) {
            throw std::runtime_error("PDF file not accessible");
        }

        // Size check
        if (fileInfo.size() > 500 * 1024 * 1024) {
            throw std::runtime_error("PDF too large (>500MB)");
        }

        // Proceed with processing
        setUIEnabled(false);
        startSpinner();
        updateStatus("Starting Zotero PDF analysis...");
        clearResults();

        m_queryRunner->processPdf(pdfPath);

    } catch (const std::exception& e) {
        handleError(QString("Zotero PDF analysis failed: %1").arg(e.what()));
        setUIEnabled(true);
        stopSpinner();
    } catch (...) {
        handleError("Zotero PDF analysis failed: Unknown error");
        setUIEnabled(true);
        stopSpinner();
    }
}
```

## Implementation Priority

1. **IMMEDIATE**: Add try-catch blocks around all QPdfDocument::load() calls
2. **IMMEDIATE**: Fix network reply cleanup to disconnect before deleteLater
3. **HIGH**: Add file size validation before PDF loading
4. **HIGH**: Implement timeout protection for PDF loading
5. **MEDIUM**: Add disk space checks for temp files
6. **MEDIUM**: Use qScopeGuard for RAII cleanup patterns

## Testing Recommendations

1. Test with corrupted PDF files
2. Test with very large PDFs (>1GB)
3. Test with password-protected PDFs
4. Test network interruption during Zotero download
5. Test disk full conditions
6. Test rapid abort/retry sequences
7. Test with PDFs containing malformed metadata

## Memory Leak Prevention

- Always use std::unique_ptr for QPdfDocument instances
- Clear QNetworkReply pointers immediately after taking ownership
- Use qScopeGuard for cleanup in complex functions
- Implement timeouts for all async operations
- Add memory usage logging for debugging