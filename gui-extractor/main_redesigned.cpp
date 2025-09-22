#include <QApplication>
#include <QMainWindow>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QPushButton>
#include <QTextEdit>
#include <QPlainTextEdit>
#include <QLineEdit>
#include <QLabel>
#include <QFileDialog>
#include <QTabWidget>
#include <QGroupBox>
#include <QCheckBox>
#include <QDialog>
#include <QDialogButtonBox>
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
#include <QMovie>
#include <QDateTime>
#include <QSpinBox>
#include <QSplitter>
#include <QFont>
#include <QFontDatabase>
#include <iostream>
#include <sstream>
#include "tomlparser.h"

// Settings Dialog with full TOML editor
class SettingsDialog : public QDialog
{
    Q_OBJECT
public:
    SettingsDialog(QWidget *parent = nullptr) : QDialog(parent) {
        setWindowTitle("Settings - TOML Configuration");
        setModal(true);
        resize(800, 600);

        auto *layout = new QVBoxLayout(this);

        // Info label
        auto *infoLabel = new QLabel("Edit the TOML configuration below. Changes will be saved to lmstudio_config.toml");
        layout->addWidget(infoLabel);

        // TOML editor with monospace font
        m_tomlEdit = new QPlainTextEdit();
        QFont monoFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
        monoFont.setPointSize(10);
        m_tomlEdit->setFont(monoFont);
        m_tomlEdit->setTabStopDistance(40);

        // Load current TOML file
        QFile file("lmstudio_config.toml");
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream stream(&file);
            m_tomlEdit->setPlainText(stream.readAll());
            file.close();
        }

        layout->addWidget(m_tomlEdit);

        // Buttons
        auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
        connect(buttonBox, &QDialogButtonBox::accepted, this, &SettingsDialog::saveConfig);
        connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
        layout->addWidget(buttonBox);
    }

    QString getTomlContent() const {
        return m_tomlEdit->toPlainText();
    }

private slots:
    void saveConfig() {
        QString content = m_tomlEdit->toPlainText();

        // Save to file
        QFile file("lmstudio_config.toml");
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream stream(&file);
            stream << content;
            file.close();

            // Verify it can be parsed
            SimpleTomlParser parser;
            auto testConfig = parser.parse("lmstudio_config.toml");
            if (testConfig.isEmpty()) {
                QMessageBox::warning(this, "Warning", "The TOML file may contain errors. Please check the format.");
            }

            accept();
        } else {
            QMessageBox::critical(this, "Error", "Failed to save configuration file.");
        }
    }

private:
    QPlainTextEdit *m_tomlEdit;
};

// Main window class
class PDFExtractorGUI : public QMainWindow
{
    Q_OBJECT

public:
    PDFExtractorGUI(QWidget *parent = nullptr) : QMainWindow(parent) {
        setWindowTitle("PDF Extractor with AI Analysis");
        resize(1200, 800);

        m_pdfDocument = std::make_unique<QPdfDocument>();
        m_networkManager = new QNetworkAccessManager(this);

        loadConfiguration();
        setupUi();

        // Initialize conversation log
        m_conversationLog.clear();

        m_commandLineMode = false;
    }

    void processCommandLine(const QCommandLineParser &parser) {
        // Command line processing remains the same
        m_commandLineMode = true;
        const QStringList args = parser.positionalArguments();
        if (args.size() < 2) {
            QMessageBox::critical(this, tr("Error"), tr("Insufficient arguments"));
            QTimer::singleShot(100, qApp, &QApplication::quit);
            return;
        }

        QString pdfPath = args[0];
        QString outputPath = args[1];

        // Process the PDF
        if (!processPDF(pdfPath, outputPath, parser)) {
            QTimer::singleShot(100, qApp, &QApplication::quit);
        }
    }

private slots:
    void onBrowseClicked() {
        QString fileName = QFileDialog::getOpenFileName(this,
            tr("Open PDF File"), "", tr("PDF Files (*.pdf)"));
        if (!fileName.isEmpty()) {
            m_filePathEdit->setText(fileName);
        }
    }

