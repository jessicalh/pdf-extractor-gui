#include "modellistfetcher.h"
#include <QNetworkRequest>
#include <QUrl>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>

ModelListFetcher::ModelListFetcher(QObject *parent)
    : QObject(parent)
    , m_networkManager(new QNetworkAccessManager(this))
    , m_currentReply(nullptr)
    , m_timeoutTimer(new QTimer(this))
    , m_timeout(10000) // 10 seconds default
{
    m_timeoutTimer->setSingleShot(true);
    connect(m_timeoutTimer, &QTimer::timeout, this, &ModelListFetcher::handleTimeout);
}

ModelListFetcher::~ModelListFetcher() {
    cleanupReply();
}

void ModelListFetcher::fetchModels(const QString& baseUrl) {
    // Clean up any previous request
    cleanupReply();
    m_models.clear();
    m_lastError.clear();

    // Validate URL
    if (baseUrl.isEmpty()) {
        m_lastError = "URL is empty";
        emit errorOccurred(m_lastError);
        return;
    }

    // Construct the models endpoint URL
    // LM Studio uses /v1/models endpoint
    QString url = baseUrl;
    if (!url.startsWith("http://") && !url.startsWith("https://")) {
        url = "http://" + url;
    }
    if (url.endsWith("/")) {
        url.chop(1);
    }
    url += "/v1/models";

    emit progressUpdate("Fetching model list from: " + url);

    // Create network request
    QNetworkRequest request;
    request.setUrl(QUrl(url));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("User-Agent", "PDFExtractor/1.0");

    // Send GET request
    m_currentReply = m_networkManager->get(request);

    // Connect signals
    connect(m_currentReply, &QNetworkReply::finished,
            this, &ModelListFetcher::handleNetworkReply);

    // Start timeout timer
    m_timeoutTimer->start(m_timeout);
}

void ModelListFetcher::handleNetworkReply() {
    // Stop timeout timer
    m_timeoutTimer->stop();

    if (!m_currentReply) {
        return;
    }

    // Check for network errors
    if (m_currentReply->error() != QNetworkReply::NoError) {
        m_lastError = QString("Network error: %1").arg(m_currentReply->errorString());

        // Add more specific error messages
        if (m_currentReply->error() == QNetworkReply::ConnectionRefusedError) {
            m_lastError += "\n\nMake sure LM Studio is running and the server is started.";
        } else if (m_currentReply->error() == QNetworkReply::TimeoutError) {
            m_lastError += "\n\nThe request timed out. Check if the server is responding.";
        } else if (m_currentReply->error() == QNetworkReply::HostNotFoundError) {
            m_lastError += "\n\nCould not find the host. Check the URL.";
        }

        emit errorOccurred(m_lastError);
        cleanupReply();
        return;
    }

    // Read response
    QByteArray response = m_currentReply->readAll();
    cleanupReply();

    // Parse JSON response
    QJsonDocument doc = QJsonDocument::fromJson(response);
    if (doc.isNull() || !doc.isObject()) {
        m_lastError = "Invalid JSON response from server";
        emit errorOccurred(m_lastError);
        return;
    }

    QJsonObject root = doc.object();

    // LM Studio returns models in a "data" array
    if (!root.contains("data") || !root["data"].isArray()) {
        m_lastError = "Response does not contain model data";
        emit errorOccurred(m_lastError);
        return;
    }

    QJsonArray modelsArray = root["data"].toArray();

    if (modelsArray.isEmpty()) {
        m_lastError = "No models found. Please load a model in LM Studio first.";
        emit errorOccurred(m_lastError);
        return;
    }

    // Extract model IDs
    for (const QJsonValue& value : modelsArray) {
        if (value.isObject()) {
            QJsonObject modelObj = value.toObject();
            if (modelObj.contains("id") && modelObj["id"].isString()) {
                QString modelId = modelObj["id"].toString();
                if (!modelId.isEmpty()) {
                    m_models.append(modelId);
                }
            }
        }
    }

    if (m_models.isEmpty()) {
        m_lastError = "Could not extract any model names from response";
        emit errorOccurred(m_lastError);
        return;
    }

    // Sort models alphabetically for easier selection
    m_models.sort(Qt::CaseInsensitive);

    emit progressUpdate(QString("Found %1 model(s)").arg(m_models.count()));
    emit modelsReady(m_models);
}

void ModelListFetcher::handleTimeout() {
    if (m_currentReply) {
        m_lastError = QString("Request timed out after %1 seconds").arg(m_timeout / 1000);
        emit errorOccurred(m_lastError);
        cleanupReply();
    }
}

void ModelListFetcher::cleanupReply() {
    if (m_currentReply) {
        m_currentReply->abort();
        m_currentReply->deleteLater();
        m_currentReply = nullptr;
    }
}