#pragma once

#include <QString>
#include <QFile>
#include <QMutex>
#include <atomic>

// Log severity ordered from most verbose to most severe.
// Verb < Debug < Info < Warn < Error
enum class LogLevel { Verb = 0, Debug = 1, Info = 2, Warn = 3, Error = 4 };

// Centralized, level-filtered logger shared across the whole app (tl4ai_core).
// Writes to a rotating file and to stderr. Lives independently of Qt's
// qDebug/qWarning (which are stripped by QT_NO_DEBUG_OUTPUT in Release), so
// file logging keeps working in Release builds.
class Logger {
public:
    static Logger &instance();

    // Apply enabled flag, level and target file path in one shot (startup).
    void configure(bool enabled, LogLevel level, const QString &filePath);
    void setEnabled(bool enabled);
    void setLevel(LogLevel level);

    bool isEnabled() const { return m_enabled.load(std::memory_order_relaxed); }
    LogLevel level() const { return m_level.load(std::memory_order_relaxed); }
    QString filePath() const;

    // Lock-free fast path so high-frequency callers (breathing animation) can
    // skip building the message string when the level is filtered out.
    bool shouldLog(LogLevel level) const
    {
        return m_enabled.load(std::memory_order_relaxed)
            && static_cast<int>(level) >= static_cast<int>(m_level.load(std::memory_order_relaxed));
    }

    void log(LogLevel level, const QString &category, const QString &message);

    static QString defaultLogFilePath();
    static LogLevel levelFromString(const QString &s); // invalid -> Warn
    static QString levelToString(LogLevel level);      // uppercase token for log lines

private:
    Logger() = default;
    ~Logger();
    Logger(const Logger &) = delete;
    Logger &operator=(const Logger &) = delete;

    void openFile();                  // assumes m_mutex held
    void closeFile();                 // assumes m_mutex held
    void rotateIfNeeded(qint64 incoming); // assumes m_mutex held

    mutable QMutex m_mutex;
    std::atomic<bool> m_enabled{false};
    std::atomic<LogLevel> m_level{LogLevel::Warn};
    QString m_filePath;
    QFile m_file;
    qint64 m_bytesWritten = 0;
};

// Short-circuit before constructing the message string when filtered out.
#define TL_LOG(lvl, cat, msg) \
    do { if (Logger::instance().shouldLog(lvl)) Logger::instance().log((lvl), (cat), (msg)); } while (0)
#define TL_LOGV(cat, msg) TL_LOG(LogLevel::Verb,  cat, msg)
#define TL_LOGD(cat, msg) TL_LOG(LogLevel::Debug, cat, msg)
#define TL_LOGI(cat, msg) TL_LOG(LogLevel::Info,  cat, msg)
#define TL_LOGW(cat, msg) TL_LOG(LogLevel::Warn,  cat, msg)
#define TL_LOGE(cat, msg) TL_LOG(LogLevel::Error, cat, msg)
