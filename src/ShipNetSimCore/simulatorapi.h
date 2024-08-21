#ifndef SIMULATORAPI_H
#define SIMULATORAPI_H

#include "export.h"
#include <QObject>
#include "simulator.h"
#include "ship/ship.h"
#include "network/optimizednetwork.h"

class SHIPNETSIM_EXPORT SimulatorAPI : public QObject
{
    Q_OBJECT
public:
    static SimulatorAPI& getInstance() {
        static SimulatorAPI instance;
        return instance;
    }

    // Simulator control methods
    void initializeSimulator(ShipNetSimCore::OptimizedNetwork* network,
                             QVector<std::shared_ptr<ShipNetSimCore::Ship>> shipList,
                             units::time::second_t timeStep);
    ShipNetSimCore::Simulator& getSimulator();
    void initSimulation();
    void pauseSimulation();
    void resumeSimulation();
    void endSimulation();
    void runOneTimeStep();
    void requestSimulationCurrentResults();

    // Ship control methods
    void addShipToSimulation(const QMap<QString, std::any>& shipParameters);
    std::shared_ptr<ShipNetSimCore::Ship> getShipByID(const QString& shipID);
    QVector<std::shared_ptr<ShipNetSimCore::Ship>> getAllShips() const;

signals:
    void shipReachedDestination(const QString& shipID);
    void simulationResultsAvailable(
        const QVector<std::pair<QString, QString>>& summaryData,
        const QString& trajectoryFile);

private slots:
    void handleShipReachedDestination(const QString& shipID);
    void handleSimulationResults(
        const QVector<std::pair<QString, QString>>& summaryData,
        const QString& trajectoryFile);

private:
    SimulatorAPI() = default;
    SimulatorAPI(const SimulatorAPI&) = delete;
    SimulatorAPI& operator=(const SimulatorAPI&) = delete;

    std::unique_ptr<ShipNetSimCore::Simulator> simulator;
    QMap<QString, std::shared_ptr<ShipNetSimCore::Ship>> ships;
};

#endif // SIMULATORAPI_H
