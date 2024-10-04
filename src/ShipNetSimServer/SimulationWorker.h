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

    Q_INVOKABLE void endSimulation();
    Q_INVOKABLE void requestSimulationCurrentResults();
    Q_INVOKABLE void addShipsToSimulation(QJsonObject shipParams);
    Q_INVOKABLE void requestShipCurrentState(QString shipID);
    Q_INVOKABLE void requestSimulatorStatus();

private:
    void addShipToSimulation(
        const QMap<QString, QString>& shipParameters);
    void addShipsToSimulation(
        const QVector<QMap<QString, QString>> shipsParameters);

signals:
    void simulatorNetworkLoaded();
    void simulatorDefined();
    void simulatorRestarted();
    void simulatorEnded();
    void shipAddedToSimulator(const QString shipID);
    void shipCurrentStateRequested(const QJsonObject shipState);
    void simulatorCurrentStateRequested(const QJsonObject simulatorState);
    void simulationResultsRequested();
    void shipReachedDestination(const QJsonObject shipState);
    void simulationResultsAvailable(ShipsResults& results);
    void errorOccurred(const QString &errorMessage);
    void workerReady(); // indicate the worker is ready for the next command


protected:
    ShipNetSimCore::Simulator* simulator;
    std::unique_ptr<ShipNetSimCore::OptimizedNetwork> network;
    QMap<QString, std::shared_ptr<ShipNetSimCore::Ship>> ships;
};

// Step-by-step simulation worker
class StepSimulationWorker : public SimulationWorker {
    Q_OBJECT

public:
    explicit StepSimulationWorker(QObject *parent = nullptr);
    Q_INVOKABLE void defineSimulator(
        double timeStep,
        QJsonObject shipsParams = QJsonObject(),
        QMetaMethod endOfInitsignalToEmit =
        QMetaMethod::fromSignal(&SimulationWorker::simulatorDefined));

    Q_INVOKABLE void restartSimulator(double timeStep_sec,
                                      QJsonObject shipsParams = QJsonObject());
    Q_INVOKABLE void runOneTimeStep();  // Run simulation step-by-step

signals:
    void simulationAdvanced(double newSimulationTime);  // Notify when the simulation advances
};

// Continuous simulation worker
class ContinuousSimulationWorker : public SimulationWorker {
    Q_OBJECT

public:
    explicit ContinuousSimulationWorker(QObject *parent = nullptr);

    Q_INVOKABLE void defineSimulator(
        double timeStep,
        QJsonObject shipsParams = QJsonObject(),
        QMetaMethod endOfInitsignalToEmit =
        QMetaMethod::fromSignal(&SimulationWorker::simulatorDefined));
    Q_INVOKABLE void restartSimulator(double timeStep_sec,
                                      QJsonObject shipsParams = QJsonObject());
    Q_INVOKABLE void runSimulation();  // Run simulation continuously
    Q_INVOKABLE void pauseSimulator();
    Q_INVOKABLE void resumeSimulator();

signals:
    void simulationPaused();
    void simulationResumed();
    void progressUpdated(int progressPercentage);

};

#endif // SIMULATIONWORKER_H
