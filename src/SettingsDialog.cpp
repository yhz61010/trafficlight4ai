#include "SettingsDialog.h"
#include "ConfigManager.h"
#include "TrafficLightWidget.h"
#include "IpcServer.h"
#include "StateManager.h"
#include "AiToolStrategy.h"
#include "SoundUtils.h"
#include "Logger.h"
#include <QCheckBox>
#include <QDesktopServices>
#include <QStandardPaths>
#include <QUrl>
#include <QComboBox>
#include <QSlider>
#include <QSpinBox>
#include <QLabel>
#include <QLineEdit>
#include <QTextEdit>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLayout>
#include <QPushButton>
#include <QDialog>
#include <QClipboard>
#include <QMessageBox>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDir>
#include <QFileInfo>
#include <QFont>
#include <QFile>
#include <QFileDialog>
#include <QTimer>
#include <QApplication>

SettingsDialog::SettingsDialog(ConfigManager *config, TrafficLightWidget *lightWidget,
                               IpcServer *ipcServer, StateManager *stateManager,
                               QWidget *parent)
    : QDialog(parent), m_config(config), m_lightWidget(lightWidget),
      m_ipcServer(ipcServer), m_stateManager(stateManager)
{
    setMinimumSize(400, 460);

    // Language
    m_langCombo = new QComboBox();
    m_langCombo->setObjectName("languageCombo");
    m_langCombo->addItem("English", "en");
    m_langCombo->addItem(QString::fromUtf8("中文"), "zh");
    m_langCombo->addItem(QString::fromUtf8("日本語"), "ja");

    // AI tool
    m_aiToolCombo = new QComboBox();
    m_aiToolCombo->setObjectName("aiToolCombo");
    for (auto *s : AiToolRegistry::strategies())
        m_aiToolCombo->addItem(s->displayName(), s->id());

    // Timeout
    m_timeoutSpin = new QSpinBox();
    m_timeoutSpin->setObjectName("timeoutSpin");
    m_timeoutSpin->setRange(0, 3600);
    m_timeoutSpin->setSingleStep(30);

    // Window size
    m_sizeCombo = new QComboBox();
    m_sizeCombo->setObjectName("windowSizeCombo");

    // Animation mode
    m_modeCombo = new QComboBox();
    m_modeCombo->setObjectName("animationModeCombo");

    // Animation period
    m_periodSlider = new QSlider(Qt::Horizontal);
    m_periodSlider->setObjectName("animationPeriodSlider");
    m_periodSlider->setRange(200, 5000);
    m_periodSlider->setSingleStep(100);
    m_periodSpin = new QSpinBox();
    m_periodSpin->setObjectName("animationPeriodSpin");
    m_periodSpin->setRange(200, 5000);
    m_periodSpin->setSingleStep(100);
    m_periodSpin->setSuffix(" ms");

    auto *periodLayout = new QHBoxLayout();
    periodLayout->addWidget(m_periodSlider);
    periodLayout->addWidget(m_periodSpin);

    // Socket path
    m_socketEdit = new QLineEdit();
    m_socketEdit->setObjectName("socketEdit");

    // Yellow sound
    m_yellowSoundCheck = new QCheckBox();
    m_yellowSoundCheck->setObjectName("yellowSoundCheck");
    m_yellowSoundEdit = new QLineEdit();
    m_yellowSoundEdit->setObjectName("yellowSoundEdit");
    m_yellowPreviewBtn = new QPushButton();
    m_yellowPreviewBtn->setObjectName("yellowPreviewButton");
    m_yellowPreviewBtn->setEnabled(false);
    m_yellowBrowseBtn = new QPushButton();
    m_yellowBrowseBtn->setObjectName("yellowBrowseButton");
    auto *yellowSoundLayout = new QHBoxLayout();
    yellowSoundLayout->addWidget(m_yellowSoundCheck);
    yellowSoundLayout->addWidget(m_yellowSoundEdit);
    yellowSoundLayout->addWidget(m_yellowPreviewBtn);
    yellowSoundLayout->addWidget(m_yellowBrowseBtn);

    // Green sound
    m_greenSoundCheck = new QCheckBox();
    m_greenSoundCheck->setObjectName("greenSoundCheck");
    m_greenSoundEdit = new QLineEdit();
    m_greenSoundEdit->setObjectName("greenSoundEdit");
    m_greenPreviewBtn = new QPushButton();
    m_greenPreviewBtn->setObjectName("greenPreviewButton");
    m_greenPreviewBtn->setEnabled(false);
    m_greenBrowseBtn = new QPushButton();
    m_greenBrowseBtn->setObjectName("greenBrowseButton");
    auto *greenSoundLayout = new QHBoxLayout();
    greenSoundLayout->addWidget(m_greenSoundCheck);
    greenSoundLayout->addWidget(m_greenSoundEdit);
    greenSoundLayout->addWidget(m_greenPreviewBtn);
    greenSoundLayout->addWidget(m_greenBrowseBtn);

    // Logging
    m_logEnabledCheck = new QCheckBox();
    m_logEnabledCheck->setObjectName("logEnabledCheck");

    m_logLevelCombo = new QComboBox();
    m_logLevelCombo->setObjectName("logLevelCombo");
    // Level tokens are technical identifiers; stored lowercase in config.
    m_logLevelCombo->addItem("VERB", "verb");
    m_logLevelCombo->addItem("DEBUG", "debug");
    m_logLevelCombo->addItem("INFO", "info");
    m_logLevelCombo->addItem("WARN", "warn");
    m_logLevelCombo->addItem("ERROR", "error");

    m_logPathEdit = new QLineEdit();
    m_logPathEdit->setObjectName("logPathEdit");
    m_logPathEdit->setReadOnly(true);
    m_logPathEdit->setText(Logger::defaultLogFilePath());
    m_logOpenBtn = new QPushButton();
    m_logOpenBtn->setObjectName("logOpenButton");
    auto *logPathLayout = new QHBoxLayout();
    logPathLayout->addWidget(m_logPathEdit);
    logPathLayout->addWidget(m_logOpenBtn);

    // Form layout
    m_formLayout = new QFormLayout();
    m_formLayout->addRow(tr("Language:"), m_langCombo);
    m_formLayout->addRow(tr("AI Tool:"), m_aiToolCombo);
    m_formLayout->addRow(tr("Timeout:"), m_timeoutSpin);
    m_formLayout->addRow(tr("Window Size:"), m_sizeCombo);
    m_formLayout->addRow(tr("Animation Mode:"), m_modeCombo);
    m_formLayout->addRow(tr("Animation Period:"), periodLayout);
    m_formLayout->addRow(tr("Socket Path:"), m_socketEdit);
    m_formLayout->addRow(tr("Yellow Sound:"), yellowSoundLayout);
    m_formLayout->addRow(tr("Green Sound:"), greenSoundLayout);
    m_formLayout->addRow(tr("Logging:"), m_logEnabledCheck);
    m_formLayout->addRow(tr("Log Level:"), m_logLevelCombo);
    m_formLayout->addRow(tr("Log File:"), logPathLayout);

    // Buttons
    m_hooksBtn = new QPushButton();
    m_hooksBtn->setObjectName("hooksButton");
    m_editHooksBtn = new QPushButton();
    m_editHooksBtn->setObjectName("editHooksButton");
    m_okBtn = new QPushButton();
    m_okBtn->setObjectName("okButton");
    m_cancelBtn = new QPushButton();
    m_cancelBtn->setObjectName("cancelButton");
    auto *btnLayout = new QHBoxLayout();
    btnLayout->addWidget(m_hooksBtn);
    btnLayout->addWidget(m_editHooksBtn);
    btnLayout->addStretch();
    btnLayout->addWidget(m_okBtn);
    btnLayout->addWidget(m_cancelBtn);

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->addLayout(m_formLayout);
    mainLayout->addLayout(btnLayout);

    // Set all translatable text
    retranslateUi();

    // Connections
    connect(m_langCombo, &QComboBox::currentIndexChanged,
            this, &SettingsDialog::onLanguageChanged);
    connect(m_aiToolCombo, &QComboBox::currentIndexChanged,
            this, &SettingsDialog::onAiToolChanged);
    connect(m_timeoutSpin, &QSpinBox::valueChanged,
            this, &SettingsDialog::onTimeoutChanged);
    connect(m_sizeCombo, &QComboBox::currentIndexChanged,
            this, &SettingsDialog::onWindowSizeChanged);
    connect(m_modeCombo, &QComboBox::currentIndexChanged,
            this, &SettingsDialog::onAnimationModeChanged);
    connect(m_periodSlider, &QSlider::valueChanged,
            this, &SettingsDialog::onAnimationPeriodChanged);
    connect(m_periodSpin, &QSpinBox::valueChanged,
            this, &SettingsDialog::onAnimationPeriodChanged);
    connect(m_yellowSoundCheck, &QCheckBox::toggled,
            this, &SettingsDialog::onYellowSoundToggled);
    connect(m_greenSoundCheck, &QCheckBox::toggled,
            this, &SettingsDialog::onGreenSoundToggled);
    connect(m_yellowSoundEdit, &QLineEdit::textChanged,
            this, [this]() { updatePreviewButtons(); });
    connect(m_greenSoundEdit, &QLineEdit::textChanged,
            this, [this]() { updatePreviewButtons(); });
    connect(m_yellowPreviewBtn, &QPushButton::clicked,
            this, &SettingsDialog::onPreviewYellowSound);
    connect(m_greenPreviewBtn, &QPushButton::clicked,
            this, &SettingsDialog::onPreviewGreenSound);
    connect(m_yellowBrowseBtn, &QPushButton::clicked,
            this, &SettingsDialog::onBrowseYellowSound);
    connect(m_greenBrowseBtn, &QPushButton::clicked,
            this, &SettingsDialog::onBrowseGreenSound);
    connect(m_logEnabledCheck, &QCheckBox::toggled,
            this, &SettingsDialog::onLogEnabledToggled);
    connect(m_logLevelCombo, &QComboBox::currentIndexChanged,
            this, &SettingsDialog::onLogLevelChanged);
    connect(m_logOpenBtn, &QPushButton::clicked,
            this, &SettingsDialog::onOpenLogFolder);
    connect(m_hooksBtn, &QPushButton::clicked,
            this, &SettingsDialog::onShowHooksTemplate);
    connect(m_editHooksBtn, &QPushButton::clicked,
            this, &SettingsDialog::onEditHooksConfig);
    connect(m_okBtn, &QPushButton::clicked, this, &SettingsDialog::onAccept);
    connect(m_cancelBtn, &QPushButton::clicked, this, &SettingsDialog::onCancel);
}

