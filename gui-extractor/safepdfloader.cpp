#include "safepdfloader.h"
#include <QFileInfo>
#include <QStorageInfo>
#include <QDir>
#include <QDebug>
#include <QEventLoop>
#include <exception>
#include <stdexcept>

SafePdfLoader::SafePdfLoader(QObject *parent) : QObject(parent) {
}

SafePdfLoader::~SafePdfLoader() {
}

bool SafePdfLoader::loadPdf(QPdfDocument* doc, const QString& path, QString& errorMsg, int timeoutMs) {
    if (!doc) {
        errorMsg = "Invalid QPdfDocument pointer";
        return false;
    }

    // First validate the file
    if (!validatePdfFile(path, errorMsg)) {
        return false;
    }

    try {
        // Create a timer for timeout protection
        QTimer timeoutTimer;
        timeoutTimer.setSingleShot(true);
        timeoutTimer.setInterval(timeoutMs);

        bool timedOut = false;

        // Connect timeout to close the document
        QObject::connect(&timeoutTimer, &QTimer::timeout, [doc, &timedOut]() {
            timedOut = true;
            if (doc->status() == QPdfDocument::Status::Loading) {
                doc->close();
            }
        });

        // Start timeout timer
        timeoutTimer.start();

        // Attempt to load
        QPdfDocument::Error result = tryLoadPdf(doc, path);

        // Stop timer
        timeoutTimer.stop();

        if (timedOut) {
            errorMsg = QString("PDF loading timed out after %1ms").arg(timeoutMs);
            doc->close();
            return false;
        }

        if (result != QPdfDocument::Error::None) {
            switch (result) {
                case QPdfDocument::Error::FileNotFound:
                    errorMsg = "PDF file not found";
                    break;
                case QPdfDocument::Error::InvalidFileFormat:
                    errorMsg = "Invalid PDF file format";
                    break;
                case QPdfDocument::Error::IncorrectPassword:
                    errorMsg = "PDF is password protected";
                    break;
                case QPdfDocument::Error::UnsupportedSecurityScheme:
                    errorMsg = "PDF has unsupported security scheme";
                    break;
                default:
                    errorMsg = QString("Failed to load PDF (error code: %1)").arg(static_cast<int>(result));
            }
            return false;
        }

        // Additional validation - check if document has pages
        if (doc->pageCount() == 0) {
            errorMsg = "PDF has no pages";
            doc->close();
            return false;
        }

        return true;

    } catch (const std::exception& e) {
        errorMsg = QString("Exception loading PDF: %1").arg(e.what());
        logError("loadPdf", errorMsg);
        try {
            doc->close();
        } catch (...) {
            // Ignore cleanup exceptions
        }
        return false;
    } catch (...) {
        errorMsg = "Unknown exception loading PDF";
        logError("loadPdf", errorMsg);
        try {
            doc->close();
        } catch (...) {
            // Ignore cleanup exceptions
        }
        return false;
    }
}

bool SafePdfLoader::validatePdfFile(const QString& path, QString& errorMsg) {
    try {
        QFileInfo fileInfo(path);

        // Check if file exists
        if (!fileInfo.exists()) {
            errorMsg = "PDF file does not exist";
            return false;
        }

        // Check if file is readable
        if (!fileInfo.isReadable()) {
            errorMsg = "PDF file is not readable (check permissions)";
            return false;
        }

        // Check file size
        if (!checkFileSize(path)) {
            errorMsg = "PDF file is too large (>500MB)";
            return false;
        }

        // Check if it's actually a file
        if (!fileInfo.isFile()) {
            errorMsg = "Path is not a file";
            return false;
        }

        // Basic PDF header check
        QFile file(path);
        if (file.open(QIODevice::ReadOnly)) {
            QByteArray header = file.read(5);
            file.close();
            if (header != "%PDF-" && !header.startsWith("%PDF")) {
                errorMsg = "File does not appear to be a PDF (invalid header)";
                return false;
            }
        } else {
            errorMsg = "Cannot open file for validation";
            return false;
        }

        return true;

    } catch (const std::exception& e) {
        errorMsg = QString("Exception validating PDF: %1").arg(e.what());
        logError("validatePdfFile", errorMsg);
        return false;
    } catch (...) {
        errorMsg = "Unknown exception validating PDF";
        logError("validatePdfFile", errorMsg);
        return false;
    }
}

QString SafePdfLoader::extractTextSafely(QPdfDocument* doc, QString& errorMsg) {
    if (!doc) {
        errorMsg = "Invalid QPdfDocument pointer";
        return QString();
    }

    try {
        QString allText;
        int pageCount = doc->pageCount();

        if (pageCount == 0) {
            errorMsg = "PDF has no pages";
            return QString();
        }

        // Limit pages to prevent excessive memory usage
        const int maxPages = 1000;
        if (pageCount > maxPages) {
            pageCount = maxPages;
            qDebug() << QString("Limiting text extraction to first %1 pages").arg(maxPages);
        }

        for (int i = 0; i < pageCount; ++i) {
            try {
                QString pageText = doc->getAllText(i).text();

                // Check for excessive page text size
                if (pageText.length() > 1000000) { // 1MB per page limit
                    pageText = pageText.left(1000000);
                    qDebug() << QString("Truncated page %1 text to 1MB").arg(i);
                }

                allText += pageText;
                allText += "\n\n";

                // Check total text size
                if (allText.length() > 10000000) { // 10MB total limit
                    allText = allText.left(10000000);
                    qDebug() << "Total text exceeded 10MB, truncating";
                    break;
                }

            } catch (const std::exception& e) {
                qDebug() << QString("Exception extracting page %1: %2").arg(i).arg(e.what());
                // Continue with other pages
            } catch (...) {
                qDebug() << QString("Unknown exception extracting page %1").arg(i);
                // Continue with other pages
            }
        }

        if (allText.isEmpty()) {
            errorMsg = "No text could be extracted from PDF";
            return QString();
        }

        return allText;

    } catch (const std::exception& e) {
        errorMsg = QString("Exception extracting text: %1").arg(e.what());
        logError("extractTextSafely", errorMsg);
        return QString();
    } catch (...) {
        errorMsg = "Unknown exception extracting text";
        logError("extractTextSafely", errorMsg);
        return QString();
    }
}

bool SafePdfLoader::checkFileSize(const QString& path, qint64 maxSizeBytes) {
    try {
        QFileInfo fileInfo(path);
        return fileInfo.size() <= maxSizeBytes;
    } catch (...) {
        return false;
    }
}

QPdfDocument::Error SafePdfLoader::tryLoadPdf(QPdfDocument* doc, const QString& path) {
    try {
        return doc->load(path);
    } catch (const std::exception& e) {
        qDebug() << "Exception in QPdfDocument::load:" << e.what();
        return QPdfDocument::Error::InvalidFileFormat;
    } catch (...) {
        qDebug() << "Unknown exception in QPdfDocument::load";
        return QPdfDocument::Error::InvalidFileFormat;
    }
}

void SafePdfLoader::logError(const QString& context, const QString& error) {
    qDebug() << QString("[SafePdfLoader::%1] %2").arg(context).arg(error);
}