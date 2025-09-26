#include "zoteroinput.h"
#include "safepdfloader.h"
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonParseError>
#include <QMessageBox>
#include <QTemporaryFile>
#include <QFile>
#include <QDir>
#include <QPdfDocument>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QEventLoop>
#include <QTimer>
#include <QDebug>
#include <QDateTime>
#include <QTextStream>
#include <QCoreApplication>
#include <QStorageInfo>
#include <QScopeGuard>
#include <exception>
#include <memory>

ZoteroInputWidget::ZoteroInputWidget(QWidget *parent)
    : QWidget(parent)
    , m_networkManager(new QNetworkAccessManager(this))
    , m_currentReply(nullptr)
    , m_isLoading(false)
    , m_logFile(nullptr)
    , m_currentState(NoCredentials) {

    setupUI();

    // Initialize log file
    QString logPath = QCoreApplication::applicationDirPath() + "/zotero.log";
    m_logFile = new QFile(logPath);

    // Open in append mode to preserve previous logs
    if (m_logFile->open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        logToFile("========================================");
        logToFile(QString("Zotero Integration Started - %1").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss")));
        logToFile("========================================");
    }

    // Load credentials from database on creation
    QSqlDatabase db = QSqlDatabase::database();
    if (db.isOpen()) {
        logToFile("Loading credentials from database...");
        QSqlQuery query(db);
        if (query.exec("SELECT zotero_user_id, zotero_api_key FROM settings LIMIT 1") && query.next()) {
            m_userId = query.value("zotero_user_id").toString();
            m_apiKey = query.value("zotero_api_key").toString();
            logToFile(QString("Credentials loaded - User ID: %1, API Key: %2").arg(
                m_userId.isEmpty() ? "NOT SET" : m_userId,
                m_apiKey.isEmpty() ? "NOT SET" : "***" + m_apiKey.right(4)));
        } else {
            logToFile("No credentials found in database");
        }
    } else {
        logToFile("Database not open - cannot load credentials");
    }

    // Set initial state based on credentials
    if (!m_apiKey.isEmpty()) {
        setState(ReadyToFetch);
    } else {
        setState(NoCredentials);
    }
}

ZoteroInputWidget::~ZoteroInputWidget() {
    // Safe cleanup of network reply
    safeCleanupReply();

    if (m_logFile) {
        logToFile("Zotero Integration Shutting Down");
        m_logFile->close();
        delete m_logFile;
    }
}

void ZoteroInputWidget::setupUI() {
    auto* layout = new QVBoxLayout(this);

    // Status label at top
    m_statusLabel = new QLabel("Click refresh to load your Zotero collections", this);
    m_statusLabel->setWordWrap(true);
    layout->addWidget(m_statusLabel);

    // Collections section
    auto* collectionsLayout = new QHBoxLayout();
    collectionsLayout->addWidget(new QLabel("Collection:"));

    m_collectionsCombo = new QComboBox(this);
    m_collectionsCombo->setMinimumWidth(300);
    m_collectionsCombo->addItem("Select a collection...");
    m_collectionsCombo->setEnabled(false);
    collectionsLayout->addWidget(m_collectionsCombo, 1);

    m_refreshButton = new QPushButton("🔄 Refresh", this);
    m_refreshButton->setMaximumWidth(100);
    collectionsLayout->addWidget(m_refreshButton);

    layout->addLayout(collectionsLayout);

    // Papers section
    auto* papersLayout = new QHBoxLayout();
    papersLayout->addWidget(new QLabel("Paper:"));

    m_papersCombo = new QComboBox(this);
    m_papersCombo->setMinimumWidth(400);
    m_papersCombo->addItem("Select a paper...");
    m_papersCombo->setEnabled(false);
    papersLayout->addWidget(m_papersCombo, 1);

    layout->addLayout(papersLayout);

    // Add stretch to push analyze button to bottom
    layout->addStretch();

    // Analyze button - positioned at bottom right like other input tabs
    auto* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    m_analyzeButton = new QPushButton("Analyze", this);
    m_analyzeButton->setEnabled(false);
    // Match the style of other Analyze buttons
    m_analyzeButton->setMinimumWidth(100);
    buttonLayout->addWidget(m_analyzeButton);

    layout->addLayout(buttonLayout);

    // Connect signals
    connect(m_refreshButton, &QPushButton::clicked, this, &ZoteroInputWidget::onRefreshCollections);
    connect(m_collectionsCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ZoteroInputWidget::onCollectionChanged);
    connect(m_papersCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ZoteroInputWidget::onPaperChanged);
    connect(m_analyzeButton, &QPushButton::clicked, this, &ZoteroInputWidget::onAnalyzeClicked);
}

void ZoteroInputWidget::setCredentials(const QString& userId, const QString& apiKey) {
    logToFile(QString("setCredentials called - User ID: %1, API Key: %2").arg(
        userId.isEmpty() ? "EMPTY" : userId,
        apiKey.isEmpty() ? "EMPTY" : "***" + apiKey.right(4)));
    m_userId = userId;
    m_apiKey = apiKey;

    // Update the status to reflect credentials are loaded
    if (!userId.isEmpty() && !apiKey.isEmpty()) {
        m_statusLabel->setText("Credentials loaded. Click refresh to load your Zotero collections");
        setState(ReadyToFetch);
    } else {
        setState(NoCredentials);
    }
}

QString ZoteroInputWidget::validate() const {
    if (m_userId.isEmpty() || m_apiKey.isEmpty()) {
        return "Zotero credentials not configured. Please set them in Settings → Zotero.";
    }

    if (m_papersCombo->currentIndex() <= 0) {
        return "Please select a paper from your Zotero collection.";
    }

    if (m_downloadedPdfPath.isEmpty()) {
        return "No PDF downloaded yet. Click Analyze to download.";
    }

    return QString(); // Valid
}

void ZoteroInputWidget::reset() {
    clearCollections();
    clearPapers();
    m_downloadedPdfPath.clear();

    // Safe cleanup of temp file
    try {
        m_tempPdfFile.reset();
    } catch (...) {
        logError("Exception during temp file cleanup");
    }

    m_statusLabel->setText("Click refresh to load your Zotero collections");
    setUIEnabled(true);

    // Reset to appropriate state based on credentials
    if (!m_apiKey.isEmpty()) {
        setState(ReadyToFetch);
    } else {
        setState(NoCredentials);
    }
}

void ZoteroInputWidget::onRefreshCollections() {
    logToFile("User clicked Refresh Collections button");

    // Only need API key now - we'll fetch the user ID
    if (m_apiKey.isEmpty()) {
        logError("API key missing - showing warning dialog");
        setState(NoCredentials);
        QMessageBox::warning(this, "Configuration Required",
            "Please configure your Zotero API key in Settings → Zotero first.");
        return;
    }

    // ALWAYS fetch user ID from API key on refresh
    // This ensures we get the numeric ID even if a username was stored
    logToFile("Fetching user ID from API key");
    setState(FetchingData);
    fetchUserIdFromApiKey();
}

void ZoteroInputWidget::onCollectionChanged(int index) {
    if (index <= 0 || m_isLoading) {
        clearPapers();
        return;
    }

    if (index - 1 < m_collections.size()) {
        const auto& collection = m_collections[index - 1];
        m_currentCollectionKey = collection.key;
        fetchItemsForCollection(collection.key);
    }
}

void ZoteroInputWidget::onPaperChanged(int index) {
    m_analyzeButton->setEnabled(index > 0 && !m_isLoading);

    if (index > 0 && index - 1 < m_items.size()) {
        m_currentItem = m_items[index - 1];
    }
}

void ZoteroInputWidget::onAnalyzeClicked() {
    if (m_papersCombo->currentIndex() <= 0) {
        QMessageBox::warning(this, "Selection Required", "Please select a paper first.");
        return;
    }

    // If we haven't checked for attachments yet, fetch them first
    if (m_currentItem.hasPdf && m_currentItem.pdfAttachmentKey.isEmpty()) {
        // Need to fetch children to get attachment key
        fetchChildrenForItem(m_currentItem.key);
        return;
    }

    if (!m_currentItem.hasPdf) {
        QMessageBox::warning(this, "No PDF Available",
            "The selected item does not have a PDF attachment.");
        return;
    }

    // Download the PDF
    downloadPdf(m_currentItem.key, m_currentItem.pdfAttachmentKey);
}

void ZoteroInputWidget::fetchCollections() {
    // Safe cleanup of any existing reply
    safeCleanupReply();

    setUIEnabled(false);
    m_isLoading = true;
    m_statusLabel->setText("Loading collections...");
    emit statusMessage("Fetching Zotero collections...");

    // Clear existing data
    clearCollections();
    clearPapers();

    // API endpoint for user's collections
    QString url = QString("https://api.zotero.org/users/%1/collections").arg(m_userId);
    logToFile(QString("==== Fetching Collections - User ID: %1 ====").arg(m_userId));

    QNetworkRequest request{QUrl(url)};
    request.setRawHeader("Zotero-API-Version", "3");
    request.setRawHeader("Authorization", QString("Bearer %1").arg(m_apiKey).toUtf8());

    // Log request details
    QString headers = QString("Zotero-API-Version: 3\nAuthorization: Bearer ***%1").arg(
        m_apiKey.isEmpty() ? "EMPTY" : m_apiKey.right(4));
    logRequest("GET", url, headers.toUtf8());

    m_currentReply = m_networkManager->get(request);
    connect(m_currentReply, &QNetworkReply::finished, this, &ZoteroInputWidget::handleCollectionsReply);
}

void ZoteroInputWidget::fetchItemsForCollection(const QString& collectionKey) {
    // Safe cleanup of any existing reply
    safeCleanupReply();

    setUIEnabled(false);
    m_isLoading = true;
    m_statusLabel->setText("Loading papers...");
    emit statusMessage("Fetching papers from collection...");

    clearPapers();

    // API endpoint for items in a collection
    QString url = QString("https://api.zotero.org/users/%1/collections/%2/items")
                    .arg(m_userId)
                    .arg(collectionKey);

    QNetworkRequest request{QUrl(url)};
    request.setRawHeader("Zotero-API-Version", "3");
    request.setRawHeader("Authorization", QString("Bearer %1").arg(m_apiKey).toUtf8());
    request.setRawHeader("Content-Type", "application/json");

    m_currentReply = m_networkManager->get(request);
    connect(m_currentReply, &QNetworkReply::finished, this, &ZoteroInputWidget::handleItemsReply);
}

void ZoteroInputWidget::fetchChildrenForItem(const QString& itemKey) {
    logToFile(QString("==== fetchChildrenForItem(%1) called ====").arg(itemKey));

    // Safe cleanup of any existing reply
    safeCleanupReply();

    setUIEnabled(false);
    m_isLoading = true;
    m_statusLabel->setText("Checking for PDF attachments...");
    emit statusMessage("Fetching item attachments...");

    // API endpoint for item's children
    QString url = QString("https://api.zotero.org/users/%1/items/%2/children")
                    .arg(m_userId)
                    .arg(itemKey);

    logToFile(QString("Fetching children from: %1").arg(url));

    QNetworkRequest request{QUrl(url)};
    request.setRawHeader("Zotero-API-Version", "3");
    request.setRawHeader("Authorization", QString("Bearer %1").arg(m_apiKey).toUtf8());

    // Log the request
    QString headers = QString("Zotero-API-Version: 3\nAuthorization: Bearer ***%1").arg(
        m_apiKey.isEmpty() ? "EMPTY" : m_apiKey.right(4));
    logRequest("GET", url, headers.toUtf8());

    m_currentReply = m_networkManager->get(request);
    connect(m_currentReply, &QNetworkReply::finished, this, &ZoteroInputWidget::handleChildrenReply);
}

void ZoteroInputWidget::downloadPdf(const QString& itemKey, const QString& attachmentKey) {
    logToFile(QString("==== downloadPdf(%1, %2) called ====").arg(itemKey).arg(attachmentKey));

    // Safe cleanup of any existing reply
    safeCleanupReply();

    setUIEnabled(false);
    m_isLoading = true;
    m_statusLabel->setText("Downloading PDF...");
    emit statusMessage("Downloading PDF from Zotero...");

    // Create temporary file with exception handling
    if (!createTempPdfFile()) {
        showError("Failed to create temporary file for PDF download");
        setUIEnabled(true);
        m_isLoading = false;
        setState(PaperSelected);  // Go back to paper selected state
        return;
    }

    // API endpoint for attachment file
    QString url = QString("https://api.zotero.org/users/%1/items/%2/file")
                    .arg(m_userId)
                    .arg(attachmentKey);

    logToFile(QString("Downloading PDF from: %1").arg(url));

    QNetworkRequest request{QUrl(url)};
    request.setRawHeader("Zotero-API-Version", "3");
    request.setRawHeader("Authorization", QString("Bearer %1").arg(m_apiKey).toUtf8());
    // Don't follow redirects automatically - we'll handle it manually
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                        QNetworkRequest::ManualRedirectPolicy);

    // Log the request
    QString headers = QString("Zotero-API-Version: 3\nAuthorization: Bearer ***%1").arg(
        m_apiKey.isEmpty() ? "EMPTY" : m_apiKey.right(4));
    logRequest("GET", url, headers.toUtf8());

    m_currentReply = m_networkManager->get(request);
    connect(m_currentReply, &QNetworkReply::finished, this, &ZoteroInputWidget::handlePdfDownloadReply);
}

