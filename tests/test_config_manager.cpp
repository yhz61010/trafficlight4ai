#include <QtTest>
#include <QTemporaryDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <unistd.h>
#include "ConfigManager.h"

struct EnvGuard {
    QByteArray key, original;
    bool had;
    explicit EnvGuard(const char *k) : key(k), had(qEnvironmentVariableIsSet(k)), original(qgetenv(k)) {
        qunsetenv(k);
    }
    ~EnvGuard() { had ? qputenv(key.constData(), original) : qunsetenv(key.constData()); }
};

class TestConfigManager : public QObject {
    Q_OBJECT

private:
    QTemporaryDir m_tempDir;
    int m_testIndex = 0;
    QString m_configPath;

    QString expectedDefaultSocketPath() const
    {
        const QByteArray runtimeDir = qgetenv("XDG_RUNTIME_DIR");
        if (!runtimeDir.isEmpty())
            return QString::fromLocal8Bit(runtimeDir) + "/trafficlight4ai.sock";
#ifdef Q_OS_MACOS
        const QByteArray tmpDir = qgetenv("TMPDIR");
        if (!tmpDir.isEmpty()) {
            QString dir = QString::fromLocal8Bit(tmpDir);
            if (!dir.endsWith('/'))
                dir += '/';
            return dir + "trafficlight4ai.sock";
        }
#endif
        return QString("/tmp/trafficlight4ai-%1.sock").arg(getuid());
    }

private slots:
    void init()
    {
        m_configPath = m_tempDir.path() + "/config_" + QString::number(m_testIndex++) + ".json";
    }
    void createsDefaultConfigWhenMissing()
    {
        ConfigManager cm(m_configPath);
        QVERIFY(QFile::exists(m_configPath));
    }

    void defaultWindowSize()
    {
        ConfigManager cm(m_configPath);
        QCOMPARE(cm.windowSize(), QString("medium"));
    }

    void defaultWindowPosition()
    {
        ConfigManager cm(m_configPath);
        QCOMPARE(cm.windowPosX(), 20);
        QCOMPARE(cm.windowPosY(), 20);
    }

    void defaultAnimationMode()
    {
        ConfigManager cm(m_configPath);
        QCOMPARE(cm.animationMode(), QString("breathing"));
    }

    void defaultAnimationPeriod()
    {
        ConfigManager cm(m_configPath);
        QCOMPARE(cm.animationPeriodMs(), 1000);
    }

    void defaultSocketPath()
    {
        ConfigManager cm(m_configPath);
        QCOMPARE(cm.socketPath(), expectedDefaultSocketPath());
    }

    void socketPathEnvVarOverride()
    {
        qputenv("TL4AI_SOCKET", "/tmp/custom_tl4ai.sock");
        ConfigManager cm(m_configPath);
        QCOMPARE(cm.socketPath(), QString("/tmp/custom_tl4ai.sock"));
        qunsetenv("TL4AI_SOCKET");
    }

    void defaultSocketPathWithoutXdgRuntimeDir()
    {
        EnvGuard guard("XDG_RUNTIME_DIR");

        ConfigManager cm(m_configPath);
        // On macOS, $TMPDIR takes over; on Linux, falls back to /tmp
        QCOMPARE(cm.socketPath(), expectedDefaultSocketPath());
    }

    void setAndGetWindowSize()
    {
        ConfigManager cm(m_configPath);
        cm.setWindowSize("large");
        QCOMPARE(cm.windowSize(), QString("large"));
    }

    void setAndGetWindowPosition()
    {
        ConfigManager cm(m_configPath);
        cm.setWindowPos(100, 200);
        QCOMPARE(cm.windowPosX(), 100);
        QCOMPARE(cm.windowPosY(), 200);
    }

    void setAndGetAnimationMode()
    {
        ConfigManager cm(m_configPath);
        cm.setAnimationMode("classic");
        QCOMPARE(cm.animationMode(), QString("classic"));
    }

    void setAndGetAnimationPeriod()
    {
        ConfigManager cm(m_configPath);
        cm.setAnimationPeriodMs(500);
        QCOMPARE(cm.animationPeriodMs(), 500);
    }

