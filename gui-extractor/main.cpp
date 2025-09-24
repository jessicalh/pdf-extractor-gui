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
#include <QFrame>
#include <QDialog>
#include <QDialogButtonBox>
#include <QMessageBox>
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
#include <QDoubleSpinBox>
#include <QSplitter>
#include <QScrollArea>
#include <QFormLayout>
#include <QFont>
#include <QFontDatabase>
#include <QScreen>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlRecord>
#include <QDir>
#include <QClipboard>
#include <QStyle>
#include <QListWidget>
#include <QListWidgetItem>
#include <QSet>
#include <QMap>
#include <QStandardPaths>
#include <iostream>
#include <QLoggingCategory>
#ifdef Q_OS_WIN
#include <windows.h>
#include <dbghelp.h>
#endif
#include "queryrunner.h"

// Default prompts for keyword refinement (new fields)
const QString DEFAULT_KEYWORD_PREPROMPT = "You are an expert scientific information specialist and keyword extraction researcher. Your role is to identify and extract precise, domain-specific keywords from academic and scientific texts. You have extensive knowledge of scientific nomenclature, research methodologies, and technical terminology across multiple disciplines. You are systematic, thorough, and precise in identifying the most relevant and specific terms that characterize the research.\n\nConstraints:\n- Extract only the most specific and relevant terms\n- List each keyword only once (no duplicates)\n- Never create permutations or variations of existing words\n- Use standard scientific nomenclature\n- Avoid generic or overly broad terms\n- Focus on unique identifiers of the research\n- Return ONLY a comma-delimited list, nothing else\n- If no keywords are found, return nothing\n- Do not include explanations or additional text\n- Ensure the keyword list is sensible and relevant";

const QString DEFAULT_KEYWORD_REFINEMENT_PREPROMPT = "You are an expert scientific information specialist and editorial assistant specializing in keyword optimization for academic research. Your role is to refine and improve keyword lists to ensure they accurately represent research content while maintaining consistency and precision. You help researchers create coherent keyword sets that improve discoverability and accurately categorize their work.\n\nConstraints:\n- Maintain all original specific terms that are accurate\n- Standardize terminology to accepted scientific conventions\n- Ensure keywords are neither too broad nor too narrow\n- Preserve domain-specific technical terms";

const QString DEFAULT_PREPROMPT_REFINEMENT_PROMPT = "Based on the current paper's content and the existing keyword extraction prompt, create an improved and effective keyword extraction prompt that:\n1. Incorporates relevant domain-specific terms from this paper\n2. Maintains ALL the original categorical requirements (organism names, chemicals, proteins, drugs, statistical tests, environments, reactions, algorithms)\n3. Retains the exact sentence structure of the original prompt\n4. Enhances specificity by adding relevant examples from the current text\n5. Preserves the comma-delimited output format\n6. Do not worry about sentence length - include all necessary categories\n\nProvide only the improved prompt text without explanation. If unable to evaluate or improve, return 'Not Evaluated'.\n\nOriginal Prompt:\n{original_prompt}\n\nCurrent Paper Keywords:\n{keywords}\n\nText:\n{text}\n\nImproved Prompt:";

// Keyword Selection Dialog
class KeywordSelectionDialog : public QDialog
{
    Q_OBJECT
public:
    KeywordSelectionDialog(const QStringList &keywords, QWidget *parent = nullptr) : QDialog(parent) {
        setWindowTitle("Select Keywords");
        setModal(true);
        resize(400, 500);

        auto *layout = new QVBoxLayout(this);
        layout->setContentsMargins(10, 10, 10, 10);

        // Create list widget with checkboxes
        m_listWidget = new QListWidget();

        // Add keywords as checkable items
        for (const QString &keyword : keywords) {
            auto *item = new QListWidgetItem(keyword);
            item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
            item->setCheckState(Qt::Checked);  // Initially all checked
            m_listWidget->addItem(item);
        }

        layout->addWidget(m_listWidget);

        // Create buttons
        auto *buttonLayout = new QHBoxLayout();
        buttonLayout->addStretch();

        auto *copyButton = new QPushButton("Copy");
        auto *cancelButton = new QPushButton("Cancel");

        // Match button sizes to app style
        copyButton->setFixedWidth(75);
        cancelButton->setFixedWidth(75);

        connect(copyButton, &QPushButton::clicked, this, &KeywordSelectionDialog::accept);
        connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);

        buttonLayout->addWidget(copyButton);
        buttonLayout->addWidget(cancelButton);

        layout->addLayout(buttonLayout);
    }

    QStringList getSelectedKeywords() const {
        QStringList selected;
        for (int i = 0; i < m_listWidget->count(); ++i) {
            QListWidgetItem *item = m_listWidget->item(i);
            if (item->checkState() == Qt::Checked) {
                selected.append(item->text());
            }
        }
        return selected;
    }

private:
    QListWidget *m_listWidget;
};

// Settings Dialog with SQLite-backed fields
class SettingsDialog : public QDialog
{
    Q_OBJECT
public:
    SettingsDialog(QWidget *parent = nullptr) : QDialog(parent) {
        setWindowTitle("Settings - Configuration");
        setModal(true);
        resize(1000, 700);

        auto *layout = new QVBoxLayout(this);
        layout->setContentsMargins(10, 10, 10, 10);

        // Create main tab widget
        auto *tabWidget = new QTabWidget();

        // === CONNECTION TAB ===
        auto *connectionTab = createConnectionTab();
        tabWidget->addTab(connectionTab, "🌐 Connection");

        // === SUMMARY TAB ===
        auto *summaryTab = createSummaryTab();
        tabWidget->addTab(summaryTab, "📝 Summary");

        // === KEYWORDS TAB ===
        auto *keywordsTab = createKeywordsTab();
        tabWidget->addTab(keywordsTab, "🔑 Keywords");

        // === PROMPT REFINEMENT TAB ===
        auto *refinementTab = createRefinementTab();
        tabWidget->addTab(refinementTab, "✨ Prompt Refinement");

        layout->addWidget(tabWidget);

        // Dialog buttons with proper alignment
        auto *buttonLayout = new QHBoxLayout();
        buttonLayout->setContentsMargins(0, 10, 0, 0);

        // Restore Defaults on the left
        auto *restoreButton = new QPushButton("Restore Defaults");
        connect(restoreButton, &QPushButton::clicked, this, &SettingsDialog::restoreDefaults);
        buttonLayout->addWidget(restoreButton);

        buttonLayout->addStretch();

        // OK and Cancel on the right
        auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
        connect(buttonBox, &QDialogButtonBox::accepted, this, &SettingsDialog::saveSettings);
        connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
        buttonLayout->addWidget(buttonBox);

        layout->addLayout(buttonLayout);

        loadSettings();
    }

private:
    QWidget* createConnectionTab() {
        auto *widget = new QWidget();
        auto *layout = new QVBoxLayout(widget);

        auto *formLayout = new QFormLayout();
        formLayout->setSpacing(15);

        auto *headerLabel = new QLabel("<h3>LM Studio Connection Settings</h3>");
        formLayout->addRow(headerLabel);

        m_urlEdit = new QLineEdit();
        m_urlEdit->setPlaceholderText("http://127.0.0.1:8090/v1/chat/completions");
        formLayout->addRow("API URL:", m_urlEdit);

        m_modelNameEdit = new QLineEdit();
        m_modelNameEdit->setPlaceholderText("gpt-oss-120b");
        formLayout->addRow("Model Name:", m_modelNameEdit);

        m_overallTimeoutEdit = new QSpinBox();
        m_overallTimeoutEdit->setRange(10000, 600000);
        m_overallTimeoutEdit->setSingleStep(10000);
        m_overallTimeoutEdit->setSuffix(" ms");
        m_overallTimeoutEdit->setValue(600000);
        formLayout->addRow("Overall Timeout:", m_overallTimeoutEdit);

        layout->addLayout(formLayout);
        layout->addStretch();

        return widget;
    }

