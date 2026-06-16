#include "SoundUtils.h"
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QMessageBox>
#include <QPointer>
#include <QSharedPointer>
#include <QUrl>

namespace {

void setEnvPath(const char *name, const QByteArray &path)
{
    qputenv(name, path);
}

}

void initBundledGStreamerPlugins()
{
    static bool initialized = false;
    if (initialized)
        return;
    initialized = true;

    const QByteArray appDir = qgetenv("APPDIR");
    if (appDir.isEmpty())
        return;

    const QString pluginDir = QString::fromLocal8Bit(appDir) + "/usr/lib/gstreamer-1.0";
    if (!QDir(pluginDir).exists())
        return;

    const QByteArray encodedPluginDir = QFile::encodeName(pluginDir);
    setEnvPath("GST_PLUGIN_PATH", encodedPluginDir);
    setEnvPath("GST_PLUGIN_PATH_1_0", encodedPluginDir);
    qputenv("GST_PLUGIN_SYSTEM_PATH_1_0", QByteArray());

    const QString scanner = QString::fromLocal8Bit(appDir)
        + "/usr/libexec/gstreamer-1.0/gst-plugin-scanner";
    if (QFile::exists(scanner))
        qputenv("GST_PLUGIN_SCANNER", QFile::encodeName(scanner));

    const QString registry = QDir::tempPath() + "/trafficlight4ai-gst-registry.bin";
    qputenv("GST_REGISTRY_1_0", QFile::encodeName(registry));
}

QUrl soundUrlForPath(const QString &filePath)
{
    if (filePath.isEmpty())
        return {};

    if (filePath.startsWith(QLatin1String(":/")))
        return QUrl("qrc" + filePath);

    if (QFile::exists(filePath))
        return QUrl::fromLocalFile(filePath);

    return {};
}

void playSound(const QString &filePath, QObject *errorContext)
{
    const QUrl url = soundUrlForPath(filePath);
    if (url.isValid()) {
        auto *player = new QMediaPlayer();
        auto *audioOutput = new QAudioOutput(player);
        player->setAudioOutput(audioOutput);
        player->setSource(url);

        QPointer<QObject> ctx(errorContext);
        auto cleaned = QSharedPointer<bool>::create(false);

        auto cleanup = [player, cleaned]() {
            if (*cleaned)
                return;
            *cleaned = true;
            player->deleteLater();
        };

        QObject::connect(player, &QMediaPlayer::errorOccurred,
                         player, [cleanup, ctx, filePath]
                         (QMediaPlayer::Error, const QString &) {
            QApplication::beep();
            if (ctx) {
                auto *widget = qobject_cast<QWidget *>(ctx.data());
                QMessageBox::warning(widget,
                    QObject::tr("Audio Error"),
                    QObject::tr("Failed to play: %1").arg(filePath));
            }
            cleanup();
        });

        QObject::connect(player, &QMediaPlayer::playbackStateChanged,
                         player, [cleanup](QMediaPlayer::PlaybackState state) {
            if (state == QMediaPlayer::StoppedState)
                cleanup();
        });

        player->play();
    } else {
        QApplication::beep();
    }
}
