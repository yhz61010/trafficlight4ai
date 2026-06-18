#include <QtTest>
#include <QTemporaryDir>
#include <QFile>
#include "Logger.h"

class TestLogger : public QObject {
    Q_OBJECT

private:
    QTemporaryDir m_tempDir;
    int m_testIndex = 0;
    QString m_logPath;

    QString readLog() const
    {
        QFile file(m_logPath);
        if (!file.open(QIODevice::ReadOnly))
            return QString();
        const QString content = QString::fromUtf8(file.readAll());
        file.close();
        return content;
    }

private slots:
    void init()
    {
        m_logPath = m_tempDir.path() + "/log_" + QString::number(m_testIndex++) + ".log";
    }

    void cleanup()
    {
        // Detach the singleton from the per-test file so it can be removed.
        Logger::instance().configure(false, LogLevel::Warn, QString());
    }

    // --- level string conversion ---

    void levelFromStringParsesAllLevels()
    {
        QCOMPARE(Logger::levelFromString("verb"), LogLevel::Verb);
        QCOMPARE(Logger::levelFromString("debug"), LogLevel::Debug);
        QCOMPARE(Logger::levelFromString("info"), LogLevel::Info);
        QCOMPARE(Logger::levelFromString("warn"), LogLevel::Warn);
        QCOMPARE(Logger::levelFromString("error"), LogLevel::Error);
    }

    void levelFromStringIsCaseInsensitiveAndTrimmed()
    {
        QCOMPARE(Logger::levelFromString("  ERROR  "), LogLevel::Error);
        QCOMPARE(Logger::levelFromString("Warning"), LogLevel::Warn);
        QCOMPARE(Logger::levelFromString("verbose"), LogLevel::Verb);
    }

    void levelFromStringDefaultsToWarnForUnknown()
    {
        QCOMPARE(Logger::levelFromString("bogus"), LogLevel::Warn);
        QCOMPARE(Logger::levelFromString(""), LogLevel::Warn);
    }

    void levelToStringRoundTrip()
    {
        QCOMPARE(Logger::levelToString(LogLevel::Verb), QString("VERB"));
        QCOMPARE(Logger::levelToString(LogLevel::Error), QString("ERROR"));
    }

    // --- shouldLog filtering ---

    void shouldLogRespectsThreshold()
    {
        Logger::instance().configure(true, LogLevel::Warn, m_logPath);
        QVERIFY(!Logger::instance().shouldLog(LogLevel::Verb));
        QVERIFY(!Logger::instance().shouldLog(LogLevel::Info));
        QVERIFY(Logger::instance().shouldLog(LogLevel::Warn));
        QVERIFY(Logger::instance().shouldLog(LogLevel::Error));
    }

    void shouldLogFalseWhenDisabled()
    {
        Logger::instance().configure(false, LogLevel::Verb, m_logPath);
        QVERIFY(!Logger::instance().shouldLog(LogLevel::Error));
    }

    // --- file output ---

    void writesLineWhenEnabled()
    {
        Logger::instance().configure(true, LogLevel::Info, m_logPath);
        Logger::instance().log(LogLevel::Warn, "Cat", "hello world");
        const QString content = readLog();
        QVERIFY(content.contains("[WARN]"));
        QVERIFY(content.contains("[Cat]"));
        QVERIFY(content.contains("hello world"));
    }

    void doesNotWriteFilteredLevel()
    {
        Logger::instance().configure(true, LogLevel::Warn, m_logPath);
        Logger::instance().log(LogLevel::Info, "Cat", "should be filtered");
        QVERIFY(!readLog().contains("should be filtered"));
    }

    void doesNotCreateFileWhenDisabled()
    {
        Logger::instance().configure(false, LogLevel::Verb, m_logPath);
        Logger::instance().log(LogLevel::Error, "Cat", "no file expected");
        QVERIFY(!QFile::exists(m_logPath));
    }

    void filePathReflectsConfiguredPath()
    {
        Logger::instance().configure(true, LogLevel::Warn, m_logPath);
        QCOMPARE(Logger::instance().filePath(), m_logPath);
    }

    void liveLevelChangeTakesEffect()
    {
        Logger::instance().configure(true, LogLevel::Error, m_logPath);
        Logger::instance().log(LogLevel::Warn, "Cat", "before");
        Logger::instance().setLevel(LogLevel::Verb);
        Logger::instance().log(LogLevel::Warn, "Cat", "after");
        const QString content = readLog();
        QVERIFY(!content.contains("before"));
        QVERIFY(content.contains("after"));
    }
};

QTEST_MAIN(TestLogger)
#include "test_logger.moc"
