#include "queryrunner.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QRegularExpression>
#include <QDebug>
#include <QFile>
#include <QTextStream>
#include <QDateTime>

QueryRunner::QueryRunner(QObject *parent)
    : QObject(parent)
    , m_currentStage(Idle)
    , m_currentInputType(PastedText)
    , m_summaryQuery(new SummaryQuery(this))
    , m_keywordsQuery(new KeywordsQuery(this))
    , m_refineQuery(new RefineKeywordsQuery(this))
    , m_refinedKeywordsQuery(new KeywordsWithRefinementQuery(this))
    , m_pdfDocument(new QPdfDocument(this))
{
    // Connect query signals
    connect(m_summaryQuery, &PromptQuery::resultReady,
            this, &QueryRunner::handleSummaryResult);
    connect(m_summaryQuery, &PromptQuery::errorOccurred,
            this, &QueryRunner::errorOccurred);
    connect(m_summaryQuery, &PromptQuery::progressUpdate,
            this, &QueryRunner::progressMessage);

    connect(m_keywordsQuery, &PromptQuery::resultReady,
            this, &QueryRunner::handleKeywordsResult);
    connect(m_keywordsQuery, &PromptQuery::errorOccurred,
            this, &QueryRunner::errorOccurred);
    connect(m_keywordsQuery, &PromptQuery::progressUpdate,
            this, &QueryRunner::progressMessage);

    connect(m_refineQuery, &PromptQuery::resultReady,
            this, &QueryRunner::handleRefinementResult);
    connect(m_refineQuery, &PromptQuery::errorOccurred,
            this, &QueryRunner::errorOccurred);
    connect(m_refineQuery, &PromptQuery::progressUpdate,
            this, &QueryRunner::progressMessage);

    connect(m_refinedKeywordsQuery, &PromptQuery::resultReady,
            this, &QueryRunner::handleRefinedKeywordsResult);
    connect(m_refinedKeywordsQuery, &PromptQuery::errorOccurred,
            this, &QueryRunner::errorOccurred);
    connect(m_refinedKeywordsQuery, &PromptQuery::progressUpdate,
            this, &QueryRunner::progressMessage);

    // Load settings on creation
    loadSettingsFromDatabase();
}

QueryRunner::~QueryRunner() {
    // Cleanup handled by QObject parent-child relationships
}

void QueryRunner::processPDF(const QString& filePath) {
    if (m_currentStage != Idle) {
        emit errorOccurred("Processing already in progress");
        return;
    }

    m_currentStage = ExtractingText;
    m_currentInputType = PDFFile;
    emit stageChanged(m_currentStage);
    emit progressMessage("Opening PDF file...");

    QString extractedText = extractTextFromPDF(filePath);

    if (extractedText.isEmpty()) {
        m_currentStage = Idle;
        emit stageChanged(m_currentStage);
        emit errorOccurred("Failed to extract text from PDF");
        return;
    }

    m_extractedText = extractedText;
    emit textExtracted(m_extractedText);

    startPipeline(extractedText, PDFFile);
}

void QueryRunner::processText(const QString& text) {
    if (m_currentStage != Idle) {
        emit errorOccurred("Processing already in progress");
        return;
    }

    if (text.isEmpty()) {
        emit errorOccurred("No text provided");
        return;
    }

    m_currentStage = ExtractingText;
    m_currentInputType = PastedText;
    emit stageChanged(m_currentStage);
    emit progressMessage("Processing pasted text...");

    m_extractedText = text;
    emit textExtracted(m_extractedText);

    startPipeline(text, PastedText);
}

