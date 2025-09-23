#include "promptquery.h"
#include <QNetworkRequest>
#include <QUrl>
#include <QDebug>

// ===== BASE CLASS IMPLEMENTATION =====

PromptQuery::PromptQuery(QObject *parent)
    : QObject(parent)
    , m_temperature(0.7)
    , m_contextLength(8000)
    , m_timeout(120000)
    , m_networkManager(new QNetworkAccessManager(this))
    , m_currentReply(nullptr)
    , m_timeoutTimer(new QTimer(this))
{
    m_timeoutTimer->setSingleShot(true);
    connect(m_timeoutTimer, &QTimer::timeout, this, &PromptQuery::handleTimeout);
}

PromptQuery::~PromptQuery() {
    if (m_currentReply) {
        m_currentReply->abort();
        m_currentReply->deleteLater();
    }
}

void PromptQuery::setConnectionSettings(const QString& url, const QString& modelName) {
    m_url = url;
    m_modelName = modelName;
}

void PromptQuery::setPromptSettings(double temperature, int contextLength, int timeout) {
    m_temperature = temperature;
    m_contextLength = contextLength;
    m_timeout = timeout;
}

void PromptQuery::setPreprompt(const QString& preprompt) {
    m_preprompt = preprompt;
}

void PromptQuery::setPrompt(const QString& prompt) {
    m_prompt = prompt;
}

void PromptQuery::execute(const QString& inputText) {
    emit progressUpdate("Preparing " + getQueryType() + " request...");

    QString fullPrompt = buildFullPrompt(inputText);

    if (fullPrompt.isEmpty()) {
        emit errorOccurred("Failed to build prompt");
        return;
    }

    sendRequest(fullPrompt);
}

void PromptQuery::sendRequest(const QString& fullPrompt) {
    QJsonObject messageObj;
    messageObj["role"] = "user";
    messageObj["content"] = fullPrompt;

    QJsonArray messages;
    messages.append(messageObj);

    QJsonObject requestBody;
    requestBody["model"] = m_modelName;
    requestBody["messages"] = messages;
    requestBody["temperature"] = m_temperature;
    requestBody["max_tokens"] = m_contextLength;

    QJsonDocument doc(requestBody);
    QByteArray requestData = doc.toJson();

    QNetworkRequest request;
    request.setUrl(QUrl(m_url));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("User-Agent", "PDFExtractor/1.0");

    emit progressUpdate("Sending request to LM Studio...");

    m_currentReply = m_networkManager->post(request, requestData);

    connect(m_currentReply, &QNetworkReply::finished,
            this, &PromptQuery::handleNetworkReply);

    m_timeoutTimer->start(m_timeout);
}

void PromptQuery::handleNetworkReply() {
    m_timeoutTimer->stop();

    if (!m_currentReply) {
        return;
    }

    if (m_currentReply->error() != QNetworkReply::NoError) {
        emit errorOccurred("Network error: " + m_currentReply->errorString());
        m_currentReply->deleteLater();
        m_currentReply = nullptr;
        return;
    }

    QByteArray response = m_currentReply->readAll();
    m_currentReply->deleteLater();
    m_currentReply = nullptr;

    QJsonDocument doc = QJsonDocument::fromJson(response);
    if (doc.isNull()) {
        emit errorOccurred("Invalid JSON response");
        return;
    }

    QJsonObject obj = doc.object();
    QJsonArray choices = obj["choices"].toArray();

    if (choices.isEmpty()) {
        emit errorOccurred("No response from model");
        return;
    }

    QString content = choices[0].toObject()["message"].toObject()["content"].toString();

    emit progressUpdate("Processing response...");
    processResponse(content);
}

void PromptQuery::handleTimeout() {
    if (m_currentReply) {
        m_currentReply->abort();
        emit errorOccurred("Request timeout after " + QString::number(m_timeout/1000) + " seconds");
    }
}

// ===== SUMMARY QUERY IMPLEMENTATION =====

SummaryQuery::SummaryQuery(QObject *parent) : PromptQuery(parent) {}

