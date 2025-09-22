#include <QApplication>
#include <QMainWindow>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QPushButton>
#include <QTextEdit>
#include <QLineEdit>
#include <QLabel>
#include <QFileDialog>
#include <QTabWidget>
#include <QGroupBox>
#include <QCheckBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QProgressBar>
#include <QMessageBox>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QPdfDocument>
#include <QFile>
#include <QTextStream>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QEventLoop>
#include <QTimer>
#include <QRegularExpression>
#include <iostream>
#include "tomlparser.h"

class PDFExtractorGUI : public QMainWindow
{
    Q_OBJECT

public:
    PDFExtractorGUI(QWidget *parent = nullptr) : QMainWindow(parent) {
        setupUi();
        loadConfiguration();
        m_networkManager = new QNetworkAccessManager(this);
        m_pdfDocument = new QPdfDocument(this);
    }

    // Process command line arguments if provided
    void processCommandLine(const QCommandLineParser &parser) {
        if (parser.positionalArguments().size() >= 2) {
            QString pdfPath = parser.positionalArguments()[0];
            QString outputPath = parser.positionalArguments()[1];

            m_filePathEdit->setText(pdfPath);
            m_outputPath = outputPath;

            // Set page range if specified
            if (parser.isSet("pages")) {
                QString range = parser.value("pages");
                if (range.contains("-")) {
                    QStringList parts = range.split("-");
                    if (parts.size() == 2) {
                        m_startPageSpin->setValue(parts[0].toInt());
                        m_endPageSpin->setValue(parts[1].toInt());
                    }
                } else {
                    int page = range.toInt();
                    m_startPageSpin->setValue(page);
                    m_endPageSpin->setValue(page);
                }
            }

            // Set preserve copyright
            if (parser.isSet("preserve")) {
                m_preserveCopyrightCheck->setChecked(true);
            }

            // Set output files
            if (parser.isSet("summary")) {
                m_summaryPath = parser.value("summary");
                m_generateSummaryCheck->setChecked(true);
            }

            if (parser.isSet("keywords")) {
                m_keywordsPath = parser.value("keywords");
                m_generateKeywordsCheck->setChecked(true);
            }

            // Set verbose mode
            if (parser.isSet("verbose")) {
                m_verbose = true;
            }

            // Auto-extract if command line mode
            if (!pdfPath.isEmpty()) {
                m_commandLineMode = true;
                // Delay extraction to allow GUI to initialize
                QTimer::singleShot(100, this, &PDFExtractorGUI::onExtractClicked);
            }
        }
    }

private slots:
    void onBrowseClicked() {
        QString fileName = QFileDialog::getOpenFileName(this,
            tr("Select PDF File"), "", tr("PDF Files (*.pdf)"));
        if (!fileName.isEmpty()) {
            m_filePathEdit->setText(fileName);
        }
    }

    void onExtractClicked() {
        QString pdfPath = m_filePathEdit->text();
        if (pdfPath.isEmpty()) {
            QMessageBox::warning(this, tr("Warning"), tr("Please select a PDF file first."));
            return;
        }

        m_extractButton->setEnabled(false);
        m_progressBar->setVisible(true);
        m_statusLabel->setText("Loading PDF...");

        // Clear previous results
        m_extractedTextEdit->clear();
        m_summaryTextEdit->clear();
        m_keywordsTextEdit->clear();

        // Load and extract PDF
        QPdfDocument::Error error = m_pdfDocument->load(pdfPath);

        if (error != QPdfDocument::Error::None) {
            QString errorMsg = "Error loading PDF: ";
            switch (error) {
                case QPdfDocument::Error::FileNotFound:
                    errorMsg += "File not found";
                    break;
                case QPdfDocument::Error::InvalidFileFormat:
                    errorMsg += "Invalid PDF format";
                    break;
                case QPdfDocument::Error::IncorrectPassword:
                    errorMsg += "Password protected";
                    break;
                default:
                    errorMsg += "Unknown error";
            }

            QMessageBox::critical(this, tr("Error"), errorMsg);
            m_extractButton->setEnabled(true);
            m_progressBar->setVisible(false);
            return;
        }

        // Update page range max values
        int pageCount = m_pdfDocument->pageCount();
        m_startPageSpin->setMaximum(pageCount);
        m_endPageSpin->setMaximum(pageCount);

        if (m_endPageSpin->value() == 0) {
            m_endPageSpin->setValue(pageCount);
        }

        extractText();
    }

