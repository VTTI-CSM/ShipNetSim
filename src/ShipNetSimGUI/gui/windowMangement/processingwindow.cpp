#include "processingwindow.h"
#include "ui_processingwindow.h"

ProcessingWindow::ProcessingWindow(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::ProcessingWindow)
{
    ui->setupUi(this);
}

ProcessingWindow::~ProcessingWindow()
{
    delete ui;
}

void ProcessingWindow::showEvent(QShowEvent *event)
{
    QDialog::showEvent(event); // Call the base class implementation
    if (ui->widget) {
        ui->widget->setStepInterval(1);  // spin fast
        ui->widget->setVisibleWhenIdle(true);   // make it always visible
        ui->widget->startSpinning(); // Start the spinner when shown
    }
}

void ProcessingWindow::hideEvent(QHideEvent *event)
{
    QDialog::hideEvent(event); // Call the base class implementation
    if (ui->widget) {
        ui->widget->stopSpinning(); // Stop the spinner when hidden
    }
}
