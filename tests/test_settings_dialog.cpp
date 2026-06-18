#include <QtTest>
#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSignalSpy>
#include <QSpinBox>
#include <QSlider>
#include <QTemporaryDir>
#include <QTextEdit>
#include <QTimer>
#include "AiToolStrategy.h"
#include "ConfigManager.h"
#include "IpcServer.h"
#include "SettingsDialog.h"
#include "StateManager.h"
#include "TrafficLightWidget.h"

class EnvGuard {
public:
    explicit EnvGuard(const char *key, const QByteArray &value)
        : m_key(key), m_had(qEnvironmentVariableIsSet(key)), m_original(qgetenv(key))
    {
        qputenv(key, value);
    }

    ~EnvGuard()
    {
        if (m_had)
            qputenv(m_key.constData(), m_original);
        else
            qunsetenv(m_key.constData());
    }

private:
    QByteArray m_key;
    bool m_had;
    QByteArray m_original;
};

class TestSettingsDialog : public QObject {
    Q_OBJECT

private:
    QTemporaryDir m_tempDir;
    int m_testIndex = 0;
    bool m_hadSocketEnv = false;
    QByteArray m_originalSocketEnv;

    QString configPath()
    {
        return m_tempDir.path() + "/config_" + QString::number(m_testIndex++) + ".json";
    }

    QString socketPath(const QString &name) const
    {
        return m_tempDir.path() + "/" + name + ".sock";
    }

    template <typename T>
    T *requireChild(QObject *parent, const char *name)
    {
        auto *child = parent->findChild<T *>(name);
        if (!child)
            qFatal("Missing child object: %s", name);
        return child;
    }

    static int indexForData(QComboBox *combo, const QString &value)
    {
        for (int i = 0; i < combo->count(); ++i) {
            if (combo->itemData(i).toString() == value)
                return i;
        }
        return -1;
    }

    static void showAndProcess(QDialog &dialog)
    {
        dialog.show();
        QApplication::processEvents();
    }

private slots:
    void init()
    {
        m_hadSocketEnv = qEnvironmentVariableIsSet("TL4AI_SOCKET");
        m_originalSocketEnv = qgetenv("TL4AI_SOCKET");
        qunsetenv("TL4AI_SOCKET");
    }

    void cleanup()
    {
        if (m_hadSocketEnv)
            qputenv("TL4AI_SOCKET", m_originalSocketEnv);
        else
            qunsetenv("TL4AI_SOCKET");
    }

    void showEventLoadsConfigValues()
    {
        ConfigManager config(configPath());
        config.setLanguage("ja");
        config.setAiTool("gemini");
        config.setTimeoutSec(600);
        config.setWindowSize("large");
        config.setAnimationMode("classic");
        config.setAnimationPeriodMs(2200);
        config.setSocketPath(socketPath("settings_load"));
        config.setYellowSoundEnabled(false);
        config.setYellowSoundFile("/tmp/yellow.ogg");
        config.setGreenSoundEnabled(false);
        config.setGreenSoundFile("/tmp/green.ogg");

        StateManager stateManager;
        IpcServer ipcServer(&stateManager, config.socketPath());
        TrafficLightWidget light;
        SettingsDialog dialog(&config, &light, &ipcServer, &stateManager);
        showAndProcess(dialog);

        QCOMPARE(requireChild<QComboBox>(&dialog, "languageCombo")->currentData().toString(), QString("ja"));
        QCOMPARE(requireChild<QComboBox>(&dialog, "aiToolCombo")->currentData().toString(), QString("gemini"));
        QCOMPARE(requireChild<QSpinBox>(&dialog, "timeoutSpin")->value(), 600);
        const QStringList sizes = {"xsmall", "small", "medium", "large", "xlarge"};
        QCOMPARE(requireChild<QComboBox>(&dialog, "windowSizeCombo")->currentIndex(),
                 sizes.indexOf("large"));
        QCOMPARE(requireChild<QComboBox>(&dialog, "animationModeCombo")->currentIndex(), 1);
        QCOMPARE(requireChild<QSlider>(&dialog, "animationPeriodSlider")->value(), 2200);
        QCOMPARE(requireChild<QSpinBox>(&dialog, "animationPeriodSpin")->value(), 2200);
        QCOMPARE(requireChild<QLineEdit>(&dialog, "socketEdit")->text(), config.socketPath());
        QCOMPARE(requireChild<QCheckBox>(&dialog, "yellowSoundCheck")->isChecked(), false);
        QCOMPARE(requireChild<QLineEdit>(&dialog, "yellowSoundEdit")->text(), QString("/tmp/yellow.ogg"));
        QCOMPARE(requireChild<QCheckBox>(&dialog, "greenSoundCheck")->isChecked(), false);
        QCOMPARE(requireChild<QLineEdit>(&dialog, "greenSoundEdit")->text(), QString("/tmp/green.ogg"));
    }