void ZoteroInputWidget::handleCollectionsReply() {
    logToFile("==== handleCollectionsReply() called ====");
    if (!m_currentReply) {
        logError("No current reply object");
        return;
    }

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

    // Log response status and data
    int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    QByteArray data = reply->readAll();
    logResponse(statusCode, data);

    if (reply->error() != QNetworkReply::NoError) {
        QString errorMsg = QString("Failed to fetch collections: %1 (HTTP %2)")
                            .arg(reply->errorString())
                            .arg(statusCode);

        logError(errorMsg);

        if (statusCode == 403) {
            errorMsg = "Authentication failed. Please check your Zotero credentials in Settings.";
        } else if (statusCode == 404) {
            errorMsg = "User ID not found. Please check your Zotero User ID in Settings (should be numeric).";
        }

        showError(errorMsg);
        m_isLoading = false;
        setUIEnabled(true);
        setState(ReadyToFetch);
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isArray()) {
        parseCollections(doc.array());
        m_statusLabel->setText(QString("Loaded %1 collections").arg(m_collections.size()));
        setState(CollectionsLoaded);
    } else {
        showError("Invalid response format from Zotero API");
        setState(ReadyToFetch);
    }

    setUIEnabled(true);
    m_isLoading = false;
}

void ZoteroInputWidget::handleItemsReply() {
    logToFile("==== handleItemsReply() called ====");
    if (!m_currentReply) {
        logError("No current reply object");
        return;
    }

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

    // Log response status and data
    int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    QByteArray data = reply->readAll();
    logResponse(statusCode, data);

    if (reply->error() != QNetworkReply::NoError) {
        QString errorMsg = QString("Failed to fetch items: %1 (HTTP %2)")
                            .arg(reply->errorString())
                            .arg(statusCode);
        logError(errorMsg);
        showError(errorMsg);
        setState(CollectionsLoaded);  // Go back to collections loaded state
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isArray()) {
        parseItems(doc.array());

        // After parsing items, check each for PDF attachments
        for (auto& item : m_items) {
            checkItemAttachments(item);
        }

        m_statusLabel->setText(QString("Loaded %1 papers").arg(m_items.size()));
        m_papersCombo->setEnabled(m_items.size() > 0);
        setState(CollectionsLoaded);
    } else {
        showError("Invalid response format from Zotero API");
        setState(CollectionsLoaded);
    }

    setUIEnabled(true);
    m_isLoading = false;
}

