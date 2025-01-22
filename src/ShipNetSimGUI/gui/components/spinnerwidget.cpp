#include "spinnerwidget.h"
#include <QPainter>

SpinnerWidget::SpinnerWidget(QWidget* parent)
    : QWidget(parent)
    , m_currentAngle(0)
    , m_timerId(-1)
    , m_stepInterval(40)
    , m_visibleWhenIdle(false)
    , m_spinnerColor(Qt::black)
{
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    setFocusPolicy(Qt::NoFocus);
}

bool SpinnerWidget::isSpinning() const
{
    return (m_timerId != -1);
}

void SpinnerWidget::setVisibleWhenIdle(bool state)
{
    m_visibleWhenIdle = state;
    update();
}

bool SpinnerWidget::isVisibleWhenIdle() const
{
    return m_visibleWhenIdle;
}

void SpinnerWidget::startSpinning()
{
    m_currentAngle = 0;

    if (m_timerId == -1)
        m_timerId = startTimer(m_stepInterval);
}

void SpinnerWidget::stopSpinning()
{
    if (m_timerId != -1)
        killTimer(m_timerId);

    m_timerId = -1;
    update();
}

void SpinnerWidget::setStepInterval(int interval)
{
    if (m_timerId != -1)
        killTimer(m_timerId);

    m_stepInterval = interval;

    if (m_timerId != -1)
        m_timerId = startTimer(m_stepInterval);
}

void SpinnerWidget::setSpinnerColor(const QColor& color)
{
    m_spinnerColor = color;
    update();
}

QSize SpinnerWidget::sizeHint() const
{
    return QSize(20, 20);
}

int SpinnerWidget::heightForWidth(int width) const
{
    return width;
}

void SpinnerWidget::timerEvent(QTimerEvent* /*event*/)
{
    m_currentAngle = (m_currentAngle + 30) % 360;
    update();
}

void SpinnerWidget::paintEvent(QPaintEvent* /*event*/)
{
    if (!m_visibleWhenIdle && !isSpinning())
        return;

    int dimension = qMin(this->width(), this->height());

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    int outerRadius = (dimension - 1) * 0.5;
    int innerRadius = (dimension - 1) * 0.5 * 0.38;

    int capsuleHeight = outerRadius - innerRadius;
    int capsuleWidth = (dimension > 32) ? capsuleHeight * .23 : capsuleHeight * .35;
    int capsuleRadius = capsuleWidth / 2;

    for (int i = 0; i < 12; i++) {
        QColor color = m_spinnerColor;
        color.setAlphaF(1.0f - (i / 12.0f));
        painter.setPen(Qt::NoPen);
        painter.setBrush(color);
        painter.save();
        painter.translate(rect().center());
        painter.rotate(m_currentAngle - i * 30.0f);
        painter.drawRoundedRect(-capsuleWidth * 0.5, -(innerRadius + capsuleHeight), capsuleWidth,
                                capsuleHeight, capsuleRadius, capsuleRadius);
        painter.restore();
    }
}
