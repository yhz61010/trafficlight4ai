#include "SoundUtils.h"
#include <QApplication>
#include <QFile>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QMessageBox>
#include <QPointer>
#include <QUrl>

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

        auto *cleaned = new bool(false);
        QObject::connect(player, &QObject::destroyed, player, [cleaned]() { delete cleaned; });

        QPointer<QObject> ctx(errorContext);

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