    void extractText() {
        m_statusLabel->setText("Extracting text...");

        int startPage = m_startPageSpin->value() - 1;  // Convert to 0-based
        int endPage = m_endPageSpin->value() - 1;

        QString fullText;
        int totalPages = endPage - startPage + 1;

        for (int i = startPage; i <= endPage; ++i) {
            QString pageText = m_pdfDocument->getAllText(i).text();

            if (!m_preserveCopyrightCheck->isChecked()) {
                pageText = cleanCopyrightText(pageText);
            }

            if (!pageText.isEmpty()) {
                fullText += pageText;
                if (i < endPage) {
                    fullText += "\n\n--- Page " + QString::number(i + 2) + " ---\n\n";
                }
            }

            // Update progress
            int progress = ((i - startPage + 1) * 100) / totalPages;
            m_progressBar->setValue(progress);
        }

        m_extractedText = fullText;
        m_extractedTextEdit->setPlainText(fullText);

        // Save to file if in command line mode
        if (m_commandLineMode && !m_outputPath.isEmpty()) {
            QFile file(m_outputPath);
            if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
                QTextStream stream(&file);
                stream << fullText;
                file.close();
                if (m_verbose) {
                    std::cout << "Text saved to: " << m_outputPath.toStdString() << std::endl;
                }
            }
        }

        // Generate AI content if requested
        if (m_generateSummaryCheck->isChecked()) {
            generateSummary();
        } else if (m_generateKeywordsCheck->isChecked()) {
            generateKeywords();
        } else {
            onProcessComplete();
        }
    }

    void generateSummary() {
        m_statusLabel->setText("Generating summary...");
        m_tabWidget->setCurrentIndex(1);  // Switch to summary tab

        QString summary = sendToLMStudio(m_summarySystemPrompt, m_summaryPrompt, m_extractedText);
        m_summaryTextEdit->setPlainText(summary);

        // Save if command line mode
        if (m_commandLineMode && !m_summaryPath.isEmpty()) {
            QFile file(m_summaryPath);
            if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
                QTextStream stream(&file);
                stream << summary;
                file.close();
                if (m_verbose) {
                    std::cout << "Summary saved to: " << m_summaryPath.toStdString() << std::endl;
                }
            }
        }

        if (m_generateKeywordsCheck->isChecked()) {
            generateKeywords();
        } else {
            onProcessComplete();
        }
    }

    void generateKeywords() {
        m_statusLabel->setText("Generating keywords...");
        m_tabWidget->setCurrentIndex(2);  // Switch to keywords tab

        QString keywords = sendToLMStudio(m_keywordsSystemPrompt, m_keywordsPrompt, m_extractedText);
        m_keywordsTextEdit->setPlainText(keywords);

        // Save if command line mode
        if (m_commandLineMode && !m_keywordsPath.isEmpty()) {
            QFile file(m_keywordsPath);
            if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
                QTextStream stream(&file);
                stream << keywords;
                file.close();
                if (m_verbose) {
                    std::cout << "Keywords saved to: " << m_keywordsPath.toStdString() << std::endl;
                }
            }
        }

        onProcessComplete();
    }

    void onProcessComplete() {
        m_statusLabel->setText("Complete!");
        m_progressBar->setValue(100);
        m_progressBar->setVisible(false);
        m_extractButton->setEnabled(true);

        // Exit if command line mode
        if (m_commandLineMode) {
            QTimer::singleShot(100, qApp, &QApplication::quit);
        }
    }

