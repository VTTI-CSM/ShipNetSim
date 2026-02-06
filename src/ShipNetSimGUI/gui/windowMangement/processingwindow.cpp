#include "processingwindow.h"
#include "ui_processingwindow.h"
#include <QColor>

ProcessingWindow::ProcessingWindow(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::ProcessingWindow)
{
    ui->setupUi(this);
    // Frameless non-modal dialog - stays in front of parent only (not all apps)
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setModal(false);  // Non-modal so user can still interact with main window

    // Professional blue color for spinner
    if (ui->widget) {
        ui->widget->setSpinnerColor(QColor("#0078D4"));
    }
}

ProcessingWindow::~ProcessingWindow()
{
    delete ui;
}

void ProcessingWindow::setTitle(const QString& title)
{
    ui->labelTitle->setText(title);
}

void ProcessingWindow::setProgress(int percentage)
{
    ui->progressBar->setValue(percentage);
    ui->progressBar->setVisible(true);
    // Keep spinner visible to indicate background work is happening
    if (ui->widget && !ui->widget->isVisible()) {
        ui->widget->setVisible(true);
        ui->widget->startSpinning();
    }
}

void ProcessingWindow::setStatusText(const QString& status)
{
    ui->labelStatus->setText(status);
}

QString ProcessingWindow::formatElapsedTime(double seconds)
{
    if (seconds < 60) {
        return QString("%1s").arg(seconds, 0, 'f', 1);
    }

    int totalSeconds = static_cast<int>(seconds);
    int days = totalSeconds / 86400;
    int hours = (totalSeconds % 86400) / 3600;
    int minutes = (totalSeconds % 3600) / 60;
    int secs = totalSeconds % 60;

    if (days > 0) {
        return QString("%1d %2h %3m").arg(days).arg(hours).arg(minutes);
    } else if (hours > 0) {
        return QString("%1h %2m").arg(hours).arg(minutes);
    } else {
        return QString("%1m %2s").arg(minutes).arg(secs);
    }
}

void ProcessingWindow::reset()
{
    ui->progressBar->setValue(0);
    ui->progressBar->setVisible(false);
    ui->widget->setVisible(true);  // Show spinner
    ui->labelTitle->setText("Processing...");
    ui->labelStatus->clear();
    // Ensure spinner is running
    if (ui->widget) {
        ui->widget->setStepInterval(1);
        ui->widget->setVisibleWhenIdle(true);
        ui->widget->startSpinning();
    }
}

void ProcessingWindow::showEvent(QShowEvent *event)
{
    QDialog::showEvent(event);
    if (ui->widget) {
        ui->widget->setStepInterval(1);
        ui->widget->setVisibleWhenIdle(true);
        ui->widget->startSpinning();
    }
}

void ProcessingWindow::hideEvent(QHideEvent *event)
{
    QDialog::hideEvent(event);
    if (ui->widget) {
        ui->widget->stopSpinning();
    }
}
