#include <QCoreApplication>
#include <QPdfDocument>
#include <QFile>
#include <QTextStream>
#include <QString>
#include <QDebug>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QRegularExpression>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QEventLoop>
#include <QTimer>
#include <iostream>
#include "tomlparser.h"

class LMStudioClient : public QObject {
    Q_OBJECT

public:
    LMStudioClient(const QString &endpoint, int timeout, double temperature, int maxTokens,
                   const QString &model, bool verbose)
        : m_endpoint(endpoint), m_timeout(timeout), m_temperature(temperature),
          m_maxTokens(maxTokens), m_model(model), m_verbose(verbose) {
        m_networkManager = new QNetworkAccessManager(this);
    }

    QString sendPrompt(const QString &systemPrompt, const QString &userPrompt, const QString &text) {
        QString fullPrompt = userPrompt;
        // No truncation - let LM Studio handle token limits
        fullPrompt.replace("{text}", text);

        QJsonObject systemMessage;
        systemMessage["role"] = "system";
        systemMessage["content"] = systemPrompt;

        QJsonObject userMessage;
        userMessage["role"] = "user";
        userMessage["content"] = fullPrompt;

        QJsonArray messages;
        messages.append(systemMessage);
        messages.append(userMessage);

        QJsonObject requestData;
        requestData["model"] = m_model;
        requestData["messages"] = messages;
        requestData["temperature"] = m_temperature;
        requestData["max_tokens"] = m_maxTokens;
        requestData["stream"] = false;

        if (m_verbose) {
            std::cout << "[VERBOSE] Sending request to: " << m_endpoint.toStdString() << std::endl;
            std::cout << "[VERBOSE] Model: " << m_model.toStdString() << std::endl;
            std::cout << "[VERBOSE] Temperature: " << m_temperature << std::endl;
            std::cout << "[VERBOSE] Max tokens: " << m_maxTokens << std::endl;
            std::cout << "[VERBOSE] System prompt: " << systemPrompt.left(100).toStdString() << "..." << std::endl;
            std::cout << "[VERBOSE] User prompt length: " << fullPrompt.length() << " chars" << std::endl;
        }

        QJsonDocument doc(requestData);
        QByteArray jsonData = doc.toJson();

        QNetworkRequest request((QUrl(m_endpoint)));
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

        QNetworkReply *reply = m_networkManager->post(request, jsonData);

        // Create event loop to wait for response
        QEventLoop loop;
        QTimer timer;
        timer.setSingleShot(true);
        timer.setInterval(m_timeout);

        connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
        connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);

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

                        // Clean up LM Studio specific tags if present
                        if (result.contains("<|message|>")) {
                            QRegularExpression tagPattern("<\\|message\\|>(.*?)(?:<\\|end\\|>|$)",
                                                        QRegularExpression::DotMatchesEverythingOption);
                            QRegularExpressionMatch match = tagPattern.match(result);
                            if (match.hasMatch()) {
                                result = match.captured(1).trimmed();
                            }
                        }
                        // Also clean up any start tags
                        result.remove(QRegularExpression("<\\|start\\|>.*?<\\|message\\|>"));
                    }
                    if (m_verbose) {
                        std::cout << "[VERBOSE] Response received (" << result.length() << " chars)" << std::endl;
                        std::cout << "[VERBOSE] Response content:\n" << result.left(500).toStdString();
                        if (result.length() > 500) std::cout << "...\n[truncated]";
                        std::cout << std::endl << std::endl;
                    }
                }
            } else {
                std::cerr << "Network error: " << reply->errorString().toStdString() << std::endl;
            }
        } else {
            std::cerr << "Request timeout" << std::endl;
        }

        reply->deleteLater();
        return result;
    }

