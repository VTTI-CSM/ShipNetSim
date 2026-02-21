#include "RabbitMQConfigDialog.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("ShipNetSim");
    app.setOrganizationName("ShipNetSim");

    RabbitMQConfigDialog dialog;
    dialog.show();

    return app.exec();
}