    void onPdfAnalyzeClicked() {
        clearAllResults();

        QString pdfPath = m_filePathEdit->text();
        if (pdfPath.isEmpty()) {
            QMessageBox::warning(this, tr("Warning"), tr("Please select a PDF file first."));
            return;
        }

        startSpinner();
        m_pdfAnalyzeButton->setEnabled(false);
        m_textAnalyzeButton->setEnabled(false);

        appendToLog("=== Starting PDF Analysis ===");
        appendToLog(QString("PDF File: %1").arg(pdfPath));

        // Extract text from PDF
        if (!extractPdfText(pdfPath)) {
            stopSpinner();
            m_pdfAnalyzeButton->setEnabled(true);
            m_textAnalyzeButton->setEnabled(true);
            return;
        }

        // Show extracted text
        m_extractedTextEdit->setPlainText(m_extractedText);
        m_resultsTabWidget->setCurrentIndex(0);

        // Run analysis
        runAnalysis();
    }

    void onTextAnalyzeClicked() {
        clearAllResults();

        QString rawText = m_pasteTextEdit->toPlainText();
        if (rawText.isEmpty()) {
            QMessageBox::warning(this, tr("Warning"), tr("Please paste or enter text first."));
            return;
        }

        startSpinner();
        m_pdfAnalyzeButton->setEnabled(false);
        m_textAnalyzeButton->setEnabled(false);

        appendToLog("=== Starting Text Analysis ===");
        appendToLog("Text source: Pasted/Manual input");

        // Sanitize the text
        m_extractedText = sanitizeText(rawText);
        appendToLog(QString("Text length: %1 characters").arg(m_extractedText.length()));

        // Show extracted/sanitized text
        m_extractedTextEdit->setPlainText(m_extractedText);
        m_resultsTabWidget->setCurrentIndex(0);

        // Run analysis
        runAnalysis();
    }

