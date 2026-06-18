#include "TrayIcon.h"
#include "FloatingWindow.h"
#include "SettingsDialog.h"
#include "Logger.h"
#include <QMenu>
#include <QApplication>
#include <QPainter>
#include <QPixmap>
#include <QTimer>

TrayIcon::TrayIcon(FloatingWindow *window, SettingsDialog *settingsDialog, QObject *parent)
    : QSystemTrayIcon(parent), m_window(window)
{
    setIcon(createIcon(QColor(40, 200, 40))); // default green

    auto *menu = new QMenu(nullptr);
    connect(this, &QObject::destroyed, menu, &QObject::deleteLater);

    m_toggleAction = menu->addAction(tr("Show/Hide"));
    m_toggleAction->setObjectName("toggleWindowAction");
    connect(m_toggleAction, &QAction::triggered, this, [this]() {
        m_window->setVisible(!m_window->isVisible());
    });

    m_settingsAction = menu->addAction(tr("Settings"));
    m_settingsAction->setObjectName("settingsAction");
    connect(m_settingsAction, &QAction::triggered, settingsDialog, [settingsDialog]() {
        settingsDialog->show();
        settingsDialog->raise();
        settingsDialog->activateWindow();
    });

    menu->addSeparator();

    m_quitAction = menu->addAction(tr("Quit"));
    m_quitAction->setObjectName("quitAction");
    connect(m_quitAction, &QAction::triggered, qApp, &QApplication::quit);

    setContextMenu(menu);

    connect(this, &QSystemTrayIcon::activated, this, &TrayIcon::onActivated);
}

void TrayIcon::retranslateUi()
{
    m_toggleAction->setText(tr("Show/Hide"));
    m_settingsAction->setText(tr("Settings"));
    m_quitAction->setText(tr("Quit"));
    if (!m_aiToolName.isEmpty())
        setToolTip(tr("Traffic Light for %1").arg(m_aiToolName));
}

void TrayIcon::onStateChanged(LightState newState)
{
    TL_LOGD("Tray", QString("onStateChanged: %1 -> %2")
            .arg(static_cast<int>(m_state)).arg(static_cast<int>(newState)));
    m_state = newState;
    switch (newState) {
    case LightState::Working:
        m_currentColor = QColor(220, 40, 40);
        break;
    case LightState::WaitingConfirm:
        m_currentColor = QColor(240, 200, 20);
        break;
    case LightState::Idle:
        m_currentColor = QColor(40, 200, 40);
        break;
    }
    TL_LOGD("Tray", QString("setIcon color=(%1,%2,%3) alpha=1.0")
            .arg(m_currentColor.red()).arg(m_currentColor.green()).arg(m_currentColor.blue()));
    setIcon(createIcon(m_currentColor));

    if (newState == LightState::Idle) {
        QTimer::singleShot(150, this, [this]() {
            TL_LOGD("Tray", "Delayed re-apply green icon");
            setIcon(createIcon(m_currentColor));
        });
    }
}

void TrayIcon::onActiveAlphaChanged(qreal alpha)
{
    if (m_state == LightState::Idle) {
        setIcon(createIcon(m_currentColor));
        return;
    }
    QColor color = m_currentColor;
    color.setAlphaF(alpha);
    setIcon(createIcon(color));
}

void TrayIcon::onAiToolChanged(const QString &displayName)
{
    m_aiToolName = displayName;
    setToolTip(tr("Traffic Light for %1").arg(displayName));
}

void TrayIcon::onActivated(QSystemTrayIcon::ActivationReason reason)
{
    if (reason == QSystemTrayIcon::Trigger) {
        m_window->setVisible(!m_window->isVisible());
    }
}

QIcon TrayIcon::createIcon(const QColor &color) const
{
    m_iconPixmap.fill(Qt::transparent);

    QPainter painter(&m_iconPixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setBrush(color);
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(4, 4, 64 - 8, 64 - 8);

    return QIcon(m_iconPixmap);
}
