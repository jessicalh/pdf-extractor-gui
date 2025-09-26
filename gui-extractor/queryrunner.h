#ifndef QUERYRUNNER_H
#define QUERYRUNNER_H

#include <QObject>
#include <QString>
#include <QPdfDocument>
#include <QSqlDatabase>
#include "promptquery.h"

// Single runner class that manages the entire pipeline
class QueryRunner : public QObject {
    Q_OBJECT

public:
    enum InputType {
        PDFFile,
        PastedText
    };

    enum ProcessingStage {
        Idle,
        ExtractingText,
        GeneratingSummary,
        ExtractingKeywords,
        RefiningPrompt,
        ExtractingRefinedKeywords,
        Complete
    };

    explicit QueryRunner(QObject *parent = nullptr);
    ~QueryRunner();

    // Main entry points
    void processPDF(const QString& filePath);
    void processText(const QString& text);

    // Configuration
    void loadSettingsFromDatabase();
    void setManualSettings(const QVariantMap& settings);

    // State queries
    ProcessingStage currentStage() const { return m_currentStage; }
    bool isProcessing() const { return m_currentStage != Idle && m_currentStage != Complete; }

    // Control methods
    void reset();    // Resets to Idle state for error recovery
    void abort();    // User-initiated graceful cancellation
    void processKeywordsOnly();  // Run just keyword extraction with current settings

    // Results access
    QString getExtractedText() const { return m_extractedText; }
    QString getSummary() const { return m_summary; }
    QString getOriginalKeywords() const { return m_originalKeywords; }
    QString getSuggestedPrompt() const { return m_suggestedPrompt; }
    QString getRefinedKeywords() const { return m_refinedKeywords; }

signals:
    // Progress signals
    void stageChanged(ProcessingStage stage);
    void progressMessage(const QString& message);
    void errorOccurred(const QString& error);

    // Control signals
    void abortRequested();  // Signal to cancel operations

    // Result signals (emitted as each stage completes)
    void textExtracted(const QString& text);
    void summaryGenerated(const QString& summary);
    void keywordsExtracted(const QString& keywords);
    void promptRefined(const QString& suggestedPrompt);
    void refinedKeywordsExtracted(const QString& keywords);

    // Completion signal
    void processingComplete();

private slots:
    void handleSummaryResult(const QString& result);
    void handleKeywordsResult(const QString& result);
    void handleRefinementResult(const QString& result);
    void handleRefinedKeywordsResult(const QString& result);
    void handleQueryError(const QString& error);  // Centralized error handler

private:
    // Text preparation
    QString extractTextFromPDF(const QString& filePath);
    QString cleanupText(const QString& text, InputType type);
    QString removeCopyrightNotices(const QString& text);

    // Pipeline management
    void startPipeline(const QString& text, InputType type);
    void runSummaryExtraction();
    void runKeywordExtraction();
    void runPromptRefinement();
    void runRefinedKeywordExtraction();
    void advanceToNextStage();

    // Settings management
    void loadConnectionSettings();
    void loadPromptSettings();

    // Helper methods
    QString getStageString(ProcessingStage stage) const;

    // State
    ProcessingStage m_currentStage;
    InputType m_currentInputType;

    // Intermediate results
    QString m_extractedText;
    QString m_cleanedText;
    QString m_summary;
    QString m_originalKeywords;
    QString m_suggestedPrompt;
    QString m_refinedKeywords;

    // Query objects
    SummaryQuery* m_summaryQuery;
    KeywordsQuery* m_keywordsQuery;
    RefineKeywordsQuery* m_refineQuery;
    KeywordsWithRefinementQuery* m_refinedKeywordsQuery;

    // PDF handling
    QPdfDocument* m_pdfDocument;

    // Single-step mode flag
    bool m_singleStepMode;

    // Settings cache
    struct Settings {
        // Connection
        QString url;
        QString modelName;
        int overallTimeout;
        int textTruncationLimit;

        // Summary
        double summaryTemp;
        int summaryContext;
        int summaryTimeout;
        QString summaryPreprompt;
        QString summaryPrompt;

        // Keywords
        double keywordTemp;
        int keywordContext;
        int keywordTimeout;
        QString keywordPreprompt;
        QString keywordPrompt;

        // Refinement
        double refinementTemp;
        int refinementContext;
        int refinementTimeout;
        bool skipRefinement;
        QString keywordRefinementPreprompt;
        QString prepromptRefinementPrompt;
    } m_settings;
};

#endif // QUERYRUNNER_H