    QWidget* createSummaryTab() {
        auto *widget = new QWidget();
        auto *layout = new QVBoxLayout(widget);

        // Settings row at top
        auto *settingsLayout = new QHBoxLayout();
        settingsLayout->addWidget(new QLabel("Temperature:"));
        m_summaryTempEdit = new QDoubleSpinBox();
        m_summaryTempEdit->setRange(0.0, 2.0);
        m_summaryTempEdit->setSingleStep(0.1);
        m_summaryTempEdit->setDecimals(2);
        m_summaryTempEdit->setValue(0.8);
        settingsLayout->addWidget(m_summaryTempEdit);

        settingsLayout->addWidget(new QLabel("Context:"));
        m_summaryContextEdit = new QSpinBox();
        m_summaryContextEdit->setRange(1000, 100000);
        m_summaryContextEdit->setSingleStep(1000);
        m_summaryContextEdit->setSuffix(" tokens");
        m_summaryContextEdit->setValue(16000);
        settingsLayout->addWidget(m_summaryContextEdit);

        settingsLayout->addWidget(new QLabel("Timeout:"));
        m_summaryTimeoutEdit = new QSpinBox();
        m_summaryTimeoutEdit->setRange(10000, 600000);
        m_summaryTimeoutEdit->setSingleStep(10000);
        m_summaryTimeoutEdit->setSuffix(" ms");
        m_summaryTimeoutEdit->setValue(600000);
        settingsLayout->addWidget(m_summaryTimeoutEdit);

        settingsLayout->addStretch();
        layout->addLayout(settingsLayout);

        // Two equal-sized text areas
        auto *promptSplitter = new QSplitter(Qt::Vertical);

        auto *setupGroup = new QGroupBox("Prompt Setup");
        auto *setupLayout = new QVBoxLayout(setupGroup);
        m_summaryPrepromptEdit = new QTextEdit();
        m_summaryPrepromptEdit->setPlaceholderText("Pre-prompt to set context and instructions");
        setupLayout->addWidget(m_summaryPrepromptEdit);
        promptSplitter->addWidget(setupGroup);

        auto *promptGroup = new QGroupBox("Prompt");
        auto *promptLayout = new QVBoxLayout(promptGroup);
        m_summaryPromptEdit = new QTextEdit();
        m_summaryPromptEdit->setPlaceholderText("Main prompt template\nUse {text} as placeholder for the input text");
        promptLayout->addWidget(m_summaryPromptEdit);
        promptSplitter->addWidget(promptGroup);

        promptSplitter->setSizes({200, 200}); // Equal sizes
        layout->addWidget(promptSplitter);

        return widget;
    }

    QWidget* createKeywordsTab() {
        auto *widget = new QWidget();
        auto *layout = new QVBoxLayout(widget);

        // Settings row at top
        auto *settingsLayout = new QHBoxLayout();
        settingsLayout->addWidget(new QLabel("Temperature:"));
        m_keywordTempEdit = new QDoubleSpinBox();
        m_keywordTempEdit->setRange(0.0, 2.0);
        m_keywordTempEdit->setSingleStep(0.1);
        m_keywordTempEdit->setDecimals(2);
        m_keywordTempEdit->setValue(0.5);
        settingsLayout->addWidget(m_keywordTempEdit);

        settingsLayout->addWidget(new QLabel("Context:"));
        m_keywordContextEdit = new QSpinBox();
        m_keywordContextEdit->setRange(1000, 100000);
        m_keywordContextEdit->setSingleStep(1000);
        m_keywordContextEdit->setSuffix(" tokens");
        m_keywordContextEdit->setValue(4000);
        settingsLayout->addWidget(m_keywordContextEdit);

        settingsLayout->addWidget(new QLabel("Timeout:"));
        m_keywordTimeoutEdit = new QSpinBox();
        m_keywordTimeoutEdit->setRange(10000, 600000);
        m_keywordTimeoutEdit->setSingleStep(10000);
        m_keywordTimeoutEdit->setSuffix(" ms");
        m_keywordTimeoutEdit->setValue(600000);
        settingsLayout->addWidget(m_keywordTimeoutEdit);

        settingsLayout->addStretch();
        layout->addLayout(settingsLayout);

        // Two equal-sized text areas
        auto *promptSplitter = new QSplitter(Qt::Vertical);

        auto *setupGroup = new QGroupBox("Prompt Setup");
        auto *setupLayout = new QVBoxLayout(setupGroup);
        m_keywordPrepromptEdit = new QTextEdit();
        m_keywordPrepromptEdit->setPlaceholderText("Pre-prompt to set context and instructions");
        setupLayout->addWidget(m_keywordPrepromptEdit);
        promptSplitter->addWidget(setupGroup);

        auto *promptGroup = new QGroupBox("Prompt");
        auto *promptLayout = new QVBoxLayout(promptGroup);
        m_keywordPromptEdit = new QTextEdit();
        m_keywordPromptEdit->setPlaceholderText("Main prompt template\nUse {text} as placeholder for the input text");
        promptLayout->addWidget(m_keywordPromptEdit);
        promptSplitter->addWidget(promptGroup);

        promptSplitter->setSizes({200, 200}); // Equal sizes
        layout->addWidget(promptSplitter);

        return widget;
    }

    QWidget* createRefinementTab() {
        auto *widget = new QWidget();
        auto *layout = new QVBoxLayout(widget);

        // Settings row at top
        auto *settingsLayout = new QHBoxLayout();
        settingsLayout->addWidget(new QLabel("Temperature:"));
        m_refinementTempEdit = new QDoubleSpinBox();
        m_refinementTempEdit->setRange(0.0, 2.0);
        m_refinementTempEdit->setSingleStep(0.1);
        m_refinementTempEdit->setDecimals(2);
        m_refinementTempEdit->setValue(0.8);
        settingsLayout->addWidget(m_refinementTempEdit);

        settingsLayout->addWidget(new QLabel("Context:"));
        m_refinementContextEdit = new QSpinBox();
        m_refinementContextEdit->setRange(1000, 100000);
        m_refinementContextEdit->setSingleStep(1000);
        m_refinementContextEdit->setSuffix(" tokens");
        m_refinementContextEdit->setValue(16000);
        settingsLayout->addWidget(m_refinementContextEdit);

        settingsLayout->addWidget(new QLabel("Timeout:"));
        m_refinementTimeoutEdit = new QSpinBox();
        m_refinementTimeoutEdit->setRange(1000, 600000);
        m_refinementTimeoutEdit->setSingleStep(1000);
        m_refinementTimeoutEdit->setSuffix(" ms");
        m_refinementTimeoutEdit->setValue(600000);
        settingsLayout->addWidget(m_refinementTimeoutEdit);
        settingsLayout->addStretch();
        layout->addLayout(settingsLayout);

        // Two equal-sized text areas
        auto *promptSplitter = new QSplitter(Qt::Vertical);

        auto *setupGroup = new QGroupBox("Prompt Setup");
        auto *setupLayout = new QVBoxLayout(setupGroup);
        m_keywordRefinementPrepromptEdit = new QTextEdit();
        m_keywordRefinementPrepromptEdit->setPlaceholderText("Pre-prompt for keyword refinement");
        setupLayout->addWidget(m_keywordRefinementPrepromptEdit);
        promptSplitter->addWidget(setupGroup);

        auto *promptGroup = new QGroupBox("Prompt");
        auto *promptLayout = new QVBoxLayout(promptGroup);
        m_prepromptRefinementPromptEdit = new QTextEdit();
        m_prepromptRefinementPromptEdit->setPlaceholderText("Prompt template for refinement");
        promptLayout->addWidget(m_prepromptRefinementPromptEdit);
        promptSplitter->addWidget(promptGroup);

        promptSplitter->setSizes({200, 200}); // Equal sizes
        layout->addWidget(promptSplitter);

        return widget;
    }

public:

    void loadSettings() {
        QSqlDatabase db = QSqlDatabase::database();
        QSqlQuery query(db);

        if (query.exec("SELECT * FROM settings LIMIT 1") && query.next()) {
            // Connection settings
            m_urlEdit->setText(query.value("url").toString());
            m_modelNameEdit->setText(query.value("model_name").toString());
            m_overallTimeoutEdit->setValue(query.value("overall_timeout").toString().toInt());

            // Summary settings
            m_summaryTempEdit->setValue(query.value("summary_temperature").toString().toDouble());
            m_summaryContextEdit->setValue(query.value("summary_context_length").toString().toInt());
            m_summaryTimeoutEdit->setValue(query.value("summary_timeout").toString().toInt());
            m_summaryPrepromptEdit->setPlainText(query.value("summary_preprompt").toString());
            m_summaryPromptEdit->setPlainText(query.value("summary_prompt").toString());

            // Keyword settings
            m_keywordTempEdit->setValue(query.value("keyword_temperature").toString().toDouble());
            m_keywordContextEdit->setValue(query.value("keyword_context_length").toString().toInt());
            m_keywordTimeoutEdit->setValue(query.value("keyword_timeout").toString().toInt());
            m_keywordPrepromptEdit->setPlainText(query.value("keyword_preprompt").toString());
            m_keywordPromptEdit->setPlainText(query.value("keyword_prompt").toString());

            // Refinement settings
            m_refinementTempEdit->setValue(query.value("refinement_temperature").toString().toDouble());
            m_refinementContextEdit->setValue(query.value("refinement_context_length").toString().toInt());
            m_refinementTimeoutEdit->setValue(query.value("refinement_timeout").toString().toInt());
            m_keywordRefinementPrepromptEdit->setPlainText(query.value("keyword_refinement_preprompt").toString());
            m_prepromptRefinementPromptEdit->setPlainText(query.value("preprompt_refinement_prompt").toString());
        }
    }

