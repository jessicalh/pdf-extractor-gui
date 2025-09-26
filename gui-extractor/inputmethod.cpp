#include "inputmethod.h"
#include "safepdfloader.h"
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QTextEdit>
#include <QLineEdit>
#include <QListWidget>
#include <QFileDialog>
#include <QMessageBox>
#include <QPdfDocument>
#include <QNetworkAccessManager>
#include <QRegularExpression>
#include <QDebug>
#include <memory>

// Base class helper method
QString InputMethod::stripCopyright(const QString& text) {
    QString processedText = text;

    // Remove Elsevier copyright
    QRegularExpression elsevierRegex(
        R"(This\s+article\s+appeared\s+in\s+a\s+journal\s+published\s+by\s+Elsevier\.[\s\S]*?)"
        R"(available\s+at\s+ScienceDirect[\s\S]*?)"
        R"(journal\s+homepage:\s*www\.elsevier\.com/locate/[\w]+\s*)",
        QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption
    );
    processedText.replace(elsevierRegex, "");

    // Remove author license lines
    QRegularExpression licenseRegex(
        R"(\d{4}\s+.{0,100}(license|published by|copyright|\(C\)|©).{0,200}\s*)",
        QRegularExpression::CaseInsensitiveOption | QRegularExpression::MultilineOption
    );
    processedText.replace(licenseRegex, "");

    // Remove page headers/footers
    QRegularExpression headerFooterRegex(
        R"(^.{0,100}\d+\s*$)",
        QRegularExpression::MultilineOption
    );
    processedText.replace(headerFooterRegex, "");

    // Clean up extra whitespace
    processedText.replace(QRegularExpression(R"(\n{3,})"), "\n\n");
    processedText = processedText.trimmed();

    return processedText;
}

// PDF File Input Method Implementation
PdfFileInputMethod::PdfFileInputMethod(QWidget *parentWidget, QObject *parent)
    : InputMethod(parent), m_pdfDocument(nullptr) {

    m_widget = new QWidget(parentWidget);
    auto layout = new QVBoxLayout(m_widget);

    // File selection area
    auto fileLayout = new QHBoxLayout();
    m_selectButton = new QPushButton("Select PDF File", m_widget);
    m_fileLabel = new QLabel("No file selected", m_widget);
    m_fileLabel->setWordWrap(true);

    fileLayout->addWidget(m_selectButton);
    fileLayout->addWidget(m_fileLabel, 1);

    layout->addLayout(fileLayout);
    layout->addStretch();

    // Connect button
    connect(m_selectButton, &QPushButton::clicked, [this]() {
        QString fileName = QFileDialog::getOpenFileName(
            m_widget,
            "Select PDF File",
            "",
            "PDF Files (*.pdf)"
        );
        if (!fileName.isEmpty()) {
            m_currentFilePath = fileName;
            m_fileLabel->setText(fileName);
            emit statusUpdate("PDF file selected: " + fileName);
        }
    });
}

PdfFileInputMethod::~PdfFileInputMethod() {
    cleanup();
    delete m_widget;
}

QWidget* PdfFileInputMethod::getWidget() {
    return m_widget;
}

QString PdfFileInputMethod::validate() {
    if (m_currentFilePath.isEmpty()) {
        return "Please select a PDF file first";
    }

    QFile file(m_currentFilePath);
    if (!file.exists()) {
        return "Selected file does not exist";
    }

    return QString(); // Valid
}

