#include "ConfigManager.h"
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QtGlobal>
#ifndef Q_OS_WIN
#include <unistd.h>
#endif
#include <algorithm>

static const QStringList kValidSizes = {"xsmall", "small", "medium", "large", "xlarge"};
static const QStringList kValidModes = {"breathing", "classic"};
static const QStringList kValidLogLevels = {"verb", "debug", "info", "warn", "error"};
static const QString kLegacyDefaultSocketPath = "/tmp/trafficlight4ai.sock";

static QString defaultSocketPath()
{
#ifdef Q_OS_WIN
    return "trafficlight4ai";
#else
    const QByteArray runtimeDir = qgetenv("XDG_RUNTIME_DIR");
    if (!runtimeDir.isEmpty())
        return QString::fromLocal8Bit(runtimeDir) + "/trafficlight4ai.sock";
#ifdef Q_OS_MACOS
    // macOS sets $TMPDIR to a per-user directory (e.g. /var/folders/.../T/)
    const QByteArray tmpDir = qgetenv("TMPDIR");
    if (!tmpDir.isEmpty()) {
        QString dir = QString::fromLocal8Bit(tmpDir);
        if (!dir.endsWith('/'))
            dir += '/';
        return dir + "trafficlight4ai.sock";
    }
#endif
    return QString("/tmp/trafficlight4ai-%1.sock").arg(getuid());
#endif
}

ConfigManager::ConfigManager(const QString &configPath, QObject *parent)
    : QObject(parent), m_configPath(configPath)
{
    load();
}

ConfigManager::~ConfigManager()
{
    save();
}

void ConfigManager::applyDefaults()
{
    QJsonObject window;
    window["size"] = "medium";
    window["posX"] = 20;
    window["posY"] = 20;

    QJsonObject animation;
    animation["mode"] = "breathing";
    animation["periodMs"] = 1000;

    QJsonObject socket;
    socket["path"] = defaultSocketPath();

    QJsonObject sound;
    sound["yellowEnabled"] = true;
    sound["greenEnabled"] = true;
    sound["yellowFile"] = QString();
    sound["greenFile"] = QString();

    QJsonObject logging;
    logging["enabled"] = true;
    logging["level"] = "warn";

    m_root["window"] = window;
    m_root["animation"] = animation;
    m_root["socket"] = socket;
    m_root["sound"] = sound;
    m_root["logging"] = logging;
    m_root["aiTool"] = "codex";
    m_root["timeoutSec"] = 300;
    m_root["language"] = "en";
}

void ConfigManager::load()
{
    bool loaded = false;
    QFile file(m_configPath);

    if (file.exists() && file.open(QIODevice::ReadOnly)) {
        QJsonParseError err;
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &err);
        file.close();

        if (err.error == QJsonParseError::NoError && doc.isObject()) {
            // Apply defaults first, then recursively merge loaded values
            applyDefaults();
            const QJsonObject obj = doc.object();
            for (auto it = obj.begin(); it != obj.end(); ++it) {
                if (it.value().isObject() && m_root[it.key()].isObject()) {
                    // Merge nested object: loaded keys override, default keys preserved
                    QJsonObject merged = m_root[it.key()].toObject();
                    const QJsonObject loaded = it.value().toObject();
                    for (auto sit = loaded.begin(); sit != loaded.end(); ++sit)
                        merged[sit.key()] = sit.value();
                    m_root[it.key()] = merged;
                } else {
                    m_root[it.key()] = it.value();
                }
            }
            loaded = true;
        }
    }

    if (!loaded) {
        applyDefaults();
        normalize();
        save(); // create config file on first run or corrupt file
    } else {
        normalize(); // only saves if values were corrected
    }
}

void ConfigManager::save()
{
    if (m_batchSave)
        return;

    QDir dir = QFileInfo(m_configPath).absoluteDir();
    if (!dir.exists())
        dir.mkpath(".");

    QFile file(m_configPath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(m_root).toJson(QJsonDocument::Indented));
        file.close();
    }
}

QString ConfigManager::windowSize() const
{
    return m_root["window"].toObject()["size"].toString("medium");
}

void ConfigManager::setWindowSize(const QString &size)
{
    if (!kValidSizes.contains(size))
        return;
    QJsonObject window = m_root["window"].toObject();
    window["size"] = size;
    m_root["window"] = window;
    save();
}

int ConfigManager::windowPosX() const
{
    return m_root["window"].toObject()["posX"].toInt(20);
}

int ConfigManager::windowPosY() const
{
    return m_root["window"].toObject()["posY"].toInt(20);
}

void ConfigManager::setWindowPos(int x, int y)
{
    QJsonObject window = m_root["window"].toObject();
    window["posX"] = x;
    window["posY"] = y;
    m_root["window"] = window;
    save();
}

QString ConfigManager::animationMode() const
{
    return m_root["animation"].toObject()["mode"].toString("breathing");
}

void ConfigManager::setAnimationMode(const QString &mode)
{
    if (!kValidModes.contains(mode))
        return;
    QJsonObject animation = m_root["animation"].toObject();
    animation["mode"] = mode;
    m_root["animation"] = animation;
    save();
}

int ConfigManager::animationPeriodMs() const
{
    return m_root["animation"].toObject()["periodMs"].toInt(1000);
}

