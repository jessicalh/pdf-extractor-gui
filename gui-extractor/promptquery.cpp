#include "promptquery.h"
#include <QNetworkRequest>
#include <QUrl>
#include <QDebug>
#include <QFile>
#include <QTextStream>
#include <QDateTime>

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

void PromptQuery::abort() {
    QString queryType = getQueryType();
    qDebug() << "PromptQuery::abort() called for" << queryType;

    // Write to abort log file
    QFile abortLog("abort_debug.log");
    if (abortLog.open(QIODevice::WriteOnly | QIODevice::Append)) {
        QTextStream stream(&abortLog);
        stream << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz")
               << " - PromptQuery::abort() called for " << queryType << Qt::endl;
        abortLog.close();
    }

    // Force close the connection to stop LM Studio from continuing
    cleanupNetworkReply(true);

    qDebug() << "PromptQuery::abort() complete for" << queryType;
    if (abortLog.open(QIODevice::WriteOnly | QIODevice::Append)) {
        QTextStream stream(&abortLog);
        stream << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz")
               << " - PromptQuery::abort() complete for " << queryType << Qt::endl;
        abortLog.close();
    }
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

    // Log the request summary (not full prompt to UI)
    emit progressUpdate(QString("=== %1 REQUEST SENT ===").arg(getQueryType().toUpper()));
    emit progressUpdate(QString("Model: %1, Temp: %2, Max Tokens: %3")
                       .arg(m_modelName).arg(m_temperature).arg(m_contextLength));

    // Write to lastrun.log
    QFile logFile("lastrun.log");
    if (logFile.open(QIODevice::WriteOnly | QIODevice::Append)) {
        QTextStream stream(&logFile);
        stream << "\n=== " << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss")
               << " - " << getQueryType() << " ===\n";
        stream << "URL: " << m_url << "\n";
        stream << "Model: " << m_modelName << "\n";
        stream << "Temperature: " << m_temperature << "\n";
        stream << "Max Tokens: " << m_contextLength << "\n";
        stream << "--- Full Prompt ---\n";
        stream << fullPrompt << "\n";
        stream << "--- End Prompt ---\n";
        logFile.close();
    }

    emit progressUpdate("Sending request to LM Studio...");

    // Log if the final prompt contains summary data
    if (fullPrompt.contains("Summary:") || fullPrompt.contains("summary:")) {
        emit progressUpdate("✓ Final prompt DOES contain summary section");

        // Find and show the summary portion
        int summaryPos = fullPrompt.indexOf("Summary:", Qt::CaseInsensitive);
        if (summaryPos >= 0) {
            int endPos = fullPrompt.indexOf("\n\n", summaryPos);
            if (endPos < 0) endPos = fullPrompt.indexOf("Text:", summaryPos);
            if (endPos < 0) endPos = summaryPos + 500;

            QString summarySection = fullPrompt.mid(summaryPos, endPos - summaryPos);
            emit progressUpdate(QString("Summary section in prompt: %1").arg(summarySection.left(300)));
        }
    } else {
        emit progressUpdate("✗ Final prompt does NOT contain 'Summary:' keyword");
    }

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
        QString errorString = m_currentReply->errorString();
        qDebug() << "Network error in" << getQueryType() << ":" << errorString;

        // Check if this was an intentional abort
        if (m_currentReply->error() == QNetworkReply::OperationCanceledError) {
            qDebug() << "  - This was an intentional abort, not emitting error signal";
            // Don't emit error for intentional aborts
            m_currentReply->deleteLater();
            m_currentReply = nullptr;
            return;
        }

        emit errorOccurred("Network error: " + errorString);
        cleanupNetworkReply(false);
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

    // Safely extract content and reasoning fields
    QString content;
    QString reasoning;

    if (!choices[0].isObject()) {
        emit errorOccurred("Invalid response structure: choices[0] is not an object");
        return;
    }

    QJsonObject choice = choices[0].toObject();
    if (choice.contains("message") && choice["message"].isObject()) {
        QJsonObject message = choice["message"].toObject();

        // Extract content field
        if (message.contains("content") && message["content"].isString()) {
            content = message["content"].toString();
        } else {
            emit progressUpdate("Warning: No content field in response");
        }

        // Extract reasoning field (this is where gpt-oss puts its reasoning)
        if (message.contains("reasoning") && message["reasoning"].isString()) {
            reasoning = message["reasoning"].toString();
        }
    } else {
        emit errorOccurred("Invalid response structure: no message object");
        return;
    }

    // Extract <think> tags from content (if present)
    QString thinkReasoning;
    content = extractThinkTags(content, thinkReasoning);

    // Log the response to UI (simplified)
    emit progressUpdate(QString("=== %1 RESPONSE RECEIVED ===").arg(getQueryType().toUpper()));

    // Show reasoning from both sources (gpt-oss reasoning field and <think> tags)
    bool hasReasoning = false;

    // First show gpt-oss style reasoning if present
    if (!reasoning.isEmpty()) {
        emit progressUpdate("--- Model Reasoning (gpt-oss format) ---");
        emit progressUpdate(reasoning);
        emit progressUpdate("--- End Reasoning ---");
        hasReasoning = true;
    }

    // Then show <think> tag reasoning if present
    if (!thinkReasoning.isEmpty()) {
        emit progressUpdate("--- Model Reasoning (<think> tags) ---");
        emit progressUpdate(thinkReasoning);
        emit progressUpdate("--- End Reasoning ---");
        hasReasoning = true;
    }

    if (!hasReasoning) {
        emit progressUpdate("(No reasoning provided by model)");
    }

    // Show abbreviated content preview (after think tags removed)
    if (!content.isEmpty()) {
        QString preview = content.left(100);
        if (content.length() > 100) preview += "...";
        emit progressUpdate(QString("Content preview: %1").arg(preview));
    }

    // Write full response to lastrun.log (content and all reasoning)
    QFile logFile("lastrun.log");
    if (logFile.open(QIODevice::WriteOnly | QIODevice::Append)) {
        QTextStream stream(&logFile);
        stream << "--- Response Content (after think tag removal) ---\n";
        stream << content << "\n";
        stream << "--- End Content ---\n";
        if (!reasoning.isEmpty()) {
            stream << "--- Response Reasoning (gpt-oss format) ---\n";
            stream << reasoning << "\n";
            stream << "--- End Reasoning ---\n";
        }
        if (!thinkReasoning.isEmpty()) {
            stream << "--- Response Reasoning (think tags) ---\n";
            stream << thinkReasoning << "\n";
            stream << "--- End Reasoning ---\n";
        }
        logFile.close();
    }

    emit progressUpdate("Processing response...");

    // Process with the content field (not reasoning)
    if (!content.isEmpty()) {
        processResponse(content);
    } else {
        emit errorOccurred("No content in response to process");
    }
}