    void saveSettings() {
        QSqlDatabase db = QSqlDatabase::database();
        QSqlQuery query(db);

        query.prepare("UPDATE settings SET "
                     "url = :url, "
                     "model_name = :model_name, "
                     "overall_timeout = :overall_timeout, "
                     "summary_temperature = :summary_temperature, "
                     "summary_context_length = :summary_context_length, "
                     "summary_timeout = :summary_timeout, "
                     "summary_preprompt = :summary_preprompt, "
                     "summary_prompt = :summary_prompt, "
                     "keyword_temperature = :keyword_temperature, "
                     "keyword_context_length = :keyword_context_length, "
                     "keyword_timeout = :keyword_timeout, "
                     "keyword_preprompt = :keyword_preprompt, "
                     "keyword_prompt = :keyword_prompt, "
                     "refinement_temperature = :refinement_temperature, "
                     "refinement_context_length = :refinement_context_length, "
                     "refinement_timeout = :refinement_timeout, "
                     "keyword_refinement_preprompt = :keyword_refinement_preprompt, "
                     "preprompt_refinement_prompt = :preprompt_refinement_prompt");

        // Connection settings
        query.bindValue(":url", m_urlEdit->text());
        query.bindValue(":model_name", m_modelNameEdit->text());
        query.bindValue(":overall_timeout", QString::number(m_overallTimeoutEdit->value()));

        // Summary settings
        query.bindValue(":summary_temperature", QString::number(m_summaryTempEdit->value()));
        query.bindValue(":summary_context_length", QString::number(m_summaryContextEdit->value()));
        query.bindValue(":summary_timeout", QString::number(m_summaryTimeoutEdit->value()));
        query.bindValue(":summary_preprompt", m_summaryPrepromptEdit->toPlainText());
        query.bindValue(":summary_prompt", m_summaryPromptEdit->toPlainText());

        // Keyword settings
        query.bindValue(":keyword_temperature", QString::number(m_keywordTempEdit->value()));
        query.bindValue(":keyword_context_length", QString::number(m_keywordContextEdit->value()));
        query.bindValue(":keyword_timeout", QString::number(m_keywordTimeoutEdit->value()));
        query.bindValue(":keyword_preprompt", m_keywordPrepromptEdit->toPlainText());
        query.bindValue(":keyword_prompt", m_keywordPromptEdit->toPlainText());

        // Refinement settings
        query.bindValue(":refinement_temperature", QString::number(m_refinementTempEdit->value()));
        query.bindValue(":refinement_context_length", QString::number(m_refinementContextEdit->value()));
        query.bindValue(":refinement_timeout", QString::number(m_refinementTimeoutEdit->value()));
        query.bindValue(":keyword_refinement_preprompt", m_keywordRefinementPrepromptEdit->toPlainText());
        query.bindValue(":preprompt_refinement_prompt", m_prepromptRefinementPromptEdit->toPlainText());

        if (!query.exec()) {
            QMessageBox::critical(this, "Error", "Failed to save settings: " + query.lastError().text());
        } else {
            accept();
        }
    }

    void restoreDefaults() {
        // Connection defaults
        m_urlEdit->setText("http://127.0.0.1:8090/v1/chat/completions");
        m_modelNameEdit->setText("gpt-oss-120b");
        m_overallTimeoutEdit->setValue(600000);

        // Summary defaults (optimized for gpt-oss-120b)
        m_summaryTempEdit->setValue(0.7);  // Lower temperature for more deterministic summaries
        m_summaryContextEdit->setValue(16000);  // Increased context for better comprehension
        m_summaryTimeoutEdit->setValue(600000);
        m_summaryPrepromptEdit->setPlainText("You are a senior academic research assistant with expertise in scientific literature analysis. Your role is to provide comprehensive yet concise research overviews to principal investigators and research teams preparing literature reviews. You specialize in identifying key contributions, methodological approaches, and the significance of research findings within the broader scientific context.\n\nConstraints:\n- Focus on objective, factual content\n- Emphasize novel contributions and methodologies\n- Maintain academic tone and precision\n- Highlight connections to existing literature\n- If unable to adequately evaluate the text, return 'Not Evaluated'");
        m_summaryPromptEdit->setPlainText("Please provide a summary:\n"
                                         "1. Main research question or hypothesis\n"
                                         "2. Key findings (3-5 bullet points with specific results)\n"
                                         "3. Methodology and approach used\n"
                                         "4. Significance and contribution to the field\n"
                                         "5. Potential applications, implications, or future directions\n\n"
                                         "Be concise yet comprehensive. Focus on information valuable for literature review inclusion. Do not include a title or preamble in your response. If unable to evaluate based on the provided text, respond only with 'Not Evaluated'.\n\n"
                                         "Text:\n{text}");

        // Keyword defaults (optimized for gpt-oss-120b)
        m_keywordTempEdit->setValue(0.3);  // Very low temperature for precise extraction
        m_keywordContextEdit->setValue(16000);  // Standardized context length
        m_keywordTimeoutEdit->setValue(600000);
        m_keywordPrepromptEdit->setPlainText(DEFAULT_KEYWORD_PREPROMPT);
        m_keywordPromptEdit->setPlainText("Extract and return a comma-delimited list containing: "
                                         "organism names (species, genus), "
                                         "chemicals (including specific proteins, enzymes, drugs, compounds), "
                                         "statistical methods (test names, analysis techniques), "
                                         "environmental factors (conditions, locations, habitats), "
                                         "chemical reactions (reaction types, mechanisms), "
                                         "computational methods (algorithms, models, software tools), "
                                         "and research techniques (experimental methods, instruments).\n\n"
                                         "Rules:\n"
                                         "- Return ONLY a comma-delimited list, no other text\n"
                                         "- List each keyword only once (no duplicates)\n"
                                         "- Never create permutations or variations of words\n"
                                         "- Use the shortest complete scientific form\n"
                                         "- No explanations, titles, suffixes, or commentary\n"
                                         "- If no relevant keywords found, return 'Not Evaluated'\n"
                                         "- Ensure keywords are sensible and actually present in the text\n\n"
                                         "Text:\n{text}");

        // Refinement defaults (optimized for gpt-oss-120b)
        m_refinementTempEdit->setValue(0.8);  // Default temperature for refinement
        m_refinementContextEdit->setValue(16000);
        m_refinementTimeoutEdit->setValue(600000);
        m_keywordRefinementPrepromptEdit->setPlainText(DEFAULT_KEYWORD_REFINEMENT_PREPROMPT);
        m_prepromptRefinementPromptEdit->setPlainText(DEFAULT_PREPROMPT_REFINEMENT_PROMPT);
    }

private:
    // Connection tab widgets
    QLineEdit *m_urlEdit;
    QLineEdit *m_modelNameEdit;
    QSpinBox *m_overallTimeoutEdit;

    // Summary tab widgets
    QDoubleSpinBox *m_summaryTempEdit;
    QSpinBox *m_summaryContextEdit;
    QSpinBox *m_summaryTimeoutEdit;
    QTextEdit *m_summaryPrepromptEdit;
    QTextEdit *m_summaryPromptEdit;

    // Keywords tab widgets
    QDoubleSpinBox *m_keywordTempEdit;
    QSpinBox *m_keywordContextEdit;
    QSpinBox *m_keywordTimeoutEdit;
    QTextEdit *m_keywordPrepromptEdit;
    QTextEdit *m_keywordPromptEdit;

    // Refinement tab widgets
    QDoubleSpinBox *m_refinementTempEdit;
    QSpinBox *m_refinementContextEdit;
    QSpinBox *m_refinementTimeoutEdit;
    QTextEdit *m_keywordRefinementPrepromptEdit;
    QTextEdit *m_prepromptRefinementPromptEdit;
};

