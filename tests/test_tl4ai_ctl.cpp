#include <QtTest>
#include <QProcess>
#include <QTemporaryDir>
#include <QFile>
#include <QCoreApplication>
#include "IpcServer.h"
#include "StateManager.h"

class TestTl4aiCtl : public QObject {
    Q_OBJECT

private:
    QString m_ctlPath;
    QString m_appPath;
    QTemporaryDir m_tempDir;

    QString socketPath() const
    {
        return m_tempDir.path() + "/test.sock";
    }

    int runBinary(const QString &binary, const QStringList &args, const QString &sockPath)
    {
        QProcess proc;
        QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
        env.insert("TL4AI_SOCKET", sockPath);
        proc.setProcessEnvironment(env);
        proc.start(binary, args);
        proc.waitForFinished(2000);
        return proc.exitCode();
    }

    int runCtl(const QStringList &args, const QString &sockPath)
    {
        return runBinary(m_ctlPath, args, sockPath);
    }

    int runAppCommand(const QStringList &args, const QString &sockPath)
    {
        return runBinary(m_appPath, args, sockPath);
    }

private slots:
    void initTestCase()
    {
        // Find the tl4ai-ctl and GUI binaries relative to the test binary.
        m_ctlPath = QCoreApplication::applicationDirPath() + "/../tools/tl4ai-ctl";
        m_appPath = QCoreApplication::applicationDirPath() + "/../src/trafficlight4ai";
        if (!QFile::exists(m_ctlPath)) {
            QSKIP("tl4ai-ctl binary not found, build it first");
        }
    }

    void sendsRedCommand()
    {
        StateManager sm;
        IpcServer server(&sm, socketPath());

        QCOMPARE(runCtl({"red"}, socketPath()), 0);
        QTRY_COMPARE_WITH_TIMEOUT(sm.state(), LightState::Working, 2000);
    }

    void sendsYellowCommand()
    {
        StateManager sm;
        IpcServer server(&sm, socketPath());

        QCOMPARE(runCtl({"yellow"}, socketPath()), 0);
        QTRY_COMPARE_WITH_TIMEOUT(sm.state(), LightState::WaitingConfirm, 2000);
    }

    void sendsGreenCommand()
    {
        StateManager sm;
        IpcServer server(&sm, socketPath());

        sm.setState(LightState::Working);
        QCOMPARE(runCtl({"green"}, socketPath()), 0);
        QTRY_COMPARE_WITH_TIMEOUT(sm.state(), LightState::Idle, 2000);
    }

    void exitZeroWhenServerNotRunning()
    {
        // No server listening - should still exit 0 silently
        QCOMPARE(runCtl({"red"}, m_tempDir.path() + "/no_server_here.sock"), 0);
    }

    void exitZeroWithNoArgs()
    {
        // No arguments - should exit 0 without crashing
        QCOMPARE(runCtl({}, socketPath()), 0);
    }

    void unknownCommandIgnored()
    {
        StateManager sm;
        IpcServer server(&sm, socketPath());

        QCOMPARE(runCtl({"purple"}, socketPath()), 0);
        QTRY_VERIFY_WITH_TIMEOUT(true, 500);
        QCOMPARE(sm.state(), LightState::Idle);
    }

    void mixedCaseCommand()
    {
        StateManager sm;
        IpcServer server(&sm, socketPath());

        QCOMPARE(runCtl({"Red"}, socketPath()), 0);
        QTRY_COMPARE_WITH_TIMEOUT(sm.state(), LightState::Working, 2000);
    }

    // The GUI binary, when invoked as `trafficlight4ai red`, must forward the
    // command over IPC just like tl4ai-ctl (this is how AppImage hooks work).
    void appExecutableCanSendCommand()
    {
        if (!QFile::exists(m_appPath))
            QSKIP("trafficlight4ai binary not found, build it first");

        StateManager sm;
        IpcServer server(&sm, socketPath());

        QCOMPARE(runAppCommand({"red"}, socketPath()), 0);
        QTRY_COMPARE_WITH_TIMEOUT(sm.state(), LightState::Working, 2000);
    }
};

QTEST_MAIN(TestTl4aiCtl)
#include "test_tl4ai_ctl.moc"