    void controlsUpdateConfigStateAndSignalsImmediately()
    {
        ConfigManager config(configPath());
        config.setSocketPath(socketPath("settings_updates"));
        StateManager stateManager;
        IpcServer ipcServer(&stateManager, config.socketPath());
        TrafficLightWidget light;
        SettingsDialog dialog(&config, &light, &ipcServer, &stateManager);
        showAndProcess(dialog);

        QSignalSpy languageSpy(&dialog, &SettingsDialog::languageChanged);
        QSignalSpy aiToolSpy(&dialog, &SettingsDialog::aiToolChanged);

        auto *languageCombo = requireChild<QComboBox>(&dialog, "languageCombo");
        languageCombo->setCurrentIndex(indexForData(languageCombo, "zh"));
        QCOMPARE(config.language(), QString("zh"));
        QCOMPARE(languageSpy.count(), 1);

        auto *aiToolCombo = requireChild<QComboBox>(&dialog, "aiToolCombo");
        aiToolCombo->setCurrentIndex(indexForData(aiToolCombo, "gemini"));
        QCOMPARE(config.aiTool(), QString("gemini"));
        QCOMPARE(aiToolSpy.count(), 1);
        QCOMPARE(aiToolSpy.takeFirst().at(0).toString(), QString("Gemini"));

        requireChild<QSpinBox>(&dialog, "timeoutSpin")->setValue(900);
        QCOMPARE(config.timeoutSec(), 900);
        QCOMPARE(stateManager.timeoutSec(), 900);

        requireChild<QComboBox>(&dialog, "windowSizeCombo")->setCurrentIndex(4);
        QCOMPARE(config.windowSize(), QString("xlarge"));

        requireChild<QComboBox>(&dialog, "animationModeCombo")->setCurrentIndex(1);
        QCOMPARE(config.animationMode(), QString("classic"));

        requireChild<QSpinBox>(&dialog, "animationPeriodSpin")->setValue(1800);
        QCOMPARE(config.animationPeriodMs(), 1800);
        QCOMPARE(requireChild<QSlider>(&dialog, "animationPeriodSlider")->value(), 1800);

        requireChild<QSlider>(&dialog, "animationPeriodSlider")->setValue(2300);
        QCOMPARE(config.animationPeriodMs(), 2300);
        QCOMPARE(requireChild<QSpinBox>(&dialog, "animationPeriodSpin")->value(), 2300);
    }

