#pragma once

#include <QObject>
#include <QJsonObject>
#include <QString>

class ConfigManager : public QObject {
    Q_OBJECT

public:
    explicit ConfigManager(const QString &configPath, QObject *parent = nullptr);
    ~ConfigManager() override;

    QString windowSize() const;
    void setWindowSize(const QString &size);

    int windowPosX() const;
    int windowPosY() const;
    void setWindowPos(int x, int y);

    QString animationMode() const;
    void setAnimationMode(const QString &mode);

    int animationPeriodMs() const;
    void setAnimationPeriodMs(int ms);

    QString socketPath() const;
    void setSocketPath(const QString &path);

    QString aiTool() const;
    void setAiTool(const QString &tool);

    int timeoutSec() const;
    void setTimeoutSec(int sec);

    QString language() const;
    void setLanguage(const QString &lang);

    bool yellowSoundEnabled() const;
    void setYellowSoundEnabled(bool enabled);
    QString yellowSoundFile() const;
    void setYellowSoundFile(const QString &path);

    bool greenSoundEnabled() const;
    void setGreenSoundEnabled(bool enabled);
    QString greenSoundFile() const;
    void setGreenSoundFile(const QString &path);

    bool loggingEnabled() const;
    void setLoggingEnabled(bool enabled);
    QString logLevel() const;
    void setLogLevel(const QString &level);

    void beginBatchSave();
    void endBatchSave();

private:
    void load();
    void save();
    void applyDefaults();
    void normalize();

    QString m_configPath;
    QJsonObject m_root;
    bool m_batchSave = false;
};
