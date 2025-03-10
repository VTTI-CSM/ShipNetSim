#ifndef SIMULATIONSERVER_H
#define SIMULATIONSERVER_H

#include "./simulatorapi.h"
#include "qwaitcondition.h"
#include "utils/shipscommon.h"
#include <QObject>
#include <QMap>
#include <QJsonDocument>
#include <QJsonObject>
#include <rabbitmq-c/amqp.h>
#include <rabbitmq-c/tcp_socket.h>
#include <containerLib/container.h>
#ifdef _WIN32
struct timeval {
    long tv_sec;  // seconds
    long tv_usec; // microseconds
};
#else
#include <sys/time.h>  // Unix-like systems
#endif
using ShipParamsMap = QMap<QString, QString>;
Q_DECLARE_METATYPE(ShipParamsMap)

class SimulationServer : public QObject {
    Q_OBJECT

public:
    explicit SimulationServer(QObject *parent = nullptr);
    ~SimulationServer();
    void startRabbitMQServer(const std::string &hostname, int port);
    void sendRabbitMQMessage(const QString &routingKey, const QJsonObject &message);
    void stopRabbitMQServer();  // stop RabbitMQ server cleanly

signals:
    void dataReceived(QJsonObject message);
    void shipReachedDestination(const QString &shipID);
    void simulationResultsAvailable(ShipsResults results);
    void stopConsuming();

private slots:
    void onDataReceivedFromRabbitMQ(QJsonObject &message,
                                    const amqp_envelope_t &envelope);
    void onWorkerReady();  // resume processing when the worker is ready

    // simulation events
    void onSimulationNetworkLoaded(QString networkName);
    void onSimulationCreated(QString networkName);
    void onSimulationPaused(QVector<QString> networkNames);
    void onSimulationResumed(QVector<QString> networkNames);
    void onSimulationRestarted(QVector<QString> networkNames);
    void onSimulationTerminated(QVector<QString> networkNames);
    void onSimulationFinished(QString networkName);
    void onSimulationAdvanced(
        QMap<QString,
             QPair<units::time::second_t, double> > newSimulationTime);
    void onSimulationProgressUpdate(QString networkName,
                                    int progressPercentage);
    void onShipAddedToSimulator(const QString networkName,
                                const QVector<QString> shipIDs);
    void onAllShipsReachDestination(const QString networkName);
    void onShipReachedDestination(const QJsonObject shipStatus);
    void onSimulationResultsAvailable(QPair<QString, ShipsResults> results);
    void onShipStateAvailable(QString networkName, QString shipID,
                              const QJsonObject shipState);
    void onSimulatorStateAvailable(const QJsonObject simulatorState);
    void onContainersAddedToShip(QString networkName, QString shipID);
    void onShipReachedSeaPort(QString networkName, QString shipID,
                              QString seaPortCode, qsizetype containersCount);
    void onPortsAvailable(QMap<QString,
                               QVector<QString>> networkPorts);
    void onContainersUnloaded(QString networkName,
                              QString shipID,
                              QString seaPortName,
                              QJsonArray containers);

    void onErrorOccurred(const QString& errorMessage);
    void onServerReset();

private:
    std::string mHostname;
    int mPort;
    QMutex mMutex;  // Mutex for protecting access to mWorkerBusy
    bool mWorkerBusy;  // To control the server run loop
    QThread *mRabbitMQThread = nullptr;
    QWaitCondition mWaitCondition;
    amqp_connection_state_t mRabbitMQConnection;
    QMetaObject::Connection m_progressConnection;

    void processCommand(QJsonObject &jsonMessage);
    void consumeFromRabbitMQ();  // Function for consuming RabbitMQ messages
    void startConsumingMessages();
    void reconnectToRabbitMQ();
    void setupServer();

};

#endif // SIMULATIONSERVER_H
