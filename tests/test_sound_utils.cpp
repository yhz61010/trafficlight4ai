#include <QtTest>
#include <QCoreApplication>
#include <QDir>
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

    void appImageGStreamerEnvironmentIsIsolated()
    {
        QTemporaryDir appDir;
        QVERIFY(appDir.isValid());

        QDir dir(appDir.path());
        QVERIFY(dir.mkpath("usr/lib/gstreamer-1.0"));
        QVERIFY(dir.mkpath("usr/libexec/gstreamer-1.0"));

        QFile scanner(appDir.path() + "/usr/libexec/gstreamer-1.0/gst-plugin-scanner");
        QVERIFY(scanner.open(QIODevice::WriteOnly));
        scanner.close();

        qputenv("APPDIR", QFile::encodeName(appDir.path()));
        qputenv("GST_PLUGIN_PATH", "/host/plugins");
        qputenv("GST_PLUGIN_PATH_1_0", "/host/plugins");
        qputenv("GST_PLUGIN_SYSTEM_PATH_1_0", "/usr/lib/host-gstreamer");
        qputenv("GST_PLUGIN_SCANNER", "/host/scanner");

        initBundledGStreamerPlugins();

        const QByteArray expectedPluginPath =
            QFile::encodeName(appDir.path() + "/usr/lib/gstreamer-1.0");
        QCOMPARE(qgetenv("GST_PLUGIN_PATH"), expectedPluginPath);
        QCOMPARE(qgetenv("GST_PLUGIN_PATH_1_0"), expectedPluginPath);
        QCOMPARE(qgetenv("GST_PLUGIN_SYSTEM_PATH_1_0"), QByteArray());
        QCOMPARE(qgetenv("GST_PLUGIN_SCANNER"),
                 QFile::encodeName(appDir.path() + "/usr/libexec/gstreamer-1.0/gst-plugin-scanner"));
        QVERIFY(qgetenv("GST_REGISTRY_1_0").contains("trafficlight4ai-gst-registry"));
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