    void onSettingsClicked() {
        SettingsDialog dialog(this);
        if (dialog.exec() == QDialog::Accepted) {
            // Reload configuration
            loadConfiguration();
            QMessageBox::information(this, tr("Settings"), tr("Configuration updated successfully."));
        }
    }

private:
    void setupUi() {
        auto *centralWidget = new QWidget(this);
        setCentralWidget(centralWidget);

        auto *mainLayout = new QVBoxLayout(centralWidget);

        // Create main splitter for input/output
        auto *splitter = new QSplitter(Qt::Vertical);

        // === INPUT SECTION ===
        auto *inputWidget = new QWidget();
        auto *inputLayout = new QVBoxLayout(inputWidget);

        auto *inputLabel = new QLabel(tr("<b>INPUT SOURCES</b>"));
        inputLayout->addWidget(inputLabel);

        m_inputTabWidget = new QTabWidget();

        // PDF Input Tab
        auto *pdfTab = new QWidget();
        auto *pdfLayout = new QVBoxLayout(pdfTab);

        auto *fileLayout = new QHBoxLayout();
        fileLayout->addWidget(new QLabel(tr("PDF File:")));
        m_filePathEdit = new QLineEdit();
        fileLayout->addWidget(m_filePathEdit);
        m_browseButton = new QPushButton(tr("Browse..."));
        connect(m_browseButton, &QPushButton::clicked, this, &PDFExtractorGUI::onBrowseClicked);
        fileLayout->addWidget(m_browseButton);
        pdfLayout->addLayout(fileLayout);

        // PDF Options
        auto *pdfOptionsLayout = new QHBoxLayout();
        pdfOptionsLayout->addWidget(new QLabel(tr("Pages:")));
        m_startPageSpin = new QSpinBox();
        m_startPageSpin->setMinimum(1);
        m_startPageSpin->setValue(1);
        pdfOptionsLayout->addWidget(m_startPageSpin);
        pdfOptionsLayout->addWidget(new QLabel(tr("to")));
        m_endPageSpin = new QSpinBox();
        m_endPageSpin->setMinimum(0);
        m_endPageSpin->setValue(0);
        m_endPageSpin->setSpecialValueText(tr("Last"));
        pdfOptionsLayout->addWidget(m_endPageSpin);

        m_preserveCopyrightCheck = new QCheckBox(tr("Preserve Copyright"));
        pdfOptionsLayout->addWidget(m_preserveCopyrightCheck);
        pdfOptionsLayout->addStretch();

        m_pdfAnalyzeButton = new QPushButton(tr("Extract && Analyze PDF"));
        m_pdfAnalyzeButton->setStyleSheet("QPushButton { background-color: #4CAF50; color: white; font-weight: bold; padding: 10px; }");
        connect(m_pdfAnalyzeButton, &QPushButton::clicked, this, &PDFExtractorGUI::onPdfAnalyzeClicked);
        pdfOptionsLayout->addWidget(m_pdfAnalyzeButton);

        pdfLayout->addLayout(pdfOptionsLayout);
        m_inputTabWidget->addTab(pdfTab, tr("PDF Input"));

        // Text Input Tab
        auto *textTab = new QWidget();
        auto *textLayout = new QVBoxLayout(textTab);

        auto *pasteLabel = new QLabel(tr("Paste or enter text below:"));
        textLayout->addWidget(pasteLabel);

        m_pasteTextEdit = new QTextEdit();
        m_pasteTextEdit->setPlaceholderText(tr("Paste text from web, OCR, or other sources here..."));
        textLayout->addWidget(m_pasteTextEdit);

        auto *textButtonLayout = new QHBoxLayout();
        textButtonLayout->addStretch();
        m_textAnalyzeButton = new QPushButton(tr("Analyze Text"));
        m_textAnalyzeButton->setStyleSheet("QPushButton { background-color: #FF9800; color: white; font-weight: bold; padding: 10px; }");
        connect(m_textAnalyzeButton, &QPushButton::clicked, this, &PDFExtractorGUI::onTextAnalyzeClicked);
        textButtonLayout->addWidget(m_textAnalyzeButton);
        textLayout->addLayout(textButtonLayout);

        m_inputTabWidget->addTab(textTab, tr("Text Input"));

        inputLayout->addWidget(m_inputTabWidget);

        // Settings and spinner row
        auto *controlLayout = new QHBoxLayout();
        m_settingsButton = new QPushButton(tr("Settings"));
        m_settingsButton->setStyleSheet("QPushButton { background-color: #2196F3; color: white; padding: 8px; }");
        connect(m_settingsButton, &QPushButton::clicked, this, &PDFExtractorGUI::onSettingsClicked);
        controlLayout->addWidget(m_settingsButton);

        m_statusLabel = new QLabel(tr("Ready"));
        controlLayout->addWidget(m_statusLabel);

        m_spinnerLabel = new QLabel();
        m_spinnerLabel->setVisible(false);
        setupSpinner();
        controlLayout->addWidget(m_spinnerLabel);

        controlLayout->addStretch();
        inputLayout->addLayout(controlLayout);

        // === RESULTS SECTION ===
        auto *resultsWidget = new QWidget();
        auto *resultsLayout = new QVBoxLayout(resultsWidget);

        auto *resultsLabel = new QLabel(tr("<b>ANALYSIS RESULTS</b>"));
        resultsLayout->addWidget(resultsLabel);

        m_resultsTabWidget = new QTabWidget();

        // Extracted Text Tab
        m_extractedTextEdit = new QTextEdit();
        m_extractedTextEdit->setReadOnly(true);
        m_resultsTabWidget->addTab(m_extractedTextEdit, tr("Extracted Text"));

        // Summary Tab
        m_summaryTextEdit = new QTextEdit();
        m_summaryTextEdit->setReadOnly(true);
        m_resultsTabWidget->addTab(m_summaryTextEdit, tr("Summary"));

        // Keywords Tab
        m_keywordsTextEdit = new QTextEdit();
        m_keywordsTextEdit->setReadOnly(true);
        m_resultsTabWidget->addTab(m_keywordsTextEdit, tr("Keywords"));

        // Log Tab
        m_logTextEdit = new QPlainTextEdit();
        m_logTextEdit->setReadOnly(true);
        QFont monoFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
        monoFont.setPointSize(9);
        m_logTextEdit->setFont(monoFont);
        m_resultsTabWidget->addTab(m_logTextEdit, tr("LLM Log"));

        resultsLayout->addWidget(m_resultsTabWidget);

        // Add to splitter
        splitter->addWidget(inputWidget);
        splitter->addWidget(resultsWidget);
        splitter->setStretchFactor(0, 1);  // Input section
        splitter->setStretchFactor(1, 2);  // Results section

        mainLayout->addWidget(splitter);
    }