    void persistsAfterSave()
    {
        {
            ConfigManager cm(m_configPath);
            cm.setWindowSize("medium");
            cm.setWindowPos(50, 60);
            cm.setAnimationMode("classic");
            cm.setAnimationPeriodMs(2000);
        }
        // Reload
        ConfigManager cm2(m_configPath);
        QCOMPARE(cm2.windowSize(), QString("medium"));
        QCOMPARE(cm2.windowPosX(), 50);
        QCOMPARE(cm2.windowPosY(), 60);
        QCOMPARE(cm2.animationMode(), QString("classic"));
        QCOMPARE(cm2.animationPeriodMs(), 2000);
    }

    void corruptFileFallsBackToDefaults()
    {
        // Write garbage to config file
        QFile f(m_configPath);
        f.open(QIODevice::WriteOnly);
        f.write("not json {{{");
        f.close();

        ConfigManager cm(m_configPath);
        QCOMPARE(cm.windowSize(), QString("medium"));
        QCOMPARE(cm.animationMode(), QString("breathing"));
    }

    void clampsAnimationPeriod()
    {
        ConfigManager cm(m_configPath);
        cm.setAnimationPeriodMs(50);   // below min 200
        QCOMPARE(cm.animationPeriodMs(), 200);
        cm.setAnimationPeriodMs(9999); // above max 5000
        QCOMPARE(cm.animationPeriodMs(), 5000);
    }

    void rejectsInvalidWindowSize()
    {
        ConfigManager cm(m_configPath);
        cm.setWindowSize("huge"); // invalid
        QCOMPARE(cm.windowSize(), QString("medium")); // unchanged default
    }

    void rejectsInvalidAnimationMode()
    {
        ConfigManager cm(m_configPath);
        cm.setAnimationMode("rainbow"); // invalid
        QCOMPARE(cm.animationMode(), QString("breathing")); // unchanged default
    }

    void setAndGetSocketPath()
    {
        ConfigManager cm(m_configPath);
        cm.setSocketPath("/tmp/custom.sock");
        QCOMPARE(cm.socketPath(), QString("/tmp/custom.sock"));
    }

    void socketPathPersists()
    {
        {
            ConfigManager cm(m_configPath);
            cm.setSocketPath("/tmp/persist.sock");
        }
        ConfigManager cm2(m_configPath);
        QCOMPARE(cm2.socketPath(), QString("/tmp/persist.sock"));
    }

    void rejectsEmptySocketPath()
    {
        ConfigManager cm(m_configPath);
        const QString originalPath = cm.socketPath();
        cm.setSocketPath("");
        QCOMPARE(cm.socketPath(), originalPath); // unchanged
    }

    void defaultAiTool()
    {
        ConfigManager cm(m_configPath);
        QCOMPARE(cm.aiTool(), QString("codex"));
    }

    void setAndGetAiTool()
    {
        ConfigManager cm(m_configPath);
        cm.setAiTool("claude-code");
        QCOMPARE(cm.aiTool(), QString("claude-code"));
    }

    void aiToolPersists()
    {
        {
            ConfigManager cm(m_configPath);
            cm.setAiTool("claude-code");
        }
        ConfigManager cm2(m_configPath);
        QCOMPARE(cm2.aiTool(), QString("claude-code"));
    }

    void defaultTimeoutSec()
    {
        ConfigManager cm(m_configPath);
        QCOMPARE(cm.timeoutSec(), 300);
    }

    void setAndGetTimeoutSec()
    {
        ConfigManager cm(m_configPath);
        cm.setTimeoutSec(600);
        QCOMPARE(cm.timeoutSec(), 600);
    }

    void timeoutSecZeroDisables()
    {
        ConfigManager cm(m_configPath);
        cm.setTimeoutSec(0);
        QCOMPARE(cm.timeoutSec(), 0);
    }

