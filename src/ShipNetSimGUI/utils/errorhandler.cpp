#include <QDebug>
#include "errorhandler.h"
#include <QMessageBox>

void ErrorHandler::showNotification(QString msg) {
    QMessageBox::information(nullptr, "NeTrainSim - Notification", msg);
}

void ErrorHandler::showWarning(QString msg) {
    QMessageBox::warning(nullptr, "NeTrainSim - Warning", msg);
}

void ErrorHandler::showError(QString msg) {
    QMessageBox::critical(nullptr, "NeTrainSim - Error", msg);
}
