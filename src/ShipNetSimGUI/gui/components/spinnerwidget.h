#ifndef SPINNERWIDGET_H
#define SPINNERWIDGET_H

#include <QWidget>

/**
    \class SpinnerWidget
    \brief The SpinnerWidget class provides a spinning widget to indicate
    that a task or process is in progress.

    This class displays an indeterminate spinning animation to signal that
    the application is busy or performing an operation.
    \sa QProgressBar
*/
class SpinnerWidget : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(int stepInterval READ getStepInterval WRITE setStepInterval)
    Q_PROPERTY(bool visibleWhenIdle READ isVisibleWhenIdle WRITE setVisibleWhenIdle)
    Q_PROPERTY(QColor spinnerColor READ getSpinnerColor WRITE setSpinnerColor)

public:
    /** Constructs a SpinnerWidget with an optional \a parent. */
    explicit SpinnerWidget(QWidget* parent = nullptr);

    /** Retrieves the interval between animation steps in milliseconds.
        \return Interval in milliseconds (default is 40 ms).
        \sa setStepInterval
     */
    int getStepInterval() const { return m_stepInterval; }

    /** Checks if the spinner animation is active.
        \return True if the spinner is running; otherwise false.
        \sa startSpinning, stopSpinning
     */
    bool isSpinning() const;

    /** Checks if the spinner is visible when not spinning.
        \return True if visible while idle; otherwise false.
        \sa setVisibleWhenIdle
     */
    bool isVisibleWhenIdle() const;

    /** Retrieves the spinner's color.
        \return The current color of the spinner.
        \sa setSpinnerColor
     */
    const QColor& getSpinnerColor() const { return m_spinnerColor; }

    virtual QSize sizeHint() const override;
    int heightForWidth(int width) const override;

public slots:
    /** Starts the spinner animation.
        \sa stopSpinning, isSpinning
     */
    void startSpinning();

    /** Stops the spinner animation.
        \sa startSpinning, isSpinning
     */
    void stopSpinning();

    /** Sets the interval between animation steps.
        \param interval Interval in milliseconds.
        \sa getStepInterval
     */
    void setStepInterval(int interval);

    /** Sets the visibility of the spinner when not spinning.
        \param state True to remain visible while idle; false to hide.
        \sa isVisibleWhenIdle
     */
    void setVisibleWhenIdle(bool state);

    /** Sets the color of the spinner.
        \param color The new color for the spinner.
        \sa getSpinnerColor
     */
    void setSpinnerColor(const QColor& color);

protected:
    virtual void timerEvent(QTimerEvent* event) override;
    virtual void paintEvent(QPaintEvent* event) override;

private:
    int m_currentAngle;
    int m_timerId;
    int m_stepInterval;
    bool m_visibleWhenIdle;
    QColor m_spinnerColor;
};

#endif // SPINNERWIDGET_H