    void clampsTimeoutSec()
    {
        ConfigManager cm(m_configPath);
        cm.setTimeoutSec(10); // below min 30, not 0
        QCOMPARE(cm.timeoutSec(), 30);
        cm.setTimeoutSec(9999); // above max 3600
        QCOMPARE(cm.timeoutSec(), 3600);
    }

    void timeoutSecPersists()
    {
        {
            ConfigManager cm(m_configPath);
            cm.setTimeoutSec(120);
        }
        ConfigManager cm2(m_configPath);
        QCOMPARE(cm2.timeoutSec(), 120);
    }

    void normalizeInvalidWindowSize()
    {
        // Write valid JSON with invalid window.size
        QFile f(m_configPath);
        f.open(QIODevice::WriteOnly);
        f.write(R"({"window":{"size":"huge","posX":20,"posY":20}})");
        f.close();

        ConfigManager cm(m_configPath);
        QCOMPARE(cm.windowSize(), QString("medium")); // normalized to default
    }

    void normalizeInvalidAnimationMode()
    {
        QFile f(m_configPath);
        f.open(QIODevice::WriteOnly);
        f.write(R"({"animation":{"mode":"rainbow","periodMs":1000}})");
        f.close();

        ConfigManager cm(m_configPath);
        QCOMPARE(cm.animationMode(), QString("breathing"));
    }

    void normalizeOutOfRangeAnimationPeriod()
    {
        QFile f(m_configPath);
        f.open(QIODevice::WriteOnly);
        f.write(R"({"animation":{"mode":"breathing","periodMs":99999}})");
        f.close();

        ConfigManager cm(m_configPath);
        QCOMPARE(cm.animationPeriodMs(), 5000); // clamped
    }

    void normalizeNegativeTimeout()
    {
        QFile f(m_configPath);
        f.open(QIODevice::WriteOnly);
        f.write(R"({"timeoutSec":-100})");
        f.close();

        ConfigManager cm(m_configPath);
        QCOMPARE(cm.timeoutSec(), 30); // clamped to min
    }

    void normalizeLegacyDefaultSocketPath()
    {
        EnvGuard guard("TL4AI_SOCKET");

        QFile f(m_configPath);
        f.open(QIODevice::WriteOnly);
        f.write(R"({"socket":{"path":"/tmp/trafficlight4ai.sock"}})");
        f.close();

        ConfigManager cm(m_configPath);
        QCOMPARE(cm.socketPath(), expectedDefaultSocketPath());
    }

    void defaultLanguage()
    {
        ConfigManager cm(m_configPath);
        QCOMPARE(cm.language(), QString("en"));
    }

    void setAndGetLanguage()
    {
        ConfigManager cm(m_configPath);
        cm.setLanguage("zh");
        QCOMPARE(cm.language(), QString("zh"));
    }

    void languagePersists()
    {
        {
            ConfigManager cm(m_configPath);
            cm.setLanguage("ja");
        }
        ConfigManager cm2(m_configPath);
        QCOMPARE(cm2.language(), QString("ja"));
    }

    void defaultYellowSoundEnabled()
    {
        ConfigManager cm(m_configPath);
        QCOMPARE(cm.yellowSoundEnabled(), true);
    }

    void setAndGetYellowSoundEnabled()
    {
        ConfigManager cm(m_configPath);
        cm.setYellowSoundEnabled(false);
        QCOMPARE(cm.yellowSoundEnabled(), false);
    }

    void defaultGreenSoundEnabled()
    {
        ConfigManager cm(m_configPath);
        QCOMPARE(cm.greenSoundEnabled(), true);
    }

    void setAndGetGreenSoundEnabled()
    {
        ConfigManager cm(m_configPath);
        cm.setGreenSoundEnabled(false);
        QCOMPARE(cm.greenSoundEnabled(), false);
    }

    void defaultYellowSoundFile()
    {
        ConfigManager cm(m_configPath);
        QCOMPARE(cm.yellowSoundFile(), QString());
    }

    void setAndGetYellowSoundFile()
    {
        ConfigManager cm(m_configPath);
        cm.setYellowSoundFile("/tmp/alert.wav");
        QCOMPARE(cm.yellowSoundFile(), QString("/tmp/alert.wav"));
    }

