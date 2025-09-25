#ifndef MODELLISTFETCHER_H
#define MODELLISTFETCHER_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QTimer>

class ModelListFetcher : public QObject {
    Q_OBJECT

public:
    explicit ModelListFetcher(QObject *parent = nullptr);
    ~ModelListFetcher();

    // Fetch models from the given URL
    void fetchModels(const QString& baseUrl);

    // Get the last fetched models
    QStringList getModels() const { return m_models; }

    // Get the last error message
    QString getLastError() const { return m_lastError; }

    // Set timeout in milliseconds (default 10000)
    void setTimeout(int msecs) { m_timeout = msecs; }

signals:
    void modelsReady(const QStringList& models);
    void errorOccurred(const QString& error);
    void progressUpdate(const QString& message);

private slots:
    void handleNetworkReply();
    void handleTimeout();

private:
    QNetworkAccessManager* m_networkManager;
    QNetworkReply* m_currentReply;
    QTimer* m_timeoutTimer;

    QStringList m_models;
    QString m_lastError;
    int m_timeout;

    void cleanupReply();
};

#endif // MODELLISTFETCHER_H