QString PdfFileInputMethod::extractText() {
    QString error = validate();
    if (!error.isEmpty()) {
        return QString();
    }

    // Clean up any previous document
    cleanup();

    try {
        // Use SafePdfLoader for validation and loading
        QString loadError;
        auto pdfDoc = std::make_unique<QPdfDocument>();

        if (!SafePdfLoader::loadPdf(pdfDoc.get(), m_currentFilePath, loadError, 30000)) {
            emit statusUpdate("Error: " + loadError);
            qDebug() << "PDF load failed:" << loadError;
            return QString();
        }

        // Extract text safely
        QString extractError;
        QString allText = SafePdfLoader::extractTextSafely(pdfDoc.get(), extractError);

        if (allText.isEmpty()) {
            emit statusUpdate("Error: " + extractError);
            qDebug() << "Text extraction failed:" << extractError;
            return QString();
        }

        QString processedText = stripCopyright(allText);
        emit statusUpdate("Text extraction completed successfully");
        return processedText;

    } catch (const std::exception& e) {
        QString errorMsg = QString("Exception during PDF extraction: %1").arg(e.what());
        emit statusUpdate(errorMsg);
        qDebug() << errorMsg;
        return QString();
    } catch (...) {
        emit statusUpdate("Unknown exception during PDF extraction");
        qDebug() << "Unknown exception in PdfFileInputMethod::extractText";
        return QString();
    }
}

void PdfFileInputMethod::cleanup() {
    try {
        if (m_pdfDocument) {
            delete m_pdfDocument;
            m_pdfDocument = nullptr;
        }
    } catch (...) {
        qDebug() << "Exception during PDF document cleanup";
        m_pdfDocument = nullptr;
    }
}

void PdfFileInputMethod::reset() {
    m_currentFilePath.clear();
    m_fileLabel->setText("No file selected");
    cleanup();
}

// Paste Text Input Method Implementation
PasteTextInputMethod::PasteTextInputMethod(QWidget *parentWidget, QObject *parent)
    : InputMethod(parent) {

    m_widget = new QWidget(parentWidget);
    auto layout = new QVBoxLayout(m_widget);

    auto label = new QLabel("Paste or type your text here:", m_widget);
    m_textEdit = new QTextEdit(m_widget);
    m_textEdit->setPlaceholderText("Paste your text here...");

    layout->addWidget(label);
    layout->addWidget(m_textEdit, 1);
}

PasteTextInputMethod::~PasteTextInputMethod() {
    delete m_widget;
}

QWidget* PasteTextInputMethod::getWidget() {
    return m_widget;
}

QString PasteTextInputMethod::validate() {
    if (m_textEdit->toPlainText().trimmed().isEmpty()) {
        return "Please paste or type some text first";
    }
    return QString(); // Valid
}

QString PasteTextInputMethod::extractText() {
    QString text = m_textEdit->toPlainText();
    return stripCopyright(text);
}

void PasteTextInputMethod::cleanup() {
    // No resources to clean up for paste text
}

void PasteTextInputMethod::reset() {
    m_textEdit->clear();
}

// Zotero Input Method Implementation (Framework)
ZoteroInputMethod::ZoteroInputMethod(QWidget *parentWidget, QObject *parent)
    : InputMethod(parent), m_networkManager(nullptr) {

    m_widget = new QWidget(parentWidget);
    auto layout = new QVBoxLayout(m_widget);

    // Search section
    auto searchLayout = new QHBoxLayout();
    m_searchField = new QLineEdit(m_widget);
    m_searchField->setPlaceholderText("Search Zotero library...");
    m_searchButton = new QPushButton("Search", m_widget);

    searchLayout->addWidget(m_searchField);
    searchLayout->addWidget(m_searchButton);

    // Results section
    auto resultsLabel = new QLabel("Search results:", m_widget);
    m_resultsList = new QListWidget(m_widget);

    // Status section
    m_statusLabel = new QLabel("Ready to search", m_widget);
    m_statusLabel->setStyleSheet("QLabel { color: #666; }");

    // Add to layout
    layout->addLayout(searchLayout);
    layout->addWidget(resultsLabel);
    layout->addWidget(m_resultsList, 1);
    layout->addWidget(m_statusLabel);

    // Connect signals
    connect(m_searchButton, &QPushButton::clicked, this, &ZoteroInputMethod::onSearchClicked);
    connect(m_resultsList, &QListWidget::itemDoubleClicked, this, &ZoteroInputMethod::onItemSelected);

    // Create network manager
    m_networkManager = new QNetworkAccessManager(this);
}

