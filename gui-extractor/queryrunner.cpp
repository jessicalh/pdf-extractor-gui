#include "queryrunner.h"
#include "safepdfloader.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QRegularExpression>
#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QDateTime>
#include <QCoreApplication>
#include <QDir>
#include <memory>
#include <exception>

QueryRunner::QueryRunner(QObject *parent)
    : QObject(parent)
    , m_currentStage(Idle)
    , m_currentInputType(PastedText)
    , m_summaryQuery(new SummaryQuery(this))
    , m_keywordsQuery(new KeywordsQuery(this))
    , m_refineQuery(new RefineKeywordsQuery(this))
    , m_refinedKeywordsQuery(new KeywordsWithRefinementQuery(this))
    , m_pdfDocument(new QPdfDocument(this))
    , m_singleStepMode(false)
{
    // Connect query signals
    connect(m_summaryQuery, &PromptQuery::resultReady,
            this, &QueryRunner::handleSummaryResult);
    connect(m_summaryQuery, &PromptQuery::errorOccurred,
            this, &QueryRunner::handleQueryError);
    connect(m_summaryQuery, &PromptQuery::progressUpdate,
            this, &QueryRunner::progressMessage);

    connect(m_keywordsQuery, &PromptQuery::resultReady,
            this, &QueryRunner::handleKeywordsResult);
    connect(m_keywordsQuery, &PromptQuery::errorOccurred,
            this, &QueryRunner::handleQueryError);
    connect(m_keywordsQuery, &PromptQuery::progressUpdate,
            this, &QueryRunner::progressMessage);

    connect(m_refineQuery, &PromptQuery::resultReady,
            this, &QueryRunner::handleRefinementResult);
    connect(m_refineQuery, &PromptQuery::errorOccurred,
            this, &QueryRunner::handleQueryError);
    connect(m_refineQuery, &PromptQuery::progressUpdate,
            this, &QueryRunner::progressMessage);

    connect(m_refinedKeywordsQuery, &PromptQuery::resultReady,
            this, &QueryRunner::handleRefinedKeywordsResult);
    connect(m_refinedKeywordsQuery, &PromptQuery::errorOccurred,
            this, &QueryRunner::handleQueryError);
    connect(m_refinedKeywordsQuery, &PromptQuery::progressUpdate,
            this, &QueryRunner::progressMessage);

    // Connect abort signal to all queries
    connect(this, &QueryRunner::abortRequested, m_summaryQuery, &PromptQuery::abort);
    connect(this, &QueryRunner::abortRequested, m_keywordsQuery, &PromptQuery::abort);
    connect(this, &QueryRunner::abortRequested, m_refineQuery, &PromptQuery::abort);
    connect(this, &QueryRunner::abortRequested, m_refinedKeywordsQuery, &PromptQuery::abort);

    // Load settings on creation
    loadSettingsFromDatabase();
}

QueryRunner::~QueryRunner() {
    // Ensure PDF document is closed before destruction
    if (m_pdfDocument) {
        m_pdfDocument->close();
    }
    // Other cleanup handled by QObject parent-child relationships
}

void QueryRunner::reset() {
    m_currentStage = Idle;
    emit stageChanged(m_currentStage);

    // Clear ALL persistent state from previous runs
    m_extractedText.clear();
    m_cleanedText.clear();
    m_summary.clear();
    m_originalKeywords.clear();
    m_suggestedPrompt.clear();
    m_refinedKeywords.clear();

    // CRITICAL: Also clear state in the reused query objects!
    m_keywordsQuery->setSummaryResult("");  // Clear old summary from keywords query
    m_refineQuery->setOriginalKeywords("");  // Clear old keywords from refine query
    m_refineQuery->setOriginalPrompt("");    // Clear old prompt from refine query
    m_refinedKeywordsQuery->setSummaryResult("");  // Clear old summary from refined keywords query

    emit progressMessage("Ready for new analysis");
}