// Main PDF Extractor GUI Window
class PDFExtractorGUI : public QMainWindow
{
    Q_OBJECT

public:
    PDFExtractorGUI(QWidget *parent = nullptr)
        : QMainWindow(parent)
        , m_queryRunner(nullptr) {

        // Initialize database BEFORE creating QueryRunner
        initDatabase();

        // Now create QueryRunner after database is ready
        m_queryRunner = new QueryRunner(this);

        setWindowTitle("PDF Extractor GUI v3.0 - AI Analysis");
        resize(1200, 800);

        // Center window on screen
        if (auto *screen = QGuiApplication::primaryScreen()) {
            QRect screenGeometry = screen->availableGeometry();
            int x = (screenGeometry.width() - width()) / 2;
            int y = (screenGeometry.height() - height()) / 2;
            move(x, y);
        }

        setupUI();

        // QueryRunner now handles all PDF and network operations

        connectSignals();
    }

private:
    // UI Elements - Main
    QTabWidget *m_mainTabWidget;

    // UI Elements - Input
    QTabWidget *m_inputTabWidget;
    QLineEdit *m_filePathEdit;
    QPushButton *m_browseButton;
    QPushButton *m_pdfAnalyzeButton;

    QTextEdit *m_pasteTextEdit;
    QPushButton *m_textAnalyzeButton;

    QPushButton *m_settingsButton;
    QLabel *m_statusLabel;
    QLabel *m_spinnerLabel;
    QTimer *m_spinnerTimer;

    // UI Elements - Results
    QTabWidget *m_resultsTabWidget;
    QTextEdit *m_extractedTextEdit;
    QTextEdit *m_summaryTextEdit;
    QTextEdit *m_keywordsTextEdit;
    QTextEdit *m_promptSuggestionsEdit;
    QTextEdit *m_refinedKeywordsEdit;
    QPlainTextEdit *m_logTextEdit;

    // Copy buttons for output tabs
    QPushButton *m_copyExtractedButton;
    QPushButton *m_copySummaryButton;
    QPushButton *m_copyKeywordsButton;

    // Core objects
    QueryRunner *m_queryRunner;

private:
    void initDatabase() {
        // Always use the executable directory for the database
        QString appDir = QCoreApplication::applicationDirPath();
        QString dbPath = QDir(appDir).absoluteFilePath("settings.db");

        // Log the path we're using
        qDebug() << "Database path:" << dbPath;

        // Ensure the directory exists and is writable
        QFileInfo dbInfo(dbPath);
        QDir dbDir = dbInfo.dir();

        if (!dbDir.exists()) {
            qWarning() << "Database directory does not exist:" << dbDir.absolutePath();
            if (!dbDir.mkpath(".")) {
                qCritical() << "Failed to create database directory";
                // Fall back to user's data directory
                QString fallbackPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
                QDir fallbackDir(fallbackPath);
                if (!fallbackDir.exists()) {
                    fallbackDir.mkpath(".");
                }
                dbPath = fallbackDir.absoluteFilePath("settings.db");
                qWarning() << "Using fallback database path:" << dbPath;
            }
        }

        // Check if we can write to the location
        if (dbInfo.exists() && !dbInfo.isWritable()) {
            qWarning() << "Database file is not writable:" << dbPath;
            // Try fallback location
            QString fallbackPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
            QDir fallbackDir(fallbackPath);
            if (!fallbackDir.exists()) {
                fallbackDir.mkpath(".");
            }
            dbPath = fallbackDir.absoluteFilePath("settings.db");
            qWarning() << "Using fallback database path:" << dbPath;
        }

        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
        db.setDatabaseName(dbPath);

        if (!db.open()) {
            qCritical() << "Failed to open database:" << db.lastError().text();

            // Try to use in-memory database as last resort
            db.setDatabaseName(":memory:");
            if (!db.open()) {
                QMessageBox::critical(nullptr, "Database Error",
                                    "Failed to open database: " + db.lastError().text() +
                                    "\n\nThe application will continue with default settings.");
                return;
            } else {
                QMessageBox::warning(nullptr, "Database Warning",
                                   "Could not access settings database at:\n" + dbPath +
                                   "\n\nUsing temporary in-memory database. Settings will not be saved.");
            }
        }

        QSqlQuery query(db);

        // Create table if it doesn't exist - with per-prompt settings
        QString createTable = R"(
            CREATE TABLE IF NOT EXISTS settings (
                id INTEGER PRIMARY KEY,
                url TEXT,
                model_name TEXT,
                overall_timeout TEXT,

                summary_temperature TEXT,
                summary_context_length TEXT,
                summary_timeout TEXT,
                summary_preprompt TEXT,
                summary_prompt TEXT,

                keyword_temperature TEXT,
                keyword_context_length TEXT,
                keyword_timeout TEXT,
                keyword_preprompt TEXT,
                keyword_prompt TEXT,

                refinement_temperature TEXT,
                refinement_context_length TEXT,
                refinement_timeout TEXT,
                keyword_refinement_preprompt TEXT,
                preprompt_refinement_prompt TEXT
            )
        )";

        if (!query.exec(createTable)) {
            QMessageBox::critical(nullptr, "Database Error",
                                "Failed to create table: " + query.lastError().text());
            return;
        }

        // Check if we have any records
        if (!query.exec("SELECT COUNT(*) FROM settings") || !query.next()) {
            return;
        }

        int count = query.value(0).toInt();

        // If no records, insert default one
        if (count == 0) {
            QString insertDefaults = R"(
                INSERT INTO settings (
                    url, model_name, overall_timeout,
                    summary_temperature, summary_context_length, summary_timeout,
                    summary_preprompt, summary_prompt,
                    keyword_temperature, keyword_context_length, keyword_timeout,
                    keyword_preprompt, keyword_prompt,
                    refinement_temperature, refinement_context_length, refinement_timeout,
                    keyword_refinement_preprompt, preprompt_refinement_prompt
                ) VALUES (
                    :url, :model_name, :overall_timeout,
                    :summary_temperature, :summary_context_length, :summary_timeout,
                    :summary_preprompt, :summary_prompt,
                    :keyword_temperature, :keyword_context_length, :keyword_timeout,
                    :keyword_preprompt, :keyword_prompt,
                    :refinement_temperature, :refinement_context_length, :refinement_timeout,
                    :keyword_refinement_preprompt, :preprompt_refinement_prompt
                )
            )";

            query.prepare(insertDefaults);
            query.bindValue(":url", "http://127.0.0.1:8090/v1/chat/completions");
            query.bindValue(":model_name", "gpt-oss-120b");
            query.bindValue(":overall_timeout", "600000");

            // Summary settings (optimized for gpt-oss-120b)
            query.bindValue(":summary_temperature", "0.7");
            query.bindValue(":summary_context_length", "16000");
            query.bindValue(":summary_timeout", "600000");
            query.bindValue(":summary_preprompt", "You are a senior academic research assistant with expertise in scientific literature analysis. Your role is to provide comprehensive yet concise research overviews to principal investigators and research teams preparing literature reviews. You specialize in identifying key contributions, methodological approaches, and the significance of research findings within the broader scientific context.\n\nConstraints:\n- Focus on objective, factual content\n- Emphasize novel contributions and methodologies\n- Maintain academic tone and precision\n- Highlight connections to existing literature\n- If unable to adequately evaluate the text, return 'Not Evaluated'");
            query.bindValue(":summary_prompt", "Please provide a summary:\n"
                          "1. Main research question or hypothesis\n"
                          "2. Key findings (3-5 bullet points with specific results)\n"
                          "3. Methodology and approach used\n"
                          "4. Significance and contribution to the field\n"
                          "5. Potential applications, implications, or future directions\n\n"
                          "Be concise yet comprehensive. Focus on information valuable for literature review inclusion. Do not include a title or preamble in your response. If unable to evaluate based on the provided text, respond only with 'Not Evaluated'.\n\n"
                          "Text:\n{text}");

            // Keyword settings (optimized for gpt-oss-120b)
            query.bindValue(":keyword_temperature", "0.3");
            query.bindValue(":keyword_context_length", "16000");
            query.bindValue(":keyword_timeout", "600000");
            query.bindValue(":keyword_preprompt", DEFAULT_KEYWORD_PREPROMPT);
            query.bindValue(":keyword_prompt", "Extract and return a comma-delimited list containing: "
                          "organism names (species, genus), "
                          "chemicals (including specific proteins, enzymes, drugs, compounds), "
                          "statistical methods (test names, analysis techniques), "
                          "environmental factors (conditions, locations, habitats), "
                          "chemical reactions (reaction types, mechanisms), "
                          "computational methods (algorithms, models, software tools), "
                          "and research techniques (experimental methods, instruments).\n\n"
                          "Rules:\n"
                          "- Return ONLY a comma-delimited list, no other text\n"
                          "- List each keyword only once (no duplicates)\n"
                          "- Never create permutations or variations of words\n"
                          "- Use the shortest complete scientific form\n"
                          "- No explanations, titles, suffixes, or commentary\n"
                          "- If no relevant keywords found, return 'Not Evaluated'\n"
                          "- Ensure keywords are sensible and actually present in the text\n\n"
                          "Text:\n{text}");