void ZoteroInputWidget::handlePdfDownloadReply() {
    logToFile("==== handlePdfDownloadReply() called ====");
    if (!m_currentReply) {
        logError("No current reply object");
        return;
    }

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

    // Log response status
    int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    logToFile(QString("PDF Download Response: Status Code %1").arg(statusCode));

    // Check for redirect (3xx status codes)
    if (statusCode >= 300 && statusCode < 400) {
        QUrl redirectUrl = reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
        if (redirectUrl.isValid()) {
            logToFile(QString("Following redirect to: %1").arg(redirectUrl.toString()));

            // Follow the redirect without Zotero headers
            QNetworkRequest redirectRequest{redirectUrl};
            // Don't add Zotero headers for S3
            m_currentReply = m_networkManager->get(redirectRequest);
            connect(m_currentReply, &QNetworkReply::finished, this, &ZoteroInputWidget::handlePdfDownloadReply);
            return;
        }
    }

    if (reply->error() != QNetworkReply::NoError) {
        QString errorMsg = QString("Failed to download PDF: %1 (HTTP %2)")
                            .arg(reply->errorString())
                            .arg(statusCode);
        logError(errorMsg);
        showError(errorMsg);
        m_downloadedPdfPath.clear();
        setUIEnabled(true);
        m_isLoading = false;
        setState(PaperSelected);  // Go back to paper selected state
        return;
    }

    // Write PDF data to temporary file
    QByteArray pdfData = reply->readAll();

    logToFile(QString("PDF Data Size: %1 bytes").arg(pdfData.size()));

    if (pdfData.isEmpty()) {
        logError("Downloaded PDF is empty");
        showError("Downloaded PDF is empty");
        m_downloadedPdfPath.clear();
        setUIEnabled(true);
        m_isLoading = false;
        setState(PaperSelected);  // Go back to paper selected state
        return;
    }

    // Write to temp file with exception handling
    try {
        if (m_tempPdfFile && m_tempPdfFile->isOpen()) {
            m_tempPdfFile->write(pdfData);
            m_tempPdfFile->flush();
            m_tempPdfFile->close();
        } else {
            throw std::runtime_error("Temp file not open");
        }
    } catch (const std::exception& e) {
        showError(QString("Failed to write PDF: %1").arg(e.what()));
        m_downloadedPdfPath.clear();
        setUIEnabled(true);
        m_isLoading = false;
        setState(PaperSelected);  // Go back to paper selected state
        return;
    }

    // Verify the PDF is valid using safe loader
    if (!validateDownloadedPdf()) {
        // Error already shown by validateDownloadedPdf
        m_downloadedPdfPath.clear();
        setUIEnabled(true);
        m_isLoading = false;
        setState(PaperSelected);  // Go back to paper selected state
        return;
    }

    m_statusLabel->setText("PDF downloaded successfully");
    emit statusMessage("PDF ready for analysis");
    emit pdfReady(m_downloadedPdfPath);
    emit analyzeRequested();

    setUIEnabled(true);
    m_isLoading = false;
    setState(ReadyToFetch);  // Reset to initial state after analysis starts
}

