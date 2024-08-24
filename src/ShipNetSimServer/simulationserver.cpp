#include "SimulationServer.h"
#include "SimulationWorker.h"
#include <QDataStream>
#include <QDebug>
#include <QMap>
#include <QTimer>
#include "./VersionConfig.h"
#include "qobjectdefs.h"

SimulationServer::SimulationServer(QObject *parent)
    : QTcpServer(parent), clientSocket(nullptr),
    workerThread(new QThread(this)),
    simulationWorker(new SimulationWorker()),
    isBusy(false) {

    // Register the typedef with the meta-object system
    qRegisterMetaType<ShipParamsMap>("ShipParamsMap");

    simulationWorker->moveToThread(workerThread);
    workerThread->start();

    // Connect worker signals to server slots
    QVector<QPair<void(SimulationWorker::*)(), QString>> signalMapping = {
        QPair<void(SimulationWorker::*)(),
              QString>(&SimulationWorker::simulationInitialized,
                       "simulationInitialized"),
        QPair<void(SimulationWorker::*)(),
              QString>(&SimulationWorker::simulationPaused,
                       "simulationPaused"),
        QPair<void(SimulationWorker::*)(),
              QString>(&SimulationWorker::simulationResumed,
                       "simulationResumed"),
        QPair<void(SimulationWorker::*)(),
              QString>(&SimulationWorker::simulationEnded,
                       "simulationEnded"),
        QPair<void(SimulationWorker::*)(),
              QString>(&SimulationWorker::simulationNetworkLoaded,
                       "networkLoaded"),
        QPair<void(SimulationWorker::*)(),
              QString>(&SimulationWorker::shipAddedToSimulator,
                       "ShipAdded")
    };

    for (const auto &pair : signalMapping) {
        connect(simulationWorker, pair.first, this, [this, pair]() {
            this->sendMessageToClient(pair.second);
            this->isBusy = false; // Set isBusy to false after handling the signal
        });
    }

    // Handle signals with parameters separately
    connect(simulationWorker, &SimulationWorker::simulationAdvanced,
            this, [this](double newSimulationTime) {
        sendMessageToClient("simulationAdvanced", newSimulationTime);
        isBusy = false;
    });

    connect(simulationWorker, &SimulationWorker::shipReachedDestination,
            this, [this](const QString &shipID) {
        sendMessageToClient("shipReachedDestination", shipID);
        isBusy = false;
    });

    connect(simulationWorker, &SimulationWorker::simulationResultsAvailable,
            this, [this](const QVector<QPair<QString, QString>> &summaryData,
                         const QString &trajectoryFile) {
        QVariant results;
        results.setValue(QPair<QVector<QPair<QString, QString>>,
                               QString>(summaryData, trajectoryFile));
        sendMessageToClient("simulationResultsAvailable", results);
        isBusy = false;
    });

    connect(simulationWorker, &SimulationWorker::errorOccurred,
            this, &SimulationServer::handleError);

}

SimulationServer::~SimulationServer() {
    workerThread->quit();
    workerThread->wait();
    delete simulationWorker;
}

void SimulationServer::startServer(quint16 port) {
    if (!this->listen(QHostAddress::Any, port)) {
        qCritical() << "Unable to start the server: " << this->errorString();
    } else {
        std::stringstream hellos;
        hellos << ShipNetSim_NAME <<
            " [Version " << ShipNetSim_VERSION << ", "  <<
            compilation_date << " " << compilation_time <<
            " Build" <<  "]" << std::endl;
        hellos << ShipNetSim_VENDOR << std::endl;
        hellos << GithubLink << std::endl;
        std::cout << hellos.str() << "\n";

        std::cout << "Server started on port:" << port << "\n";

        QTimer* timer = new QTimer(this);
        connect(timer, &QTimer::timeout, this, [this]() {
            if (!isBusy) {
                std::cout << "Server is running and waiting for requests...\n";
            }
        });
        timer->start(60'000);
    }
}

void SimulationServer::incomingConnection(qintptr socketDescriptor) {
    clientSocket = new QTcpSocket(this);
    clientSocket->setSocketDescriptor(socketDescriptor);

    connect(clientSocket, &QTcpSocket::disconnected,
            this, &SimulationServer::onClientDisconnected);
    connect(clientSocket, &QTcpSocket::readyRead,
            this, &SimulationServer::onReadyRead);

    qDebug() << "Client connected with descriptor:" << socketDescriptor;
}

void SimulationServer::onClientDisconnected() {
    qDebug() << "Client disconnected";
    clientSocket->deleteLater();
    clientSocket = nullptr;
}

void SimulationServer::onReadyRead() {
    if (!clientSocket) return;

    isBusy = true;

    QDataStream in(clientSocket);
    in.setVersion(QDataStream::Qt_6_0);

    QString command;
    in >> command;

    if (command == "initializeSimulator") {
        double timeStepValue;
        in >> timeStepValue;
        std::cout << "Initializing the simulator. "
                  << command.toStdString() << "\n";
        QMetaObject::invokeMethod(simulationWorker,
                                  "initializeSimulator",
                                  Qt::QueuedConnection,
                                  Q_ARG(double, timeStepValue));
    } else if (command == "pauseSimulation") {
        std::cout << "Pausing the simulator. "
                  << command.toStdString() << "\n";
        QMetaObject::invokeMethod(simulationWorker,
                                  "pauseSimulation",
                                  Qt::QueuedConnection);
    } else if (command == "resumeSimulation") {
        std::cout << "Resuming the simulator. "
                  << command.toStdString() << "\n";
        QMetaObject::invokeMethod(simulationWorker,
                                  "resumeSimulation",
                                  Qt::QueuedConnection);
    } else if (command == "endSimulation") {
        std::cout << "Ending the simulator. "
                  << command.toStdString() << "\n";
        QMetaObject::invokeMethod(simulationWorker,
                                  "endSimulation", Qt::QueuedConnection);
    } else if (command == "runOneTimeStep") {
        std::cout << "Running one time step. "
                  << command.toStdString() << "\n";
        QMetaObject::invokeMethod(simulationWorker,
                                  "runOneTimeStep",
                                  Qt::QueuedConnection);
    } else if (command == "requestSimulationCurrentResults") {
        std::cout << "Requesting simulator results. "
                  << command.toStdString() << "\n";
        QMetaObject::invokeMethod(simulationWorker,
                                  "requestSimulationCurrentResults",
                                  Qt::QueuedConnection);
    } else if (command == "addShipToSimulation") {
        std::cout << "Adding ship to the simulator. "
                  << command.toStdString() << "\n";
        ShipParamsMap shipParameters;
        in >> shipParameters;
        QMetaObject::invokeMethod(simulationWorker,
                                  "addShipToSimulation",
                                  Qt::QueuedConnection,
                                  Q_ARG(ShipParamsMap, shipParameters));
    } else {
        std::cout << "Unknown Command: " << command.toStdString() << "\n";
        handleError("Unknown command received: " + command);
    }
}

void SimulationServer::sendMessageToClient(const QString &message,
                                           const QVariant &data) {
    if (!clientSocket) {
        qWarning() << "No client connected, cannot send message: " << message;
        return;
    }

    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_6_0);

    out << message;

    if (!data.isNull()) {
        out << data;
    }

    clientSocket->write(block);
    clientSocket->flush();
}

void SimulationServer::handleError(const QString &errorMessage) {
    qCritical() << errorMessage;
    if (clientSocket) {
        QByteArray block;
        QDataStream out(&block, QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_6_0);
        out << QString("error") << errorMessage;
        clientSocket->write(block);
        clientSocket->flush();
    }
}