void SettingsDialog::retranslateUi()
{
    setWindowTitle(tr("Settings - Traffic Light for AI"));

    // Form labels — safely update each row's label
    const QStringList labels = {
        tr("Language:"), tr("AI Tool:"), tr("Timeout:"), tr("Window Size:"),
        tr("Animation Mode:"), tr("Animation Period:"), tr("Socket Path:"),
        tr("Yellow Sound:"), tr("Green Sound:"),
        tr("Logging:"), tr("Log Level:"), tr("Log File:")
    };
    for (int i = 0; i < labels.size() && i < m_formLayout->rowCount(); ++i) {
        auto *item = m_formLayout->itemAt(i, QFormLayout::LabelRole);
        if (!item) continue;
        auto *label = qobject_cast<QLabel *>(item->widget());
        if (label) label->setText(labels[i]);
    }

    // Timeout suffix and special value
    m_timeoutSpin->setSuffix(" " + tr("sec"));
    m_timeoutSpin->setSpecialValueText(tr("Disabled"));

    // Window size combo
    const int sizeIdx = m_sizeCombo->currentIndex();
    m_sizeCombo->blockSignals(true);
    m_sizeCombo->clear();
    m_sizeCombo->addItems({tr("Extra Small"), tr("Small"), tr("Medium"), tr("Large"), tr("Extra Large")});
    if (sizeIdx >= 0) m_sizeCombo->setCurrentIndex(sizeIdx);
    m_sizeCombo->blockSignals(false);

    // Animation mode combo
    const int modeIdx = m_modeCombo->currentIndex();
    m_modeCombo->blockSignals(true);
    m_modeCombo->clear();
    m_modeCombo->addItems({tr("Breathing"), tr("Classic Blink")});
    if (modeIdx >= 0) m_modeCombo->setCurrentIndex(modeIdx);
    m_modeCombo->blockSignals(false);

    // Sound controls
    m_yellowSoundCheck->setText(tr("Enable"));
    m_yellowSoundEdit->setPlaceholderText(tr("Default: yellow.ogg"));
    m_yellowPreviewBtn->setText(tr("Preview"));
    m_yellowBrowseBtn->setText(tr("Browse"));

    m_greenSoundCheck->setText(tr("Enable"));
    m_greenSoundEdit->setPlaceholderText(tr("Default: green.ogg"));
    m_greenPreviewBtn->setText(tr("Preview"));
    m_greenBrowseBtn->setText(tr("Browse"));

    // Logging controls
    m_logEnabledCheck->setText(tr("Enable"));
    m_logOpenBtn->setText(tr("Open Folder"));

    // Buttons
    m_hooksBtn->setText(tr("View Recommended Hooks Config"));
    m_editHooksBtn->setText(tr("Edit Hooks Config"));
    m_okBtn->setText(tr("OK"));
    m_cancelBtn->setText(tr("Cancel"));

    // Env override placeholder
    if (!qgetenv("TL4AI_SOCKET").isEmpty())
        m_socketEdit->setPlaceholderText(tr("Controlled by TL4AI_SOCKET env var"));

    updatePreviewButtons();
}