void QueryRunner::abort() {
    QString stage = getStageString(m_currentStage);
    qDebug() << "QueryRunner::abort() called at stage:" << stage;

    // Write to abort log file for debugging hangs
    QFile abortLog("abort_debug.log");
    if (abortLog.open(QIODevice::WriteOnly | QIODevice::Append)) {
        QTextStream stream(&abortLog);
        stream << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz")
               << " - QueryRunner::abort() called at stage: " << stage << Qt::endl;
        abortLog.close();
    }

    emit progressMessage("Aborting current operation...");

    // Set flag to prevent further processing
    m_singleStepMode = false;

    qDebug() << "Emitting abortRequested signal...";
    emit abortRequested();  // Tell queries to stop

    qDebug() << "Calling reset()...";
    reset();

    qDebug() << "QueryRunner::abort() complete";
    if (abortLog.open(QIODevice::WriteOnly | QIODevice::Append)) {
        QTextStream stream(&abortLog);
        stream << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz")
               << " - QueryRunner::abort() complete" << Qt::endl;
        abortLog.close();
    }
}

void QueryRunner::processKeywordsOnly() {
    // Check prerequisites
    if (m_cleanedText.isEmpty()) {
        emit errorOccurred("No text available for keyword extraction. Please extract or paste text first.");
        return;
    }

    if (m_currentStage != Idle) {
        emit progressMessage("Note: Resetting from previous operation");
        reset();
    }

    // ALWAYS reload settings from database to get user's manual edits
    loadSettingsFromDatabase();

    emit progressMessage("=== RE-RUNNING KEYWORD EXTRACTION ===");
    emit progressMessage("Using keyword prompt from Settings");

    // Set single-step mode flag
    m_singleStepMode = true;

    // Run keyword extraction with fresh database settings
    runKeywordExtraction();
}

void QueryRunner::processKeywordsOnly(const QString& extractedText, const QString& summaryText) {
    // Reset state but preserve the provided text
    if (m_currentStage != Idle) {
        emit progressMessage("Note: Resetting from previous operation");
        reset();
    }

    // Set the text from UI (source of truth)
    m_cleanedText = extractedText;
    m_extractedText = extractedText;
    m_summary = summaryText;

    // Check prerequisites after setting text
    if (m_cleanedText.isEmpty()) {
        emit errorOccurred("No text available for keyword extraction. Please provide extracted text.");
        return;
    }

    // ALWAYS reload settings from database to get user's manual edits
    loadSettingsFromDatabase();

    emit progressMessage("=== RE-RUNNING KEYWORD EXTRACTION ===");
    emit progressMessage("Using text from UI display");
    emit progressMessage(QString("Text length: %1 characters").arg(m_cleanedText.length()));
    if (!m_summary.isEmpty()) {
        emit progressMessage(QString("Summary available: %1 characters").arg(m_summary.length()));
    }

    // Set single-step mode flag
    m_singleStepMode = true;

    // Run keyword extraction with fresh database settings and UI text
    runKeywordExtraction();
}

void QueryRunner::processPDF(const QString& filePath) {
    // SAFETY CHECKS FIRST - Apply to ALL PDFs, not just Zotero
    try {
        if (filePath.isEmpty()) {
            emit errorOccurred("Empty PDF path provided");
            return;
        }

        // Validate file exists and is readable
        QFileInfo fileInfo(filePath);
        if (!fileInfo.exists()) {
            emit errorOccurred("PDF file does not exist: " + filePath);
            return;
        }
        if (!fileInfo.isReadable()) {
            emit errorOccurred("PDF file is not readable: " + filePath);
            return;
        }

        // Check file size using SafePdfLoader
        if (!SafePdfLoader::checkFileSize(filePath)) {
            emit errorOccurred("PDF file too large (>500MB): " + filePath);
            return;
        }

        // Validate it's actually a PDF
        QString validateError;
        if (!SafePdfLoader::validatePdfFile(filePath, validateError)) {
            emit errorOccurred("Invalid PDF file: " + validateError);
            return;
        }

    } catch (const std::exception& e) {
        emit errorOccurred(QString("PDF validation failed: %1").arg(e.what()));
        return;
    } catch (...) {
        emit errorOccurred("PDF validation failed: Unknown error");
        return;
    }

    // Now proceed with normal processing
    if (m_currentStage != Idle) {
        emit progressMessage("Note: Resetting from previous incomplete operation");
        reset();
    }

    m_currentStage = ExtractingText;
    m_currentInputType = PDFFile;
    emit stageChanged(m_currentStage);
    emit progressMessage("Opening PDF file...");

    QString extractedText = extractTextFromPDF(filePath);

    qDebug() << "PDF extraction result length:" << extractedText.length();

    if (extractedText.isEmpty()) {
        // Ensure PDF is closed on error (though extractTextFromPDF should have already closed it)
        m_pdfDocument->close();
        m_currentStage = Idle;
        emit stageChanged(m_currentStage);
        emit errorOccurred("Failed to extract text from PDF");
        return;
    }

    m_extractedText = extractedText;
    emit textExtracted(m_extractedText);

    qDebug() << "Starting pipeline with" << extractedText.length() << "characters";
    startPipeline(extractedText, PDFFile);
}

