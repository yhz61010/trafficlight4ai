#include "TrafficLightWidget.h"
#include "Logger.h"
#include <QPainter>
#include <QEasingCurve>

TrafficLightWidget::TrafficLightWidget(QWidget *parent)
    : QWidget(parent)
{
    m_imgOff = QPixmap(":/images/images/light_off.png");
    m_imgRed = QPixmap(":/images/images/light_red.png");
    m_imgYellow = QPixmap(":/images/images/light_yellow.png");
    m_imgGreen = QPixmap(":/images/images/light_green.png");

    setAttribute(Qt::WA_TranslucentBackground);
    setSizePreset(Small);
}

TrafficLightWidget::SizePreset TrafficLightWidget::sizePresetFromString(const QString &size)
{
    if (size == "xsmall") return ExtraSmall;
    if (size == "small")  return Small;
    if (size == "medium") return Medium;
    if (size == "large")  return Large;
    if (size == "xlarge") return ExtraLarge;
    return Small;
}

void TrafficLightWidget::setSizePreset(SizePreset preset)
{
    m_sizePreset = preset;
    setFixedSize(sizeForPreset(preset));
    rescalePixmaps();
    update();
}

QSize TrafficLightWidget::sizeForPreset(SizePreset preset) const
{
    int targetWidth = 80;
    switch (preset) {
    case ExtraSmall: targetWidth = 33; break;
    case Small:      targetWidth = 41; break;
    case Medium:     targetWidth = 51; break;
    case Large:      targetWidth = 67; break;
    case ExtraLarge: targetWidth = 89; break;
    }

    if (m_imgOff.isNull() || m_imgOff.width() == 0)
        return {targetWidth, targetWidth * 3}; // fallback

    const qreal ratio = static_cast<qreal>(m_imgOff.height()) / m_imgOff.width();
    return {targetWidth, static_cast<int>(targetWidth * ratio)};
}

void TrafficLightWidget::rescalePixmaps()
{
    const QSize sz = size();
    const auto mode = Qt::SmoothTransformation;
    m_scaledOff = m_imgOff.scaled(sz, Qt::KeepAspectRatio, mode);
    m_scaledRed = m_imgRed.scaled(sz, Qt::KeepAspectRatio, mode);
    m_scaledYellow = m_imgYellow.scaled(sz, Qt::KeepAspectRatio, mode);
    m_scaledGreen = m_imgGreen.scaled(sz, Qt::KeepAspectRatio, mode);
}

void TrafficLightWidget::setAnimationMode(const QString &mode)
{
    m_animationMode = mode;
    if (m_state != LightState::Idle)
        startAnimation();
}

void TrafficLightWidget::setAnimationPeriodMs(int ms)
{
    m_animationPeriodMs = ms;
    if (m_state != LightState::Idle)
        startAnimation();
}

qreal TrafficLightWidget::activeAlpha() const
{
    return m_activeAlpha;
}

void TrafficLightWidget::setActiveAlpha(qreal alpha)
{
    m_activeAlpha = alpha;
    TL_LOGV("Widget", QString("setActiveAlpha: %1 state=%2")
            .arg(alpha, 0, 'f', 2).arg(static_cast<int>(m_state)));
    emit activeAlphaChanged(m_activeAlpha);
    update();
}

void TrafficLightWidget::onStateChanged(LightState newState)
{
    TL_LOGD("Widget", QString("onStateChanged: %1 -> %2")
            .arg(static_cast<int>(m_state)).arg(static_cast<int>(newState)));
    m_state = newState;

    if (m_state == LightState::Idle) {
        stopAnimation();
        m_activeAlpha = 1.0;
        TL_LOGD("Widget", "Set alpha=1.0 directly (no signal)");
    } else {
        startAnimation();
    }
    update();
}

void TrafficLightWidget::startAnimation()
{
    stopAnimation();

    TL_LOGD("Widget", QString("Start animation: mode=%1 period=%2ms")
            .arg(m_animationMode).arg(m_animationPeriodMs));
    m_animation = new QPropertyAnimation(this, "activeAlpha", this);
    m_animation->setDuration(m_animationPeriodMs);
    m_animation->setLoopCount(-1); // infinite

    if (m_animationMode == "breathing") {
        m_animation->setStartValue(0.3);
        m_animation->setKeyValueAt(0.25, 1.0);
        m_animation->setKeyValueAt(0.75, 1.0);
        m_animation->setEndValue(0.3);
        m_animation->setEasingCurve(QEasingCurve::InOutSine);
    } else {
        // Classic blink: step function between 0 and 1
        m_animation->setStartValue(0.0);
        m_animation->setKeyValueAt(0.49, 0.0);
        m_animation->setKeyValueAt(0.50, 1.0);
        m_animation->setEndValue(1.0);
    }

    m_animation->start();
}

void TrafficLightWidget::stopAnimation()
{
    if (m_animation) {
        m_animation->disconnect(); // prevent stale alpha updates after stop
        m_animation->stop();
        m_animation->deleteLater();
        m_animation = nullptr;
    }
}

void TrafficLightWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    // Draw the "all off" base image
    painter.drawPixmap(0, 0, m_scaledOff);

    // Select the active light image
    const QPixmap *activeImg = nullptr;
    switch (m_state) {
    case LightState::Working:       activeImg = &m_scaledRed;    break;
    case LightState::WaitingConfirm: activeImg = &m_scaledYellow; break;
    case LightState::Idle:          activeImg = &m_scaledGreen;  break;
    }

    // Overlay the active light with animated opacity
    if (activeImg && !activeImg->isNull()) {
        painter.setOpacity(m_activeAlpha);
        painter.drawPixmap(0, 0, *activeImg);
    }
}
