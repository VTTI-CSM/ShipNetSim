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
signals:
    void shipReachedDestination(const QJsonObject shipStatus);
    void simulationResultsAvailable(ShipsResults& results);
    void simulationReachedReportingTime(units::time::second_t simulationTime);

private slots:
    void handleSimulationReachedReportingTime(
        units::time::second_t simulationTime);
    void handleShipReachedDestination(const QJsonObject shipStatus);
    void handleSimulationResults(ShipsResults& results);

private:
    SimulatorAPI() = default;
    SimulatorAPI(const SimulatorAPI&) = delete;
    SimulatorAPI& operator=(const SimulatorAPI&) = delete;

    // Create a new thread
    QThread* apiThread;

    std::unique_ptr<ShipNetSimCore::Simulator> simulator;
    QMap<QString, std::shared_ptr<ShipNetSimCore::Ship>> ships;


    static SimulatorAPI& getInstance() {
        static SimulatorAPI instance;
        return instance;
    }


    // Simulator control methods
    void initializeSimulator(
        ShipNetSimCore::OptimizedNetwork* network,
        QVector<std::shared_ptr<ShipNetSimCore::Ship>> shipList,
        units::time::second_t timeStep,
        bool runAsAServer = false);

    ShipNetSimCore::Simulator& getSimulator();

    // Ship control methods
    void addShipToSimulation(const QMap<QString, std::any>& shipParameters);
    std::shared_ptr<ShipNetSimCore::Ship> getShipByID(const QString& shipID);
    QVector<std::shared_ptr<ShipNetSimCore::Ship>> getAllShips() const;

    void initSimulation();
    void pauseSimulation();
    void resumeSimulation();
    void endSimulation();

    void runOneTimeStep();

    void requestSimulationCurrentResults();

public:
    class SHIPNETSIM_EXPORT InteractiveMode {
    public:
        static void defineSimulator(
            ShipNetSimCore::OptimizedNetwork* network,
            QVector<std::shared_ptr<ShipNetSimCore::Ship>> shipList,
            units::time::second_t timeStep,
            bool runAsAServer = false);
        static ShipNetSimCore::Simulator& getSimulator();
        // Ship control methods
        static void addShipToSimulation(
            const QMap<QString, std::any>& shipParameters);
        static std::shared_ptr<ShipNetSimCore::Ship>
        getShipByID(const QString& shipID);

        static QVector<std::shared_ptr<ShipNetSimCore::Ship>>
        getAllShips();


        static void initSimulation();
        static void runOneTimeStep();
        static void runSimulation(double byTimeSteps);
        static void endSimulation();

        static void requestSimulationCurrentResults();
    };

    class SHIPNETSIM_EXPORT ContinuousMode {
    public:
        static void defineSimulator(
            ShipNetSimCore::OptimizedNetwork* network,
            QVector<std::shared_ptr<ShipNetSimCore::Ship>> shipList,
            units::time::second_t timeStep,
            bool runAsAServer = false);

        static ShipNetSimCore::Simulator& getSimulator();

        static void runSimulation();
        static void pauseSimulation();
        static void resumeSimulation();

        static std::shared_ptr<ShipNetSimCore::Ship>
        getShipByID(const QString& shipID);

        static QVector<std::shared_ptr<ShipNetSimCore::Ship>> getAllShips();
    };

};

#endif // SIMULATORAPI_H
