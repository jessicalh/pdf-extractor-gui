#ifndef TOMLPARSER_H
#define TOMLPARSER_H

#include <QString>
#include <QMap>
#include <QFile>
#include <QTextStream>
#include <QRegularExpression>

class SimpleTomlParser {
public:
    QMap<QString, QString> parse(const QString &filePath) {
        QMap<QString, QString> result;
        QFile file(filePath);

        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            return result;
        }

        QTextStream in(&file);
        QString currentSection;
        QString currentKey;
        bool inMultilineString = false;
        QString multilineValue;

        while (!in.atEnd()) {
            QString line = in.readLine();

            // Skip comments and empty lines
            if (line.trimmed().isEmpty() || line.trimmed().startsWith("#")) {
                if (!inMultilineString) continue;
            }

            // Handle multiline strings
            if (inMultilineString) {
                if (line.contains("\"\"\"")) {
                    multilineValue += line.left(line.indexOf("\"\"\""));
                    result[currentSection + "." + currentKey] = multilineValue.trimmed();
                    inMultilineString = false;
                    multilineValue.clear();
                } else {
                    multilineValue += line + "\n";
                }
                continue;
            }

            // Check for section headers
            if (line.startsWith("[") && line.endsWith("]")) {
                currentSection = line.mid(1, line.length() - 2);
                continue;
            }

            // Parse key-value pairs
            if (line.contains("=")) {
                int eqPos = line.indexOf("=");
                QString key = line.left(eqPos).trimmed();
                QString value = line.mid(eqPos + 1).trimmed();

                // Handle multiline strings starting with """
                if (value.startsWith("\"\"\"")) {
                    currentKey = key;
                    value = value.mid(3);
                    if (value.contains("\"\"\"")) {
                        // Single line multiline string
                        value = value.left(value.indexOf("\"\"\""));
                        result[currentSection + "." + key] = value.trimmed();
                    } else {
                        // Start of multiline string
                        inMultilineString = true;
                        multilineValue = value + "\n";
                    }
                } else {
                    // Remove quotes if present
                    if (value.startsWith("\"") && value.endsWith("\"")) {
                        value = value.mid(1, value.length() - 2);
                    }
                    result[currentSection + "." + key] = value;
                }
            }
        }

        file.close();
        return result;
    }
};

#endif // TOMLPARSER_H