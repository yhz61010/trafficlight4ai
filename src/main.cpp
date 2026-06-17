#include <QApplication>
#include <QCoreApplication>
#include <QDir>
#include <QStandardPaths>
#include <QString>
#include <QTranslator>
#include "Tl4aiClient.h"
#include "StateManager.h"
#include "ConfigManager.h"
#include "IpcServer.h"
#include "TrafficLightWidget.h"
#include "FloatingWindow.h"
#include "TrayIcon.h"
#include "SettingsDialog.h"
#include "AiToolStrategy.h"
#include "SoundUtils.h"

static QString defaultConfigPath()
{
    return QStandardPaths::writableLocation(QStandardPaths::ConfigLocation)
           + "/trafficlight4ai/config.json";
}

static QTranslator *s_translator = nullptr;

static void loadLanguage(QApplication &app, const QString &lang)
{
    if (s_translator) {
        app.removeTranslator(s_translator);
        delete s_translator;
        s_translator = nullptr;
    }

    if (lang != "en") {
        s_translator = new QTranslator();
        if (s_translator->load(":/i18n/trafficlight4ai_" + lang + ".qm")) {
            app.installTranslator(s_translator);
        } else {
            delete s_translator;
            s_translator = nullptr;
        }
    }
}

int main(int argc, char *argv[])
{
    // Lightweight CLI-forwarding mode. The AppImage's AppRun launches this GUI
    // binary, so accepting the state words here lets hooks target the stable
    // .AppImage path (e.g. `foo.AppImage red`) instead of the transient mount
    // path of the bundled tl4ai-ctl. Shares the send logic with tl4ai-ctl.
    if (argc >= 2 && Tl4aiClient::isStateCommand(QString::fromLocal8Bit(argv[1]))) {
        Tl4aiClient::drainStdin();
        QCoreApplication app(argc, argv);
        app.setApplicationName("trafficlight4ai");
        return Tl4aiClient::sendState(QString::fromLocal8Bit(argv[1]));
    }

    QApplication app(argc, argv);
    app.setApplicationName("trafficlight4ai");
    app.setQuitOnLastWindowClosed(false); // keep running in tray

    initBundledGStreamerPlugins();

    // Core
    ConfigManager config(defaultConfigPath());
    StateManager stateManager;
    stateManager.setTimeoutSec(config.timeoutSec());
    IpcServer ipcServer(&stateManager, config.socketPath());

    // Load initial language
    loadLanguage(app, config.language());

    // GUI
    auto *lightWidget = new TrafficLightWidget();
    lightWidget->setAnimationMode(config.animationMode());
    lightWidget->setAnimationPeriodMs(config.animationPeriodMs());

    lightWidget->setSizePreset(TrafficLightWidget::sizePresetFromString(config.windowSize()));

    auto *floatingWindow = new FloatingWindow(lightWidget, &config);
    floatingWindow->show();

    // Settings dialog
    auto *settingsDialog = new SettingsDialog(&config, lightWidget, &ipcServer, &stateManager);
    floatingWindow->setSettingsDialog(settingsDialog);

    // Tray icon with dynamic tooltip
    auto *trayIcon = new TrayIcon(floatingWindow, settingsDialog);
    if (auto *strategy = AiToolRegistry::find(config.aiTool()))
        trayIcon->onAiToolChanged(strategy->displayName());
    trayIcon->show();

    // Connect state changes to UI (trayIcon first so m_state is updated
    // before lightWidget's animation stop triggers activeAlphaChanged)
    QObject::connect(&stateManager, &StateManager::stateChanged,
                     trayIcon, &TrayIcon::onStateChanged);
    QObject::connect(&stateManager, &StateManager::stateChanged,
                     lightWidget, &TrafficLightWidget::onStateChanged);

    // Sound notifications on state change
    QObject::connect(&stateManager, &StateManager::stateChanged,
                     [&config](LightState state) {
        if (state == LightState::WaitingConfirm && config.yellowSoundEnabled()) {
            QString f = config.yellowSoundFile();
            playSound(f.isEmpty() ? kDefaultYellowSound : f);
        } else if (state == LightState::Idle && config.greenSoundEnabled()) {
            QString f = config.greenSoundFile();
            playSound(f.isEmpty() ? kDefaultGreenSound : f);
        }
    });

    // Connect animation alpha to tray icon blinking
    QObject::connect(lightWidget, &TrafficLightWidget::activeAlphaChanged,
                     trayIcon, &TrayIcon::onActiveAlphaChanged);

    // Connect AI tool changes to tray tooltip
    QObject::connect(settingsDialog, &SettingsDialog::aiToolChanged,
                     trayIcon, &TrayIcon::onAiToolChanged);

    // Language switching
    QObject::connect(settingsDialog, &SettingsDialog::languageChanged,
                     [&app, settingsDialog, trayIcon](const QString &lang) {
        loadLanguage(app, lang);
        settingsDialog->retranslateUi();
        trayIcon->retranslateUi();
    });

    return app.exec();
}