    void clearAllResults() {
        m_extractedText.clear();
        m_extractedTextEdit->clear();
        m_summaryTextEdit->clear();
        m_keywordsTextEdit->clear();
        m_logTextEdit->clear();
        m_conversationLog.clear();
    }

    void appendToLog(const QString &message) {
        QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss");
        QString logEntry = QString("[%1] %2").arg(timestamp).arg(message);
        m_conversationLog.append(logEntry);
        m_logTextEdit->appendPlainText(logEntry);

        // Also print to console for debugging
        std::cout << logEntry.toStdString() << std::endl;
    }

    void setupSpinner() {
        m_spinnerMovie = new QMovie(":/spinner.gif");
        if (m_spinnerMovie->isValid()) {
            m_spinnerLabel->setMovie(m_spinnerMovie);
        } else {
            delete m_spinnerMovie;
            m_spinnerMovie = nullptr;
            // Fallback to text-based spinner
            m_spinnerTimer = new QTimer(this);
            connect(m_spinnerTimer, &QTimer::timeout, this, [this]() {
                static int frame = 0;
                const QString frames[] = {"⠋", "⠙", "⠹", "⠸", "⠼", "⠴", "⠦", "⠧", "⠇", "⠏"};
                m_spinnerLabel->setText(frames[frame % 10]);
                frame++;
            });
        }
    }

    void startSpinner() {
        m_spinnerLabel->setVisible(true);
        if (m_spinnerMovie) {
            m_spinnerMovie->start();
        } else if (m_spinnerTimer) {
            m_spinnerTimer->start(100);
        }
    }

    void stopSpinner() {
        m_spinnerLabel->setVisible(false);
        if (m_spinnerMovie) {
            m_spinnerMovie->stop();
        } else if (m_spinnerTimer) {
            m_spinnerTimer->stop();
        }
    }

    bool extractPdfText(const QString &pdfPath) {
        appendToLog("Loading PDF document...");

        if (m_pdfDocument->load(pdfPath) != QPdfDocument::Error::None) {
            appendToLog("ERROR: Failed to load PDF");
            QMessageBox::critical(this, tr("Error"), tr("Failed to load PDF file."));
            return false;
        }

        int pageCount = m_pdfDocument->pageCount();
        int startPage = m_startPageSpin->value() - 1;
        int endPage = (m_endPageSpin->value() == 0) ? pageCount - 1 : m_endPageSpin->value() - 1;

        appendToLog(QString("Extracting pages %1 to %2 of %3").arg(startPage+1).arg(endPage+1).arg(pageCount));
        m_statusLabel->setText(QString("Extracting %1 pages...").arg(endPage - startPage + 1));

        QString extractedText;
        for (int i = startPage; i <= endPage && i < pageCount; ++i) {
            extractedText += m_pdfDocument->getAllText(i).text() + "\n";
        }

        if (!m_preserveCopyrightCheck->isChecked()) {
            extractedText = cleanCopyrightText(extractedText);
            appendToLog("Removed copyright notices");
        }

        m_extractedText = extractedText;
        appendToLog(QString("Extracted %1 characters").arg(m_extractedText.length()));
        return true;
    }

    void runAnalysis() {
        m_statusLabel->setText("Generating summary...");
        generateSummary();

        m_statusLabel->setText("Extracting keywords...");
        generateKeywords();

        m_statusLabel->setText("Analysis complete");
        stopSpinner();
        m_pdfAnalyzeButton->setEnabled(true);
        m_textAnalyzeButton->setEnabled(true);
    }