void QueryRunner::processText(const QString& text) {
    if (m_currentStage != Idle) {
        emit progressMessage("Note: Resetting from previous incomplete operation");
        reset();
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
    try {
        // Close any previously open document first
        try {
            m_pdfDocument->close();
        } catch (...) {
            // Ignore cleanup exceptions
        }

        // Use SafePdfLoader for safe loading and extraction
        QString loadError;
        auto tempDoc = std::make_unique<QPdfDocument>();

        emit progressMessage("Loading PDF file...");

        if (!SafePdfLoader::loadPdf(tempDoc.get(), filePath, loadError, 60000)) { // 60 second timeout
            emit errorOccurred("Failed to load PDF: " + loadError);
            return QString();
        }

        emit progressMessage("Extracting text from PDF...");

        // Extract text safely
        QString extractError;
        QString extractedText = SafePdfLoader::extractTextSafely(tempDoc.get(), extractError);

        if (extractedText.isEmpty()) {
            emit errorOccurred("Failed to extract text: " + extractError);
            return QString();
        }

        // Document is automatically cleaned up when tempDoc goes out of scope
        emit progressMessage("PDF extraction completed successfully");

        return extractedText;

    } catch (const std::exception& e) {
        QString errorMsg = QString("Exception during PDF extraction: %1").arg(e.what());
        emit errorOccurred(errorMsg);
        qDebug() << errorMsg;
        return QString();
    } catch (...) {
        emit errorOccurred("Unknown exception during PDF extraction");
        qDebug() << "Unknown exception in QueryRunner::extractTextFromPDF";
        return QString();
    }
}

QString QueryRunner::cleanupText(const QString& text, InputType type) {
    QString cleaned = text;

    // Normalize line endings FIRST (Windows \r\n to Unix \n)
    cleaned.replace("\r\n", "\n");
    cleaned.replace("\r", "\n");

    // Always remove copyright notices for LLM processing
    cleaned = removeCopyrightNotices(cleaned);

    // Remove problematic Unicode characters that confuse LLMs
    // Remove soft hyphens and other invisible formatting characters
    cleaned.remove(QChar(0x00AD)); // Soft hyphen
    cleaned.remove(QChar(0xFFFD)); // Replacement character
    cleaned.remove(QChar(0xFFF9)); // Interlinear annotation anchor
    cleaned.remove(QChar(0xFFFA)); // Interlinear annotation separator
    cleaned.remove(QChar(0xFFFB)); // Interlinear annotation terminator

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

    // Remove standalone license lines
    result.remove(QRegularExpression("\\bLicense\\b.*\\n", QRegularExpression::CaseInsensitiveOption));

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

    // Clear the transcript.log file at the start of each run
    QString appDir = QCoreApplication::applicationDirPath();
    QString transcriptPath = QDir(appDir).absoluteFilePath("transcript.log");
    QFile transcriptFile(transcriptPath);
    if (transcriptFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        QTextStream stream(&transcriptFile);
        stream << "=== NETWORK TRANSCRIPT LOG ===" << Qt::endl;
        stream << "Started: " << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz") << Qt::endl;
        stream << "This log contains complete request/response JSON for all API calls" << Qt::endl;
        stream << "Input Type: " << (type == PDFFile ? "PDF File" : "Pasted Text") << Qt::endl;
        transcriptFile.close();
    }

    // Clean up the text
    qDebug() << "Text before cleanup:" << text.length() << "characters";
    qDebug() << "First 200 chars before cleanup:" << text.left(200);
    m_cleanedText = cleanupText(text, type);
    qDebug() << "Text after cleanup:" << m_cleanedText.length() << "characters";
    qDebug() << "First 200 chars after cleanup:" << m_cleanedText.left(200);

    if (m_cleanedText.length() < text.length() / 2) {
        qDebug() << "WARNING: Cleanup removed more than half the text!";
    }

    if (m_cleanedText.isEmpty()) {
        // Ensure PDF is closed if we're processing a PDF
        if (type == PDFFile) {
            m_pdfDocument->close();
        }
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

    // Log summary availability
    if (!m_summary.isEmpty()) {
        emit progressMessage(QString("Summary available for keyword extraction (%1 chars)").arg(m_summary.length()));
    } else {
        emit progressMessage("No summary available for keyword extraction");
    }

    m_keywordsQuery->setConnectionSettings(m_settings.url, m_settings.modelName);
    m_keywordsQuery->setPromptSettings(m_settings.keywordTemp,
                                       m_settings.keywordContext,
                                       m_settings.keywordTimeout);
    m_keywordsQuery->setPreprompt(m_settings.keywordPreprompt);
    m_keywordsQuery->setPrompt(m_settings.keywordPrompt);
    m_keywordsQuery->setSummaryResult(m_summary);  // Pass the summary result

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

    // Log summary availability
    if (!m_summary.isEmpty()) {
        emit progressMessage(QString("Summary available for refined keyword extraction (%1 chars)").arg(m_summary.length()));
    } else {
        emit progressMessage("No summary available for refined keyword extraction");
    }

    m_refinedKeywordsQuery->setConnectionSettings(m_settings.url, m_settings.modelName);
    m_refinedKeywordsQuery->setPromptSettings(m_settings.keywordTemp,
                                              m_settings.keywordContext,
                                              m_settings.keywordTimeout);
    m_refinedKeywordsQuery->setPreprompt(m_settings.keywordPreprompt);
    m_refinedKeywordsQuery->setRefinedPrompt(m_suggestedPrompt);
    m_refinedKeywordsQuery->setSummaryResult(m_summary);  // Pass the summary result

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

    if (m_singleStepMode) {
        // Single-step keyword extraction - don't advance
        m_singleStepMode = false;  // Reset flag
        m_currentStage = Complete;
        emit stageChanged(m_currentStage);
        emit processingComplete();  // This triggers UI re-enable
        emit progressMessage("Keyword re-extraction complete");
        m_currentStage = Idle;
    } else {
        // Part of full pipeline - continue normally
        advanceToNextStage();
    }
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
            if (m_settings.skipRefinement) {
                // Skip refinement stages and complete the process
                emit progressMessage("Skipping keyword refinement as per settings");
                m_currentStage = Complete;
                emit stageChanged(m_currentStage);
                emit processingComplete();
                emit progressMessage("All processing complete");
                m_currentStage = Idle;
            } else {
                runPromptRefinement();
            }
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
    // Load skip_refinement setting, default to false if not present
    QString skipRefinement = query.value("skip_refinement").toString();
    m_settings.skipRefinement = (skipRefinement == "true");
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

void QueryRunner::handleQueryError(const QString& error) {
    QString contextError = QString("[%1] %2")
        .arg(getStageString(m_currentStage))
        .arg(error);

    // Check if it's a timeout - these are expected for large docs
    bool isTimeout = error.contains("timeout", Qt::CaseInsensitive) ||
                     error.contains("timed out", Qt::CaseInsensitive);

    if (isTimeout) {
        // Log timeouts with special message
        emit progressMessage("WARNING: Request timed out - this is normal for large documents");
        emit progressMessage("You can retry with a shorter document or adjust timeout in settings");
    }

    // ALWAYS emit errorOccurred so UI gets re-enabled
    emit errorOccurred(contextError);

    // Always reset state so user can retry
    reset();
}

QString QueryRunner::getStageString(ProcessingStage stage) const {
    switch (stage) {
        case Idle: return "Idle";
        case ExtractingText: return "Extracting Text";
        case GeneratingSummary: return "Generating Summary";
        case ExtractingKeywords: return "Extracting Keywords";
        case RefiningPrompt: return "Refining Prompt";
        case ExtractingRefinedKeywords: return "Extracting Refined Keywords";
        case Complete: return "Complete";
        default: return "Unknown";
    }
}