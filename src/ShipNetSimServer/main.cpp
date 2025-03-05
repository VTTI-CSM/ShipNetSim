#include <QCoreApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include "SimulationServer.h"
#include "utils/logger.h"
#include <QLocalSocket>
#include <QLocalServer>

bool isAnotherInstanceRunning(const QString &serverName) {
    QLocalSocket socket;
    socket.connectToServer(serverName);
    if (socket.waitForConnected(100)) {
        return true; // Another instance is already running
    }
    return false; // No instance running
}

void createLocalServer(const QString &serverName) {
    QLocalServer *localServer = new QLocalServer();
    localServer->setSocketOptions(QLocalServer::WorldAccessOption);
    if (!localServer->listen(serverName)) {
        qCritical() << "Failed to create local server:"
                    << localServer->errorString();
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);

    // Unique name for the local server
    const QString uniqueServerName = "ShipNetSimServerInstance";

    // Check if another instance is already running
    if (isAnotherInstanceRunning(uniqueServerName)) {
        qCritical() << "Another instance of ShipNetSim "
                       "Server is already running.";
        return EXIT_FAILURE;
    }

    // Create the local server to mark this instance as the active one
    createLocalServer(uniqueServerName);

    // Attach the logger first thing:
    ShipNetSimCore::Logger::attach("ShipNetSimServer");
    ShipNetSimCore::Logger::setStdOutMinLogLevel(QtInfoMsg);

    // Set up the command-line parser
    QCommandLineParser parser;
    parser.setApplicationDescription("ShipNetSim Server with RabbitMQ");
    parser.addHelpOption();

    // Add hostname option
    QCommandLineOption hostnameOption(
        QStringList() << "n" << "hostname",
        "RabbitMQ server hostname (default: localhost).",
        "hostname",
        "localhost");
    parser.addOption(hostnameOption);

    // Add port option (default: 5672)
    QCommandLineOption portOption(
        QStringList() << "p" << "port",
        "RabbitMQ server port (default: 5672).",
        "port",
        "5672");
    parser.addOption(portOption);

    // Process the command-line arguments
    parser.process(app);

    // Retrieve the hostname and port from CLI arguments or use default values
    QString hostname = parser.value(hostnameOption);
    int port = parser.value(portOption).toInt();

    // Start the simulation server
    SimulationServer server;
    server.startRabbitMQServer(hostname.toStdString(), port);

    QObject::connect(&app, &QCoreApplication::aboutToQuit, []() {
        ShipNetSimCore::Logger::detach();
    });

    return app.exec();
}
