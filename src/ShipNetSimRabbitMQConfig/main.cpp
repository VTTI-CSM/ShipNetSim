#include "RabbitMQConfigDialog.h"
#include <QApplication>
#include <QIcon>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("ShipNetSim");
    app.setOrganizationName("ShipNetSim");
    app.setWindowIcon(QIcon(":/Logo256x256.png"));

    RabbitMQConfigDialog dialog;
    dialog.show();

    return app.exec();
}
