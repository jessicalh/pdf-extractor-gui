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
#include <iostream>
#include "tomlparser.h"

// Settings Dialog for TOML configuration
class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    SettingsDialog(const QMap<QString, QString> &config, QWidget *parent = nullptr)
        : QDialog(parent), m_config(config) {
        setWindowTitle("Settings - LM Studio Configuration");
        setModal(true);
        resize(800, 700);

        auto *mainLayout = new QVBoxLayout(this);

        // Create tabs for different sections
        auto *tabWidget = new QTabWidget();

        // LM Studio tab
        auto *lmWidget = new QWidget();
        auto *lmLayout = new QVBoxLayout(lmWidget);

        auto *lmGroup = new QGroupBox("LM Studio Settings");
        auto *lmGrid = new QGridLayout(lmGroup);

        m_endpointEdit = new QLineEdit(config.value("lmstudio.endpoint"));
        m_timeoutEdit = new QLineEdit(config.value("lmstudio.timeout"));
        m_temperatureEdit = new QLineEdit(config.value("lmstudio.temperature"));
        m_maxTokensEdit = new QLineEdit(config.value("lmstudio.max_tokens"));
        m_modelEdit = new QLineEdit(config.value("lmstudio.model_name"));

        lmGrid->addWidget(new QLabel("Endpoint:"), 0, 0);
        lmGrid->addWidget(m_endpointEdit, 0, 1);
        lmGrid->addWidget(new QLabel("Timeout (ms):"), 1, 0);
        lmGrid->addWidget(m_timeoutEdit, 1, 1);
        lmGrid->addWidget(new QLabel("Temperature:"), 2, 0);
        lmGrid->addWidget(m_temperatureEdit, 2, 1);
        lmGrid->addWidget(new QLabel("Max Tokens:"), 3, 0);
        lmGrid->addWidget(m_maxTokensEdit, 3, 1);
        lmGrid->addWidget(new QLabel("Model Name:"), 4, 0);
        lmGrid->addWidget(m_modelEdit, 4, 1);

        // Keyword specific settings
        auto *kwGroup = new QGroupBox("Keyword Extraction Settings");
        auto *kwGrid = new QGridLayout(kwGroup);

        QString kwSystemPrompt = config.value("lm_studio.keyword_system_prompt");
        kwSystemPrompt.replace("\\n", "\n");
        m_kwSystemPromptEdit = new QTextEdit(kwSystemPrompt);
        m_kwSystemPromptEdit->setMaximumHeight(80);
        QString kwUserPrompt = config.value("prompts.keywords");
        kwUserPrompt.replace("\\n", "\n");
        m_kwUserPromptEdit = new QTextEdit(kwUserPrompt);
        m_kwUserPromptEdit->setMaximumHeight(120);
        m_kwTempEdit = new QLineEdit(config.value("lm_studio.keyword_temperature"));
        m_kwMaxTokensEdit = new QLineEdit(config.value("lm_studio.keyword_max_tokens"));
        m_kwModelEdit = new QLineEdit(config.value("lm_studio.keyword_model_name"));

        kwGrid->addWidget(new QLabel("System Prompt:"), 0, 0);
        kwGrid->addWidget(m_kwSystemPromptEdit, 0, 1);
        kwGrid->addWidget(new QLabel("User Prompt:"), 1, 0);
        kwGrid->addWidget(m_kwUserPromptEdit, 1, 1);
        kwGrid->addWidget(new QLabel("Temperature:"), 2, 0);
        kwGrid->addWidget(m_kwTempEdit, 2, 1);
        kwGrid->addWidget(new QLabel("Max Tokens:"), 3, 0);
        kwGrid->addWidget(m_kwMaxTokensEdit, 3, 1);
        kwGrid->addWidget(new QLabel("Model:"), 4, 0);
        kwGrid->addWidget(m_kwModelEdit, 4, 1);

        // Summary specific settings
        auto *sumGroup = new QGroupBox("Summary Generation Settings");
        auto *sumGrid = new QGridLayout(sumGroup);

        QString sumSystemPrompt = config.value("lm_studio.summary_system_prompt");
        sumSystemPrompt.replace("\\n", "\n");
        m_sumSystemPromptEdit = new QTextEdit(sumSystemPrompt);
        m_sumSystemPromptEdit->setMaximumHeight(80);
        QString sumUserPrompt = config.value("prompts.summary");
        sumUserPrompt.replace("\\n", "\n");
        m_sumUserPromptEdit = new QTextEdit(sumUserPrompt);
        m_sumUserPromptEdit->setMaximumHeight(120);
        m_sumTempEdit = new QLineEdit(config.value("lm_studio.summary_temperature"));
        m_sumMaxTokensEdit = new QLineEdit(config.value("lm_studio.summary_max_tokens"));
        m_sumModelEdit = new QLineEdit(config.value("lm_studio.summary_model_name"));

        sumGrid->addWidget(new QLabel("System Prompt:"), 0, 0);
        sumGrid->addWidget(m_sumSystemPromptEdit, 0, 1);
        sumGrid->addWidget(new QLabel("User Prompt:"), 1, 0);
        sumGrid->addWidget(m_sumUserPromptEdit, 1, 1);
        sumGrid->addWidget(new QLabel("Temperature:"), 2, 0);
        sumGrid->addWidget(m_sumTempEdit, 2, 1);
        sumGrid->addWidget(new QLabel("Max Tokens:"), 3, 0);
        sumGrid->addWidget(m_sumMaxTokensEdit, 3, 1);
        sumGrid->addWidget(new QLabel("Model:"), 4, 0);
        sumGrid->addWidget(m_sumModelEdit, 4, 1);

        lmLayout->addWidget(lmGroup);
        lmLayout->addWidget(kwGroup);
        lmLayout->addWidget(sumGroup);
        lmLayout->addStretch();

        tabWidget->addTab(lmWidget, "LM Studio");

        // The Prompts tab is no longer needed since we have user prompts in the LM Studio tab

        // Buttons
        auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
        connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
        connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

        mainLayout->addWidget(tabWidget);
        mainLayout->addWidget(buttonBox);
    }

    QMap<QString, QString> getConfiguration() {
        QMap<QString, QString> newConfig = m_config;

        newConfig["lmstudio.endpoint"] = m_endpointEdit->text();
        newConfig["lmstudio.timeout"] = m_timeoutEdit->text();
        newConfig["lmstudio.temperature"] = m_temperatureEdit->text();
        newConfig["lmstudio.max_tokens"] = m_maxTokensEdit->text();
        newConfig["lmstudio.model_name"] = m_modelEdit->text();

        newConfig["lm_studio.keyword_system_prompt"] = m_kwSystemPromptEdit->toPlainText();
        newConfig["prompts.keywords"] = m_kwUserPromptEdit->toPlainText();
        newConfig["lm_studio.keyword_temperature"] = m_kwTempEdit->text();
        newConfig["lm_studio.keyword_max_tokens"] = m_kwMaxTokensEdit->text();
        newConfig["lm_studio.keyword_model_name"] = m_kwModelEdit->text();

        newConfig["lm_studio.summary_system_prompt"] = m_sumSystemPromptEdit->toPlainText();
        newConfig["prompts.summary"] = m_sumUserPromptEdit->toPlainText();
        newConfig["lm_studio.summary_temperature"] = m_sumTempEdit->text();
        newConfig["lm_studio.summary_max_tokens"] = m_sumMaxTokensEdit->text();
        newConfig["lm_studio.summary_model_name"] = m_sumModelEdit->text();

        return newConfig;
    }

