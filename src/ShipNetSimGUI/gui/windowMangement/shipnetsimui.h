#ifndef SHIPNETSIMUI_H
#define SHIPNETSIMUI_H

#include <QMainWindow>

#include "shipnetsim.h"

class ShipNetSimUI {
public:
    static void showNotificationBox(QString msg);
    static void showWarningBox(QString msg);
    static void showErrorBox(QString msg);
    static void showWarning(ShipNetSim *parent, QString text);
    static void showNotification(ShipNetSim *parent, QString text);
    static void handleError(ShipNetSim *parent, QString error);
    static void openManual(ShipNetSim *parent);
    static void openSettingsPage(ShipNetSim *parent);
    static void openAboutPage(ShipNetSim *parent);
    static void handleSampleProject(ShipNetSim *parent);
    static void closeApplication(ShipNetSim *parent);
    static QString browseFiles(ShipNetSim *parent,
                               QLineEdit* theLineEdit,
                               const QString& theFileName);
    static void browseFolder(ShipNetSim *parent,
                             QLineEdit* theLineEdit,
                             const QString& theHelpMessage);
};
#endif // SHIPNETSIMUI_H