void ZoteroInputWidget::handleChildrenReply() {
    logToFile("==== handleChildrenReply() called ====");
    if (!m_currentReply) {
        logError("No current reply object");
        return;
    }

    QNetworkReply* reply = m_currentReply;
    m_currentReply = nullptr;

    // Log response status and data
    int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    QByteArray data = reply->readAll();
    logResponse(statusCode, data);

    if (reply->error() != QNetworkReply::NoError) {
        QString errorMsg = QString("Failed to fetch attachments: %1 (HTTP %2)")
                            .arg(reply->errorString())
                            .arg(statusCode);
        logError(errorMsg);
        showError(errorMsg);
        reply->deleteLater();
        setState(CollectionsLoaded);  // Go back to collections loaded state
        return;
    }

    reply->deleteLater();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isArray()) {
        QJsonArray children = doc.array();
        QString pdfAttachmentKey;

        // Look for PDF attachment
        for (const auto& value : children) {
            QJsonObject obj = value.toObject();
            QJsonObject dataObj = obj["data"].toObject();

            QString itemType = dataObj["itemType"].toString();
            QString contentType = dataObj["contentType"].toString();

            if (itemType == "attachment" && contentType == "application/pdf") {
                pdfAttachmentKey = obj["key"].toString();
                break;
            }
        }

        if (!pdfAttachmentKey.isEmpty()) {
            m_currentItem.pdfAttachmentKey = pdfAttachmentKey;
            m_currentItem.hasPdf = true;
            m_statusLabel->setText("PDF attachment found");

            // Now download the PDF
            setState(Analyzing);
            downloadPdf(m_currentItem.key, pdfAttachmentKey);
        } else {
            m_currentItem.hasPdf = false;
            showError("No PDF attachment found for this item");
            setState(CollectionsLoaded);  // Go back to collections loaded state
        }
    } else {
        showError("Invalid response format from Zotero API");
        setState(CollectionsLoaded);  // Go back to collections loaded state
    }

    setUIEnabled(true);
    m_isLoading = false;
}

