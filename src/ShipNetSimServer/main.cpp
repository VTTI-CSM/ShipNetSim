#include <QCoreApplication>
#include "SimulationServer.h"

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);

    SimulationServer server;
    server.startServer(8888); // Start the server on port 8888

    return app.exec();
}