QString QueryRunner::extractTextFromPDF(const QString& filePath) {
    QPdfDocument::Error error = m_pdfDocument->load(filePath);

    if (error != QPdfDocument::Error::None) {
        emit errorOccurred("Failed to load PDF: " + QString::number(static_cast<int>(error)));
        return QString();
    }

    QString extractedText;
    int pageCount = m_pdfDocument->pageCount();

    for (int i = 0; i < pageCount; ++i) {
        QString pageText = m_pdfDocument->getAllText(i).text();
        extractedText += pageText + "\n";

        // Update progress
        int progress = ((i + 1) * 100) / pageCount;
        emit progressMessage(QString("Extracting page %1 of %2 (%3%)...")
                           .arg(i + 1).arg(pageCount).arg(progress));
    }

    return extractedText;
}

QString QueryRunner::cleanupText(const QString& text, InputType type) {
    QString cleaned = text;

    // Always remove copyright notices for LLM processing
    cleaned = removeCopyrightNotices(cleaned);

    // Remove excessive whitespace
    cleaned.replace(QRegularExpression("\\n{3,}"), "\n\n");
    cleaned.replace(QRegularExpression("[ \\t]+"), " ");

    // Handle special characters based on input type
    if (type == PastedText) {
        // Pasted text might have formatting artifacts
        // Remove zero-width characters
        cleaned.remove(QChar(0x200B)); // Zero-width space
        cleaned.remove(QChar(0xFEFF)); // Zero-width no-break space

        // Normalize quotes
        cleaned.replace(QRegularExpression("[""„]"), "\"");
        cleaned.replace(QRegularExpression("['']"), "'");
    }

    // Trim leading/trailing whitespace
    cleaned = cleaned.trimmed();

    // Ensure text isn't too long for the model
    if (cleaned.length() > m_settings.textTruncationLimit) {
        emit progressMessage(QString("Text truncated to %1 characters").arg(m_settings.textTruncationLimit));
        cleaned = cleaned.left(m_settings.textTruncationLimit);
    }

    return cleaned;
}

QString QueryRunner::removeCopyrightNotices(const QString& text) {
    QString result = text;

    // Remove copyright lines
    result.remove(QRegularExpression("Copyright.*\\n", QRegularExpression::CaseInsensitiveOption));
    result.remove(QRegularExpression("©.*\\n"));

    // Remove All Rights Reserved variations
    result.remove(QRegularExpression("All [Rr]ights [Rr]eserved.*\\n"));

    // Remove license headers (common patterns)
    result.remove(QRegularExpression("Licensed under.*\\n"));
    result.remove(QRegularExpression("This .* is licensed.*\\n"));

    return result;
}

void QueryRunner::startPipeline(const QString& text, InputType type) {
    // Clear the lastrun.log file at the start of each run
    QFile logFile("lastrun.log");
    if (logFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        QTextStream stream(&logFile);
        stream << "=== PDF EXTRACTOR RUN LOG ===" << Qt::endl;
        stream << "Started: " << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << Qt::endl;
        stream << "Input Type: " << (type == PDFFile ? "PDF File" : "Pasted Text") << Qt::endl;
        stream << Qt::endl;
        logFile.close();
    }

    // Clean up the text
    m_cleanedText = cleanupText(text, type);

    if (m_cleanedText.isEmpty()) {
        m_currentStage = Idle;
        emit stageChanged(m_currentStage);
        emit errorOccurred("No text remaining after cleanup");
        return;
    }

    // Start with summary extraction
    runSummaryExtraction();
}

void QueryRunner::runSummaryExtraction() {
    m_currentStage = GeneratingSummary;
    emit stageChanged(m_currentStage);
    emit progressMessage("=== STAGE 1: Running Summary Extraction ===");

    m_summaryQuery->setConnectionSettings(m_settings.url, m_settings.modelName);
    m_summaryQuery->setPromptSettings(m_settings.summaryTemp,
                                      m_settings.summaryContext,
                                      m_settings.summaryTimeout);
    m_summaryQuery->setPreprompt(m_settings.summaryPreprompt);
    m_summaryQuery->setPrompt(m_settings.summaryPrompt);

    m_summaryQuery->execute(m_cleanedText);
}