void ZoteroInputWidget::checkItemAttachments(const ZoteroItem& item) {
    // Note: A more complete implementation would fetch children via API
    // For now, we'll need to fetch children when the user selects an item
    // This is called after parsing items but doesn't have attachment info yet
}

void ZoteroInputWidget::parseCollections(const QJsonArray& data) {
    m_collections.clear();
    m_collectionsCombo->clear();
    m_collectionsCombo->addItem("Select a collection...");

    for (const auto& value : data) {
        QJsonObject obj = value.toObject();
        QJsonObject dataObj = obj["data"].toObject();

        ZoteroCollection collection;
        collection.key = obj["key"].toString();
        collection.name = dataObj["name"].toString();
        collection.parentKey = dataObj["parentCollection"].toString();

        if (!collection.key.isEmpty() && !collection.name.isEmpty()) {
            m_collections.append(collection);
        }
    }

    // Sort collections by name
    std::sort(m_collections.begin(), m_collections.end(),
              [](const ZoteroCollection& a, const ZoteroCollection& b) {
                  return a.name.compare(b.name, Qt::CaseInsensitive) < 0;
              });

    // Add to combo box
    for (const auto& collection : m_collections) {
        m_collectionsCombo->addItem(formatCollectionName(collection));
    }
}

void ZoteroInputWidget::parseItems(const QJsonArray& data) {
    m_items.clear();
    m_papersCombo->clear();
    m_papersCombo->addItem("Select a paper...");

    for (const auto& value : data) {
        QJsonObject obj = value.toObject();
        QJsonObject dataObj = obj["data"].toObject();

        // Skip non-document items
        QString itemType = dataObj["itemType"].toString();
        if (itemType == "attachment" || itemType == "note") {
            continue;
        }

        ZoteroItem item;
        item.key = obj["key"].toString();
        item.title = dataObj["title"].toString();
        item.year = dataObj["date"].toString().left(4); // Extract year

        // Build authors string
        QJsonArray creators = dataObj["creators"].toArray();
        QStringList authorsList;
        for (const auto& creator : creators) {
            QJsonObject creatorObj = creator.toObject();
            QString lastName = creatorObj["lastName"].toString();
            if (!lastName.isEmpty()) {
                authorsList.append(lastName);
            }
        }
        item.authors = authorsList.join(", ");

        // Check for PDF attachments (simplified - would need children API call)
        QJsonObject meta = obj["meta"].toObject();
        int numChildren = meta["numChildren"].toInt();
        item.hasPdf = (numChildren > 0); // Assume at least one child might be PDF

        // Leave pdfAttachmentKey empty - we'll fetch it when needed
        // item.pdfAttachmentKey will be populated when we fetch children

        if (!item.key.isEmpty() && !item.title.isEmpty()) {
            m_items.append(item);
        }
    }

    // Sort items by year (descending) then title
    std::sort(m_items.begin(), m_items.end(),
              [](const ZoteroItem& a, const ZoteroItem& b) {
                  if (a.year != b.year) {
                      return a.year > b.year; // Newer first
                  }
                  return a.title.compare(b.title, Qt::CaseInsensitive) < 0;
              });

    // Add to combo box
    for (const auto& item : m_items) {
        m_papersCombo->addItem(formatPaperDisplay(item));
    }
}