void SettingsDialog::showEvent(QShowEvent *event)
{
    QDialog::showEvent(event);
    takeSnapshot();

    // Language
    const QString lang = m_config->language();
    for (int i = 0; i < m_langCombo->count(); ++i) {
        if (m_langCombo->itemData(i).toString() == lang) {
            m_langCombo->blockSignals(true);
            m_langCombo->setCurrentIndex(i);
            m_langCombo->blockSignals(false);
            break;
        }
    }

    // AI tool
    const QString toolId = m_config->aiTool();
    for (int i = 0; i < m_aiToolCombo->count(); ++i) {
        if (m_aiToolCombo->itemData(i).toString() == toolId) {
            m_aiToolCombo->blockSignals(true);
            m_aiToolCombo->setCurrentIndex(i);
            m_aiToolCombo->blockSignals(false);
            break;
        }
    }

    // Timeout
    m_timeoutSpin->blockSignals(true);
    m_timeoutSpin->setValue(m_config->timeoutSec());
    m_timeoutSpin->blockSignals(false);

    // Window size
    const QStringList sizes = {"xsmall", "small", "medium", "large", "xlarge"};
    m_sizeCombo->blockSignals(true);
    m_sizeCombo->setCurrentIndex(sizes.indexOf(m_config->windowSize()));
    m_sizeCombo->blockSignals(false);

    // Animation mode
    const QStringList modes = {"breathing", "classic"};
    m_modeCombo->blockSignals(true);
    m_modeCombo->setCurrentIndex(modes.indexOf(m_config->animationMode()));
    m_modeCombo->blockSignals(false);

    // Animation period
    m_periodSlider->blockSignals(true);
    m_periodSlider->setValue(m_config->animationPeriodMs());
    m_periodSlider->blockSignals(false);
    m_periodSpin->blockSignals(true);
    m_periodSpin->setValue(m_config->animationPeriodMs());
    m_periodSpin->blockSignals(false);

    // Socket path
    const bool envOverride = !qgetenv("TL4AI_SOCKET").isEmpty();
    m_socketEdit->setEnabled(!envOverride);
    m_socketEdit->setText(m_config->socketPath());

    // Sound settings
    m_yellowSoundCheck->blockSignals(true);
    m_yellowSoundCheck->setChecked(m_config->yellowSoundEnabled());
    m_yellowSoundCheck->blockSignals(false);
    m_yellowSoundEdit->setText(m_config->yellowSoundFile());

    m_greenSoundCheck->blockSignals(true);
    m_greenSoundCheck->setChecked(m_config->greenSoundEnabled());
    m_greenSoundCheck->blockSignals(false);
    m_greenSoundEdit->setText(m_config->greenSoundFile());

    // Logging settings
    const bool logEnabled = m_config->loggingEnabled();
    m_logEnabledCheck->blockSignals(true);
    m_logEnabledCheck->setChecked(logEnabled);
    m_logEnabledCheck->blockSignals(false);

    m_logLevelCombo->blockSignals(true);
    const int levelIdx = m_logLevelCombo->findData(m_config->logLevel());
    m_logLevelCombo->setCurrentIndex(levelIdx >= 0 ? levelIdx : m_logLevelCombo->findData("warn"));
    m_logLevelCombo->blockSignals(false);
    m_logLevelCombo->setEnabled(logEnabled);

    m_logPathEdit->setText(Logger::defaultLogFilePath());

    updatePreviewButtons();
}