    void defaultGreenSoundFile()
    {
        ConfigManager cm(m_configPath);
        QCOMPARE(cm.greenSoundFile(), QString());
    }

    void setAndGetGreenSoundFile()
    {
        ConfigManager cm(m_configPath);
        cm.setGreenSoundFile("/tmp/done.ogg");
        QCOMPARE(cm.greenSoundFile(), QString("/tmp/done.ogg"));
    }

    void soundSettingsPersist()
    {
        {
            ConfigManager cm(m_configPath);
            cm.setYellowSoundEnabled(false);
            cm.setYellowSoundFile("/tmp/y.wav");
            cm.setGreenSoundEnabled(false);
            cm.setGreenSoundFile("/tmp/g.mp3");
        }
        ConfigManager cm2(m_configPath);
        QCOMPARE(cm2.yellowSoundEnabled(), false);
        QCOMPARE(cm2.yellowSoundFile(), QString("/tmp/y.wav"));
        QCOMPARE(cm2.greenSoundEnabled(), false);
        QCOMPARE(cm2.greenSoundFile(), QString("/tmp/g.mp3"));
    }
    void defaultLoggingEnabled()
    {
        ConfigManager cm(m_configPath);
        QCOMPARE(cm.loggingEnabled(), true);
    }

    void defaultLogLevel()
    {
        ConfigManager cm(m_configPath);
        QCOMPARE(cm.logLevel(), QString("warn"));
    }

    void setAndGetLoggingEnabled()
    {
        ConfigManager cm(m_configPath);
        cm.setLoggingEnabled(false);
        QCOMPARE(cm.loggingEnabled(), false);
    }

    void setAndGetLogLevel()
    {
        ConfigManager cm(m_configPath);
        cm.setLogLevel("debug");
        QCOMPARE(cm.logLevel(), QString("debug"));
    }

    void rejectsInvalidLogLevel()
    {
        ConfigManager cm(m_configPath);
        cm.setLogLevel("info");
        cm.setLogLevel("bogus"); // ignored, keeps previous valid value
        QCOMPARE(cm.logLevel(), QString("info"));
    }

    void loggingSettingsPersist()
    {
        {
            ConfigManager cm(m_configPath);
            cm.setLoggingEnabled(false);
            cm.setLogLevel("error");
        }
        ConfigManager cm2(m_configPath);
        QCOMPARE(cm2.loggingEnabled(), false);
        QCOMPARE(cm2.logLevel(), QString("error"));
    }

    void normalizeInvalidLogLevel()
    {
        {
            QFile f(m_configPath);
            QVERIFY(f.open(QIODevice::WriteOnly));
            f.write(R"({"logging":{"enabled":true,"level":"nonsense"}})");
            f.close();
        }
        ConfigManager cm(m_configPath);
        QCOMPARE(cm.logLevel(), QString("warn"));
    }

    void batchSaveSuppressesWrites()
    {
        ConfigManager cm(m_configPath);
        cm.beginBatchSave();
        cm.setWindowSize("large");
        cm.setAnimationMode("classic");
        cm.endBatchSave();

        // Reload and verify changes persisted (endBatchSave triggers save)
        ConfigManager cm2(m_configPath);
        QCOMPARE(cm2.windowSize(), QString("large"));
        QCOMPARE(cm2.animationMode(), QString("classic"));
    }

    void batchSaveWritesOnEnd()
    {
        {
            ConfigManager cm(m_configPath);
            cm.beginBatchSave();
            cm.setLanguage("ja");
            cm.setTimeoutSec(120);
            // Destroy without endBatchSave — save() is suppressed by m_batchSave
        }
        // Changes should NOT persist because batch was never committed
        ConfigManager cm2(m_configPath);
        QCOMPARE(cm2.language(), QString("en"));   // unchanged
        QCOMPARE(cm2.timeoutSec(), 300);           // unchanged
    }
};

QTEST_MAIN(TestConfigManager)
#include "test_config_manager.moc"