QString ZoteroInputWidget::formatCollectionName(const ZoteroCollection& collection) const {
    // Could add indentation for subcollections if needed
    return collection.name;
}

QString ZoteroInputWidget::formatPaperDisplay(const ZoteroItem& item) const {
    QString display = item.title;

    if (!item.authors.isEmpty()) {
        display = QString("%1 - %2").arg(item.authors).arg(display);
    }

    if (!item.year.isEmpty()) {
        display = QString("[%1] %2").arg(item.year).arg(display);
    }

    if (!item.hasPdf) {
        display += " (No PDF)";
    }

    return display;
}

void ZoteroInputWidget::setUIEnabled(bool enabled) {
    m_refreshButton->setEnabled(enabled);
    m_collectionsCombo->setEnabled(enabled && m_collections.size() > 0);
    m_papersCombo->setEnabled(enabled && m_items.size() > 0);
    m_analyzeButton->setEnabled(enabled && m_papersCombo->currentIndex() > 0);
}

void ZoteroInputWidget::showError(const QString& error) {
    m_statusLabel->setText(QString("Error: %1").arg(error));
    emit errorOccurred(error);
    setUIEnabled(true);
    m_isLoading = false;

    // Always reset to ready state on error (or no credentials if no API key)
    if (!m_apiKey.isEmpty()) {
        setState(ReadyToFetch);
    } else {
        setState(NoCredentials);
    }

    QMessageBox::warning(this, "Zotero Error", error);
}

void ZoteroInputWidget::clearCollections() {
    m_collections.clear();
    m_collectionsCombo->clear();
    m_collectionsCombo->addItem("Select a collection...");
}

void ZoteroInputWidget::clearPapers() {
    m_items.clear();
    m_papersCombo->clear();
    m_papersCombo->addItem("Select a paper...");
    m_papersCombo->setEnabled(false);
    m_analyzeButton->setEnabled(false);
}

void ZoteroInputWidget::fetchUserIdFromApiKey() {
    logToFile("==== fetchUserIdFromApiKey() called ====");

    if (m_currentReply) {
        logToFile("Aborting previous request");
        m_currentReply->abort();
    }

    setUIEnabled(false);
    m_isLoading = true;
    m_statusLabel->setText("Validating API key...");

    // API endpoint for current key info
    QString url = "https://api.zotero.org/keys/current";
    logToFile(QString("Fetching key info from: %1").arg(url));

    QNetworkRequest request{QUrl(url)};
    request.setRawHeader("Zotero-API-Version", "3");
    request.setRawHeader("Authorization", QString("Bearer %1").arg(m_apiKey).toUtf8());

    // Log the request
    QString headers = QString("Zotero-API-Version: 3\nAuthorization: Bearer ***%1").arg(
        m_apiKey.isEmpty() ? "EMPTY" : m_apiKey.right(4));
    logRequest("GET", url, headers.toUtf8());

    m_currentReply = m_networkManager->get(request);
    connect(m_currentReply, &QNetworkReply::finished, this, &ZoteroInputWidget::handleKeyInfoReply);
}

