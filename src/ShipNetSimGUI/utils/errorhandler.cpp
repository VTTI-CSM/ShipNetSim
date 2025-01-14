#include <QMessageBox>
#include "errorhandler.h"

namespace ErrorHandler {

void showNotification(QString msg) {
    QMessageBox::information(nullptr, "ShipNetSim - Notification", msg);
}

void showWarning(QString msg) {
    QMessageBox::warning(nullptr, "ShipNetSim - Warning", msg);
}

void showError(QString msg) {
    QMessageBox::critical(nullptr, "ShipNetSim - Error", msg);
}

} // namespace ErrorHandler
