#ifndef PROMPTQUERY_H
#define PROMPTQUERY_H

#include <QObject>
#include <QString>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QTimer>

// Base class for all prompt queries
class PromptQuery : public QObject {
    Q_OBJECT

public:
    explicit PromptQuery(QObject *parent = nullptr);
    virtual ~PromptQuery();

    // Configuration methods
    void setConnectionSettings(const QString& url, const QString& modelName);
    void setPromptSettings(double temperature, int contextLength, int timeout);
    void setPreprompt(const QString& preprompt);
    void setPrompt(const QString& prompt);

    // Execute the query
    void execute(const QString& inputText);

    // Control methods
    void abort();  // Cancel current request

    // Virtual methods for customization
    virtual QString buildFullPrompt(const QString& text) = 0;
    virtual void processResponse(const QString& response) = 0;
    virtual QString getQueryType() const = 0;

signals:
    void resultReady(const QString& result);
    void errorOccurred(const QString& error);
    void progressUpdate(const QString& status);

protected:
    // Common implementation
    void sendRequest(const QString& fullPrompt);
    void handleNetworkReply();
    void handleTimeout();
    QString removeHarmonyArtifacts(const QString& text);

    // Settings
    QString m_url;
    QString m_modelName;
    double m_temperature;
    int m_contextLength;
    int m_timeout;

    // Prompt components
    QString m_preprompt;
    QString m_prompt;

    // Network handling
    QNetworkAccessManager* m_networkManager;
    QNetworkReply* m_currentReply;
    QTimer* m_timeoutTimer;
};

// Query for extracting summaries
class SummaryQuery : public PromptQuery {
    Q_OBJECT

public:
    explicit SummaryQuery(QObject *parent = nullptr);

    QString buildFullPrompt(const QString& text) override;
    void processResponse(const QString& response) override;
    QString getQueryType() const override;
};

// Query for extracting keywords
class KeywordsQuery : public PromptQuery {
    Q_OBJECT

public:
    explicit KeywordsQuery(QObject *parent = nullptr);

    QString buildFullPrompt(const QString& text) override;
    void processResponse(const QString& response) override;
    QString getQueryType() const override;
};

// Query for refining keywords and generating improved prompts
class RefineKeywordsQuery : public PromptQuery {
    Q_OBJECT

public:
    explicit RefineKeywordsQuery(QObject *parent = nullptr);

    void setOriginalKeywords(const QString& keywords);
    void setOriginalPrompt(const QString& prompt);

    QString buildFullPrompt(const QString& text) override;
    void processResponse(const QString& response) override;
    QString getQueryType() const override;

private:
    QString m_originalKeywords;
    QString m_originalPrompt;
};

// Query for extracting keywords with a refined prompt
class KeywordsWithRefinementQuery : public KeywordsQuery {
    Q_OBJECT

public:
    explicit KeywordsWithRefinementQuery(QObject *parent = nullptr);

    void setRefinedPrompt(const QString& refinedPrompt);
    QString getQueryType() const override;
};

#endif // PROMPTQUERY_H