void PromptQuery::handleTimeout() {
    emit errorOccurred("Request timeout after " + QString::number(m_timeout/1000) + " seconds");
    // Use centralized cleanup but don't force close for timeouts
    cleanupNetworkReply(false);
}

QString PromptQuery::removeHarmonyArtifacts(const QString& text) {
    QString cleaned = text;

    // Check if text starts with <|start|> and has <|message|> within first 60 chars
    const QString startTag = "<|start|>";
    const QString messageTag = "<|message|>";
    if (cleaned.startsWith(startTag)) {
        qsizetype messagePos = cleaned.indexOf(messageTag, 0);
        if (messagePos != -1 && messagePos <= 60) {
            // Found Harmony header within 60 chars, remove it
            QString removedSequence = cleaned.left(messagePos + messageTag.length());
            cleaned = cleaned.mid(messagePos + messageTag.length());

            // Log what we removed
            emit progressUpdate(QString("Removed Harmony artifact: %1").arg(removedSequence));

            // Also log to file for debugging
            QFile logFile("harmony_artifacts.log");
            if (logFile.open(QIODevice::WriteOnly | QIODevice::Append)) {
                QTextStream stream(&logFile);
                stream << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << " - ";
                stream << "Removed: " << removedSequence << Qt::endl;
                logFile.close();
            }
        }
    }

    // Check for orphaned end tags at the very end
    const QString endTag = "<|end|>";
    const QString returnTag = "<|return|>";
    if (cleaned.endsWith(endTag)) {
        cleaned.chop(endTag.length());
        emit progressUpdate("Removed orphaned <|end|> tag at end of response");
    } else if (cleaned.endsWith(returnTag)) {
        cleaned.chop(returnTag.length());
        emit progressUpdate("Removed orphaned <|return|> tag at end of response");
    }

    // Also check for other common incomplete patterns at the end
    // Pattern: <|start|> with no closing (at the very end)
    qsizetype lastStart = cleaned.lastIndexOf(startTag);
    if (lastStart != -1 && lastStart > cleaned.length() - 100) {
        // Found <|start|> near the end, check if it's incomplete
        QString tail = cleaned.mid(lastStart);
        if (!tail.contains(messageTag) && !tail.contains(endTag)) {
            // Incomplete sequence at the end
            const QString& removedSequence = tail;
            cleaned = cleaned.left(lastStart);
            emit progressUpdate(QString("Removed incomplete Harmony sequence at end: %1").arg(removedSequence));

            // Log this too
            QFile logFile("harmony_artifacts.log");
            if (logFile.open(QIODevice::WriteOnly | QIODevice::Append)) {
                QTextStream stream(&logFile);
                stream << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << " - ";
                stream << "Removed incomplete: " << removedSequence << Qt::endl;
                logFile.close();
            }
        }
    }

    return cleaned.trimmed();
}

