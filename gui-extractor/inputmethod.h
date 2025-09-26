#ifndef INPUTMETHOD_H
#define INPUTMETHOD_H

#include <QObject>
#include <QString>
#include <QWidget>

// Base class for all input methods
// Provides a uniform interface for obtaining text from various sources
class InputMethod : public QObject {
    Q_OBJECT

public:
    explicit InputMethod(QObject *parent = nullptr) : QObject(parent) {}
    virtual ~InputMethod() = default;

    // Returns the widget to be displayed in the input tab
    virtual QWidget* getWidget() = 0;

    // Validates that the input is ready for extraction
    // Returns empty string if valid, error message otherwise
    virtual QString validate() = 0;

    // Extracts and returns the text content
    // Should perform copyright stripping and cleanup
    // Called after validate() returns success
    virtual QString extractText() = 0;

    // Cleans up any resources (file handles, network connections, etc.)
    // Called after extraction is complete or on error
    virtual void cleanup() = 0;

    // Returns true if this input method requires async operations
    virtual bool isAsync() const { return false; }

    // Returns a short name for this input method (for UI)
    virtual QString getName() const = 0;

    // Resets the input method to initial state
    virtual void reset() = 0;

signals:
    // For async input methods to signal completion
    void extractionComplete(const QString& text);
    void extractionError(const QString& error);
    void statusUpdate(const QString& message);

protected:
    // Helper method for copyright stripping (shared logic)
    QString stripCopyright(const QString& text);
};

// PDF file input method
class PdfFileInputMethod : public InputMethod {
    Q_OBJECT

public:
    explicit PdfFileInputMethod(QWidget *parentWidget, QObject *parent = nullptr);
    ~PdfFileInputMethod() override;

    QWidget* getWidget() override;
    QString validate() override;
    QString extractText() override;
    void cleanup() override;
    QString getName() const override { return "PDF File"; }
    void reset() override;

private:
    QWidget* m_widget;
    QString m_currentFilePath;
    class QPushButton* m_selectButton;
    class QLabel* m_fileLabel;
    class QPdfDocument* m_pdfDocument;
};

// Paste text input method
class PasteTextInputMethod : public InputMethod {
    Q_OBJECT

public:
    explicit PasteTextInputMethod(QWidget *parentWidget, QObject *parent = nullptr);
    ~PasteTextInputMethod() override;

    QWidget* getWidget() override;
    QString validate() override;
    QString extractText() override;
    void cleanup() override;
    QString getName() const override { return "Paste Text"; }
    void reset() override;

private:
    QWidget* m_widget;
    class QTextEdit* m_textEdit;
};

// Zotero input method (framework for future implementation)
class ZoteroInputMethod : public InputMethod {
    Q_OBJECT

public:
    explicit ZoteroInputMethod(QWidget *parentWidget, QObject *parent = nullptr);
    ~ZoteroInputMethod() override;

    QWidget* getWidget() override;
    QString validate() override;
    QString extractText() override;
    void cleanup() override;
    bool isAsync() const override { return true; }
    QString getName() const override { return "Zotero"; }
    void reset() override;

private slots:
    void onSearchClicked();
    void onItemSelected();
    void onDownloadComplete();

private:
    QWidget* m_widget;
    class QLineEdit* m_searchField;
    class QPushButton* m_searchButton;
    class QListWidget* m_resultsList;
    class QLabel* m_statusLabel;
    class QNetworkAccessManager* m_networkManager;
    QString m_downloadedPdfPath;
    QString m_extractedText;

    // Zotero API integration
    void searchZotero(const QString& query);
    void downloadPdf(const QString& itemId);
    QString extractFromPdf(const QString& pdfPath);
};

#endif // INPUTMETHOD_H