            // Refinement settings (optimized for gpt-oss-120b)
            query.bindValue(":refinement_temperature", "0.8");
            query.bindValue(":refinement_context_length", "16000");
            query.bindValue(":refinement_timeout", "600000");
            query.bindValue(":keyword_refinement_preprompt", DEFAULT_KEYWORD_REFINEMENT_PREPROMPT);
            query.bindValue(":preprompt_refinement_prompt", DEFAULT_PREPROMPT_REFINEMENT_PROMPT);

            if (!query.exec()) {
                QMessageBox::warning(nullptr, "Database Warning",
                                   "Failed to insert default settings: " + query.lastError().text());
            }
        }
    }

    void setupUI() {
        auto *centralWidget = new QWidget();
        setCentralWidget(centralWidget);

        auto *mainLayout = new QVBoxLayout(centralWidget);
        mainLayout->setContentsMargins(5, 5, 5, 0);
        mainLayout->setSpacing(0);

        // Top toolbar with just settings button
        auto *toolbar = new QHBoxLayout();
        toolbar->setContentsMargins(5, 5, 5, 5);

        toolbar->addStretch();

        // Settings button on the right with gear icon only
        m_settingsButton = new QPushButton("⚙");
        m_settingsButton->setFixedSize(28, 28);
        m_settingsButton->setToolTip("Settings");
        m_settingsButton->setStyleSheet(
            "QPushButton { "
            "   font-size: 18px; "
            "   border: 1px solid #ccc; "
            "   border-radius: 3px; "
            "   background-color: #f8f8f8; "
            "}"
            "QPushButton:hover { "
            "   background-color: #e8e8e8; "
            "}"
        );
        toolbar->addWidget(m_settingsButton);

        mainLayout->addLayout(toolbar);

        // Create main tab widget that uses full window
        m_mainTabWidget = new QTabWidget();
        m_mainTabWidget->setStyleSheet("QTabWidget::pane { border: 1px solid #ccc; }");

        // === INPUT TAB ===
        auto *inputTab = new QWidget();
        auto *inputLayout = new QVBoxLayout(inputTab);

        m_inputTabWidget = new QTabWidget();

        // PDF Input Tab
        auto *pdfTab = new QWidget();
        auto *pdfLayout = new QVBoxLayout(pdfTab);

        auto *fileGroup = new QGroupBox("PDF File");
        auto *fileLayout = new QGridLayout(fileGroup);

        m_filePathEdit = new QLineEdit();
        m_filePathEdit->setPlaceholderText("Select a PDF file...");
        fileLayout->addWidget(m_filePathEdit, 0, 0);

        m_browseButton = new QPushButton("Browse...");
        fileLayout->addWidget(m_browseButton, 0, 1);

        // Page range and copyright controls removed as requested

        pdfLayout->addWidget(fileGroup);
        pdfLayout->addStretch();

        // Button layout for PDF tab
        auto *pdfButtonLayout = new QHBoxLayout();
        pdfButtonLayout->addStretch();
        m_pdfAnalyzeButton = new QPushButton("Analyze");
        // Use default button style, no custom stylesheet
        pdfButtonLayout->addWidget(m_pdfAnalyzeButton);
        pdfLayout->addLayout(pdfButtonLayout);

        m_inputTabWidget->addTab(pdfTab, "PDF File");

        // Text Input Tab
        auto *textTab = new QWidget();
        auto *textLayout = new QVBoxLayout(textTab);

        // Instruction label removed as requested

        m_pasteTextEdit = new QTextEdit();
        m_pasteTextEdit->setPlaceholderText("Paste your text here for analysis...");
        textLayout->addWidget(m_pasteTextEdit);

        // Button layout for Text tab
        auto *textButtonLayout = new QHBoxLayout();
        textButtonLayout->addStretch();
        m_textAnalyzeButton = new QPushButton("Analyze");
        // Use default button style, no custom stylesheet
        textButtonLayout->addWidget(m_textAnalyzeButton);
        textLayout->addLayout(textButtonLayout);

        m_inputTabWidget->addTab(textTab, "Paste Text");

        inputLayout->addWidget(m_inputTabWidget);
        m_mainTabWidget->addTab(inputTab, "📥 Input");

        // === OUTPUT/RESULTS TAB ===
        auto *outputTab = new QWidget();
        auto *outputLayout = new QVBoxLayout(outputTab);

        m_resultsTabWidget = new QTabWidget();

        // Extracted Text tab with copy button
        auto *extractedWidget = new QWidget();
        auto *extractedLayout = new QHBoxLayout(extractedWidget);
        extractedLayout->setContentsMargins(0, 0, 0, 0);
        extractedLayout->setSpacing(0);

        m_extractedTextEdit = new QTextEdit();
        m_extractedTextEdit->setReadOnly(true);
        m_extractedTextEdit->setStyleSheet("QTextEdit { font-family: 'Consolas', monospace; }");
        extractedLayout->addWidget(m_extractedTextEdit);

        // Vertical separator line
        auto *extractedSeparator = new QFrame();
        extractedSeparator->setFrameShape(QFrame::VLine);
        extractedSeparator->setFrameShadow(QFrame::Sunken);
        extractedLayout->addWidget(extractedSeparator);

        // Right column for copy button
        auto *extractedButtonColumn = new QWidget();
        extractedButtonColumn->setFixedWidth(40);
        auto *extractedButtonLayout = new QVBoxLayout(extractedButtonColumn);
        extractedButtonLayout->setContentsMargins(5, 5, 5, 5);
        extractedButtonLayout->setAlignment(Qt::AlignTop);

        m_copyExtractedButton = new QPushButton();
        m_copyExtractedButton->setIcon(style()->standardIcon(QStyle::SP_FileDialogDetailedView));
        m_copyExtractedButton->setToolTip("Copy to clipboard");
        m_copyExtractedButton->setFixedSize(30, 30);
        extractedButtonLayout->addWidget(m_copyExtractedButton);
        extractedButtonLayout->addStretch();

        extractedLayout->addWidget(extractedButtonColumn);
        m_resultsTabWidget->addTab(extractedWidget, "Extracted Text");

        // Summary Result tab with copy button
        auto *summaryWidget = new QWidget();
        auto *summaryLayout = new QHBoxLayout(summaryWidget);
        summaryLayout->setContentsMargins(0, 0, 0, 0);
        summaryLayout->setSpacing(0);

        m_summaryTextEdit = new QTextEdit();
        // Make editable so user can modify if needed
        m_summaryTextEdit->setStyleSheet("QTextEdit { font-family: 'Arial', sans-serif; font-size: 14px; line-height: 1.6; }");
        summaryLayout->addWidget(m_summaryTextEdit);

        // Vertical separator line
        auto *summarySeparator = new QFrame();
        summarySeparator->setFrameShape(QFrame::VLine);
        summarySeparator->setFrameShadow(QFrame::Sunken);
        summaryLayout->addWidget(summarySeparator);

        // Right column for copy button
        auto *summaryButtonColumn = new QWidget();
        summaryButtonColumn->setFixedWidth(40);
        auto *summaryButtonLayout = new QVBoxLayout(summaryButtonColumn);
        summaryButtonLayout->setContentsMargins(5, 5, 5, 5);
        summaryButtonLayout->setAlignment(Qt::AlignTop);

        m_copySummaryButton = new QPushButton();
        m_copySummaryButton->setIcon(style()->standardIcon(QStyle::SP_FileDialogDetailedView));
        m_copySummaryButton->setToolTip("Copy to clipboard");
        m_copySummaryButton->setFixedSize(30, 30);
        summaryButtonLayout->addWidget(m_copySummaryButton);
        summaryButtonLayout->addStretch();

        summaryLayout->addWidget(summaryButtonColumn);
        m_resultsTabWidget->addTab(summaryWidget, "Summary Result");

        // Keywords tab with three sections and copy button
        auto *keywordsWidget = new QWidget();
        auto *keywordsMainLayout = new QHBoxLayout(keywordsWidget);
        keywordsMainLayout->setContentsMargins(0, 0, 0, 0);
        keywordsMainLayout->setSpacing(0);

        // Left side with the keyword sections
        auto *keywordsContentWidget = new QWidget();
        auto *keywordsLayout = new QVBoxLayout(keywordsContentWidget);
        keywordsMainLayout->addWidget(keywordsContentWidget);

        auto *keywordsSplitter = new QSplitter(Qt::Vertical);

        // Original keywords
        auto *originalGroup = new QGroupBox("Original Keywords");
        auto *originalLayout = new QVBoxLayout(originalGroup);
        m_keywordsTextEdit = new QTextEdit();
        // Make editable so user can modify if needed
        m_keywordsTextEdit->setStyleSheet("QTextEdit { font-family: 'Arial', sans-serif; font-size: 13px; }");
        originalLayout->addWidget(m_keywordsTextEdit);
        keywordsSplitter->addWidget(originalGroup);

        // Suggested improvements
        auto *suggestionsGroup = new QGroupBox("Suggested Prompt Improvements");
        auto *suggestionsLayout = new QVBoxLayout(suggestionsGroup);
        m_promptSuggestionsEdit = new QTextEdit();
        // Make editable so user can modify if needed
        m_promptSuggestionsEdit->setStyleSheet("QTextEdit { font-family: 'Arial', sans-serif; font-size: 13px; }");
        suggestionsLayout->addWidget(m_promptSuggestionsEdit);
        keywordsSplitter->addWidget(suggestionsGroup);

        // Refined keywords
        auto *refinedGroup = new QGroupBox("Keywords with Suggestions");
        auto *refinedLayout = new QVBoxLayout(refinedGroup);
        m_refinedKeywordsEdit = new QTextEdit();
        // Make editable so user can modify if needed
        m_refinedKeywordsEdit->setStyleSheet("QTextEdit { font-family: 'Arial', sans-serif; font-size: 13px; }");
        refinedLayout->addWidget(m_refinedKeywordsEdit);
        keywordsSplitter->addWidget(refinedGroup);

        keywordsSplitter->setSizes({100, 100, 100}); // Equal sizes
        keywordsLayout->addWidget(keywordsSplitter);

        // Vertical separator line
        auto *keywordsSeparator = new QFrame();
        keywordsSeparator->setFrameShape(QFrame::VLine);
        keywordsSeparator->setFrameShadow(QFrame::Sunken);
        keywordsMainLayout->addWidget(keywordsSeparator);

        // Right column for copy button
        auto *keywordsButtonColumn = new QWidget();
        keywordsButtonColumn->setFixedWidth(40);
        auto *keywordsButtonLayout = new QVBoxLayout(keywordsButtonColumn);
        keywordsButtonLayout->setContentsMargins(5, 5, 5, 5);
        keywordsButtonLayout->setAlignment(Qt::AlignTop);

        m_copyKeywordsButton = new QPushButton();
        m_copyKeywordsButton->setIcon(style()->standardIcon(QStyle::SP_FileDialogDetailedView));
        m_copyKeywordsButton->setToolTip("Copy to clipboard");
        m_copyKeywordsButton->setFixedSize(30, 30);
        keywordsButtonLayout->addWidget(m_copyKeywordsButton);
        keywordsButtonLayout->addStretch();

        keywordsMainLayout->addWidget(keywordsButtonColumn);
        m_resultsTabWidget->addTab(keywordsWidget, "Keywords Result");

        m_logTextEdit = new QPlainTextEdit();
        m_logTextEdit->setReadOnly(true);
        m_logTextEdit->setMaximumBlockCount(1000);
        // Use default background color for log
        m_resultsTabWidget->addTab(m_logTextEdit, "Run Log");

        outputLayout->addWidget(m_resultsTabWidget);
        m_mainTabWidget->addTab(outputTab, "📤 Output");

        mainLayout->addWidget(m_mainTabWidget);

        // Create status bar at the bottom
        auto *statusBar = new QFrame();
        statusBar->setFrameStyle(QFrame::Box | QFrame::Sunken);
        statusBar->setMaximumHeight(30);
        auto *statusLayout = new QHBoxLayout(statusBar);
        statusLayout->setContentsMargins(5, 2, 5, 2);

        // Status label on the left
        m_statusLabel = new QLabel("Ready");
        m_statusLabel->setStyleSheet("QLabel { font-size: 13px; }");
        statusLayout->addWidget(m_statusLabel);

        statusLayout->addStretch();

        // Spinner on the right (hidden by default)
        m_spinnerLabel = new QLabel();
        m_spinnerLabel->setStyleSheet("QLabel { font-size: 16px; color: #2196F3; }");
        m_spinnerLabel->hide();
        statusLayout->addWidget(m_spinnerLabel);

        // Create simple text spinner timer
        m_spinnerTimer = new QTimer(this);
        m_spinnerTimer->setInterval(100);
        connect(m_spinnerTimer, &QTimer::timeout, this, [this]() {
            static int frame = 0;
            const QString frames[] = {"⠋", "⠙", "⠹", "⠸", "⠼", "⠴", "⠦", "⠧", "⠇", "⠏"};
            m_spinnerLabel->setText(frames[frame % 10]);
            frame++;
        });

        mainLayout->addWidget(statusBar);
    }

    // Helper function to strip AI artifact markers
    QString stripAIArtifacts(const QString& text) {
        QString cleaned = text;
        // Remove the specific artifact marker that can appear anywhere
        cleaned.remove("<|start|>final<|message|>");
        // Also remove any variations with whitespace
        cleaned.remove(QRegularExpression("<\\|start\\|>\\s*final\\s*<\\|message\\|>"));
        return cleaned.trimmed();
    }

    void connectSignals() {
        connect(m_browseButton, &QPushButton::clicked, this, &PDFExtractorGUI::browseForPDF);
        connect(m_pdfAnalyzeButton, &QPushButton::clicked, this, &PDFExtractorGUI::analyzePDF);
        connect(m_textAnalyzeButton, &QPushButton::clicked, this, &PDFExtractorGUI::analyzeText);

        // Connect QueryRunner signals to UI updates
        connect(m_queryRunner, &QueryRunner::stageChanged, this, &PDFExtractorGUI::handleStageChanged);
        connect(m_queryRunner, &QueryRunner::progressMessage, this, &PDFExtractorGUI::log);
        connect(m_queryRunner, &QueryRunner::errorOccurred, this, &PDFExtractorGUI::handleError);

        // Connect result signals
        connect(m_queryRunner, &QueryRunner::textExtracted, [this](const QString& text) {
            m_extractedTextEdit->setPlainText(text);
            m_resultsTabWidget->setCurrentWidget(m_extractedTextEdit);
        });

        connect(m_queryRunner, &QueryRunner::summaryGenerated, [this](const QString& summary) {
            QString cleanedSummary = stripAIArtifacts(summary);
            m_summaryTextEdit->setPlainText(cleanedSummary);
        });

        connect(m_queryRunner, &QueryRunner::keywordsExtracted, [this](const QString& keywords) {
            QString cleanedKeywords = stripAIArtifacts(keywords);
            m_keywordsTextEdit->setPlainText(cleanedKeywords);
        });

        connect(m_queryRunner, &QueryRunner::promptRefined, [this](const QString& prompt) {
            QString cleanedPrompt = stripAIArtifacts(prompt);
            m_promptSuggestionsEdit->setPlainText(cleanedPrompt);
        });

        connect(m_queryRunner, &QueryRunner::refinedKeywordsExtracted, [this](const QString& keywords) {
            QString cleanedKeywords = stripAIArtifacts(keywords);
            m_refinedKeywordsEdit->setPlainText(cleanedKeywords);
        });

        connect(m_queryRunner, &QueryRunner::processingComplete, [this]() {
            setUIEnabled(true);
            stopSpinner();
            updateStatus("Processing complete");
            m_resultsTabWidget->setCurrentIndex(1); // Show summary tab
        });
        connect(m_settingsButton, &QPushButton::clicked, this, &PDFExtractorGUI::openSettings);

        // Connect copy buttons
        connect(m_copyExtractedButton, &QPushButton::clicked, [this]() {
            QString text = m_extractedTextEdit->toPlainText();
            if (text.isEmpty()) {
                updateStatus("No extracted text to copy");
                return;
            }
            // Add "Extracted Text" header to clipboard text only
            QString clipboardText = "Extracted Text\n\n" + text;
            QClipboard *clipboard = QApplication::clipboard();
            clipboard->setText(clipboardText);
            updateStatus("Extracted text copied to clipboard");
        });

        connect(m_copySummaryButton, &QPushButton::clicked, [this]() {
            QString summary = m_summaryTextEdit->toPlainText();
            if (summary.isEmpty()) {
                updateStatus("No summary to copy");
                return;
            }
            // Add "Paper Summary" header to clipboard text only
            QString clipboardText = "Paper Summary\n\n" + summary;
            QClipboard *clipboard = QApplication::clipboard();
            clipboard->setText(clipboardText);
            updateStatus("Summary copied to clipboard");
        });

        connect(m_copyKeywordsButton, &QPushButton::clicked, [this]() {
            // Collect all unique keywords from both fields (case-insensitive comparison)
            // Use QMap to track lowercase keys with original case values preserved
            QMap<QString, QString> uniqueKeywords; // lowercase key -> original case value

            // Process original keywords
            QString originalText = m_keywordsTextEdit->toPlainText();
            if (!originalText.isEmpty()) {
                // Split by comma and clean each keyword
                QStringList originalList = originalText.split(',', Qt::SkipEmptyParts);
                for (QString keyword : originalList) {
                    keyword = keyword.trimmed();
                    // Remove any newlines within keywords
                    keyword.replace('\n', ' ');
                    keyword = keyword.simplified();
                    if (!keyword.isEmpty()) {
                        QString lowerKey = keyword.toLower();
                        // Only insert if not already present (preserves first occurrence)
                        if (!uniqueKeywords.contains(lowerKey)) {
                            uniqueKeywords.insert(lowerKey, keyword);
                        }
                    }
                }
            }

            // Process refined keywords (Keywords with Suggestions)
            QString refinedText = m_refinedKeywordsEdit->toPlainText();
            if (!refinedText.isEmpty()) {
                // Split by comma and clean each keyword
                QStringList refinedList = refinedText.split(',', Qt::SkipEmptyParts);
                for (QString keyword : refinedList) {
                    keyword = keyword.trimmed();
                    // Remove any newlines within keywords
                    keyword.replace('\n', ' ');
                    keyword = keyword.simplified();
                    if (!keyword.isEmpty()) {
                        QString lowerKey = keyword.toLower();
                        // Only insert if not already present (preserves first occurrence)
                        if (!uniqueKeywords.contains(lowerKey)) {
                            uniqueKeywords.insert(lowerKey, keyword);
                        }
                    }
                }
            }

            // Check if we have any keywords
            if (uniqueKeywords.isEmpty()) {
                updateStatus("No keywords to copy");
                return;
            }

            // Convert to sorted list for consistent display (using original case)
            QStringList keywordList = uniqueKeywords.values();
            keywordList.sort(Qt::CaseInsensitive);

            // Show selection dialog
            KeywordSelectionDialog dialog(keywordList, this);
            if (dialog.exec() == QDialog::Accepted) {
                QStringList selectedKeywords = dialog.getSelectedKeywords();
                if (!selectedKeywords.isEmpty()) {
                    // Join with newlines for Zotero
                    QString clipboardText = selectedKeywords.join('\n');
                    QClipboard *clipboard = QApplication::clipboard();
                    clipboard->setText(clipboardText);
                    updateStatus(QString("Copied %1 keywords to clipboard").arg(selectedKeywords.size()));
                } else {
                    updateStatus("No keywords selected");
                }
            }
        });
    }

