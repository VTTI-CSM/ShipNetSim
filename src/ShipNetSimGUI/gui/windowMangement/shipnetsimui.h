#ifndef SHIPNETSIMUI_H
#define SHIPNETSIMUI_H

#include <QMainWindow>
#include <qcombobox.h>

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
    static QString saveFile(ShipNetSim *parent,
                            const QString& windowTitle,
                            const QString& fileExtensions);
    static QString browseFiles(ShipNetSim *parent,
                               QLineEdit* theLineEdit,
                               const QString& theFileName,
                               const QString &fileExtensions =
                               "DAT Files (*.DAT *.dat)");
    static void browseFolder(ShipNetSim *parent,
                             QLineEdit* theLineEdit,
                             const QString& theHelpMessage);
    static void updateGraphs(ShipNetSim *parent,
                             QComboBox *theComboBox,
                             QComboBox *axisComboBox,
                             QString trajectoryFilename);
    static void showReport();

};
#endif // SHIPNETSIMUI_H