void QueryRunner::runKeywordExtraction() {
    m_currentStage = ExtractingKeywords;
    emit stageChanged(m_currentStage);
    emit progressMessage("=== STAGE 2: Running Keyword Extraction ===");

    m_keywordsQuery->setConnectionSettings(m_settings.url, m_settings.modelName);
    m_keywordsQuery->setPromptSettings(m_settings.keywordTemp,
                                       m_settings.keywordContext,
                                       m_settings.keywordTimeout);
    m_keywordsQuery->setPreprompt(m_settings.keywordPreprompt);
    m_keywordsQuery->setPrompt(m_settings.keywordPrompt);

    m_keywordsQuery->execute(m_cleanedText);
}

void QueryRunner::runPromptRefinement() {
    m_currentStage = RefiningPrompt;
    emit stageChanged(m_currentStage);
    emit progressMessage("=== STAGE 3: Running Prompt Refinement ===");

    m_refineQuery->setConnectionSettings(m_settings.url, m_settings.modelName);
    m_refineQuery->setPromptSettings(m_settings.refinementTemp,
                                     m_settings.refinementContext,
                                     m_settings.refinementTimeout);
    m_refineQuery->setPreprompt(m_settings.keywordRefinementPreprompt);
    m_refineQuery->setPrompt(m_settings.prepromptRefinementPrompt);
    m_refineQuery->setOriginalKeywords(m_originalKeywords);
    m_refineQuery->setOriginalPrompt(m_settings.keywordPrompt);

    m_refineQuery->execute(m_cleanedText);
}

void QueryRunner::runRefinedKeywordExtraction() {
    m_currentStage = ExtractingRefinedKeywords;
    emit stageChanged(m_currentStage);
    emit progressMessage("=== STAGE 4: Running Refined Keyword Extraction ===");

    if (m_suggestedPrompt.isEmpty()) {
        emit progressMessage("ERROR: Suggested prompt is empty! Using original keyword prompt.");
        m_suggestedPrompt = m_settings.keywordPrompt;
    }

    emit progressMessage(QString("Using refined prompt (first 200 chars): %1").arg(m_suggestedPrompt.left(200)));

    m_refinedKeywordsQuery->setConnectionSettings(m_settings.url, m_settings.modelName);
    m_refinedKeywordsQuery->setPromptSettings(m_settings.keywordTemp,
                                              m_settings.keywordContext,
                                              m_settings.keywordTimeout);
    m_refinedKeywordsQuery->setPreprompt(m_settings.keywordPreprompt);
    m_refinedKeywordsQuery->setRefinedPrompt(m_suggestedPrompt);

    emit progressMessage("About to execute refined keywords query...");
    m_refinedKeywordsQuery->execute(m_cleanedText);
    emit progressMessage("Refined keywords query execute() called");
}

void QueryRunner::handleSummaryResult(const QString& result) {
    m_summary = result;
    emit summaryGenerated(m_summary);

    // If summary failed or returned empty, stop processing
    if (m_summary.isEmpty() ||
        m_summary.compare("Not Evaluated", Qt::CaseInsensitive) == 0) {
        emit progressMessage("Summary not successful - ending process");
        m_currentStage = Complete;
        emit stageChanged(m_currentStage);
        emit processingComplete();
        emit progressMessage("Processing ended due to summary failure");
        m_currentStage = Idle;
        return;
    }

    advanceToNextStage();
}

void QueryRunner::handleKeywordsResult(const QString& result) {
    m_originalKeywords = result;
    emit keywordsExtracted(m_originalKeywords);
    advanceToNextStage();
}