private slots:
    void browseForPDF() {
        QString fileName = QFileDialog::getOpenFileName(this,
            "Select PDF File", "", "PDF Files (*.pdf);;All Files (*)");

        if (!fileName.isEmpty()) {
            m_filePathEdit->setText(fileName);

            // QueryRunner will handle PDF loading when Analyze is clicked
            log(QString("PDF selected: %1").arg(fileName));
        }
    }

    void analyzePDF() {
        QString pdfPath = m_filePathEdit->text();
        if (pdfPath.isEmpty()) {
            QMessageBox::warning(this, "Warning", "Please select a PDF file first.");
            return;
        }

        if (m_queryRunner->isProcessing()) {
            QMessageBox::warning(this, "Warning", "Processing already in progress.");
            return;
        }

        setUIEnabled(false);
        startSpinner();
        updateStatus("Starting PDF analysis...");
        m_mainTabWidget->setCurrentIndex(1);  // Switch to Output tab

        // Clear previous results
        clearResults();

        // Start processing with QueryRunner
        m_queryRunner->processPDF(pdfPath);
    }

    void analyzeText() {
        QString text = m_pasteTextEdit->toPlainText();
        if (text.isEmpty()) {
            QMessageBox::warning(this, "Warning", "Please paste some text first.");
            return;
        }

        if (m_queryRunner->isProcessing()) {
            QMessageBox::warning(this, "Warning", "Processing already in progress.");
            return;
        }

        setUIEnabled(false);
        startSpinner();
        updateStatus("Starting text analysis...");
        m_mainTabWidget->setCurrentIndex(1);  // Switch to Output tab

        // Clear previous results
        clearResults();

        // Start processing with QueryRunner
        m_queryRunner->processText(text);
    }

    void openSettings() {
        SettingsDialog dialog(this);
        if (dialog.exec() == QDialog::Accepted) {
            // Reload settings into QueryRunner
            m_queryRunner->loadSettingsFromDatabase();
            log("Settings updated and reloaded");
        }
    }

    void handleStageChanged(QueryRunner::ProcessingStage stage) {
        QString stageText;
        switch (stage) {
            case QueryRunner::ExtractingText:
                stageText = "Extracting text...";
                break;
            case QueryRunner::GeneratingSummary:
                stageText = "Generating summary...";
                break;
            case QueryRunner::ExtractingKeywords:
                stageText = "Extracting keywords...";
                break;
            case QueryRunner::RefiningPrompt:
                stageText = "Refining prompt...";
                break;
            case QueryRunner::ExtractingRefinedKeywords:
                stageText = "Extracting refined keywords...";
                break;
            case QueryRunner::Complete:
                stageText = "Complete";
                break;
            default:
                stageText = "Ready";
                break;
        }
        updateStatus(stageText);
    }

    void handleError(const QString& error) {
        log("ERROR: " + error);
        QMessageBox::critical(this, "Error", error);
        setUIEnabled(true);
        stopSpinner();
        updateStatus("Error occurred");
    }

    void clearResults() {
        m_extractedTextEdit->clear();
        m_summaryTextEdit->clear();
        m_keywordsTextEdit->clear();
        m_promptSuggestionsEdit->clear();
        m_refinedKeywordsEdit->clear();
        m_logTextEdit->clear();
    }