QString PromptQuery::extractThinkTags(const QString& text, QString& reasoning) {
    QString cleaned = text;
    reasoning.clear();

    // Look for <think> ... </think> tags
    const QString thinkStartTag = "<think>";
    const QString thinkEndTag = "</think>";
    qsizetype startPos = cleaned.indexOf(thinkStartTag);
    if (startPos != -1) {
        qsizetype endPos = cleaned.indexOf(thinkEndTag, startPos);
        if (endPos != -1) {
            // Extract the reasoning content
            qsizetype contentStart = startPos + thinkStartTag.length();
            qsizetype contentLength = endPos - contentStart;
            reasoning = cleaned.mid(contentStart, contentLength).trimmed();

            // Calculate how much to remove, checking for newline after </think>
            qsizetype removeLength = endPos + thinkEndTag.length() - startPos;

            // Check if there's a newline immediately after </think>
            qsizetype checkPos = endPos + thinkEndTag.length();
            if (checkPos < cleaned.length()) {
                if (cleaned[checkPos] == '\n') {
                    removeLength++; // Also remove the newline
                    emit progressUpdate("Found and removed newline after </think> tag");
                } else if (cleaned[checkPos] == '\r' && checkPos + 1 < cleaned.length() && cleaned[checkPos + 1] == '\n') {
                    removeLength += 2; // Remove \r\n
                    emit progressUpdate("Found and removed \\r\\n after </think> tag");
                }
            }

            // Remove the entire <think>...</think> block (and optional newline) from the main text
            cleaned.remove(startPos, removeLength);

            // Log what we found
            emit progressUpdate(QString("Found and extracted <think> block (%1 characters)").arg(reasoning.length()));

            // Also log to file for debugging
            QFile logFile("think_tags.log");
            if (logFile.open(QIODevice::WriteOnly | QIODevice::Append)) {
                QTextStream stream(&logFile);
                stream << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << " - ";
                stream << getQueryType() << " - Extracted think content:\n";
                stream << reasoning << "\n";
                stream << "--- End Think Content ---\n";
                logFile.close();
            }
        } else {
            // Found opening <think> but no closing tag
            emit progressUpdate("Warning: Found <think> tag without closing </think>");
        }
    }

    return cleaned.trimmed();
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
    // Clean Harmony artifacts first
    QString result = removeHarmonyArtifacts(response);

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

void KeywordsQuery::setSummaryResult(const QString& summary) {
    m_summaryResult = summary;
}

QString KeywordsQuery::buildFullPrompt(const QString& text) {
    if (text.isEmpty()) {
        return QString();
    }

    QString fullPrompt = m_preprompt;
    if (!fullPrompt.isEmpty()) {
        fullPrompt += "\n\n";
    }

    QString processedPrompt = m_prompt;

    // Debug logging BEFORE replacement
    bool hasSummaryPlaceholder = m_prompt.contains("{summary_result}");
    if (hasSummaryPlaceholder) {
        qDebug() << "Keywords prompt contains {summary_result} placeholder";
        qDebug() << "Summary content length:" << m_summaryResult.length();
        if (!m_summaryResult.isEmpty()) {
            qDebug() << "Summary first 100 chars:" << m_summaryResult.left(100);
        } else {
            qDebug() << "WARNING: Summary is EMPTY!";
        }
    }

    processedPrompt.replace("{text}", text);
    processedPrompt.replace("{summary_result}", m_summaryResult);
    fullPrompt += processedPrompt;

    return fullPrompt;
}

void KeywordsQuery::processResponse(const QString& response) {
    // Clean Harmony artifacts first
    QString result = removeHarmonyArtifacts(response);

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
    // Clean Harmony artifacts first
    QString result = removeHarmonyArtifacts(response);

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
    // Make sure the refined prompt has {text} placeholder, if not add it
    if (!refinedPrompt.contains("{text}")) {
        // Append the text placeholder if missing
        m_prompt = refinedPrompt + "\n\nText:\n{text}";
    } else {
        m_prompt = refinedPrompt;
    }
}

QString KeywordsWithRefinementQuery::getQueryType() const {
    return "Keywords (Refined)";
}

// Centralized network reply cleanup with optional forced socket closure
void PromptQuery::cleanupNetworkReply(bool forceClose) {
    if (m_currentReply) {
        qDebug() << "cleanupNetworkReply() - forceClose:" << forceClose;

        // CRITICAL: Disconnect all signals first to prevent any callbacks
        m_currentReply->disconnect();
        qDebug() << "  - Disconnected all signals";

        if (m_currentReply->isRunning()) {
            if (forceClose && m_networkManager) {
                // Force close the underlying socket to stop server processing
                // This is more aggressive than abort() and tells the server we're done
                // Clear all connections to force socket closure
                m_networkManager->clearConnectionCache();
                m_networkManager->clearAccessCache();
                qDebug() << "  - Cleared connection cache to force socket closure";
            }

            // Now abort the request
            m_currentReply->abort();
            qDebug() << "  - Aborted network reply";
        }

        // Schedule for deletion
        m_currentReply->deleteLater();
        m_currentReply = nullptr;
        qDebug() << "  - Network reply scheduled for deletion";
    }

    // Always stop the timer
    if (m_timeoutTimer && m_timeoutTimer->isActive()) {
        m_timeoutTimer->stop();
        qDebug() << "  - Stopped timeout timer";
    }
}