private:
    void setupUi() {
        auto *centralWidget = new QWidget(this);
        setCentralWidget(centralWidget);

        auto *mainLayout = new QVBoxLayout(centralWidget);

        // File selection section
        auto *fileGroup = new QGroupBox(tr("PDF File Selection"));
        auto *fileLayout = new QHBoxLayout(fileGroup);

        m_filePathEdit = new QLineEdit();
        m_browseButton = new QPushButton(tr("Browse..."));
        m_extractButton = new QPushButton(tr("Extract && Process"));
        m_extractButton->setStyleSheet("QPushButton { background-color: #4CAF50; color: white; font-weight: bold; padding: 8px; }");

        fileLayout->addWidget(new QLabel(tr("File:")));
        fileLayout->addWidget(m_filePathEdit);
        fileLayout->addWidget(m_browseButton);
        fileLayout->addWidget(m_extractButton);

        // Settings section
        auto *settingsGroup = new QGroupBox(tr("Settings"));
        auto *settingsLayout = new QGridLayout(settingsGroup);

        m_startPageSpin = new QSpinBox();
        m_startPageSpin->setMinimum(1);
        m_startPageSpin->setValue(1);

        m_endPageSpin = new QSpinBox();
        m_endPageSpin->setMinimum(0);
        m_endPageSpin->setValue(0);
        m_endPageSpin->setSpecialValueText(tr("Last"));

        m_preserveCopyrightCheck = new QCheckBox(tr("Preserve Copyright Notices"));
        m_generateSummaryCheck = new QCheckBox(tr("Generate Summary"));
        m_generateSummaryCheck->setChecked(true);
        m_generateKeywordsCheck = new QCheckBox(tr("Generate Keywords"));
        m_generateKeywordsCheck->setChecked(true);

        m_temperatureSpin = new QDoubleSpinBox();
        m_temperatureSpin->setRange(0.0, 2.0);
        m_temperatureSpin->setSingleStep(0.1);
        m_temperatureSpin->setValue(0.8);

        m_maxTokensSpin = new QSpinBox();
        m_maxTokensSpin->setRange(100, 32000);
        m_maxTokensSpin->setValue(8000);
        m_maxTokensSpin->setSingleStep(100);

        m_modelEdit = new QLineEdit("gpt-oss-120b");

        settingsLayout->addWidget(new QLabel(tr("Start Page:")), 0, 0);
        settingsLayout->addWidget(m_startPageSpin, 0, 1);
        settingsLayout->addWidget(new QLabel(tr("End Page:")), 0, 2);
        settingsLayout->addWidget(m_endPageSpin, 0, 3);

        settingsLayout->addWidget(m_preserveCopyrightCheck, 1, 0, 1, 2);
        settingsLayout->addWidget(m_generateSummaryCheck, 1, 2);
        settingsLayout->addWidget(m_generateKeywordsCheck, 1, 3);

        settingsLayout->addWidget(new QLabel(tr("Temperature:")), 2, 0);
        settingsLayout->addWidget(m_temperatureSpin, 2, 1);
        settingsLayout->addWidget(new QLabel(tr("Max Tokens:")), 2, 2);
        settingsLayout->addWidget(m_maxTokensSpin, 2, 3);

        settingsLayout->addWidget(new QLabel(tr("Model:")), 3, 0);
        settingsLayout->addWidget(m_modelEdit, 3, 1, 1, 3);

        // Tab widget for results
        m_tabWidget = new QTabWidget();

        m_extractedTextEdit = new QTextEdit();
        m_extractedTextEdit->setReadOnly(true);
        m_tabWidget->addTab(m_extractedTextEdit, tr("Extracted Text"));

        m_summaryTextEdit = new QTextEdit();
        m_summaryTextEdit->setReadOnly(true);
        m_tabWidget->addTab(m_summaryTextEdit, tr("Summary"));

        m_keywordsTextEdit = new QTextEdit();
        m_keywordsTextEdit->setReadOnly(true);
        m_tabWidget->addTab(m_keywordsTextEdit, tr("Keywords"));

        // Status section
        m_progressBar = new QProgressBar();
        m_progressBar->setVisible(false);
        m_statusLabel = new QLabel(tr("Ready"));

        auto *statusLayout = new QHBoxLayout();
        statusLayout->addWidget(m_statusLabel);
        statusLayout->addStretch();
        statusLayout->addWidget(m_progressBar);

        // Add all to main layout
        mainLayout->addWidget(fileGroup);
        mainLayout->addWidget(settingsGroup);
        mainLayout->addWidget(m_tabWidget);
        mainLayout->addLayout(statusLayout);

        // Connect signals
        connect(m_browseButton, &QPushButton::clicked, this, &PDFExtractorGUI::onBrowseClicked);
        connect(m_extractButton, &QPushButton::clicked, this, &PDFExtractorGUI::onExtractClicked);

        // Set window properties
        setWindowTitle(tr("PDF Extractor with AI"));
        resize(900, 700);
    }

    void loadConfiguration() {
        SimpleTomlParser parser;
        QMap<QString, QString> config = parser.parse("lmstudio_config.toml");

        m_lmStudioEndpoint = config.value("lmstudio.endpoint", "http://172.20.10.3:8090/v1/chat/completions");
        m_summaryPrompt = config.value("prompts.summary");
        m_keywordsPrompt = config.value("prompts.keywords");
        m_summarySystemPrompt = "You are an expert scientific reviewer. Provide clear, concise analysis of research papers focusing on key findings and significance.";
        m_keywordsSystemPrompt = "You are a scientific keyword extraction assistant. Focus on extracting specific scientific terms, organisms, chemicals, methods, and concepts from research papers.";

        m_temperatureSpin->setValue(config.value("lmstudio.temperature", "0.8").toDouble());
        m_maxTokensSpin->setValue(config.value("lmstudio.max_tokens", "8000").toInt());
        m_modelEdit->setText(config.value("lmstudio.model_name", "gpt-oss-120b"));
    }

    QString cleanCopyrightText(const QString &text) {
        QString cleaned = text;

        QList<QRegularExpression> copyrightPatterns = {
            QRegularExpression("\\(c\\)", QRegularExpression::CaseInsensitiveOption),
            QRegularExpression("\\(C\\)", QRegularExpression::CaseInsensitiveOption),
            QRegularExpression("©", QRegularExpression::CaseInsensitiveOption),
            QRegularExpression("\\bcopyright\\b", QRegularExpression::CaseInsensitiveOption),
            QRegularExpression("\\ball\\s+rights\\s+reserved\\b", QRegularExpression::CaseInsensitiveOption)
        };

        for (const auto &pattern : copyrightPatterns) {
            cleaned.remove(pattern);
        }

        cleaned = cleaned.simplified();
        QRegularExpression multiSpace("\\s{2,}");
        cleaned.replace(multiSpace, " ");

        return cleaned;
    }

    QString sendToLMStudio(const QString &systemPrompt, const QString &userPrompt, const QString &text) {
        QString fullPrompt = userPrompt;
        fullPrompt.replace("{text}", text);

        QJsonObject systemMessage;
        systemMessage["role"] = "system";
        systemMessage["content"] = systemPrompt;

        QJsonObject userMessage;
        userMessage["role"] = "user";
        userMessage["content"] = fullPrompt;

        QJsonArray messages;
        messages.append(systemMessage);
        messages.append(userMessage);

        QJsonObject requestData;
        requestData["model"] = m_modelEdit->text();
        requestData["messages"] = messages;
        requestData["temperature"] = m_temperatureSpin->value();
        requestData["max_tokens"] = m_maxTokensSpin->value();
        requestData["stream"] = false;

        if (m_verbose) {
            std::cout << "[VERBOSE] Sending to LM Studio: " << m_lmStudioEndpoint.toStdString() << std::endl;
            std::cout << "[VERBOSE] Model: " << m_modelEdit->text().toStdString() << std::endl;
            std::cout << "[VERBOSE] Temperature: " << m_temperatureSpin->value() << std::endl;
            std::cout << "[VERBOSE] Max tokens: " << m_maxTokensSpin->value() << std::endl;
        }

        QJsonDocument doc(requestData);
        QByteArray jsonData = doc.toJson();

        QNetworkRequest request((QUrl(m_lmStudioEndpoint)));
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

        QNetworkReply *reply = m_networkManager->post(request, jsonData);

        QEventLoop loop;
        QTimer timer;
        timer.setSingleShot(true);
        timer.setInterval(120000);  // 2 minute timeout

        connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
        connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);

        timer.start();
        loop.exec();

        QString result;

        if (timer.isActive()) {
            timer.stop();
            if (reply->error() == QNetworkReply::NoError) {
                QByteArray response = reply->readAll();
                QJsonDocument responseDoc = QJsonDocument::fromJson(response);
                QJsonObject responseObj = responseDoc.object();

                if (responseObj.contains("choices")) {
                    QJsonArray choices = responseObj["choices"].toArray();
                    if (!choices.isEmpty()) {
                        QJsonObject firstChoice = choices[0].toObject();
                        QJsonObject messageObj = firstChoice["message"].toObject();
                        result = messageObj["content"].toString();

                        // Clean up LM Studio specific tags
                        if (result.contains("<|message|>")) {
                            QRegularExpression tagPattern("<\\|message\\|>(.*?)(?:<\\|end\\|>|$)",
                                                        QRegularExpression::DotMatchesEverythingOption);
                            QRegularExpressionMatch match = tagPattern.match(result);
                            if (match.hasMatch()) {
                                result = match.captured(1).trimmed();
                            }
                        }
                        result.remove(QRegularExpression("<\\|start\\|>.*?<\\|message\\|>"));

                        if (m_verbose) {
                            std::cout << "[VERBOSE] Response received: " << result.left(200).toStdString() << "..." << std::endl;
                        }
                    }
                }
            } else {
                if (m_verbose) {
                    std::cout << "[ERROR] Network error: " << reply->errorString().toStdString() << std::endl;
                }
            }
        } else {
            if (m_verbose) {
                std::cout << "[ERROR] Request timeout" << std::endl;
            }
        }

        reply->deleteLater();
        return result;
    }