void SettingsDialog::takeSnapshot()
{
    m_snapLang = m_config->language();
    m_snapAiTool = m_config->aiTool();
    m_snapTimeoutSec = m_config->timeoutSec();
    m_snapSize = m_config->windowSize();
    m_snapMode = m_config->animationMode();
    m_snapPeriodMs = m_config->animationPeriodMs();
    m_snapSocketPath = m_config->socketPath();
    m_snapYellowSoundEnabled = m_config->yellowSoundEnabled();
    m_snapYellowSoundFile = m_config->yellowSoundFile();
    m_snapGreenSoundEnabled = m_config->greenSoundEnabled();
    m_snapGreenSoundFile = m_config->greenSoundFile();
    m_snapLogEnabled = m_config->loggingEnabled();
    m_snapLogLevel = m_config->logLevel();
}

void SettingsDialog::restoreSnapshot()
{
    m_config->beginBatchSave();

    // Restore language
    if (m_config->language() != m_snapLang) {
        m_config->setLanguage(m_snapLang);
        emit languageChanged(m_snapLang);
    }

    // Restore AI tool
    m_config->setAiTool(m_snapAiTool);
    if (auto *strategy = AiToolRegistry::find(m_snapAiTool))
        emit aiToolChanged(strategy->displayName());

    // Restore timeout
    m_config->setTimeoutSec(m_snapTimeoutSec);
    m_stateManager->setTimeoutSec(m_snapTimeoutSec);

    // Restore window size (preserve position)
    QWidget *window = m_lightWidget->window();
    QPoint pos = window->pos();
    m_config->setWindowSize(m_snapSize);
    m_lightWidget->setSizePreset(TrafficLightWidget::sizePresetFromString(m_snapSize));
    resizeFloatingWindowAt(pos, false);

    // Restore animation
    m_lightWidget->setAnimationMode(m_snapMode);
    m_config->setAnimationMode(m_snapMode);
    m_lightWidget->setAnimationPeriodMs(m_snapPeriodMs);
    m_config->setAnimationPeriodMs(m_snapPeriodMs);

    // Restore sound settings
    m_config->setYellowSoundEnabled(m_snapYellowSoundEnabled);
    m_config->setYellowSoundFile(m_snapYellowSoundFile);
    m_config->setGreenSoundEnabled(m_snapGreenSoundEnabled);
    m_config->setGreenSoundFile(m_snapGreenSoundFile);

    // Restore logging settings (config + live Logger)
    m_config->setLoggingEnabled(m_snapLogEnabled);
    m_config->setLogLevel(m_snapLogLevel);
    Logger::instance().setEnabled(m_snapLogEnabled);
    Logger::instance().setLevel(Logger::levelFromString(m_snapLogLevel));

    m_config->endBatchSave();
}