    void generateSummary() {
        appendToLog("\n=== SUMMARY GENERATION ===");

        QString systemPrompt = m_config.value("lm_studio.summary_system_prompt",
            "You are an expert scientific reviewer. Provide clear, concise analysis of research papers.");
        QString userPrompt = m_config.value("prompts.summary");
        double temp = m_config.value("lm_studio.summary_temperature", m_config.value("lmstudio.temperature")).toDouble();
        int maxTokens = m_config.value("lm_studio.summary_max_tokens", m_config.value("lmstudio.max_tokens")).toInt();
        QString model = m_config.value("lm_studio.summary_model_name", m_config.value("lmstudio.model_name"));

        QString summary = sendToLMStudio(systemPrompt, userPrompt, m_extractedText, temp, maxTokens, model);
        m_summaryTextEdit->setPlainText(summary);
    }

    void generateKeywords() {
        appendToLog("\n=== KEYWORD EXTRACTION ===");

        QString systemPrompt = m_config.value("lm_studio.keyword_system_prompt",
            "You are a scientific keyword extraction assistant.");
        QString userPrompt = m_config.value("prompts.keywords");
        double temp = m_config.value("lm_studio.keyword_temperature", m_config.value("lmstudio.temperature")).toDouble();
        int maxTokens = m_config.value("lm_studio.keyword_max_tokens", m_config.value("lmstudio.max_tokens")).toInt();
        QString model = m_config.value("lm_studio.keyword_model_name", m_config.value("lmstudio.model_name"));

        // First extraction
        appendToLog("Running keyword extraction with current prompt...");
        QString keywords = sendToLMStudio(systemPrompt, userPrompt, m_extractedText, temp, maxTokens, model);

        // Meta-analysis for improvement
        appendToLog("\nRunning meta-analysis for prompt improvement...");
        QString metaSystemPrompt = "You are an expert at analyzing scientific papers and optimizing keyword extraction prompts. You understand prompt engineering for LLMs.";
        QString metaUserPrompt = QString(
            "Analyze this keyword extraction scenario:\n\n"
            "Current prompt: %1\n\n"
            "Paper content (first 3000 chars):\n%2\n\n"
            "Keywords extracted: %3\n\n"
            "Based on the paper content and extraction results, provide an improved keyword extraction prompt "
            "that would better capture the key terms, concepts, methods, and entities from this specific type of paper.\n\n"
            "IMPORTANT REQUIREMENTS:\n"
            "- Return ONLY the improved prompt text, nothing else\n"
            "- The prompt MUST end with exactly: Text:\\n{text}\n"
            "- The {text} placeholder is critical - it will be replaced with the paper content\n"
            "- Include the literal string {text} in curly braces\n"
            "- Be specific to the domain shown in the paper\n"
            "- Ask for comma-delimited output\n\n"
            "Example format:\n"
            "Extract comma-delimited list of [specific terms for this domain]... Text:\\n{text}"
        ).arg(userPrompt).arg(m_extractedText.left(3000)).arg(keywords.left(500));

        QString improvedPrompt = sendToLMStudio(metaSystemPrompt, metaUserPrompt, "", 0.7, 1000, model);

        if (!improvedPrompt.isEmpty() && improvedPrompt != userPrompt) {
            appendToLog("Testing improved prompt...");
            QString improvedKeywords = sendToLMStudio(systemPrompt, improvedPrompt, m_extractedText, temp, maxTokens, model);

            QString finalOutput = QString(
                "=== KEYWORD EXTRACTION RESULTS ===\n\n"
                "## Keywords from CURRENT prompt:\n%1\n\n"
                "---\n\n"
                "## Keywords from SUGGESTED prompt:\n%2\n\n"
                "---\n\n"
                "## CURRENT PROMPT (from config):\n%3\n\n"
                "---\n\n"
                "## SUGGESTED PROMPT (AI-optimized for this paper):\n%4\n\n"
                "---\n\n"
                "To update your config with the suggested prompt:\n"
                "1. Click Settings button\n"
                "2. Replace the keywords prompt with the suggested version\n"
                "3. Save the configuration"
            ).arg(keywords.isEmpty() ? "(No keywords extracted)" : keywords)
             .arg(improvedKeywords.isEmpty() ? "(No keywords extracted)" : improvedKeywords)
             .arg(userPrompt)
             .arg(improvedPrompt);

            m_keywordsTextEdit->setPlainText(finalOutput);

            // Save to learning file
            QFile learningFile("keyword_prompts_learned.txt");
            if (learningFile.open(QIODevice::Append | QIODevice::Text)) {
                QTextStream stream(&learningFile);
                stream << "\n=== " << QDateTime::currentDateTime().toString(Qt::ISODate) << " ===" << Qt::endl;
                stream << "Original keywords: " << keywords.left(200) << Qt::endl;
                stream << "Improved keywords: " << improvedKeywords.left(200) << Qt::endl;
                stream << "Original prompt: " << userPrompt << Qt::endl;
                stream << "Improved prompt: " << improvedPrompt << Qt::endl;
                stream << "===\n" << Qt::endl;
                learningFile.close();
            }
        } else {
            m_keywordsTextEdit->setPlainText(QString(
                "=== KEYWORD EXTRACTION RESULTS ===\n\n%1\n\n"
                "---\n\n"
                "Current prompt is well-suited for this paper type.\n"
                "No improvements suggested."
            ).arg(keywords));
        }
    }