private:  // Methods
    void log(const QString &msg) {
        QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss");
        m_logTextEdit->appendPlainText(QString("[%1] %2").arg(timestamp).arg(msg));
    }

    void updateStatus(const QString &status) {
        m_statusLabel->setText(status);
    }

    void setUIEnabled(bool enabled) {
        m_pdfAnalyzeButton->setEnabled(enabled);
        m_textAnalyzeButton->setEnabled(enabled);
        m_browseButton->setEnabled(enabled);
        m_settingsButton->setEnabled(enabled);
    }

    void startSpinner() {
        m_spinnerLabel->setVisible(true);
        m_spinnerTimer->start();
    }

    void stopSpinner() {
        m_spinnerLabel->setVisible(false);
        m_spinnerTimer->stop();
    }

/* OLD METHODS - Replaced by QueryRunner
    bool extractPDFText(const QString &pdfPath) {
        logMessage("Starting PDF extraction...");

        if (m_pdfDocument->load(pdfPath) != QPdfDocument::Error::None) {
            logMessage("Failed to load PDF");
            return false;
        }

        int startPage = 0;
        int endPage = m_pdfDocument->pageCount() - 1;
        int pageCount = m_pdfDocument->pageCount();

        if (endPage >= pageCount) {
            endPage = pageCount - 1;
        }

        QString extractedText;

        for (int i = startPage; i <= endPage; ++i) {
            QString pageText = m_pdfDocument->getAllText(i).text();

            // Always strip copyright notices for LLM
            pageText = removeCopyrightNotices(pageText);

            pageText = sanitizeText(pageText);
            extractedText += pageText + "\n\n";

            logMessage(QString("Extracted page %1/%2").arg(i + 1).arg(pageCount));
        }

        m_extractedText = extractedText.trimmed();
        m_extractedTextEdit->setPlainText(m_extractedText);
        m_resultsTabWidget->setCurrentIndex(0);
        m_mainTabWidget->setCurrentIndex(1);  // Switch to Output tab

        logMessage(QString("Extraction complete: %1 characters").arg(m_extractedText.length()));
        return true;
    }

    void processWithAI() {
        if (m_extractedText.isEmpty()) {
            logMessage("No text to process");
            return;
        }

        // Get settings from database
        QSqlQuery query("SELECT * FROM settings LIMIT 1");
        if (!query.exec() || !query.next()) {
            logMessage("Failed to load settings from database");
            return;
        }

        QString url = query.value("url").toString();
        QString modelName = query.value("model_name").toString();

        // Generate summary with per-prompt settings
        double summaryTemp = query.value("summary_temperature").toString().toDouble();
        int summaryContext = query.value("summary_context_length").toString().toInt();
        int summaryTimeout = query.value("summary_timeout").toString().toInt();
        QString summaryPreprompt = query.value("summary_preprompt").toString();
        QString summaryPrompt = query.value("summary_prompt").toString();
        summaryPrompt.replace("{text}", m_extractedText);

        QString fullSummaryPrompt = summaryPreprompt + "\n\n" + summaryPrompt;
        QString summary = callLMStudio(url, modelName, fullSummaryPrompt, summaryTemp, summaryContext, summaryTimeout);

        if (!summary.isEmpty()) {
            m_summaryTextEdit->setPlainText(summary);
            logMessage("Summary generated");
        }

        // Generate keywords with per-prompt settings
        double keywordTemp = query.value("keyword_temperature").toString().toDouble();
        int keywordContext = query.value("keyword_context_length").toString().toInt();
        int keywordTimeout = query.value("keyword_timeout").toString().toInt();
        QString keywordPreprompt = query.value("keyword_preprompt").toString();
        QString keywordPrompt = query.value("keyword_prompt").toString();
        keywordPrompt.replace("{text}", m_extractedText);

        QString fullKeywordPrompt = keywordPreprompt + "\n\n" + keywordPrompt;
        QString keywords = callLMStudio(url, modelName, fullKeywordPrompt, keywordTemp, keywordContext, keywordTimeout);

        if (!keywords.isEmpty()) {
            m_keywordsTextEdit->setPlainText(keywords);
            logMessage("Keywords extracted");
        }
    }

    QString callLMStudio(const QString &url, const QString &model, const QString &prompt,
                        double temperature, int maxTokens, int timeout) {
        logMessage(QString("Calling LM Studio API at %1").arg(url));

        QJsonObject message;
        message["role"] = "user";
        message["content"] = prompt;

        QJsonArray messages;
        messages.append(message);

        QJsonObject requestData;
        requestData["model"] = model;
        requestData["messages"] = messages;
        requestData["temperature"] = temperature;
        requestData["max_tokens"] = maxTokens;

        QJsonDocument doc(requestData);
        QByteArray data = doc.toJson();

        QNetworkRequest request;
        request.setUrl(QUrl(url));
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

        QEventLoop loop;
        QTimer timer;
        timer.setSingleShot(true);
        timer.start(timeout); // Use provided timeout

        QNetworkReply *reply = m_networkManager->post(request, data);

        connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);

        loop.exec();

        QString result;

        if (reply->error() == QNetworkReply::NoError) {
            QByteArray response = reply->readAll();
            QJsonDocument responseDoc = QJsonDocument::fromJson(response);
            QJsonObject responseObj = responseDoc.object();

            if (responseObj.contains("choices")) {
                QJsonArray choices = responseObj["choices"].toArray();
                if (!choices.isEmpty()) {
                    QJsonObject firstChoice = choices[0].toObject();
                    QJsonObject message = firstChoice["message"].toObject();
                    result = message["content"].toString();
                }
            }

            logMessage("API call successful");
        } else {
            logMessage(QString("API call failed: %1").arg(reply->errorString()));
        }

        reply->deleteLater();
        return result;
    }

    QString removeCopyrightNotices(const QString &text) {
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
*/