void SettingsDialog::onLanguageChanged(int index)
{
    const QString lang = m_langCombo->itemData(index).toString();
    m_config->setLanguage(lang);
    emit languageChanged(lang);
}

void SettingsDialog::onAiToolChanged(int index)
{
    const QString toolId = m_aiToolCombo->itemData(index).toString();
    m_config->setAiTool(toolId);
    if (auto *strategy = AiToolRegistry::find(toolId))
        emit aiToolChanged(strategy->displayName());
}

void SettingsDialog::onTimeoutChanged(int value)
{
    m_config->setTimeoutSec(value);
    m_stateManager->setTimeoutSec(value);
}

void SettingsDialog::onWindowSizeChanged(int index)
{
    const QStringList sizes = {"xsmall", "small", "medium", "large", "xlarge"};
    if (index < 0 || index >= sizes.size())
        return;

    // Preserve window position across size change
    QWidget *window = m_lightWidget->window();
    QPoint pos = window->pos();

    m_config->setWindowSize(sizes.at(index));
    m_lightWidget->setSizePreset(TrafficLightWidget::sizePresetFromString(sizes.at(index)));

    resizeFloatingWindowAt(pos, true);
}

void SettingsDialog::resizeFloatingWindowAt(const QPoint &pos, bool savePosition)
{
    QWidget *window = m_lightWidget->window();

    if (QLayout *layout = window->layout())
        layout->activate();
    window->adjustSize();
    window->move(pos);

    // Some window managers apply top-level geometry after the event loop turn.
    QTimer::singleShot(0, window, [this, window, pos, savePosition]() {
        if (QLayout *layout = window->layout())
            layout->activate();
        window->adjustSize();
        window->move(pos);
        if (savePosition)
            m_config->setWindowPos(pos.x(), pos.y());
    });
}

