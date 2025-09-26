#ifndef ZOTEROINPUT_H
#define ZOTEROINPUT_H

#include <QWidget>
#include <QString>
#include <QStringList>
#include <QJsonObject>
#include <QJsonArray>
#include <QTemporaryFile>
#include <memory>

class QComboBox;
class QFile;
class QPushButton;
class QLabel;
class QNetworkAccessManager;
class QNetworkReply;
class QPdfDocument;

// Represents a Zotero collection
struct ZoteroCollection {
    QString key;
    QString name;
    QString parentKey;
    int level = 0;  // For indentation in display
};

// Represents a Zotero item (paper)
struct ZoteroItem {
    QString key;
    QString title;
    QString authors;
    QString year;
    bool hasPdf = false;
    QString pdfAttachmentKey;
};

// Widget for Zotero input functionality
class ZoteroInputWidget : public QWidget {
    Q_OBJECT

public:
    // State machine for UI control
    enum State {
        NoCredentials,      // No credentials set - only refresh enabled
        ReadyToFetch,       // Credentials set - refresh enabled
        FetchingData,       // Loading - all disabled
        CollectionsLoaded,  // Collections ready - collection combo enabled
        PaperSelected,      // Paper selected - analyze enabled
        Analyzing           // Processing - all disabled
    };
    explicit ZoteroInputWidget(QWidget *parent = nullptr);
    ~ZoteroInputWidget() override;

    // Initialize with credentials from settings
    void setCredentials(const QString& userId, const QString& apiKey);

    // Check if ready to analyze
    QString validate() const;

    // Get the downloaded PDF path
    QString getPdfPath() const { return m_downloadedPdfPath; }

    // Reset state
    void reset();

    // Fetch user ID from API key
    void fetchUserIdFromApiKey();

signals:
    void statusMessage(const QString& message);
    void errorOccurred(const QString& error);
    void pdfReady(const QString& pdfPath);
    void analyzeRequested();

private slots:
    void onRefreshCollections();
    void onCollectionChanged(int index);
    void onPaperChanged(int index);
    void onAnalyzeClicked();

    // Network reply handlers
    void handleCollectionsReply();
    void handleItemsReply();
    void handlePdfDownloadReply();
    void handleChildrenReply();
    void handleKeyInfoReply();

private:
    // UI setup
    void setupUI();

    // API calls
    void fetchCollections();
    void fetchItemsForCollection(const QString& collectionKey);
    void fetchChildrenForItem(const QString& itemKey);
    void downloadPdf(const QString& itemKey, const QString& attachmentKey);

    // Helper methods
    void clearCollections();
    void clearPapers();
    void parseCollections(const QJsonArray& data);
    void parseItems(const QJsonArray& data);
    QString formatCollectionName(const ZoteroCollection& collection) const;
    QString formatPaperDisplay(const ZoteroItem& item) const;
    void setUIEnabled(bool enabled);
    void showError(const QString& error);

    // Check for PDF attachments in item's children
    void checkItemAttachments(const ZoteroItem& item);

    // Logging
    void logToFile(const QString& message);
    void logRequest(const QString& method, const QString& url, const QByteArray& headers = QByteArray());
    void logResponse(int statusCode, const QByteArray& response);
    void logError(const QString& error);

    // Safe cleanup and validation
    void safeCleanupReply();
    bool createTempPdfFile();
    bool validateDownloadedPdf();

    // UI elements
    QComboBox* m_collectionsCombo;
    QComboBox* m_papersCombo;
    QPushButton* m_refreshButton;
    QPushButton* m_analyzeButton;
    QLabel* m_statusLabel;

    // Network
    QNetworkAccessManager* m_networkManager;
    QNetworkReply* m_currentReply;

    // Data
    QString m_userId;
    QString m_apiKey;
    QList<ZoteroCollection> m_collections;
    QList<ZoteroItem> m_items;
    QString m_downloadedPdfPath;

    // Temporary file for PDF
    std::unique_ptr<QTemporaryFile> m_tempPdfFile;

    // State
    bool m_isLoading;
    QString m_currentCollectionKey;
    ZoteroItem m_currentItem;
    State m_currentState;

    // State management
    void setState(State newState);
    void updateUIState();

    // Logging
    mutable QFile* m_logFile;
};

#endif // ZOTEROINPUT_H