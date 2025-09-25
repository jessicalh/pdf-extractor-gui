#ifndef DEBUGLOG_H
#define DEBUGLOG_H

#include <QString>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QMutex>
#include <QDebug>

class DebugLog {
public:
    static void init(const QString& filename = "debug_abort.log") {
        if (m_file) {
            m_file->close();
            delete m_file;
        }
        m_file = new QFile(filename);
        if (m_file->open(QIODevice::WriteOnly | QIODevice::Append)) {
            m_stream = new QTextStream(m_file);
            m_stream->setAutoDetectUnicode(true);
            write("===== APPLICATION STARTED =====");
            write(QString("Time: %1").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz")));
        }
    }

    static void write(const QString& message) {
        QMutexLocker locker(&m_mutex);
        if (m_stream) {
            QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss.zzz");
            *m_stream << "[" << timestamp << "] " << message << Qt::endl;
            m_stream->flush(); // Force immediate write to disk
            m_file->flush();   // Extra flush to ensure it's written
        }
        // Also write to console
        qDebug() << message;
    }

    static void cleanup() {
        if (m_stream) {
            write("===== APPLICATION CLOSING =====");
            delete m_stream;
            m_stream = nullptr;
        }
        if (m_file) {
            m_file->close();
            delete m_file;
            m_file = nullptr;
        }
    }

private:
    static QFile* m_file;
    static QTextStream* m_stream;
    static QMutex m_mutex;
};

// Macro for easy logging
#define DEBUG_LOG(msg) DebugLog::write(msg)

#endif // DEBUGLOG_H