void SettingsDialog::onAnimationModeChanged(int index)
{
    const QStringList modes = {"breathing", "classic"};
    m_config->setAnimationMode(modes.at(index));
    m_lightWidget->setAnimationMode(modes.at(index));
}

void SettingsDialog::onAnimationPeriodChanged(int value)
{
    m_periodSlider->blockSignals(true);
    m_periodSlider->setValue(value);
    m_periodSlider->blockSignals(false);
    m_periodSpin->blockSignals(true);
    m_periodSpin->setValue(value);
    m_periodSpin->blockSignals(false);

    m_config->setAnimationPeriodMs(value);
    m_lightWidget->setAnimationPeriodMs(value);
}

void SettingsDialog::onYellowSoundToggled(bool checked)
{
    TL_LOGI("Sound", QString("Yellow sound enabled: %1")
            .arg(checked ? QStringLiteral("true") : QStringLiteral("false")));
    m_config->setYellowSoundEnabled(checked);
}

void SettingsDialog::onGreenSoundToggled(bool checked)
{
    TL_LOGI("Sound", QString("Green sound enabled: %1")
            .arg(checked ? QStringLiteral("true") : QStringLiteral("false")));
    m_config->setGreenSoundEnabled(checked);
}

void SettingsDialog::onPreviewYellowSound()
{
    QString path = m_yellowSoundEdit->text().trimmed();
    if (path.isEmpty()) path = kDefaultYellowSound;
    playSound(path, this);
}

void SettingsDialog::onPreviewGreenSound()
{
    QString path = m_greenSoundEdit->text().trimmed();
    if (path.isEmpty()) path = kDefaultGreenSound;
    playSound(path, this);
}

void SettingsDialog::updatePreviewButtons()
{
    const QString yellowPath = m_yellowSoundEdit->text().trimmed();
    m_yellowPreviewBtn->setEnabled(yellowPath.isEmpty() || QFile::exists(yellowPath));

    const QString greenPath = m_greenSoundEdit->text().trimmed();
    m_greenPreviewBtn->setEnabled(greenPath.isEmpty() || QFile::exists(greenPath));
}