void ZoteroInputWidget::handleKeyInfoReply() {
    logToFile("==== handleKeyInfoReply() called ====");
    if (!m_currentReply) {
        logError("No current reply object");
        return;
    }

    QNetworkReply* reply = m_currentReply;
    m_currentReply = nullptr;

    // Log response status and data
    int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    QByteArray data = reply->readAll();
    logResponse(statusCode, data);

    if (reply->error() != QNetworkReply::NoError) {
        QString errorMsg = QString("Failed to fetch key info: %1 (HTTP %2)")
                            .arg(reply->errorString())
                            .arg(statusCode);
        logError(errorMsg);

        if (statusCode == 403) {
            errorMsg = "Invalid API key. Please check your Zotero API key in Settings.";
        }

        showError(errorMsg);
        reply->deleteLater();
        m_isLoading = false;
        setUIEnabled(true);
        setState(ReadyToFetch);  // Reset to ready state
        return;
    }

    reply->deleteLater();

    // Parse the response to get the user ID
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isObject()) {
        QJsonObject obj = doc.object();

        // The userID should be in the response
        if (obj.contains("userID")) {
            m_userId = QString::number(obj["userID"].toInt());
            logToFile(QString("Extracted User ID: %1").arg(m_userId));

            // Update the status
            m_statusLabel->setText("API key validated. Loading collections...");

            // Save the user ID to database
            QSqlDatabase db = QSqlDatabase::database();
            if (db.isOpen()) {
                QSqlQuery query(db);
                query.prepare("UPDATE settings SET zotero_user_id = :user_id WHERE id = 1");
                query.bindValue(":user_id", m_userId);
                if (query.exec()) {
                    logToFile("User ID saved to database");
                } else {
                    logError(QString("Failed to save user ID to database: %1").arg(query.lastError().text()));
                }
            }

            // Now fetch the collections
            fetchCollections();
        } else {
            logError("No userID found in key info response");
            showError("Failed to extract user ID from API response");
            m_isLoading = false;
            setUIEnabled(true);
            setState(ReadyToFetch);  // Reset to ready state
        }
    } else {
        logError("Invalid response format from /keys/current");
        showError("Invalid response format from Zotero API");
        m_isLoading = false;
        setUIEnabled(true);
        setState(ReadyToFetch);  // Reset to ready state
    }
}

// Logging implementation
void ZoteroInputWidget::logToFile(const QString& message) {
    if (!m_logFile || !m_logFile->isOpen()) {
        return;
    }

    QTextStream stream(m_logFile);
    stream << QDateTime::currentDateTime().toString("[hh:mm:ss.zzz] ")
           << message << Qt::endl;
    stream.flush();
}

void ZoteroInputWidget::logRequest(const QString& method, const QString& url, const QByteArray& headers) {
    logToFile("----------------------------------------");
    logToFile(QString("REQUEST: %1 %2").arg(method, url));

    if (!headers.isEmpty()) {
        logToFile("Headers:");
        QStringList headerLines = QString::fromUtf8(headers).split('\n');
        for (const QString& line : headerLines) {
            if (!line.trimmed().isEmpty()) {
                logToFile("  " + line.trimmed());
            }
        }
    }
}

void ZoteroInputWidget::logResponse(int statusCode, const QByteArray& response) {
    logToFile(QString("RESPONSE: Status Code %1").arg(statusCode));
    logToFile(QString("Response Size: %1 bytes").arg(response.size()));

    if (!response.isEmpty()) {
        // Try to parse as JSON for prettier output
        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(response, &parseError);

        if (parseError.error == QJsonParseError::NoError) {
            logToFile("Response Data (Valid JSON):");
            // Always log full JSON, never truncate
            QString jsonString = doc.toJson(QJsonDocument::Indented);
            logToFile(jsonString);

            // Also log some structural info for debugging
            if (doc.isArray()) {
                logToFile(QString("JSON Type: Array with %1 items").arg(doc.array().size()));
            } else if (doc.isObject()) {
                QJsonObject obj = doc.object();
                logToFile(QString("JSON Type: Object with %1 keys").arg(obj.keys().size()));
                logToFile(QString("Keys: %1").arg(obj.keys().join(", ")));
            }
        } else {
            // Not JSON, log parse error and raw data
            logToFile(QString("JSON Parse Error: %1 at offset %2")
                     .arg(parseError.errorString())
                     .arg(parseError.offset));

            QString responseStr = QString::fromUtf8(response);
            // For non-JSON, we still truncate very large responses
            if (responseStr.length() > 10000) {
                logToFile("Response Data (truncated to 10000 chars):");
                logToFile(responseStr.left(10000) + "\n... (truncated)");
            } else {
                logToFile("Response Data (Raw):");
                logToFile(responseStr);
            }
        }
    } else {
        logToFile("Response: (empty)");
    }
}

void ZoteroInputWidget::logError(const QString& error) {
    logToFile(QString("ERROR: %1").arg(error));
}

