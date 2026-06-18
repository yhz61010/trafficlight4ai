#pragma once

#include <QDialog>
#include <QPoint>

class QComboBox;
class QSlider;
class QSpinBox;
class QLineEdit;
class QCheckBox;
class QPushButton;
class QFormLayout;
class ConfigManager;
class TrafficLightWidget;
class IpcServer;
class StateManager;

class SettingsDialog : public QDialog {
    Q_OBJECT

public:
    explicit SettingsDialog(ConfigManager *config, TrafficLightWidget *lightWidget,
                            IpcServer *ipcServer, StateManager *stateManager,
                            QWidget *parent = nullptr);

    void retranslateUi();

signals:
    void aiToolChanged(const QString &displayName);
    void languageChanged(const QString &lang);

public slots:
    void reject() override;

protected:
    void showEvent(QShowEvent *event) override;

private slots:
    void onLanguageChanged(int index);
    void onAiToolChanged(int index);
    void onTimeoutChanged(int value);
    void onWindowSizeChanged(int index);
    void onAnimationModeChanged(int index);
    void onAnimationPeriodChanged(int value);
    void onYellowSoundToggled(bool checked);
    void onGreenSoundToggled(bool checked);
    void onPreviewYellowSound();
    void onPreviewGreenSound();
    void onBrowseYellowSound();
    void onBrowseGreenSound();
    void onLogEnabledToggled(bool checked);
    void onLogLevelChanged(int index);
    void onOpenLogFolder();
    void updatePreviewButtons();
    void onShowHooksTemplate();
    void onEditHooksConfig();
    void onAccept();
    void onCancel();

private:
    void takeSnapshot();
    void restoreSnapshot();
    void resizeFloatingWindowAt(const QPoint &pos, bool savePosition);

    ConfigManager *m_config;
    TrafficLightWidget *m_lightWidget;
    IpcServer *m_ipcServer;
    StateManager *m_stateManager;

    QFormLayout *m_formLayout;
    QComboBox *m_langCombo;
    QComboBox *m_aiToolCombo;
    QSpinBox *m_timeoutSpin;
    QComboBox *m_sizeCombo;
    QComboBox *m_modeCombo;
    QSlider *m_periodSlider;
    QSpinBox *m_periodSpin;
    QLineEdit *m_socketEdit;
    QCheckBox *m_yellowSoundCheck;
    QLineEdit *m_yellowSoundEdit;
    QPushButton *m_yellowPreviewBtn;
    QPushButton *m_yellowBrowseBtn;
    QCheckBox *m_greenSoundCheck;
    QLineEdit *m_greenSoundEdit;
    QPushButton *m_greenPreviewBtn;
    QPushButton *m_greenBrowseBtn;
    QCheckBox *m_logEnabledCheck;
    QComboBox *m_logLevelCombo;
    QLineEdit *m_logPathEdit;
    QPushButton *m_logOpenBtn;
    QPushButton *m_hooksBtn;
    QPushButton *m_editHooksBtn;
    QPushButton *m_okBtn;
    QPushButton *m_cancelBtn;

    // Snapshot for cancel
    QString m_snapLang;
    QString m_snapAiTool;
    int m_snapTimeoutSec;
    QString m_snapSize;
    QString m_snapMode;
    int m_snapPeriodMs;
    QString m_snapSocketPath;
    bool m_snapYellowSoundEnabled;
    QString m_snapYellowSoundFile;
    bool m_snapGreenSoundEnabled;
    QString m_snapGreenSoundFile;
    bool m_snapLogEnabled;
    QString m_snapLogLevel;
};