void SettingsDialog::onBrowseYellowSound()
{
    QString file = QFileDialog::getOpenFileName(this, tr("Select Yellow Sound"), QString(),
                                                 tr("Audio Files (*.wav *.mp3 *.ogg)"));
    if (!file.isEmpty()) {
        TL_LOGI("Sound", QString("Yellow sound file set: %1").arg(file));
        m_yellowSoundEdit->setText(file);
        m_config->setYellowSoundFile(file);
    }
}

void SettingsDialog::onBrowseGreenSound()
{
    QString file = QFileDialog::getOpenFileName(this, tr("Select Green Sound"), QString(),
                                                 tr("Audio Files (*.wav *.mp3 *.ogg)"));
    if (!file.isEmpty()) {
        TL_LOGI("Sound", QString("Green sound file set: %1").arg(file));
        m_greenSoundEdit->setText(file);
        m_config->setGreenSoundFile(file);
    }
}

void SettingsDialog::onLogEnabledToggled(bool checked)
{
    m_config->setLoggingEnabled(checked);
    Logger::instance().setEnabled(checked);
    m_logLevelCombo->setEnabled(checked);
}

void SettingsDialog::onLogLevelChanged(int index)
{
    const QString level = m_logLevelCombo->itemData(index).toString();
    if (level.isEmpty())
        return;
    m_config->setLogLevel(level);
    Logger::instance().setLevel(Logger::levelFromString(level));
}

void SettingsDialog::onOpenLogFolder()
{
    const QString dir = QFileInfo(Logger::defaultLogFilePath()).absolutePath();
    QDir().mkpath(dir); // ensure the folder exists before opening
    QDesktopServices::openUrl(QUrl::fromLocalFile(dir));
}

void SettingsDialog::onAccept()
{
    const QString newPath = m_socketEdit->text().trimmed();
    if (!newPath.isEmpty() && newPath != m_config->socketPath()) {
        if (m_ipcServer->restart(newPath)) {
            m_config->setSocketPath(newPath);
        } else {
            m_socketEdit->setText(m_config->socketPath());
            QMessageBox::warning(this, tr("Socket Error"),
                tr("Cannot listen on: %1\nKept original path.").arg(newPath));
            return;
        }
    }

    m_config->setYellowSoundFile(m_yellowSoundEdit->text().trimmed());
    m_config->setGreenSoundFile(m_greenSoundEdit->text().trimmed());

    accept();
}

void SettingsDialog::onShowHooksTemplate()
{
    const QString toolId = m_aiToolCombo->currentData().toString();
    auto *strategy = AiToolRegistry::find(toolId);
    if (!strategy)
        return;

    auto *dlg = new QDialog(this);
    dlg->setObjectName("hooksTemplateDialog");
    dlg->setWindowTitle(tr("Recommended Hooks - %1").arg(strategy->displayName()));
    dlg->setMinimumSize(450, 350);

    auto *textEdit = new QTextEdit();
    textEdit->setObjectName("hooksTemplateTextEdit");
    textEdit->setReadOnly(true);
    textEdit->setPlainText(AiToolRegistry::resolvedTemplate(strategy));

    auto *copyBtn = new QPushButton(tr("Copy"));
    auto *closeBtn = new QPushButton(tr("Close"));

    connect(copyBtn, &QPushButton::clicked, this, [textEdit]() {
        QApplication::clipboard()->setText(textEdit->toPlainText());
    });
    connect(closeBtn, &QPushButton::clicked, dlg, &QDialog::accept);

    auto *btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    btnLayout->addWidget(copyBtn);
    btnLayout->addWidget(closeBtn);

    auto *layout = new QVBoxLayout(dlg);
    layout->addWidget(textEdit);
    layout->addLayout(btnLayout);

    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->show();
}