    QString sendToLMStudio(const QString &systemPrompt, const QString &userPrompt, const QString &text,
                          double temperature, int maxTokens, const QString &model) {
        appendToLog("\n--- LLM Request ---");
        appendToLog(QString("Model: %1, Temp: %2, MaxTokens: %3").arg(model).arg(temperature).arg(maxTokens));

        // Convert escaped newlines
        QString cleanSystemPrompt = systemPrompt;
        cleanSystemPrompt.replace("\\n", "\n");
        QString cleanUserPrompt = userPrompt;
        cleanUserPrompt.replace("\\n", "\n");

        // Replace {text} placeholder
        QString fullPrompt = cleanUserPrompt;
        fullPrompt.replace("{text}", text);

        appendToLog(QString("System: %1...").arg(cleanSystemPrompt.left(100)));
        appendToLog(QString("User: %1...").arg(fullPrompt.left(200)));

        // Build JSON request
        QJsonObject systemMessage;
        systemMessage["role"] = "system";
        systemMessage["content"] = cleanSystemPrompt;

        QJsonObject userMessage;
        userMessage["role"] = "user";
        userMessage["content"] = fullPrompt;

        QJsonArray messages;
        messages.append(systemMessage);
        messages.append(userMessage);

        QJsonObject requestData;
        requestData["model"] = model;
        requestData["messages"] = messages;
        requestData["temperature"] = temperature;
        requestData["max_tokens"] = maxTokens;
        requestData["stream"] = false;

        QJsonDocument doc(requestData);
        QByteArray jsonData = doc.toJson();

        // Log the request
        appendToLog("Sending request to LM Studio...");

        QNetworkRequest request((QUrl(m_config.value("lmstudio.endpoint"))));
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

        QNetworkReply *reply = m_networkManager->post(request, jsonData);

        QEventLoop loop;
        QTimer timer;
        timer.setSingleShot(true);
        timer.setInterval(m_config.value("lmstudio.timeout", "120000").toInt());

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
                        appendToLog(QString("Response received (%1 chars)").arg(result.length()));
                    }
                }
            } else {
                appendToLog(QString("Network error: %1").arg(reply->errorString()));
            }
        } else {
            appendToLog("Request timed out");
        }

        reply->deleteLater();
        return result;
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
        return cleaned;
    }

    QString sanitizeText(const QString &input) {
        QString result = input;

        // Remove null and control characters
        result.remove(QChar::Null);
        result.remove(QChar(0xFFFD));

        // Replace Unicode spaces
        result.replace(QRegularExpression("[\\x{00A0}\\x{1680}\\x{2000}-\\x{200B}\\x{202F}\\x{205F}\\x{3000}\\x{FEFF}]+"), " ");

        // Remove zero-width characters
        result.remove(QRegularExpression("[\\x{200B}-\\x{200D}\\x{FEFF}]+"));

        // Remove object replacement characters
        result.remove(QChar(0xFFFC));

        // Clean up whitespace
        result.replace(QRegularExpression("[ \\t]+"), " ");
        result.replace(QRegularExpression("\\n{3,}"), "\n\n");

        // Trim lines
        QStringList lines = result.split('\n');
        for (int i = 0; i < lines.size(); ++i) {
            lines[i] = lines[i].trimmed();
        }
        result = lines.join('\n');

        return result.trimmed();
    }

    void loadConfiguration() {
        SimpleTomlParser parser;
        m_config = parser.parse("lmstudio_config.toml");

        // Set defaults if missing
        if (!m_config.contains("lmstudio.endpoint")) {
            m_config["lmstudio.endpoint"] = "http://172.20.10.3:8090/v1/chat/completions";
        }
        if (!m_config.contains("lmstudio.timeout")) {
            m_config["lmstudio.timeout"] = "1200000";
        }
        if (!m_config.contains("lmstudio.model_name")) {
            m_config["lmstudio.model_name"] = "gpt-oss-120b";
        }
    }

    bool processPDF(const QString &pdfPath, const QString &outputPath, const QCommandLineParser &parser) {
        // Command line mode processing (simplified for space)
        return true;
    }

