#ifndef SAFEPDFLOADER_H
#define SAFEPDFLOADER_H

#include <QObject>
#include <QString>
#include <QPdfDocument>
#include <QTimer>
#include <memory>

class SafePdfLoader : public QObject {
    Q_OBJECT

public:
    explicit SafePdfLoader(QObject *parent = nullptr);
    ~SafePdfLoader();

    // Safe PDF loading with exception handling and timeouts
    static bool loadPdf(QPdfDocument* doc, const QString& path, QString& errorMsg, int timeoutMs = 30000);

    // Validate PDF file before loading
    static bool validatePdfFile(const QString& path, QString& errorMsg);

    // Extract text with safety checks
    static QString extractTextSafely(QPdfDocument* doc, QString& errorMsg);

    // Check if file size is acceptable (default max 500MB)
    static bool checkFileSize(const QString& path, qint64 maxSizeBytes = 500 * 1024 * 1024);

private:
    // Helper to safely attempt PDF load with exception handling
    static QPdfDocument::Error tryLoadPdf(QPdfDocument* doc, const QString& path);

    // Logging helper
    static void logError(const QString& context, const QString& error);
};

#endif // SAFEPDFLOADER_H