ZoteroInputMethod::~ZoteroInputMethod() {
    cleanup();
    delete m_widget;
}

QWidget* ZoteroInputMethod::getWidget() {
    return m_widget;
}

QString ZoteroInputMethod::validate() {
    if (m_extractedText.isEmpty()) {
        return "Please search and select a paper from Zotero first";
    }
    return QString(); // Valid
}

QString ZoteroInputMethod::extractText() {
    // For async method, the text is already extracted during download
    return m_extractedText;
}

void ZoteroInputMethod::cleanup() {
    // Clean up downloaded PDF if exists
    if (!m_downloadedPdfPath.isEmpty()) {
        QFile::remove(m_downloadedPdfPath);
        m_downloadedPdfPath.clear();
    }

    // Clean up network resources
    if (m_networkManager) {
        m_networkManager->clearAccessCache();
        m_networkManager->clearConnectionCache();
    }

    m_extractedText.clear();
}

void ZoteroInputMethod::reset() {
    cleanup();
    m_searchField->clear();
    m_resultsList->clear();
    m_statusLabel->setText("Ready to search");
}

void ZoteroInputMethod::onSearchClicked() {
    QString query = m_searchField->text().trimmed();
    if (query.isEmpty()) {
        QMessageBox::warning(m_widget, "Search Error", "Please enter a search query");
        return;
    }

    m_statusLabel->setText("Searching Zotero...");
    searchZotero(query);
}

void ZoteroInputMethod::onItemSelected() {
    // This would be implemented to handle item selection
    m_statusLabel->setText("Downloading PDF...");
    // downloadPdf(itemId);
}

void ZoteroInputMethod::onDownloadComplete() {
    m_statusLabel->setText("Extracting text from PDF...");
    m_extractedText = extractFromPdf(m_downloadedPdfPath);

    if (!m_extractedText.isEmpty()) {
        m_statusLabel->setText("PDF downloaded and text extracted - ready to analyze");
        emit extractionComplete(m_extractedText);
    } else {
        m_statusLabel->setText("Failed to extract text from PDF");
        emit extractionError("Could not extract text from the downloaded PDF");
    }
}

void ZoteroInputMethod::searchZotero(const QString& query) {
    // TODO: Implement Zotero API search
    // This is a framework - actual implementation would:
    // 1. Make API call to Zotero
    // 2. Parse results
    // 3. Populate m_resultsList

    // Placeholder for now
    m_resultsList->clear();
    m_resultsList->addItem("Sample Paper 1 - Author et al. (2024)");
    m_resultsList->addItem("Sample Paper 2 - Another Author (2023)");
    m_statusLabel->setText("Search complete - double-click to select");
}

void ZoteroInputMethod::downloadPdf(const QString& itemId) {
    // TODO: Implement PDF download from Zotero
    // This is a framework - actual implementation would:
    // 1. Get PDF URL from Zotero API
    // 2. Download PDF to temp file
    // 3. Call onDownloadComplete when done

    // Placeholder
    m_statusLabel->setText("Download feature not yet implemented");
}

QString ZoteroInputMethod::extractFromPdf(const QString& pdfPath) {
    try {
        // Use SafePdfLoader for safe PDF loading and extraction
        QString loadError;
        auto pdfDoc = std::make_unique<QPdfDocument>();

        if (!SafePdfLoader::loadPdf(pdfDoc.get(), pdfPath, loadError, 30000)) {
            qDebug() << "Zotero PDF load failed:" << loadError;
            return QString();
        }

        // Extract text safely
        QString extractError;
        QString allText = SafePdfLoader::extractTextSafely(pdfDoc.get(), extractError);

        if (allText.isEmpty()) {
            qDebug() << "Zotero text extraction failed:" << extractError;
            return QString();
        }

        return stripCopyright(allText);

    } catch (const std::exception& e) {
        qDebug() << "Exception in ZoteroInputMethod::extractFromPdf:" << e.what();
        return QString();
    } catch (...) {
        qDebug() << "Unknown exception in ZoteroInputMethod::extractFromPdf";
        return QString();
    }
}