    void rejectRestoresSnapshot()
    {
        ConfigManager config(configPath());
        config.setLanguage("en");
        config.setAiTool("codex");
        config.setTimeoutSec(300);
        config.setWindowSize("small");
        config.setAnimationMode("breathing");
        config.setAnimationPeriodMs(1000);
        config.setSocketPath(socketPath("settings_reject"));
        config.setYellowSoundEnabled(true);
        config.setYellowSoundFile("/tmp/original-yellow.ogg");
        config.setGreenSoundEnabled(true);
        config.setGreenSoundFile("/tmp/original-green.ogg");

        StateManager stateManager;
        IpcServer ipcServer(&stateManager, config.socketPath());
        TrafficLightWidget light;
        SettingsDialog dialog(&config, &light, &ipcServer, &stateManager);
        showAndProcess(dialog);

        auto *languageCombo = requireChild<QComboBox>(&dialog, "languageCombo");
        languageCombo->setCurrentIndex(indexForData(languageCombo, "ja"));
        auto *aiToolCombo = requireChild<QComboBox>(&dialog, "aiToolCombo");
        aiToolCombo->setCurrentIndex(indexForData(aiToolCombo, "gemini"));
        requireChild<QSpinBox>(&dialog, "timeoutSpin")->setValue(600);
        requireChild<QComboBox>(&dialog, "windowSizeCombo")->setCurrentIndex(3);
        requireChild<QComboBox>(&dialog, "animationModeCombo")->setCurrentIndex(1);
        requireChild<QSpinBox>(&dialog, "animationPeriodSpin")->setValue(2000);
        requireChild<QCheckBox>(&dialog, "yellowSoundCheck")->setChecked(false);
        requireChild<QCheckBox>(&dialog, "greenSoundCheck")->setChecked(false);
        config.setYellowSoundFile("/tmp/changed-yellow.ogg");
        config.setGreenSoundFile("/tmp/changed-green.ogg");

        dialog.reject();
        QApplication::processEvents();

        QCOMPARE(config.language(), QString("en"));
        QCOMPARE(config.aiTool(), QString("codex"));
        QCOMPARE(config.timeoutSec(), 300);
        QCOMPARE(stateManager.timeoutSec(), 300);
        QCOMPARE(config.windowSize(), QString("small"));
        QCOMPARE(config.animationMode(), QString("breathing"));
        QCOMPARE(config.animationPeriodMs(), 1000);
        QCOMPARE(config.socketPath(), socketPath("settings_reject"));
        QCOMPARE(config.yellowSoundEnabled(), true);
        QCOMPARE(config.yellowSoundFile(), QString("/tmp/original-yellow.ogg"));
        QCOMPARE(config.greenSoundEnabled(), true);
        QCOMPARE(config.greenSoundFile(), QString("/tmp/original-green.ogg"));
    }

    void socketEnvOverrideDisablesInput()
    {
        const QByteArray envSocket = socketPath("settings_env").toLocal8Bit();
        EnvGuard guard("TL4AI_SOCKET", envSocket);

        ConfigManager config(configPath());
        StateManager stateManager;
        IpcServer ipcServer(&stateManager, config.socketPath());
        TrafficLightWidget light;
        SettingsDialog dialog(&config, &light, &ipcServer, &stateManager);
        showAndProcess(dialog);

        auto *socketEdit = requireChild<QLineEdit>(&dialog, "socketEdit");
        QVERIFY(!socketEdit->isEnabled());
        QCOMPARE(socketEdit->text(), QString::fromLocal8Bit(envSocket));
        QVERIFY(socketEdit->placeholderText().contains("TL4AI_SOCKET"));
    }

    void previewButtonsReflectEmptyExistingAndMissingPaths()
    {
        ConfigManager config(configPath());
        config.setSocketPath(socketPath("settings_preview"));
        StateManager stateManager;
        IpcServer ipcServer(&stateManager, config.socketPath());
        TrafficLightWidget light;
        SettingsDialog dialog(&config, &light, &ipcServer, &stateManager);
        showAndProcess(dialog);

        auto *yellowEdit = requireChild<QLineEdit>(&dialog, "yellowSoundEdit");
        auto *yellowPreview = requireChild<QPushButton>(&dialog, "yellowPreviewButton");
        auto *greenEdit = requireChild<QLineEdit>(&dialog, "greenSoundEdit");
        auto *greenPreview = requireChild<QPushButton>(&dialog, "greenPreviewButton");

        QVERIFY(yellowPreview->isEnabled());
        QVERIFY(greenPreview->isEnabled());

        yellowEdit->setText(m_tempDir.path() + "/missing-yellow.ogg");
        greenEdit->setText(m_tempDir.path() + "/missing-green.ogg");
        QVERIFY(!yellowPreview->isEnabled());
        QVERIFY(!greenPreview->isEnabled());

        const QString yellowPath = m_tempDir.path() + "/existing-yellow.ogg";
        const QString greenPath = m_tempDir.path() + "/existing-green.ogg";
        QFile yellowFile(yellowPath);
        QVERIFY(yellowFile.open(QIODevice::WriteOnly));
        yellowFile.write("yellow");
        yellowFile.close();
        QFile greenFile(greenPath);
        QVERIFY(greenFile.open(QIODevice::WriteOnly));
        greenFile.write("green");
        greenFile.close();

        yellowEdit->setText(yellowPath);
        greenEdit->setText(greenPath);
        QVERIFY(yellowPreview->isEnabled());
        QVERIFY(greenPreview->isEnabled());
    }