void QueryRunner::handleRefinementResult(const QString& result) {
    m_suggestedPrompt = result;
    emit progressMessage(QString("Refinement result (first 100 chars): %1").arg(result.left(100)));
    emit promptRefined(m_suggestedPrompt);

    // If refinement failed or returned empty, we're done
    if (m_suggestedPrompt.isEmpty() ||
        m_suggestedPrompt.compare("Not Evaluated", Qt::CaseInsensitive) == 0) {
        emit progressMessage("Refinement not successful - completing process");
        m_currentStage = Complete;
        emit stageChanged(m_currentStage);
        emit processingComplete();
        emit progressMessage("All processing complete");
        m_currentStage = Idle;
        return;
    }

    emit progressMessage(QString("Current stage before advance: %1").arg(m_currentStage));
    advanceToNextStage();
    emit progressMessage(QString("Current stage after advance: %1").arg(m_currentStage));
}

void QueryRunner::handleRefinedKeywordsResult(const QString& result) {
    m_refinedKeywords = result;
    emit progressMessage(QString("Refined keywords result (first 100 chars): %1").arg(result.left(100)));
    emit refinedKeywordsExtracted(m_refinedKeywords);
    advanceToNextStage();
}

void QueryRunner::advanceToNextStage() {
    switch (m_currentStage) {
        case GeneratingSummary:
            runKeywordExtraction();
            break;

        case ExtractingKeywords:
            runPromptRefinement();
            break;

        case RefiningPrompt:
            runRefinedKeywordExtraction();
            break;

        case ExtractingRefinedKeywords:
            m_currentStage = Complete;
            emit stageChanged(m_currentStage);
            emit processingComplete();
            emit progressMessage("All processing complete");

            // Reset to idle after completion
            m_currentStage = Idle;
            break;

        default:
            break;
    }
}

void QueryRunner::loadSettingsFromDatabase() {
    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery query(db);

    if (!query.exec("SELECT * FROM settings LIMIT 1") || !query.next()) {
        emit errorOccurred("Failed to load settings from database");
        return;
    }

    // Connection settings
    m_settings.url = query.value("url").toString();
    m_settings.modelName = query.value("model_name").toString();
    m_settings.overallTimeout = query.value("overall_timeout").toString().toInt();
    // Text truncation limit - use default if not in database (for backwards compatibility)
    m_settings.textTruncationLimit = query.value("text_truncation_limit").isNull()
        ? 100000
        : query.value("text_truncation_limit").toString().toInt();

    // Summary settings
    m_settings.summaryTemp = query.value("summary_temperature").toString().toDouble();
    m_settings.summaryContext = query.value("summary_context_length").toString().toInt();
    m_settings.summaryTimeout = query.value("summary_timeout").toString().toInt();
    m_settings.summaryPreprompt = query.value("summary_preprompt").toString();
    m_settings.summaryPrompt = query.value("summary_prompt").toString();

    // Keywords settings
    m_settings.keywordTemp = query.value("keyword_temperature").toString().toDouble();
    m_settings.keywordContext = query.value("keyword_context_length").toString().toInt();
    m_settings.keywordTimeout = query.value("keyword_timeout").toString().toInt();
    m_settings.keywordPreprompt = query.value("keyword_preprompt").toString();
    m_settings.keywordPrompt = query.value("keyword_prompt").toString();

    // Refinement settings
    m_settings.refinementTemp = query.value("refinement_temperature").toString().toDouble();
    m_settings.refinementContext = query.value("refinement_context_length").toString().toInt();
    m_settings.refinementTimeout = query.value("refinement_timeout").toString().toInt();
    m_settings.keywordRefinementPreprompt = query.value("keyword_refinement_preprompt").toString();
    m_settings.prepromptRefinementPrompt = query.value("preprompt_refinement_prompt").toString();

    emit progressMessage("Settings loaded from database");
}

void QueryRunner::setManualSettings(const QVariantMap& settings) {
    // Allow manual override of settings for testing
    if (settings.contains("url"))
        m_settings.url = settings["url"].toString();
    if (settings.contains("modelName"))
        m_settings.modelName = settings["modelName"].toString();
    // ... etc for other settings as needed
}