private:
    // UI Elements - Input
    QTabWidget *m_inputTabWidget;
    QLineEdit *m_filePathEdit;
    QPushButton *m_browseButton;
    QPushButton *m_pdfAnalyzeButton;
    QSpinBox *m_startPageSpin;
    QSpinBox *m_endPageSpin;
    QCheckBox *m_preserveCopyrightCheck;

    QTextEdit *m_pasteTextEdit;
    QPushButton *m_textAnalyzeButton;

    QPushButton *m_settingsButton;
    QLabel *m_statusLabel;
    QLabel *m_spinnerLabel;

    // UI Elements - Results
    QTabWidget *m_resultsTabWidget;
    QTextEdit *m_extractedTextEdit;
    QTextEdit *m_summaryTextEdit;
    QTextEdit *m_keywordsTextEdit;
    QPlainTextEdit *m_logTextEdit;

    // Core objects
    std::unique_ptr<QPdfDocument> m_pdfDocument;
    QNetworkAccessManager *m_networkManager;
    QMovie *m_spinnerMovie = nullptr;
    QTimer *m_spinnerTimer = nullptr;

    // Configuration and state
    QMap<QString, QString> m_config;
    QString m_extractedText;
    QStringList m_conversationLog;
    bool m_commandLineMode;
};

#include "main_redesigned.moc"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName("PDF Extractor GUI");
    QApplication::setApplicationVersion("3.0");

    QCommandLineParser parser;
    parser.setApplicationDescription("PDF Extractor with AI Analysis - Redesigned");
    parser.addHelpOption();
    parser.addVersionOption();

    // Command line options for backward compatibility
    parser.addPositionalArgument("pdf", "PDF file to extract text from");
    parser.addPositionalArgument("output", "Output text file");

    QCommandLineOption pageRangeOption(QStringList() << "p" << "pages", "Page range", "range");
    parser.addOption(pageRangeOption);

    QCommandLineOption preserveOption("preserve", "Preserve copyright");
    parser.addOption(preserveOption);

    QCommandLineOption guiOption(QStringList() << "g" << "gui", "Force GUI mode");
    parser.addOption(guiOption);

    parser.process(app);

    PDFExtractorGUI window;

    if (parser.positionalArguments().size() >= 2 && !parser.isSet(guiOption)) {
        window.processCommandLine(parser);
    }

    window.show();
    return app.exec();
}