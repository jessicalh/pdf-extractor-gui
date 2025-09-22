#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPdfDocument>
#include <QNetworkAccessManager>
#include <QString>
#include <memory>

QT_BEGIN_NAMESPACE
class QPushButton;
class QTextEdit;
class QLineEdit;
class QLabel;
class QProgressBar;
class QTabWidget;
class QCheckBox;
class QSpinBox;
class QDoubleSpinBox;
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onBrowseClicked();
    void onExtractClicked();
    void onProcessComplete();
    void onNetworkReply();
    void updateProgress(const QString &message);

private:
    void setupUi();
    void extractText();
    void generateSummary(const QString &text);
    void generateKeywords(const QString &text);
    QString cleanCopyrightText(const QString &text);
    QString sendToLMStudio(const QString &systemPrompt, const QString &userPrompt, const QString &text);
    void loadConfiguration();

    // UI Elements
    QLineEdit *m_filePathEdit;
    QPushButton *m_browseButton;
    QPushButton *m_extractButton;
    QProgressBar *m_progressBar;
    QLabel *m_statusLabel;
    QTabWidget *m_tabWidget;

    // Text display areas
    QTextEdit *m_extractedTextEdit;
    QTextEdit *m_summaryTextEdit;
    QTextEdit *m_keywordsTextEdit;

    // Settings widgets
    QSpinBox *m_startPageSpin;
    QSpinBox *m_endPageSpin;
    QCheckBox *m_preserveCopyrightCheck;
    QDoubleSpinBox *m_temperatureSpin;
    QSpinBox *m_maxTokensSpin;
    QLineEdit *m_modelEdit;

    // Core objects
    std::unique_ptr<QPdfDocument> m_pdfDocument;
    QNetworkAccessManager *m_networkManager;

    // Configuration
    QString m_lmStudioEndpoint;
    QString m_currentPdfPath;
    QString m_extractedText;

    // Prompts from config
    QString m_summaryPrompt;
    QString m_keywordsPrompt;
    QString m_summarySystemPrompt;
    QString m_keywordsSystemPrompt;
};

#endif // MAINWINDOW_H