private:
    // UI Elements
    QLineEdit *m_filePathEdit;
    QPushButton *m_browseButton;
    QPushButton *m_extractButton;
    QProgressBar *m_progressBar;
    QLabel *m_statusLabel;
    QTabWidget *m_tabWidget;
    QTextEdit *m_extractedTextEdit;
    QTextEdit *m_summaryTextEdit;
    QTextEdit *m_keywordsTextEdit;
    QSpinBox *m_startPageSpin;
    QSpinBox *m_endPageSpin;
    QCheckBox *m_preserveCopyrightCheck;
    QCheckBox *m_generateSummaryCheck;
    QCheckBox *m_generateKeywordsCheck;
    QDoubleSpinBox *m_temperatureSpin;
    QSpinBox *m_maxTokensSpin;
    QLineEdit *m_modelEdit;

    // Core objects
    QPdfDocument *m_pdfDocument;
    QNetworkAccessManager *m_networkManager;

    // Configuration
    QString m_lmStudioEndpoint;
    QString m_summaryPrompt;
    QString m_keywordsPrompt;
    QString m_summarySystemPrompt;
    QString m_keywordsSystemPrompt;

    // State
    QString m_extractedText;
    QString m_outputPath;
    QString m_summaryPath;
    QString m_keywordsPath;
    bool m_commandLineMode = false;
    bool m_verbose = false;
};