void SettingsDialog::onEditHooksConfig()
{
    const QString toolId = m_aiToolCombo->currentData().toString();
    auto *strategy = AiToolRegistry::find(toolId);
    if (!strategy)
        return;

    const QString configPath = strategy->hooksConfigPath();
    const bool entireFile = strategy->hooksIsEntireFile();

    // Read current content
    QString content;
    QFile file(configPath);
    if (file.exists() && file.open(QIODevice::ReadOnly)) {
        QByteArray raw = file.readAll();
        file.close();
        if (entireFile) {
            content = QString::fromUtf8(raw);
        } else {
            QJsonParseError err;
            QJsonDocument doc = QJsonDocument::fromJson(raw, &err);
            if (err.error == QJsonParseError::NoError && doc.isObject()) {
                QJsonObject hooksObj;
                hooksObj["hooks"] = doc.object()["hooks"];
                content = QJsonDocument(hooksObj).toJson(QJsonDocument::Indented);
            } else {
                content = AiToolRegistry::resolvedTemplate(strategy);
            }
        }
    } else {
        content = AiToolRegistry::resolvedTemplate(strategy);
    }

    // Build editor dialog
    auto *dlg = new QDialog(this);
    dlg->setObjectName("hooksConfigEditorDialog");
    dlg->setWindowTitle(tr("Edit Hooks Config - %1").arg(strategy->displayName()));
    dlg->setMinimumSize(500, 400);

    auto *textEdit = new QTextEdit();
    textEdit->setObjectName("hooksConfigEditorTextEdit");
    textEdit->setPlainText(content);
    QFont monoFont("monospace");
    monoFont.setStyleHint(QFont::Monospace);
    textEdit->setFont(monoFont);

    auto *pathLabel = new QLabel(configPath);
    pathLabel->setObjectName("hooksConfigPathLabel");
    pathLabel->setWordWrap(true);

    auto *saveBtn = new QPushButton(tr("Save"));
    saveBtn->setObjectName("hooksConfigSaveButton");
    auto *cancelBtn = new QPushButton(tr("Cancel"));
    cancelBtn->setObjectName("hooksConfigCancelButton");

    connect(saveBtn, &QPushButton::clicked, dlg, [this, dlg, textEdit, configPath, entireFile]() {
        const QString text = textEdit->toPlainText().trimmed();
        QJsonParseError err;
        QJsonDocument editedDoc = QJsonDocument::fromJson(text.toUtf8(), &err);
        if (err.error != QJsonParseError::NoError) {
            QMessageBox::warning(dlg, tr("JSON Error"),
                tr("Invalid JSON at offset %1:\n%2").arg(err.offset).arg(err.errorString()));
            return;
        }

        // Prepare the final content to write
        QByteArray output;
        if (entireFile) {
            output = editedDoc.toJson(QJsonDocument::Indented);
        } else {
            // Read existing file to preserve non-hooks fields
            QJsonObject root;
            QFile existing(configPath);
            if (existing.exists() && existing.open(QIODevice::ReadOnly)) {
                QJsonDocument existingDoc = QJsonDocument::fromJson(existing.readAll());
                existing.close();
                if (existingDoc.isObject())
                    root = existingDoc.object();
            }
            // Merge hooks from edited content
            QJsonObject editedObj = editedDoc.object();
            if (editedObj.contains("hooks"))
                root["hooks"] = editedObj["hooks"];
            else
                root["hooks"] = editedObj;
            output = QJsonDocument(root).toJson(QJsonDocument::Indented);
        }

        // Create parent directory if needed
        QDir dir = QFileInfo(configPath).absoluteDir();
        if (!dir.exists())
            dir.mkpath(".");

        QFile outFile(configPath);
        if (outFile.open(QIODevice::WriteOnly)) {
            outFile.write(output);
            outFile.close();
            dlg->accept();
        } else {
            QMessageBox::warning(dlg, tr("Save Error"),
                tr("Cannot write to: %1").arg(configPath));
        }
    });
    connect(cancelBtn, &QPushButton::clicked, dlg, &QDialog::reject);

    auto *btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    btnLayout->addWidget(saveBtn);
    btnLayout->addWidget(cancelBtn);

    auto *layout = new QVBoxLayout(dlg);
    layout->addWidget(pathLabel);
    layout->addWidget(textEdit);
    layout->addLayout(btnLayout);

    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->exec();
}

void SettingsDialog::reject()
{
    restoreSnapshot();
    QDialog::reject();
}

void SettingsDialog::onCancel()
{
    reject();
}