private:
    // Member variables already declared above
};

#include "main.moc"

#ifdef Q_OS_WIN
// Windows exception handler
LONG WINAPI UnhandledExceptionHandler(EXCEPTION_POINTERS* pExceptionInfo)
{
    // Create crash log directory if it doesn't exist
    QString appDir = QCoreApplication::applicationDirPath();
    QString crashDir = appDir + "/logs";
    QDir().mkpath(crashDir);

    // Create a crash log file with timestamp
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd_HH-mm-ss");
    QString crashLogPath = crashDir + "/crash_" + timestamp + ".log";
    QFile crashLog(crashLogPath);

    if (crashLog.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream stream(&crashLog);
        stream << "=== CRASH REPORT ===" << Qt::endl;
        stream << "Time: " << QDateTime::currentDateTime().toString(Qt::ISODate) << Qt::endl;
        stream << "Exception Code: 0x" << QString::number(pExceptionInfo->ExceptionRecord->ExceptionCode, 16) << Qt::endl;

        // Common exception codes
        QString exceptionName;
        switch(pExceptionInfo->ExceptionRecord->ExceptionCode) {
            case EXCEPTION_ACCESS_VIOLATION:
                exceptionName = "Access Violation";
                break;
            case EXCEPTION_DATATYPE_MISALIGNMENT:
                exceptionName = "Datatype Misalignment";
                break;
            case EXCEPTION_FLT_DIVIDE_BY_ZERO:
            case EXCEPTION_INT_DIVIDE_BY_ZERO:
                exceptionName = "Divide by Zero";
                break;
            case EXCEPTION_STACK_OVERFLOW:
                exceptionName = "Stack Overflow";
                break;
            default:
                exceptionName = "Unknown Exception";
        }

        stream << "Exception Type: " << exceptionName << Qt::endl;
        stream << "Exception Address: 0x" << QString::number((quintptr)pExceptionInfo->ExceptionRecord->ExceptionAddress, 16) << Qt::endl;
        stream << Qt::endl;

        crashLog.close();
    }

    // Show error message to user
    MessageBoxW(NULL,
                L"PDF Extractor GUI has encountered a critical error and needs to close.\n\n"
                L"A crash log has been saved to the logs directory.\n"
                L"Please restart the application.",
                L"Application Error",
                MB_OK | MB_ICONERROR);

    return EXCEPTION_EXECUTE_HANDLER;
}
#endif

// Helper function to verify required resources
bool verifyResources()
{
    QString appDir = QCoreApplication::applicationDirPath();

    // Check if we can write to the application directory
    QFileInfo appDirInfo(appDir);
    if (!appDirInfo.isWritable()) {
        qWarning() << "Application directory is not writable:" << appDir;
        // Not fatal - we have fallback paths
    }

    // Log startup information
    qDebug() << "Application started from:" << appDir;
    qDebug() << "Current working directory:" << QDir::currentPath();
    qDebug() << "Qt version:" << qVersion();

    return true;
}

// Safe main function wrapper
int safeMain(int argc, char *argv[])
{
    // Set up Qt platform paths before QApplication
    QString appDir = QCoreApplication::applicationDirPath();

#ifdef Q_OS_WIN
    // Set DLL search path to application directory first
    SetDllDirectoryW(reinterpret_cast<const wchar_t*>(appDir.utf16()));

    // Also add to PATH for extra safety
    QString currentPath = QString::fromLocal8Bit(qgetenv("PATH"));
    QString newPath = appDir + ";" + currentPath;
    qputenv("PATH", newPath.toLocal8Bit());
#endif

    // Add application directory to library paths for Qt plugins
    QCoreApplication::addLibraryPath(appDir);
    QCoreApplication::addLibraryPath(appDir + "/platforms");
    QCoreApplication::addLibraryPath(appDir + "/styles");
    QCoreApplication::addLibraryPath(appDir + "/imageformats");
    QCoreApplication::addLibraryPath(appDir + "/sqldrivers");

    // High DPI support is automatic in Qt 6

    QApplication app(argc, argv);
    QApplication::setApplicationName("PDF Extractor GUI");
    QApplication::setApplicationVersion("3.0");
    QApplication::setOrganizationName("PDFExtractor");

    // Set up logging
    QLoggingCategory::setFilterRules("*.debug=true\n"
                                     "qt.network.ssl.warning=false");

    // Verify resources
    if (!verifyResources()) {
        QMessageBox::critical(nullptr, "Startup Error",
                            "Failed to verify required resources.\n"
                            "Please check the application installation.");
        return 1;
    }

    try {
        PDFExtractorGUI window;
        window.show();

        return app.exec();
    }
    catch (const std::exception& e) {
        QMessageBox::critical(nullptr, "Fatal Error",
                            QString("An unexpected error occurred:\n%1").arg(e.what()));
        return 1;
    }
    catch (...) {
        QMessageBox::critical(nullptr, "Fatal Error",
                            "An unknown fatal error occurred.");
        return 1;
    }
}

int main(int argc, char *argv[])
{
#ifdef Q_OS_WIN
    // Install Windows exception handler
    SetUnhandledExceptionFilter(UnhandledExceptionHandler);

    // Enable mini dumps for post-mortem debugging
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
#endif

    return safeMain(argc, argv);
}