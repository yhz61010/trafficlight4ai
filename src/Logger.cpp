#include "Logger.h"
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QMutexLocker>
#include <QStandardPaths>
#include <cstdio>

namespace {
// Rotate when the active file would exceed this size; keep a single ".1" backup.
constexpr qint64 kMaxLogBytes = 5 * 1024 * 1024;
}

Logger &Logger::instance()
{
    static Logger s_instance;
    return s_instance;
}

Logger::~Logger()
{
    QMutexLocker locker(&m_mutex);
    closeFile();
}

void Logger::configure(bool enabled, LogLevel level, const QString &filePath)
{
    QMutexLocker locker(&m_mutex);
    m_enabled.store(enabled, std::memory_order_relaxed);
    m_level.store(level, std::memory_order_relaxed);
    if (filePath != m_filePath) {
        closeFile();
        m_filePath = filePath;
    }
    if (enabled)
        openFile();
    else
        closeFile();
}

void Logger::setEnabled(bool enabled)
{
    QMutexLocker locker(&m_mutex);
    m_enabled.store(enabled, std::memory_order_relaxed);
    if (enabled)
        openFile();
    else
        closeFile();
}

void Logger::setLevel(LogLevel level)
{
    m_level.store(level, std::memory_order_relaxed);
}

QString Logger::filePath() const
{
    QMutexLocker locker(&m_mutex);
    return m_filePath;
}

void Logger::log(LogLevel level, const QString &category, const QString &message)
{
    if (!shouldLog(level))
        return;

    const QString line = QStringLiteral("%1 [%2] [%3] %4\n")
        .arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss.zzz")),
             levelToString(level), category, message);
    const QByteArray bytes = line.toUtf8();

    QMutexLocker locker(&m_mutex);

    // Console (stderr): preserves the prior terminal-debugging experience.
    std::fputs(bytes.constData(), stderr);

    if (m_file.isOpen()) {
        rotateIfNeeded(bytes.size());
        if (m_file.isOpen()) {
            m_file.write(bytes);
            m_file.flush();
            m_bytesWritten += bytes.size();
        }
    }
}

void Logger::openFile()
{
    if (m_file.isOpen())
        return;
    if (m_filePath.isEmpty())
        return;

    const QDir dir = QFileInfo(m_filePath).absoluteDir();
    if (!dir.exists())
        dir.mkpath(QStringLiteral("."));

    m_file.setFileName(m_filePath);
    if (m_file.open(QIODevice::WriteOnly | QIODevice::Append)) {
        m_bytesWritten = m_file.size();
    } else {
        // Degrade gracefully to stderr-only if the file cannot be opened.
        std::fputs(QStringLiteral("trafficlight4ai: failed to open log file: %1\n")
                       .arg(m_filePath).toUtf8().constData(),
                   stderr);
    }
}

void Logger::closeFile()
{
    if (m_file.isOpen())
        m_file.close();
    m_bytesWritten = 0;
}

void Logger::rotateIfNeeded(qint64 incoming)
{
    if (m_bytesWritten + incoming <= kMaxLogBytes)
        return;

    m_file.close();
    const QString backup = m_filePath + QStringLiteral(".1");
    QFile::remove(backup);
    QFile::rename(m_filePath, backup);

    m_file.setFileName(m_filePath);
    if (m_file.open(QIODevice::WriteOnly | QIODevice::Append))
        m_bytesWritten = 0;
}

QString Logger::defaultLogFilePath()
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (dir.isEmpty())
        dir = QDir::tempPath() + QStringLiteral("/trafficlight4ai");
    return dir + QStringLiteral("/logs/trafficlight4ai.log");
}

LogLevel Logger::levelFromString(const QString &s)
{
    const QString v = s.trimmed().toLower();
    if (v == QLatin1String("verb") || v == QLatin1String("verbose"))
        return LogLevel::Verb;
    if (v == QLatin1String("debug"))
        return LogLevel::Debug;
    if (v == QLatin1String("info"))
        return LogLevel::Info;
    if (v == QLatin1String("warn") || v == QLatin1String("warning"))
        return LogLevel::Warn;
    if (v == QLatin1String("error"))
        return LogLevel::Error;
    return LogLevel::Warn; // default for unknown values
}

QString Logger::levelToString(LogLevel level)
{
    switch (level) {
    case LogLevel::Verb:  return QStringLiteral("VERB");
    case LogLevel::Debug: return QStringLiteral("DEBUG");
    case LogLevel::Info:  return QStringLiteral("INFO");
    case LogLevel::Warn:  return QStringLiteral("WARN");
    case LogLevel::Error: return QStringLiteral("ERROR");
    }
    return QStringLiteral("WARN");
}