    void hooksButtonShowsReadOnlyTemplateDialog()
    {
        ConfigManager config(configPath());
        config.setAiTool("gemini");
        config.setSocketPath(socketPath("settings_hooks"));
        StateManager stateManager;
        IpcServer ipcServer(&stateManager, config.socketPath());
        TrafficLightWidget light;
        SettingsDialog dialog(&config, &light, &ipcServer, &stateManager);
        showAndProcess(dialog);

        QTest::mouseClick(requireChild<QPushButton>(&dialog, "hooksButton"), Qt::LeftButton);
        QApplication::processEvents();

        auto *templateDialog = requireChild<QDialog>(&dialog, "hooksTemplateDialog");
        auto *textEdit = requireChild<QTextEdit>(templateDialog, "hooksTemplateTextEdit");
        QVERIFY(templateDialog->isVisible());
        QVERIFY(textEdit->isReadOnly());
        QCOMPARE(textEdit->toPlainText(), AiToolRegistry::resolvedTemplate(AiToolRegistry::find("gemini")));

        templateDialog->close();
        QApplication::processEvents();
    }

    void editHooksConfigSavesEntireFileTemplate()
    {
        const QString homeDir = m_tempDir.path() + "/home_codex";
        QVERIFY(QDir().mkpath(homeDir));
        EnvGuard homeGuard("HOME", homeDir.toLocal8Bit());

        ConfigManager config(configPath());
        config.setAiTool("codex");
        config.setSocketPath(socketPath("settings_edit_codex"));
        StateManager stateManager;
        IpcServer ipcServer(&stateManager, config.socketPath());
        TrafficLightWidget light;
        SettingsDialog dialog(&config, &light, &ipcServer, &stateManager);
        showAndProcess(dialog);

        const QString editedJson = R"({
  "hooks": {
    "Stop": [
      { "hooks": [{ "type": "command", "command": "tl4ai-ctl green" }] }
    ]
  }
})";

        QTimer::singleShot(0, &dialog, [this, &dialog, editedJson, homeDir]() {
            auto *editor = requireChild<QDialog>(&dialog, "hooksConfigEditorDialog");
            auto *pathLabel = requireChild<QLabel>(editor, "hooksConfigPathLabel");
            auto *textEdit = requireChild<QTextEdit>(editor, "hooksConfigEditorTextEdit");
            QVERIFY(pathLabel->text().startsWith(homeDir));
            QCOMPARE(textEdit->toPlainText(), AiToolRegistry::resolvedTemplate(AiToolRegistry::find("codex")));
            textEdit->setPlainText(editedJson);
            QTest::mouseClick(requireChild<QPushButton>(editor, "hooksConfigSaveButton"), Qt::LeftButton);
        });

        QTest::mouseClick(requireChild<QPushButton>(&dialog, "editHooksButton"), Qt::LeftButton);

        QFile file(homeDir + "/.codex/hooks.json");
        QVERIFY(file.open(QIODevice::ReadOnly));
        const QJsonDocument saved = QJsonDocument::fromJson(file.readAll());
        QVERIFY(saved.isObject());
        QVERIFY(saved.object().contains("hooks"));
        QVERIFY(saved.object()["hooks"].toObject().contains("Stop"));
    }

    void editHooksConfigMergesHooksIntoExistingFile()
    {
        const QString homeDir = m_tempDir.path() + "/home_gemini_merge";
        QVERIFY(QDir().mkpath(homeDir + "/.gemini"));
        EnvGuard homeGuard("HOME", homeDir.toLocal8Bit());

        QFile existing(homeDir + "/.gemini/settings.json");
        QVERIFY(existing.open(QIODevice::WriteOnly));
        existing.write(R"({"theme":"dark","hooks":{"BeforeAgent":[{"old":true}]}})");
        existing.close();

        ConfigManager config(configPath());
        config.setAiTool("gemini");
        config.setSocketPath(socketPath("settings_edit_gemini"));
        StateManager stateManager;
        IpcServer ipcServer(&stateManager, config.socketPath());
        TrafficLightWidget light;
        SettingsDialog dialog(&config, &light, &ipcServer, &stateManager);
        showAndProcess(dialog);

        QTimer::singleShot(0, &dialog, [this, &dialog]() {
            auto *editor = requireChild<QDialog>(&dialog, "hooksConfigEditorDialog");
            auto *textEdit = requireChild<QTextEdit>(editor, "hooksConfigEditorTextEdit");
            const QJsonDocument loaded = QJsonDocument::fromJson(textEdit->toPlainText().toUtf8());
            QVERIFY(loaded.isObject());
            QVERIFY(loaded.object().contains("hooks"));
            QVERIFY(!loaded.object().contains("theme"));
            textEdit->setPlainText(R"({"hooks":{"AfterAgent":[{"hooks":[{"type":"command","command":"tl4ai-ctl green"}]}]}})");
            QTest::mouseClick(requireChild<QPushButton>(editor, "hooksConfigSaveButton"), Qt::LeftButton);
        });

        QTest::mouseClick(requireChild<QPushButton>(&dialog, "editHooksButton"), Qt::LeftButton);

        QFile file(homeDir + "/.gemini/settings.json");
        QVERIFY(file.open(QIODevice::ReadOnly));
        const QJsonDocument saved = QJsonDocument::fromJson(file.readAll());
        QVERIFY(saved.isObject());
        QCOMPARE(saved.object()["theme"].toString(), QString("dark"));
        QVERIFY(saved.object()["hooks"].toObject().contains("AfterAgent"));
        QVERIFY(!saved.object()["hooks"].toObject().contains("BeforeAgent"));
    }

    void editHooksConfigCanSaveBareHooksObjectForPartialFileStrategy()
    {
        const QString homeDir = m_tempDir.path() + "/home_gemini_bare";
        QVERIFY(QDir().mkpath(homeDir));
        EnvGuard homeGuard("HOME", homeDir.toLocal8Bit());

        ConfigManager config(configPath());
        config.setAiTool("gemini");
        config.setSocketPath(socketPath("settings_edit_gemini_bare"));
        StateManager stateManager;
        IpcServer ipcServer(&stateManager, config.socketPath());
        TrafficLightWidget light;
        SettingsDialog dialog(&config, &light, &ipcServer, &stateManager);
        showAndProcess(dialog);

        QTimer::singleShot(0, &dialog, [this, &dialog]() {
            auto *editor = requireChild<QDialog>(&dialog, "hooksConfigEditorDialog");
            auto *textEdit = requireChild<QTextEdit>(editor, "hooksConfigEditorTextEdit");
            QCOMPARE(textEdit->toPlainText(), AiToolRegistry::resolvedTemplate(AiToolRegistry::find("gemini")));
            textEdit->setPlainText(R"({"BeforeAgent":[{"hooks":[{"type":"command","command":"tl4ai-ctl red"}]}]})");
            QTest::mouseClick(requireChild<QPushButton>(editor, "hooksConfigSaveButton"), Qt::LeftButton);
        });

        QTest::mouseClick(requireChild<QPushButton>(&dialog, "editHooksButton"), Qt::LeftButton);

        QFile file(homeDir + "/.gemini/settings.json");
        QVERIFY(file.open(QIODevice::ReadOnly));
        const QJsonDocument saved = QJsonDocument::fromJson(file.readAll());
        QVERIFY(saved.isObject());
        QVERIFY(saved.object()["hooks"].toObject().contains("BeforeAgent"));
    }

    void acceptWithUnchangedSocketSavesSoundPaths()
    {
        ConfigManager config(configPath());
        config.setSocketPath(socketPath("settings_accept"));
        StateManager stateManager;
        IpcServer ipcServer(&stateManager, config.socketPath());
        TrafficLightWidget light;
        SettingsDialog dialog(&config, &light, &ipcServer, &stateManager);
        showAndProcess(dialog);

        const QString yellowPath = "/tmp/accepted-yellow.ogg";
        const QString greenPath = "/tmp/accepted-green.ogg";
        requireChild<QLineEdit>(&dialog, "yellowSoundEdit")->setText(yellowPath);
        requireChild<QLineEdit>(&dialog, "greenSoundEdit")->setText(greenPath);

        QSignalSpy acceptedSpy(&dialog, &QDialog::accepted);
        QTest::mouseClick(requireChild<QPushButton>(&dialog, "okButton"), Qt::LeftButton);

        QCOMPARE(acceptedSpy.count(), 1);
        QCOMPARE(config.socketPath(), socketPath("settings_accept"));
        QCOMPARE(config.yellowSoundFile(), yellowPath);
        QCOMPARE(config.greenSoundFile(), greenPath);
    }

    void showEventLoadsLoggingValues()
    {
        ConfigManager config(configPath());
        config.setSocketPath(socketPath("settings_log_load"));
        config.setLoggingEnabled(false);
        config.setLogLevel("debug");

        StateManager stateManager;
        IpcServer ipcServer(&stateManager, config.socketPath());
        TrafficLightWidget light;
        SettingsDialog dialog(&config, &light, &ipcServer, &stateManager);
        showAndProcess(dialog);

        auto *logCheck = requireChild<QCheckBox>(&dialog, "logEnabledCheck");
        auto *logLevel = requireChild<QComboBox>(&dialog, "logLevelCombo");
        auto *logPath = requireChild<QLineEdit>(&dialog, "logPathEdit");

        QCOMPARE(logCheck->isChecked(), false);
        QCOMPARE(logLevel->currentData().toString(), QString("debug"));
        QVERIFY(!logLevel->isEnabled()); // disabled because logging is off
        QVERIFY(logPath->isReadOnly());
        QVERIFY(!logPath->text().isEmpty());
    }

    void logControlsUpdateConfigImmediately()
    {
        ConfigManager config(configPath());
        config.setSocketPath(socketPath("settings_log_update"));
        config.setLoggingEnabled(true);
        config.setLogLevel("warn");

        StateManager stateManager;
        IpcServer ipcServer(&stateManager, config.socketPath());
        TrafficLightWidget light;
        SettingsDialog dialog(&config, &light, &ipcServer, &stateManager);
        showAndProcess(dialog);

        auto *logCheck = requireChild<QCheckBox>(&dialog, "logEnabledCheck");
        auto *logLevel = requireChild<QComboBox>(&dialog, "logLevelCombo");

        logLevel->setCurrentIndex(indexForData(logLevel, "error"));
        QCOMPARE(config.logLevel(), QString("error"));

        logCheck->setChecked(false);
        QCOMPARE(config.loggingEnabled(), false);
        QVERIFY(!logLevel->isEnabled());

        logCheck->setChecked(true);
        QCOMPARE(config.loggingEnabled(), true);
        QVERIFY(logLevel->isEnabled());
    }

    void rejectRestoresLoggingSettings()
    {
        ConfigManager config(configPath());
        config.setSocketPath(socketPath("settings_log_reject"));
        config.setLoggingEnabled(true);
        config.setLogLevel("warn");

        StateManager stateManager;
        IpcServer ipcServer(&stateManager, config.socketPath());
        TrafficLightWidget light;
        SettingsDialog dialog(&config, &light, &ipcServer, &stateManager);
        showAndProcess(dialog);

        auto *logLevel = requireChild<QComboBox>(&dialog, "logLevelCombo");
        logLevel->setCurrentIndex(indexForData(logLevel, "verb"));
        requireChild<QCheckBox>(&dialog, "logEnabledCheck")->setChecked(false);

        dialog.reject();
        QApplication::processEvents();

        QCOMPARE(config.loggingEnabled(), true);
        QCOMPARE(config.logLevel(), QString("warn"));
    }
};

QTEST_MAIN(TestSettingsDialog)
#include "test_settings_dialog.moc"
