#include <QCoreApplication>
#include <QPdfDocument>
#include <QFile>
#include <QTextStream>
#include <QString>
#include <QDebug>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QRegularExpression>
#include <iostream>

QString cleanCopyrightText(const QString &text) {
    QString cleaned = text;

    // List of copyright patterns to remove (case insensitive)
    QList<QRegularExpression> copyrightPatterns = {
        QRegularExpression("\\(c\\)", QRegularExpression::CaseInsensitiveOption),
        QRegularExpression("\\(C\\)", QRegularExpression::CaseInsensitiveOption),
        QRegularExpression("©", QRegularExpression::CaseInsensitiveOption),
        QRegularExpression("\\bcopyright\\b", QRegularExpression::CaseInsensitiveOption),
        QRegularExpression("\\ball\\s+rights\\s+reserved\\b", QRegularExpression::CaseInsensitiveOption)
    };

    // Remove each pattern
    for (const auto &pattern : copyrightPatterns) {
        cleaned.remove(pattern);
    }

    // Clean up multiple spaces that may result from removals
    cleaned = cleaned.simplified();

    // Replace multiple consecutive spaces with single space
    QRegularExpression multiSpace("\\s{2,}");
    cleaned.replace(multiSpace, " ");

    return cleaned;
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("PDF Text Extractor");
    QCoreApplication::setApplicationVersion("1.0");

    // Set up command line parser
    QCommandLineParser parser;
    parser.setApplicationDescription("Extract text from PDF files and clean copyright notices");
    parser.addHelpOption();
    parser.addVersionOption();

    // Add positional arguments
    parser.addPositionalArgument("pdf", "PDF file to extract text from");
    parser.addPositionalArgument("output", "Output text file");

    // Add optional page range
    QCommandLineOption pageRangeOption(QStringList() << "p" << "pages",
                                       "Page range to extract (e.g., 1-10 or 5)",
                                       "range");
    parser.addOption(pageRangeOption);

    // Add option to preserve copyright
    QCommandLineOption preserveOption(QStringList() << "preserve",
                                      "Preserve copyright notices");
    parser.addOption(preserveOption);

    // Process the arguments
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
                startPage = parts[0].toInt() - 1;  // Convert to 0-based
                endPage = parts[1].toInt() - 1;
            }
        } else {
            startPage = endPage = range.toInt() - 1;  // Single page
        }

        // Validate range
        startPage = qMax(0, startPage);
        endPage = qMin(pdfDocument.pageCount() - 1, endPage);
    }

    // Extract text from all pages
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

        // Show progress
        if ((i - startPage + 1) % 10 == 0 || i == endPage) {
            std::cout << "Processed " << (i - startPage + 1) << " pages" << std::endl;
        }
    }

    // Write to output file
    QFile outputFile(outputPath);
    if (!outputFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        std::cerr << "Error: Cannot open output file for writing: "
                  << outputPath.toStdString() << std::endl;
        return 1;
    }

    QTextStream out(&outputFile);
    out << fullText;
    outputFile.close();

    // Summary
    std::cout << "\nExtraction complete!" << std::endl;
    std::cout << "Pages extracted: " << (endPage - startPage + 1) << std::endl;
    std::cout << "Output written to: " << outputPath.toStdString() << std::endl;
    if (!preserveCopyright) {
        std::cout << "Copyright notices removed" << std::endl;
    }
    std::cout << "Text length: " << fullText.length() << " characters" << std::endl;

    return 0;
}