private:
    QMap<QString, QString> m_config;
    QLineEdit *m_endpointEdit;
    QLineEdit *m_timeoutEdit;
    QLineEdit *m_temperatureEdit;
    QLineEdit *m_maxTokensEdit;
    QLineEdit *m_modelEdit;

    QTextEdit *m_kwSystemPromptEdit;
    QTextEdit *m_kwUserPromptEdit;
    QLineEdit *m_kwTempEdit;
    QLineEdit *m_kwMaxTokensEdit;
    QLineEdit *m_kwModelEdit;

    QTextEdit *m_sumSystemPromptEdit;
    QTextEdit *m_sumUserPromptEdit;
    QLineEdit *m_sumTempEdit;
    QLineEdit *m_sumMaxTokensEdit;
    QLineEdit *m_sumModelEdit;
};

// Main GUI Application
class PDFExtractorGUI : public QMainWindow
{
    Q_OBJECT

public:
    PDFExtractorGUI(QWidget *parent = nullptr) : QMainWindow(parent) {
        m_networkManager = new QNetworkAccessManager(this);
        m_pdfDocument = new QPdfDocument(this);
        m_ignoreTextChange = true;
        setupUi();
        loadConfiguration();
        m_ignoreTextChange = false;
    }

    void processCommandLine(const QCommandLineParser &parser) {
        if (parser.positionalArguments().size() >= 2) {
            QString pdfPath = parser.positionalArguments()[0];
            QString outputPath = parser.positionalArguments()[1];

            m_filePathEdit->setText(pdfPath);
            m_outputPath = outputPath;

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

            if (parser.isSet("preserve")) {
                m_preserveCopyrightCheck->setChecked(true);
            }

            if (parser.isSet("summary")) {
                m_summaryPath = parser.value("summary");
                m_generateSummaryCheck->setChecked(true);
            }

            if (parser.isSet("keywords")) {
                m_keywordsPath = parser.value("keywords");
                m_generateKeywordsCheck->setChecked(true);
            }

            if (parser.isSet("verbose")) {
                m_verbose = true;
            }

            if (!pdfPath.isEmpty()) {
                m_commandLineMode = true;
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

    void onSettingsClicked() {
        // Debug output to see what's in config
        std::cout << "\n=== Current Configuration ===" << std::endl;
        std::cout << "prompts.keywords exists: " << (m_config.contains("prompts.keywords") ? "YES" : "NO") << std::endl;
        std::cout << "prompts.summary exists: " << (m_config.contains("prompts.summary") ? "YES" : "NO") << std::endl;

        if (m_config.contains("prompts.keywords")) {
            std::cout << "Keywords prompt length: " << m_config["prompts.keywords"].length() << std::endl;
            std::cout << "Keywords prompt full: [" << m_config["prompts.keywords"].toStdString() << "]" << std::endl;
        }
        if (m_config.contains("prompts.summary")) {
            std::cout << "Summary prompt length: " << m_config["prompts.summary"].length() << std::endl;
            std::cout << "Summary prompt full: [" << m_config["prompts.summary"].toStdString() << "]" << std::endl;
        }

        std::cout << "==========================\n" << std::endl;

        SettingsDialog dialog(m_config, this);
        if (dialog.exec() == QDialog::Accepted) {
            m_config = dialog.getConfiguration();
        }
    }

    void onExtractClicked() {
        QString pdfPath = m_filePathEdit->text();
        if (pdfPath.isEmpty()) {
            QMessageBox::warning(this, tr("Warning"), tr("Please select a PDF file first."));
            return;
        }

        // Start spinning indicator
        startSpinner();
        m_extractButton->setEnabled(false);
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
            stopSpinner();
            return;
        }

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

        int startPage = m_startPageSpin->value() - 1;
        int endPage = m_endPageSpin->value() - 1;

        QString fullText;
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
        }

        m_extractedText = fullText;
        m_extractedTextEdit->setPlainText(fullText);

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

        // In GUI mode, just enable the analyze button and stop
        if (!m_commandLineMode) {
            m_analyzeButton->setEnabled(true);
            onProcessComplete();
            m_statusLabel->setText("Text extracted. Ready to analyze.");
        } else {
            // Command line mode continues with analysis if requested
            if (m_generateSummaryCheck->isChecked()) {
                generateSummary();
            } else if (m_generateKeywordsCheck->isChecked()) {
                generateKeywords();
            } else {
                onProcessComplete();
            }
        }
    }

    void onAnalyzeClicked() {
        // Start spinner
        startSpinner();
        m_analyzeButton->setEnabled(false);

        // Get current text from the text edit (could be pasted)
        QString rawText = m_extractedTextEdit->toPlainText();
        std::cout << "\n=== DEBUG: onAnalyzeClicked ===" << std::endl;
        std::cout << "Raw text length from edit widget: " << rawText.length() << std::endl;

        // Sanitize the text - remove non-text elements and clean up
        m_extractedText = sanitizeText(rawText);
        std::cout << "Sanitized text length: " << m_extractedText.length() << std::endl;
        std::cout << "First 200 chars after sanitization: " << m_extractedText.left(200).toStdString() << "..." << std::endl;

        if (m_extractedText.isEmpty()) {
            QMessageBox::warning(this, tr("Warning"), tr("No valid text to analyze. Please extract or paste text first."));
            stopSpinner();
            m_analyzeButton->setEnabled(true);
            return;
        }

        // Replace the text in the edit field with the sanitized version
        // This shows the user what will actually be analyzed
        if (rawText != m_extractedText) {
            m_ignoreTextChange = true;  // Prevent triggering textChanged signal
            m_extractedTextEdit->setPlainText(m_extractedText);
            m_ignoreTextChange = false;
            m_statusLabel->setText("Text sanitized and ready for analysis.");
        }

        // Clear previous analysis results
        m_summaryTextEdit->clear();
        m_keywordsTextEdit->clear();

        // Start analysis
        if (m_generateSummaryCheck->isChecked()) {
            generateSummary();
        } else if (m_generateKeywordsCheck->isChecked()) {
            generateKeywords();
        } else {
            QMessageBox::information(this, tr("Info"), tr("Please enable Summary or Keywords generation in settings."));
            stopSpinner();
            m_analyzeButton->setEnabled(true);
        }
    }

    void onExtractedTextChanged() {
        if (m_ignoreTextChange) return;

        // Enable analyze button if there's text
        bool hasText = !m_extractedTextEdit->toPlainText().isEmpty();
        m_analyzeButton->setEnabled(hasText);

        if (hasText && m_extractedTextEdit->toPlainText() != m_extractedText) {
            // Text was modified (pasted or edited)
            m_statusLabel->setText("Text modified. Ready to analyze.");
        }
    }

    void generateSummary() {
        m_statusLabel->setText("Generating summary...");
        m_tabWidget->setCurrentIndex(1);

        QString systemPrompt = m_config.value("lm_studio.summary_system_prompt",
            "You are an expert scientific reviewer. Provide clear, concise analysis of research papers focusing on key findings and significance.");
        QString userPrompt = m_config.value("prompts.summary");
        double temp = m_config.value("lm_studio.summary_temperature", m_config.value("lmstudio.temperature")).toDouble();
        int maxTokens = m_config.value("lm_studio.summary_max_tokens", m_config.value("lmstudio.max_tokens")).toInt();
        QString model = m_config.value("lm_studio.summary_model_name", m_config.value("lmstudio.model_name"));

        std::cout << "\n=== DEBUG: generateSummary ===" << std::endl;
        std::cout << "m_extractedText length: " << m_extractedText.length() << std::endl;
        std::cout << "User prompt from config: " << userPrompt.toStdString() << std::endl;

        QString summary = sendToLMStudio(systemPrompt, userPrompt, m_extractedText, temp, maxTokens, model);
        m_summaryTextEdit->setPlainText(summary);

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
        m_tabWidget->setCurrentIndex(2);

        QString systemPrompt = m_config.value("lm_studio.keyword_system_prompt",
            "You are a scientific keyword extraction assistant. Focus on extracting specific scientific terms, organisms, chemicals, methods, and concepts from research papers.");
        QString userPrompt = m_config.value("prompts.keywords");
        double temp = m_config.value("lm_studio.keyword_temperature", m_config.value("lmstudio.temperature")).toDouble();
        int maxTokens = m_config.value("lm_studio.keyword_max_tokens", m_config.value("lmstudio.max_tokens")).toInt();
        QString model = m_config.value("lm_studio.keyword_model_name", m_config.value("lmstudio.model_name"));

        std::cout << "\n=== DEBUG: generateKeywords ===" << std::endl;
        std::cout << "m_extractedText length: " << m_extractedText.length() << std::endl;
        std::cout << "User prompt from config: " << userPrompt.toStdString() << std::endl;

        // First, extract keywords with current prompt
        QString keywords = sendToLMStudio(systemPrompt, userPrompt, m_extractedText, temp, maxTokens, model);

        // Always run meta-analysis to suggest improvements
        m_statusLabel->setText("Analyzing prompt effectiveness...");
        std::cout << "\n=== Running meta-analysis for prompt improvement ===" << std::endl;

        // Meta-prompt to analyze and potentially improve the keyword extraction prompt
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
            m_statusLabel->setText("Testing improved prompt...");
            std::cout << "Suggested improved prompt: " << improvedPrompt.toStdString() << std::endl;

            // Try the improved prompt
            QString improvedKeywords = sendToLMStudio(systemPrompt, improvedPrompt, m_extractedText, temp, maxTokens, model);

            // Present both results for comparison
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

            // Save the comparison to learning file
            QFile learningFile("keyword_prompts_learned.txt");
            if (learningFile.open(QIODevice::Append | QIODevice::Text)) {
                QTextStream stream(&learningFile);
                stream << "\n=== " << QDateTime::currentDateTime().toString(Qt::ISODate) << " ===" << Qt::endl;
                stream << "Paper excerpt: " << m_extractedText.left(200) << "..." << Qt::endl;
                stream << "Original keywords: " << keywords.left(200) << Qt::endl;
                stream << "Improved keywords: " << improvedKeywords.left(200) << Qt::endl;
                stream << "Original prompt: " << userPrompt << Qt::endl;
                stream << "Improved prompt: " << improvedPrompt << Qt::endl;
                stream << "===\n" << Qt::endl;
                learningFile.close();
                std::cout << "Saved comparison to keyword_prompts_learned.txt" << std::endl;
            }
        } else {
            // No improvement suggested or same prompt - just show the results
            m_keywordsTextEdit->setPlainText(QString(
                "=== KEYWORD EXTRACTION RESULTS ===\n\n%1\n\n"
                "---\n\n"
                "Current prompt is well-suited for this paper type.\n"
                "No improvements suggested."
            ).arg(keywords));
        }

        m_statusLabel->setText("Keyword extraction complete.");

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
        stopSpinner();
        m_statusLabel->setText("Complete!");

        // Re-enable the appropriate button
        if (!m_commandLineMode) {
            if (m_tabWidget->currentIndex() == 0) {
                // On extracted text tab
                m_extractButton->setEnabled(true);
            } else {
                // On analysis tabs
                m_analyzeButton->setEnabled(true);
            }
        }

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
        m_extractButton = new QPushButton(tr("Extract Text"));
        m_extractButton->setStyleSheet("QPushButton { background-color: #4CAF50; color: white; font-weight: bold; padding: 8px; }");

        m_analyzeButton = new QPushButton(tr("Analyze Text"));
        m_analyzeButton->setStyleSheet("QPushButton { background-color: #FF9800; color: white; font-weight: bold; padding: 8px; }");
        m_analyzeButton->setEnabled(false);

        m_settingsButton = new QPushButton(tr("Settings"));
        m_settingsButton->setStyleSheet("QPushButton { background-color: #2196F3; color: white; padding: 8px; }");

        fileLayout->addWidget(new QLabel(tr("File:")));
        fileLayout->addWidget(m_filePathEdit);
        fileLayout->addWidget(m_browseButton);
        fileLayout->addWidget(m_settingsButton);
        fileLayout->addWidget(m_extractButton);
        fileLayout->addWidget(m_analyzeButton);

        // Settings section
        auto *settingsGroup = new QGroupBox(tr("Extraction Settings"));
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

        settingsLayout->addWidget(new QLabel(tr("Start Page:")), 0, 0);
        settingsLayout->addWidget(m_startPageSpin, 0, 1);
        settingsLayout->addWidget(new QLabel(tr("End Page:")), 0, 2);
        settingsLayout->addWidget(m_endPageSpin, 0, 3);
        settingsLayout->addWidget(m_preserveCopyrightCheck, 1, 0, 1, 2);
        settingsLayout->addWidget(m_generateSummaryCheck, 1, 2);
        settingsLayout->addWidget(m_generateKeywordsCheck, 1, 3);

        // Tab widget for results
        m_tabWidget = new QTabWidget();

        m_extractedTextEdit = new QTextEdit();
        m_extractedTextEdit->setReadOnly(false);  // Allow editing for pasting
        m_tabWidget->addTab(m_extractedTextEdit, tr("Extracted Text"));

        m_summaryTextEdit = new QTextEdit();
        m_summaryTextEdit->setReadOnly(true);
        m_tabWidget->addTab(m_summaryTextEdit, tr("Summary"));

        m_keywordsTextEdit = new QTextEdit();
        m_keywordsTextEdit->setReadOnly(true);
        m_tabWidget->addTab(m_keywordsTextEdit, tr("Keywords"));

        // Status section with spinner
        auto *statusLayout = new QHBoxLayout();

        m_statusLabel = new QLabel(tr("Ready"));

        // Create spinner (using animated GIF or movie)
        m_spinnerLabel = new QLabel();
        m_spinnerMovie = new QMovie(":/spinner.gif");  // You can use a built-in or custom spinner
        // If no custom spinner, create a simple text-based one
        if (!m_spinnerMovie->isValid()) {
            delete m_spinnerMovie;
            m_spinnerMovie = nullptr;
            m_spinnerLabel->setText("⣾⣽⣻⢿⡿⣟⣯⣷");  // Unicode spinner characters
        } else {
            m_spinnerLabel->setMovie(m_spinnerMovie);
        }
        m_spinnerLabel->setVisible(false);

        statusLayout->addWidget(m_statusLabel);
        statusLayout->addStretch();
        statusLayout->addWidget(m_spinnerLabel);

        // Add all to main layout
        mainLayout->addWidget(fileGroup);
        mainLayout->addWidget(settingsGroup);
        mainLayout->addWidget(m_tabWidget);
        mainLayout->addLayout(statusLayout);

        // Connect signals
        connect(m_browseButton, &QPushButton::clicked, this, &PDFExtractorGUI::onBrowseClicked);
        connect(m_settingsButton, &QPushButton::clicked, this, &PDFExtractorGUI::onSettingsClicked);
        connect(m_extractButton, &QPushButton::clicked, this, &PDFExtractorGUI::onExtractClicked);
        connect(m_analyzeButton, &QPushButton::clicked, this, &PDFExtractorGUI::onAnalyzeClicked);
        connect(m_extractedTextEdit, &QTextEdit::textChanged, this, &PDFExtractorGUI::onExtractedTextChanged);

        // Set window properties
        setWindowTitle(tr("PDF Extractor with AI"));
        resize(900, 700);

        // Create simple text spinner timer if no movie
        if (!m_spinnerMovie) {
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
        } else if (!m_spinnerTimer) {
            // Create a simple text-based spinner using a timer
            m_spinnerTimer = new QTimer(this);
            connect(m_spinnerTimer, &QTimer::timeout, this, [this]() {
                static int frame = 0;
                const QString frames[] = {"⠋", "⠙", "⠹", "⠸", "⠼", "⠴", "⠦", "⠧", "⠇", "⠏"};
                m_spinnerLabel->setText(frames[frame % 10]);
                frame++;
            });
            m_spinnerTimer->start(100);
        } else {
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
        m_extractButton->setEnabled(true);
    }

    QString sanitizeText(const QString &input) {
        QString result = input;

        // Remove null characters and other control characters except newlines and tabs
        result.remove(QChar::Null);
        result.remove(QChar(0xFFFD)); // Unicode replacement character

        // Replace various Unicode spaces with regular spaces
        result.replace(QRegularExpression("[\\x{00A0}\\x{1680}\\x{2000}-\\x{200B}\\x{202F}\\x{205F}\\x{3000}\\x{FEFF}]+"), " ");

        // Remove zero-width characters
        result.remove(QRegularExpression("[\\x{200B}-\\x{200D}\\x{FEFF}]+"));

        // Remove object replacement characters (often used for images/graphics)
        result.remove(QChar(0xFFFC));

        // Replace multiple consecutive whitespaces with single space
        result.replace(QRegularExpression("[ \\t]+"), " ");

        // Replace multiple consecutive newlines with double newline
        result.replace(QRegularExpression("\\n{3,}"), "\n\n");

        // Remove leading/trailing whitespace from each line
        QStringList lines = result.split('\n');
        for (int i = 0; i < lines.size(); ++i) {
            lines[i] = lines[i].trimmed();
        }
        result = lines.join('\n');

        // Final trim
        result = result.trimmed();

        // Ensure text is not too large (limit to reasonable size for API)
        if (result.length() > 100000) {
            result = result.left(100000);
            result += "\n\n[Text truncated due to length]";
        }

        return result;
    }

    void loadConfiguration() {
        SimpleTomlParser parser;
        m_config = parser.parse("lmstudio_config.toml");


        // Set defaults for any missing values
        if (!m_config.contains("lmstudio.endpoint")) {
            m_config["lmstudio.endpoint"] = "http://172.20.10.3:8090/v1/chat/completions";
        }
        if (!m_config.contains("lmstudio.timeout")) {
            m_config["lmstudio.timeout"] = "1200000";
        }
        if (!m_config.contains("lmstudio.temperature")) {
            m_config["lmstudio.temperature"] = "0.8";
        }
        if (!m_config.contains("lmstudio.max_tokens")) {
            m_config["lmstudio.max_tokens"] = "8000";
        }
        if (!m_config.contains("lmstudio.model_name")) {
            m_config["lmstudio.model_name"] = "gpt-oss-120b";
        }

        // Default prompts if not in config
        if (!m_config.contains("prompts.keywords")) {
            m_config["prompts.keywords"] = "Extract a comma delimited list of keywords from the text.";
        }
        if (!m_config.contains("prompts.summary")) {
            m_config["prompts.summary"] = "Provide a concise summary of the text.";
        }

        // Default system prompts
        if (!m_config.contains("lm_studio.keyword_system_prompt")) {
            m_config["lm_studio.keyword_system_prompt"] = "You are an expert at extracting key terms from scientific papers.";
        }
        if (!m_config.contains("lm_studio.summary_system_prompt")) {
            m_config["lm_studio.summary_system_prompt"] = "You are an expert scientific reviewer.";
        }

        // Use main settings as defaults for keyword/summary specific settings
        if (!m_config.contains("lm_studio.keyword_temperature")) {
            m_config["lm_studio.keyword_temperature"] = m_config["lmstudio.temperature"];
        }
        if (!m_config.contains("lm_studio.keyword_max_tokens")) {
            m_config["lm_studio.keyword_max_tokens"] = m_config["lmstudio.max_tokens"];
        }
        if (!m_config.contains("lm_studio.keyword_model_name")) {
            m_config["lm_studio.keyword_model_name"] = m_config["lmstudio.model_name"];
        }
        if (!m_config.contains("lm_studio.summary_temperature")) {
            m_config["lm_studio.summary_temperature"] = m_config["lmstudio.temperature"];
        }
        if (!m_config.contains("lm_studio.summary_max_tokens")) {
            m_config["lm_studio.summary_max_tokens"] = m_config["lmstudio.max_tokens"];
        }
        if (!m_config.contains("lm_studio.summary_model_name")) {
            m_config["lm_studio.summary_model_name"] = m_config["lmstudio.model_name"];
        }
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

    QString sendToLMStudio(const QString &systemPrompt, const QString &userPrompt, const QString &text,
                          double temperature, int maxTokens, const QString &model) {
        // Debug: Check what we're receiving
        std::cout << "\n=== DEBUG: sendToLMStudio called ===" << std::endl;
        std::cout << "Text parameter length: " << text.length() << " characters" << std::endl;
        std::cout << "First 100 chars of text: " << text.left(100).toStdString() << "..." << std::endl;
        std::cout << "UserPrompt (raw): " << userPrompt.toStdString() << std::endl;

        // Convert escaped newlines to actual newlines
        QString cleanSystemPrompt = systemPrompt;
        cleanSystemPrompt.replace("\\n", "\n");

        QString cleanUserPrompt = userPrompt;
        cleanUserPrompt.replace("\\n", "\n");

        // Replace the {text} placeholder with actual text
        QString fullPrompt = cleanUserPrompt;
        bool hadPlaceholder = fullPrompt.contains("{text}");
        fullPrompt.replace("{text}", text);

        std::cout << "Had {text} placeholder: " << (hadPlaceholder ? "YES" : "NO") << std::endl;
        std::cout << "Full prompt length after replacement: " << fullPrompt.length() << " characters" << std::endl;
        std::cout << "Full prompt preview (500 chars): " << fullPrompt.left(500).toStdString() << "..." << std::endl;

        if (m_verbose) {
            std::cout << "\n=== Sending to LM Studio ===" << std::endl;
            std::cout << "System prompt: " << cleanSystemPrompt.left(100).toStdString() << "..." << std::endl;
            std::cout << "User prompt (first 200 chars): " << fullPrompt.left(200).toStdString() << "..." << std::endl;
            std::cout << "Text length included: " << text.length() << " characters" << std::endl;
        }

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

        if (m_verbose) {
            std::cout << "[VERBOSE] Sending to: " << m_config.value("lmstudio.endpoint").toStdString() << std::endl;
            std::cout << "[VERBOSE] Model: " << model.toStdString() << std::endl;
            std::cout << "[VERBOSE] Temperature: " << temperature << std::endl;
            std::cout << "[VERBOSE] Max tokens: " << maxTokens << std::endl;
        }

        QJsonDocument doc(requestData);
        QByteArray jsonData = doc.toJson();

        std::cout << "\n=== DEBUG: JSON being sent to LM Studio ===" << std::endl;
        std::cout << "JSON length: " << jsonData.length() << " bytes" << std::endl;
        std::cout << "JSON content (first 1000 chars): " << QString::fromUtf8(jsonData.left(1000)).toStdString() << "..." << std::endl;

        QNetworkRequest request((QUrl(m_config.value("lmstudio.endpoint"))));
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

        QNetworkReply *reply = m_networkManager->post(request, jsonData);

        QEventLoop loop;
        QTimer timer;
        timer.setSingleShot(true);
        timer.setInterval(m_config.value("lmstudio.timeout", "120000").toInt());

        connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
        connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);

        // Keep spinner running
        if (!m_spinnerMovie && m_spinnerTimer) {
            m_spinnerTimer->start(100);
        }

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
    QPushButton *m_analyzeButton;
    QPushButton *m_settingsButton;
    QLabel *m_statusLabel;
    QLabel *m_spinnerLabel;
    QMovie *m_spinnerMovie = nullptr;
    QTimer *m_spinnerTimer = nullptr;
    QTabWidget *m_tabWidget;
    QTextEdit *m_extractedTextEdit;
    QTextEdit *m_summaryTextEdit;
    QTextEdit *m_keywordsTextEdit;
    QSpinBox *m_startPageSpin;
    QSpinBox *m_endPageSpin;
    QCheckBox *m_preserveCopyrightCheck;
    QCheckBox *m_generateSummaryCheck;
    QCheckBox *m_generateKeywordsCheck;

    // Core objects
    QPdfDocument *m_pdfDocument;
    QNetworkAccessManager *m_networkManager;

    // Configuration
    QMap<QString, QString> m_config;

    // State
    QString m_extractedText;
    QString m_outputPath;
    QString m_summaryPath;
    QString m_keywordsPath;
    bool m_commandLineMode = false;
    bool m_verbose = false;
    bool m_ignoreTextChange = false;
};

#include "main.moc"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName("PDF Extractor GUI");
    QApplication::setApplicationVersion("2.0");

    QCommandLineParser parser;
    parser.setApplicationDescription("PDF Extractor with AI - GUI and CLI");
    parser.addHelpOption();
    parser.addVersionOption();

    parser.addPositionalArgument("pdf", "PDF file to extract text from");
    parser.addPositionalArgument("output", "Output text file");

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

    if (parser.positionalArguments().size() >= 2 && !parser.isSet(guiOption)) {
        window.processCommandLine(parser);
    }

    window.show();

    return app.exec();
}