void ConfigManager::setAnimationPeriodMs(int ms)
{
    ms = std::clamp(ms, 200, 5000);
    QJsonObject animation = m_root["animation"].toObject();
    animation["periodMs"] = ms;
    m_root["animation"] = animation;
    save();
}

QString ConfigManager::socketPath() const
{
    const QByteArray envPath = qgetenv("TL4AI_SOCKET");
    if (!envPath.isEmpty())
        return QString::fromLocal8Bit(envPath);
    return m_root["socket"].toObject()["path"].toString(defaultSocketPath());
}

void ConfigManager::setSocketPath(const QString &path)
{
    if (path.isEmpty())
        return;
    QJsonObject socket = m_root["socket"].toObject();
    socket["path"] = path;
    m_root["socket"] = socket;
    save();
}

QString ConfigManager::aiTool() const
{
    return m_root["aiTool"].toString("codex");
}

void ConfigManager::setAiTool(const QString &tool)
{
    m_root["aiTool"] = tool;
    save();
}

int ConfigManager::timeoutSec() const
{
    return m_root["timeoutSec"].toInt(300);
}

void ConfigManager::setTimeoutSec(int sec)
{
    if (sec != 0)
        sec = std::clamp(sec, 30, 3600);
    m_root["timeoutSec"] = sec;
    save();
}

QString ConfigManager::language() const
{
    return m_root["language"].toString("en");
}

void ConfigManager::setLanguage(const QString &lang)
{
    m_root["language"] = lang;
    save();
}

bool ConfigManager::yellowSoundEnabled() const
{
    return m_root["sound"].toObject()["yellowEnabled"].toBool(true);
}

void ConfigManager::setYellowSoundEnabled(bool enabled)
{
    QJsonObject sound = m_root["sound"].toObject();
    sound["yellowEnabled"] = enabled;
    m_root["sound"] = sound;
    save();
}

QString ConfigManager::yellowSoundFile() const
{
    return m_root["sound"].toObject()["yellowFile"].toString();
}

void ConfigManager::setYellowSoundFile(const QString &path)
{
    QJsonObject sound = m_root["sound"].toObject();
    sound["yellowFile"] = path;
    m_root["sound"] = sound;
    save();
}

bool ConfigManager::greenSoundEnabled() const
{
    return m_root["sound"].toObject()["greenEnabled"].toBool(true);
}

void ConfigManager::setGreenSoundEnabled(bool enabled)
{
    QJsonObject sound = m_root["sound"].toObject();
    sound["greenEnabled"] = enabled;
    m_root["sound"] = sound;
    save();
}

QString ConfigManager::greenSoundFile() const
{
    return m_root["sound"].toObject()["greenFile"].toString();
}

void ConfigManager::setGreenSoundFile(const QString &path)
{
    QJsonObject sound = m_root["sound"].toObject();
    sound["greenFile"] = path;
    m_root["sound"] = sound;
    save();
}

bool ConfigManager::loggingEnabled() const
{
    return m_root["logging"].toObject()["enabled"].toBool(true);
}

void ConfigManager::setLoggingEnabled(bool enabled)
{
    QJsonObject logging = m_root["logging"].toObject();
    logging["enabled"] = enabled;
    m_root["logging"] = logging;
    save();
}

QString ConfigManager::logLevel() const
{
    return m_root["logging"].toObject()["level"].toString("warn");
}

void ConfigManager::setLogLevel(const QString &level)
{
    if (!kValidLogLevels.contains(level))
        return;
    QJsonObject logging = m_root["logging"].toObject();
    logging["level"] = level;
    m_root["logging"] = logging;
    save();
}

void ConfigManager::beginBatchSave()
{
    m_batchSave = true;
}

void ConfigManager::endBatchSave()
{
    m_batchSave = false;
    save();
}

void ConfigManager::normalize()
{
    const QJsonObject before = m_root;

    // Validate window.size
    QJsonObject window = m_root["window"].toObject();
    if (!kValidSizes.contains(window["size"].toString()))
        window["size"] = "medium";
    m_root["window"] = window;

    // Validate animation.mode and animation.periodMs
    QJsonObject animation = m_root["animation"].toObject();
    if (!kValidModes.contains(animation["mode"].toString()))
        animation["mode"] = "breathing";
    int periodMs = animation["periodMs"].toInt(1000);
    animation["periodMs"] = std::clamp(periodMs, 200, 5000);
    m_root["animation"] = animation;

    // Validate timeoutSec
    int timeout = m_root["timeoutSec"].toInt(300);
    if (timeout != 0)
        timeout = std::clamp(timeout, 30, 3600);
    m_root["timeoutSec"] = timeout;

    // Validate socket.path
    QJsonObject socket = m_root["socket"].toObject();
    const QString socketPath = socket["path"].toString();
    if (socketPath.isEmpty()
        || (socketPath == kLegacyDefaultSocketPath && qgetenv("TL4AI_SOCKET").isEmpty()))
        socket["path"] = defaultSocketPath();
    m_root["socket"] = socket;

    // Validate logging.level
    QJsonObject logging = m_root["logging"].toObject();
    if (!kValidLogLevels.contains(logging["level"].toString()))
        logging["level"] = "warn";
    m_root["logging"] = logging;

    if (m_root != before)
        save();
}