private:
    QString m_endpoint;
    int m_timeout;
    double m_temperature;
    int m_maxTokens;
    QString m_model;
    bool m_verbose;
    QNetworkAccessManager *m_networkManager;
};

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

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("PDF Text Extractor with AI");
    QCoreApplication::setApplicationVersion("2.0");

    QCommandLineParser parser;
    parser.setApplicationDescription("Extract text from PDF files with optional AI processing");
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

    // AI options
    QCommandLineOption configOption(QStringList() << "c" << "config",
                                    "TOML configuration file for LM Studio",
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

    parser.process(app);

    const QStringList args = parser.positionalArguments();
    if (args.size() != 2) {
        std::cerr << "Error: Please provide both input PDF and output text file." << std::endl;
        parser.showHelp(1);
    }

    QString pdfPath = args[0];
    QString outputPath = args[1];
    bool preserveCopyright = parser.isSet(preserveOption);

    // Load PDF document
    QPdfDocument pdfDocument;
    QPdfDocument::Error error = pdfDocument.load(pdfPath);

    if (error != QPdfDocument::Error::None) {
        std::cerr << "Error loading PDF file: ";
        switch (error) {
            case QPdfDocument::Error::FileNotFound:
                std::cerr << "File not found" << std::endl;
                break;
            case QPdfDocument::Error::InvalidFileFormat:
                std::cerr << "Invalid PDF format" << std::endl;
                break;
            case QPdfDocument::Error::IncorrectPassword:
                std::cerr << "Password protected PDF" << std::endl;
                break;
            case QPdfDocument::Error::UnsupportedSecurityScheme:
                std::cerr << "Unsupported security scheme" << std::endl;
                break;
            default:
                std::cerr << "Unknown error" << std::endl;
        }
        return 1;
    }

    // Determine page range
    int startPage = 0;
    int endPage = pdfDocument.pageCount() - 1;

    if (parser.isSet(pageRangeOption)) {
        QString range = parser.value(pageRangeOption);
        if (range.contains("-")) {
            QStringList parts = range.split("-");
            if (parts.size() == 2) {
                startPage = parts[0].toInt() - 1;
                endPage = parts[1].toInt() - 1;
            }
        } else {
            startPage = endPage = range.toInt() - 1;
        }

        startPage = qMax(0, startPage);
        endPage = qMin(pdfDocument.pageCount() - 1, endPage);
    }

    // Extract text from pages
    QString fullText;
    std::cout << "Extracting text from " << (endPage - startPage + 1) << " pages..." << std::endl;

    for (int i = startPage; i <= endPage; ++i) {
        QString pageText = pdfDocument.getAllText(i).text();

        if (!preserveCopyright) {
            pageText = cleanCopyrightText(pageText);
        }

        if (!pageText.isEmpty()) {
            fullText += pageText;
            if (i < endPage) {
                fullText += "\n\n--- Page " + QString::number(i + 2) + " ---\n\n";
            }
        }

        if ((i - startPage + 1) % 10 == 0 || i == endPage) {
            std::cout << "Processed " << (i - startPage + 1) << " pages" << std::endl;
        }
    }

    // Write text to output file
    QFile outputFile(outputPath);
    if (!outputFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        std::cerr << "Error: Cannot open output file for writing: "
                  << outputPath.toStdString() << std::endl;
        return 1;
    }

    QTextStream out(&outputFile);
    out << fullText;
    outputFile.close();

    std::cout << "\nExtraction complete!" << std::endl;
    std::cout << "Pages extracted: " << (endPage - startPage + 1) << std::endl;
    std::cout << "Output written to: " << outputPath.toStdString() << std::endl;
    if (!preserveCopyright) {
        std::cout << "Copyright notices removed" << std::endl;
    }
    std::cout << "Text length: " << fullText.length() << " characters" << std::endl;

    // Process with LM Studio if config is provided
    if (parser.isSet(configOption)) {
        QString configPath = parser.value(configOption);
        bool verbose = parser.isSet(verboseOption);
        SimpleTomlParser tomlParser;
        QMap<QString, QString> config = tomlParser.parse(configPath);

        if (config.isEmpty()) {
            std::cerr << "Error: Cannot parse config file: " << configPath.toStdString() << std::endl;
        } else {
            QString endpoint = config.value("lmstudio.endpoint", "http://localhost:1234/v1/chat/completions");
            int timeout = config.value("lmstudio.timeout", "30000").toInt();
            double temperature = config.value("lmstudio.temperature", "0.7").toDouble();
            int maxTokens = config.value("lmstudio.max_tokens", "500").toInt();
            QString model = config.value("lmstudio.model_name", "gpt-oss-120b");

            if (verbose) {
                std::cout << "\n[VERBOSE] Configuration loaded from: " << configPath.toStdString() << std::endl;
                std::cout << "[VERBOSE] Endpoint: " << endpoint.toStdString() << std::endl;
                std::cout << "[VERBOSE] Model: " << model.toStdString() << std::endl;
                std::cout << "[VERBOSE] Temperature: " << temperature << std::endl;
                std::cout << "[VERBOSE] Max tokens: " << maxTokens << std::endl;
                std::cout << "[VERBOSE] Timeout: " << timeout << " ms" << std::endl << std::endl;
            }

            LMStudioClient client(endpoint, timeout, temperature, maxTokens, model, verbose);

            // Generate summary if requested
            if (parser.isSet(summaryOption)) {
                std::cout << "\nGenerating summary..." << std::endl;

                // Use task-specific parameters if available
                double summaryTemp = config.value("lm_studio.summary_temperature", QString::number(temperature)).toDouble();
                int summaryMaxTokens = config.value("lm_studio.summary_max_tokens", QString::number(maxTokens)).toInt();
                QString summaryModel = config.value("lm_studio.summary_model_name", model);

                // Create client with summary-specific settings
                LMStudioClient summaryClient(endpoint, timeout, summaryTemp, summaryMaxTokens, summaryModel, verbose);

                QString summarySystemPrompt = config.value("lm_studio.summary_system_prompt",
                    "You are an expert scientific reviewer. Provide clear, concise analysis of research papers focusing on key findings and significance.");
                QString summaryPrompt = config.value("prompts.summary");

                if (verbose) {
                    std::cout << "[VERBOSE] Summary settings - Temp: " << summaryTemp
                             << ", Max tokens: " << summaryMaxTokens
                             << ", Model: " << summaryModel.toStdString() << std::endl;
                }

                QString summary = summaryClient.sendPrompt(summarySystemPrompt, summaryPrompt, fullText);

                if (!summary.isEmpty()) {
                    QString summaryPath = parser.value(summaryOption);
                    QFile summaryFile(summaryPath);
                    if (summaryFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
                        QTextStream summaryOut(&summaryFile);
                        summaryOut << summary;
                        summaryFile.close();
                        std::cout << "Summary written to: " << summaryPath.toStdString() << std::endl;
                    }
                }
            }

            // Generate keywords if requested
            if (parser.isSet(keywordsOption)) {
                std::cout << "\nGenerating keywords..." << std::endl;

                // Use task-specific parameters if available
                double keywordsTemp = config.value("lm_studio.keyword_temperature", QString::number(temperature)).toDouble();
                int keywordsMaxTokens = config.value("lm_studio.keyword_max_tokens", QString::number(maxTokens)).toInt();
                QString keywordsModel = config.value("lm_studio.keyword_model_name", model);

                // Create client with keyword-specific settings
                LMStudioClient keywordsClient(endpoint, timeout, keywordsTemp, keywordsMaxTokens, keywordsModel, verbose);

                QString keywordsSystemPrompt = config.value("lm_studio.keyword_system_prompt",
                    "You are a scientific keyword extraction assistant. Focus on extracting specific scientific terms, organisms, chemicals, methods, and concepts from research papers.");
                QString keywordsPrompt = config.value("prompts.keywords");

                if (verbose) {
                    std::cout << "[VERBOSE] Keywords settings - Temp: " << keywordsTemp
                             << ", Max tokens: " << keywordsMaxTokens
                             << ", Model: " << keywordsModel.toStdString() << std::endl;
                }

                QString keywords = keywordsClient.sendPrompt(keywordsSystemPrompt, keywordsPrompt, fullText);

                if (!keywords.isEmpty()) {
                    QString keywordsPath = parser.value(keywordsOption);
                    QFile keywordsFile(keywordsPath);
                    if (keywordsFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
                        QTextStream keywordsOut(&keywordsFile);
                        keywordsOut << keywords;
                        keywordsFile.close();
                        std::cout << "Keywords written to: " << keywordsPath.toStdString() << std::endl;
                    }
                }
            }
        }
    }

    return 0;
}

#include "main_enhanced.moc"