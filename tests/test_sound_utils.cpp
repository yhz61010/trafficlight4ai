#include <QtTest>
#include <QCoreApplication>
#include <QTemporaryDir>
#include <QFile>
#include "SoundUtils.h"

class TestSoundUtils : public QObject {
    Q_OBJECT

private slots:
    void emptyPathReturnsInvalidUrl()
    {
        QVERIFY(!soundUrlForPath(QString()).isValid());
    }

    void missingLocalFileReturnsInvalidUrl()
    {
        QVERIFY(!soundUrlForPath("/tmp/trafficlight4ai-missing-sound.ogg").isValid());
    }

    void resourcePathReturnsQrcUrl()
    {
        // URL mapping is pure path logic; this test binary does not embed the app qrc resources.
        QCOMPARE(soundUrlForPath(":/effects/effects/yellow.ogg"),
                 QUrl("qrc:/effects/effects/yellow.ogg"));
    }

    void localFileReturnsFileUrl()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        const QString path = dir.path() + "/sound.ogg";
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("not real audio");
        file.close();

        QCOMPARE(soundUrlForPath(path), QUrl::fromLocalFile(path));
    }

    void playSoundFallsBackForInvalidPath()
    {
        playSound(QString());
        playSound("/tmp/trafficlight4ai-missing-playback.ogg");
        QCoreApplication::processEvents();
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    }

    void playSoundFallbackCanBeCalledRepeatedly()
    {
        playSound(QString());
        playSound(QString());
        playSound("/tmp/trafficlight4ai-missing-playback.ogg");
        QCoreApplication::processEvents();
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    }

};

QTEST_MAIN(TestSoundUtils)
#include "test_sound_utils.moc"
