#ifndef SIMULATIONWORKER_H
#define SIMULATIONWORKER_H

#include <QObject>
#include <QMap>
#include <QString>
#include <QVector>
#include <memory>
#include "simulator.h"
#include "ship/ship.h"
#include "network/optimizednetwork.h"

class SimulationWorker : public QObject {
    Q_OBJECT

public:
    explicit SimulationWorker(QObject *parent = nullptr);

    Q_INVOKABLE void initializeSimulator(double timeStep);
    Q_INVOKABLE void pauseSimulation();
    Q_INVOKABLE void resumeSimulation();
    Q_INVOKABLE void endSimulation();
    Q_INVOKABLE void runOneTimeStep();
    Q_INVOKABLE void requestSimulationCurrentResults();
    Q_INVOKABLE void addShipToSimulation(const QMap<QString, QString>& shipParameters);

signals:
    void simulationNetworkLoaded();
    void simulationInitialized();
    void simulationPaused();
    void simulationResumed();
    void simulationEnded();
    void simulationAdvanced(double newSimulationTime);
    void shipAddedToSimulator();
    void simulationResultsRetrieved();
    void shipReachedDestination(const QString& shipID);
    void simulationResultsAvailable(
        const QVector<QPair<QString, QString>>& summaryData,
        const QString& trajectoryFile);
    void errorOccurred(const QString &errorMessage);


private:
    ShipNetSimCore::Simulator* simulator;
    std::unique_ptr<ShipNetSimCore::OptimizedNetwork> network;
    QMap<QString, std::shared_ptr<ShipNetSimCore::Ship>> ships;
};

#endif // SIMULATIONWORKER_H