#include "main.moc"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName("PDF Extractor GUI");
    QApplication::setApplicationVersion("1.0");

    // Set up command line parser
    QCommandLineParser parser;
    parser.setApplicationDescription("PDF Extractor with AI - GUI and CLI");
    parser.addHelpOption();
    parser.addVersionOption();

    // Add positional arguments
    parser.addPositionalArgument("pdf", "PDF file to extract text from");
    parser.addPositionalArgument("output", "Output text file");

    // Add options
    QCommandLineOption pageRangeOption(QStringList() << "p" << "pages",
                                       "Page range to extract (e.g., 1-10 or 5)",
                                       "range");
    parser.addOption(pageRangeOption);

    QCommandLineOption preserveOption(QStringList() << "preserve",
                                      "Preserve copyright notices");
    parser.addOption(preserveOption);

    QCommandLineOption configOption(QStringList() << "c" << "config",
                                    "TOML configuration file",
                                    "config");
    parser.addOption(configOption);

    QCommandLineOption summaryOption(QStringList() << "s" << "summary",
                                     "Output file for summary",
                                     "summary");
    parser.addOption(summaryOption);

    QCommandLineOption keywordsOption(QStringList() << "k" << "keywords",
                                      "Output file for keywords",
                                      "keywords");
    parser.addOption(keywordsOption);

    QCommandLineOption verboseOption(QStringList() << "verbose",
                                     "Enable verbose output");
    parser.addOption(verboseOption);

    QCommandLineOption guiOption(QStringList() << "g" << "gui",
                                 "Force GUI mode even with arguments");
    parser.addOption(guiOption);

    parser.process(app);

    PDFExtractorGUI window;

    // Process command line if arguments provided and not forced GUI
    if (parser.positionalArguments().size() >= 2 && !parser.isSet(guiOption)) {
        window.processCommandLine(parser);
    }

    window.show();

    return app.exec();
}