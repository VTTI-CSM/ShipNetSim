// SimulationServer.h
#ifndef SIMULATIONSERVER_H
#define SIMULATIONSERVER_H

#include <QTcpServer>
#include <QTcpSocket>
#include <QObject>
#include <QVector>
#include <QMap>
#include <QPair>
#include <QThread>
#include "SimulationWorker.h"
#include <QMetaType>

// Define a typedef for QMap<QString, QString>
using ShipParamsMap = QMap<QString, QString>;
// Declare the type with the meta-object system
Q_DECLARE_METATYPE(ShipParamsMap)

// Compilation date and time are set by the preprocessor.
const std::string compilation_date = __DATE__;
const std::string compilation_time = __TIME__;
const std::string GithubLink = "https://github.com/VTTI-CSM/ShipNetSim";

class SimulationServer : public QTcpServer {
    Q_OBJECT

public:
    explicit SimulationServer(QObject *parent = nullptr);
    ~SimulationServer();

    void startServer(quint16 port);

signals:
    void networkLoaded();
    void shipReachedDestination(const QString& shipID);
    void simulationResultsAvailable(
        const QVector<std::pair<QString, QString>>& summaryData,
        const QString& trajectoryFile);

protected:
    void incomingConnection(qintptr socketDescriptor) override;

private slots:
    void onClientDisconnected();
    void onReadyRead();
    void handleError(const QString &errorMessage);

private:
    QTcpSocket *clientSocket;
    QThread *workerThread;
    SimulationWorker *simulationWorker;
    bool isBusy;

    // Send data to the client
    void sendMessageToClient(const QString &message,
                             const QVariant &data = QVariant());
};

#endif // SIMULATIONSERVER_H