// Safe cleanup methods
void ZoteroInputWidget::safeCleanupReply() {
    if (!m_currentReply) return;

    try {
        // Disconnect all signals first
        m_currentReply->disconnect();

        // Abort if still running
        if (m_currentReply->isRunning()) {
            m_currentReply->abort();
        }

        // Schedule deletion
        m_currentReply->deleteLater();
        m_currentReply = nullptr;
    } catch (const std::exception& e) {
        logError(QString("Exception during reply cleanup: %1").arg(e.what()));
        m_currentReply = nullptr;
    } catch (...) {
        logError("Unknown exception during reply cleanup");
        m_currentReply = nullptr;
    }
}

bool ZoteroInputWidget::createTempPdfFile() {
    try {
        // Check available disk space first
        QStorageInfo storage(QDir::temp());
        if (storage.bytesAvailable() < 100 * 1024 * 1024) { // 100MB min
            logError(QString("Insufficient disk space: only %1 MB available")
                    .arg(storage.bytesAvailable() / (1024 * 1024)));
            return false;
        }

        m_tempPdfFile = std::make_unique<QTemporaryFile>(
            QDir::temp().absoluteFilePath("zotero_XXXXXX.pdf"));

        if (!m_tempPdfFile->open()) {
            throw std::runtime_error("Failed to create temp file");
        }

        m_downloadedPdfPath = m_tempPdfFile->fileName();
        m_tempPdfFile->setAutoRemove(false); // Keep file after object destruction

        logToFile(QString("Created temp file: %1").arg(m_downloadedPdfPath));
        return true;

    } catch (const std::exception& e) {
        logError(QString("Temp file creation failed: %1").arg(e.what()));
        m_tempPdfFile.reset();
        m_downloadedPdfPath.clear();
        return false;
    } catch (...) {
        logError("Unknown exception creating temp file");
        m_tempPdfFile.reset();
        m_downloadedPdfPath.clear();
        return false;
    }
}

bool ZoteroInputWidget::validateDownloadedPdf() {
    try {
        QString errorMsg;

        // First validate the file exists and is readable
        if (!SafePdfLoader::validatePdfFile(m_downloadedPdfPath, errorMsg)) {
            logError(QString("PDF validation failed: %1").arg(errorMsg));
            showError(errorMsg);
            return false;
        }

        // Try to load it to ensure it's a valid PDF
        auto testDoc = std::make_unique<QPdfDocument>();
        if (!SafePdfLoader::loadPdf(testDoc.get(), m_downloadedPdfPath, errorMsg, 10000)) { // 10 second timeout
            logError(QString("PDF load test failed: %1").arg(errorMsg));
            showError(QString("Downloaded file is not a valid PDF: %1").arg(errorMsg));
            return false;
        }

        logToFile("PDF validated successfully");
        return true;

    } catch (const std::exception& e) {
        QString error = QString("Exception validating PDF: %1").arg(e.what());
        logError(error);
        showError(error);
        return false;
    } catch (...) {
        logError("Unknown exception validating PDF");
        showError("Unknown error validating PDF");
        return false;
    }
}

void ZoteroInputWidget::setState(State newState) {
    m_currentState = newState;
    updateUIState();
}

void ZoteroInputWidget::updateUIState() {
    switch (m_currentState) {
        case NoCredentials:
            m_refreshButton->setEnabled(true);
            m_collectionsCombo->setEnabled(false);
            m_papersCombo->setEnabled(false);
            m_analyzeButton->setEnabled(false);
            m_statusLabel->setText("No Zotero credentials set. Please configure in Settings.");
            break;

        case ReadyToFetch:
            m_refreshButton->setEnabled(true);
            m_collectionsCombo->setEnabled(false);
            m_papersCombo->setEnabled(false);
            m_analyzeButton->setEnabled(false);
            break;

        case FetchingData:
            m_refreshButton->setEnabled(false);
            m_collectionsCombo->setEnabled(false);
            m_papersCombo->setEnabled(false);
            m_analyzeButton->setEnabled(false);
            break;

        case CollectionsLoaded:
            m_refreshButton->setEnabled(true);
            m_collectionsCombo->setEnabled(true);
            m_papersCombo->setEnabled(m_papersCombo->count() > 1);
            m_analyzeButton->setEnabled(false);
            break;

        case PaperSelected:
            m_refreshButton->setEnabled(true);
            m_collectionsCombo->setEnabled(true);
            m_papersCombo->setEnabled(true);
            m_analyzeButton->setEnabled(true);
            break;

        case Analyzing:
            m_refreshButton->setEnabled(false);
            m_collectionsCombo->setEnabled(false);
            m_papersCombo->setEnabled(false);
            m_analyzeButton->setEnabled(false);
            break;
    }
}