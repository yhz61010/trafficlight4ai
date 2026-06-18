#include "StateManager.h"
#include "Logger.h"

namespace {
const char *lightStateName(LightState state)
{
    switch (state) {
    case LightState::Working:        return "Working(RED)";
    case LightState::WaitingConfirm: return "WaitingConfirm(YELLOW)";
    case LightState::Idle:           return "Idle(GREEN)";
    }
    return "Unknown";
}
}

StateManager::StateManager(QObject *parent)
    : QObject(parent)
{
    m_timeoutTimer.setSingleShot(true);
    connect(&m_timeoutTimer, &QTimer::timeout, this, &StateManager::onTimeout);
}

LightState StateManager::state() const
{
    return m_state;
}

void StateManager::setState(LightState newState)
{
    const bool changed = (m_state != newState);
    const LightState oldState = m_state;
    m_state = newState;

    if (changed) {
        TL_LOGI("State", QString("State changed: %1 -> %2")
                .arg(QLatin1String(lightStateName(oldState)),
                     QLatin1String(lightStateName(newState))));
        emit stateChanged(m_state);
    }

    // Always refresh timeout, even on duplicate commands (e.g. repeated RED from hooks)
    if (m_state == LightState::Idle)
        stopTimer();
    else
        restartTimer();
}

void StateManager::handleCommand(const QString &command)
{
    const QString cmd = command.trimmed().toUpper();
    if (cmd == "RED")
        setState(LightState::Working);
    else if (cmd == "YELLOW")
        setState(LightState::WaitingConfirm);
    else if (cmd == "GREEN")
        setState(LightState::Idle);
    else
        TL_LOGW("State", QString("Ignored unknown command: '%1'").arg(cmd));
}

int StateManager::timeoutSec() const
{
    return m_timeoutSec;
}

void StateManager::setTimeoutSec(int sec)
{
    m_timeoutSec = sec;
    // Restart timer if currently in a non-idle state
    if (m_state != LightState::Idle)
        restartTimer();
}

void StateManager::onTimeout()
{
    setState(LightState::Idle);
}

void StateManager::restartTimer()
{
    if (m_timeoutSec > 0)
        m_timeoutTimer.start(m_timeoutSec * 1000);
    else
        stopTimer();
}

void StateManager::stopTimer()
{
    m_timeoutTimer.stop();
}