QString SummaryQuery::buildFullPrompt(const QString& text) {
    if (text.isEmpty()) {
        return QString();
    }

    QString fullPrompt = m_preprompt;
    if (!fullPrompt.isEmpty()) {
        fullPrompt += "\n\n";
    }

    QString processedPrompt = m_prompt;
    processedPrompt.replace("{text}", text);
    fullPrompt += processedPrompt;

    return fullPrompt;
}

void SummaryQuery::processResponse(const QString& response) {
    QString result = response.trimmed();

    // Check for "Not Evaluated" response
    if (result.compare("Not Evaluated", Qt::CaseInsensitive) == 0) {
        emit errorOccurred("Model unable to evaluate text");
        return;
    }

    emit resultReady(result);
    emit progressUpdate("Summary extraction complete");
}

QString SummaryQuery::getQueryType() const {
    return "Summary Extraction";
}

// ===== KEYWORDS QUERY IMPLEMENTATION =====

KeywordsQuery::KeywordsQuery(QObject *parent) : PromptQuery(parent) {}

QString KeywordsQuery::buildFullPrompt(const QString& text) {
    if (text.isEmpty()) {
        return QString();
    }

    QString fullPrompt = m_preprompt;
    if (!fullPrompt.isEmpty()) {
        fullPrompt += "\n\n";
    }

    QString processedPrompt = m_prompt;
    processedPrompt.replace("{text}", text);
    fullPrompt += processedPrompt;

    return fullPrompt;
}

void KeywordsQuery::processResponse(const QString& response) {
    QString result = response.trimmed();

    // Check for "Not Evaluated" response
    if (result.compare("Not Evaluated", Qt::CaseInsensitive) == 0) {
        emit errorOccurred("Model unable to extract keywords");
        return;
    }

    // Clean up the keyword list
    QStringList keywords = result.split(",");
    QStringList cleanedKeywords;

    for (const QString& keyword : keywords) {
        QString cleaned = keyword.trimmed();
        if (!cleaned.isEmpty()) {
            cleanedKeywords.append(cleaned);
        }
    }

    emit resultReady(cleanedKeywords.join(", "));
    emit progressUpdate("Keyword extraction complete");
}

QString KeywordsQuery::getQueryType() const {
    return "Keyword Extraction";
}

// ===== REFINE KEYWORDS QUERY IMPLEMENTATION =====

RefineKeywordsQuery::RefineKeywordsQuery(QObject *parent) : PromptQuery(parent) {}

void RefineKeywordsQuery::setOriginalKeywords(const QString& keywords) {
    m_originalKeywords = keywords;
}

void RefineKeywordsQuery::setOriginalPrompt(const QString& prompt) {
    m_originalPrompt = prompt;
}

QString RefineKeywordsQuery::buildFullPrompt(const QString& text) {
    if (text.isEmpty()) {
        return QString();
    }

    QString fullPrompt = m_preprompt;
    if (!fullPrompt.isEmpty()) {
        fullPrompt += "\n\n";
    }

    QString processedPrompt = m_prompt;
    processedPrompt.replace("{text}", text);
    processedPrompt.replace("{keywords}", m_originalKeywords);
    processedPrompt.replace("{original_prompt}", m_originalPrompt);
    fullPrompt += processedPrompt;

    return fullPrompt;
}

void RefineKeywordsQuery::processResponse(const QString& response) {
    QString result = response.trimmed();

    // Check for "Not Evaluated" response
    if (result.compare("Not Evaluated", Qt::CaseInsensitive) == 0) {
        // If refinement fails, return the original prompt
        emit resultReady(m_originalPrompt);
        emit progressUpdate("Refinement not possible, using original prompt");
        return;
    }

    emit resultReady(result);
    emit progressUpdate("Prompt refinement complete");
}

QString RefineKeywordsQuery::getQueryType() const {
    return "Keyword Refinement";
}

// ===== KEYWORDS WITH REFINEMENT QUERY IMPLEMENTATION =====

KeywordsWithRefinementQuery::KeywordsWithRefinementQuery(QObject *parent)
    : KeywordsQuery(parent) {}

void KeywordsWithRefinementQuery::setRefinedPrompt(const QString& refinedPrompt) {
    // Override the base prompt with the refined version
    m_prompt = refinedPrompt;
}

QString KeywordsWithRefinementQuery::getQueryType() const {
    return "Keywords (Refined)";
}