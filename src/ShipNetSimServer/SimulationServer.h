#ifndef SIMULATIONSERVER_H
#define SIMULATIONSERVER_H

#include "SimulationWorker.h"
#include "utils/shipscommon.h"
#include <QObject>
#include <QMap>
#include <QThread>
#include <QJsonDocument>
#include <QJsonObject>
#include <amqp.h>
#include <amqp_tcp_socket.h>

using ShipParamsMap = QMap<QString, QString>;
Q_DECLARE_METATYPE(ShipParamsMap)

class SimulationServer : public QObject {
    Q_OBJECT

public:
    explicit SimulationServer(QObject *parent = nullptr);
    ~SimulationServer();
    void startRabbitMQServer(const std::string &hostname, int port);
    void sendRabbitMQMessage(const QString &queue, const QJsonObject &message);
    void stopRabbitMQServer();  // stop RabbitMQ server cleanly

signals:
    void dataReceived(QJsonObject message);
    void shipReachedDestination(const QString &shipID);
    void simulationResultsAvailable(ShipsResults &results);

private slots:
    void onDataReceivedFromRabbitMQ(const QJsonObject &message,
                                    const amqp_envelope_t &envelope);
    void onWorkerReady();  // resume processing when the worker is ready

    // simulation events
    void onSimulationNetworkLoaded();
    void onSimulationInitialized();
    void onSimulationPaused();
    void onSimulationResumed();
    void onSimulationRestarted();
    void onSimulationEnded();
    void onSimulationAdvanced(double newSimulationTime);
    void onSimulationProgressUpdate(int progressPercentage);
    void onShipAddedToSimulator(const QString shipID);
    void onShipReachedDestination(const QJsonObject shipStatus);
    void onSimulationResultsAvailable(ShipsResults& results);
    void onShipStateAvailable(const QJsonObject shipState);
    void onSimulatorStateAvailable(const QJsonObject simulatorState);
    void onErrorOccurred(const QString& errorMessage);

private:

    std::string mHostname;
    int mPort;

    void processCommand(const QJsonObject &jsonMessage);

    amqp_connection_state_t mRabbitMQConnection;
    std::unique_ptr<ContinuousSimulationWorker> mSimulationWorker;  // Declare the worker
    bool mWorkerBusy;  // To control the server run loop

    QQueue<QJsonObject> mCommandQueue;  // Queue to store incoming commands
};

#endif // SIMULATIONSERVER_H
