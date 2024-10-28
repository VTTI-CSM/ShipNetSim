#include "simulatorapi.h"

void SimulatorAPI::initializeSimulator(
    ShipNetSimCore::OptimizedNetwork* network,
    QVector<std::shared_ptr<ShipNetSimCore::Ship>> shipList,
    units::time::second_t timeStep,
    bool runAsAServer)
{
    // Set locale to US format (comma for thousands separator, dot for decimals)
    QLocale::setDefault(QLocale(QLocale::English, QLocale::UnitedStates));
    std::locale::global(std::locale("en_US.UTF-8"));
    std::cout.imbue(std::locale());

    simulator =
        std::make_unique<ShipNetSimCore::Simulator>(network,
                                                    shipList,
                                                    timeStep,
                                                    runAsAServer,
                                                    this);
    ships.clear();

    for (const auto& ship : shipList) {
        ships.insert(ship->getUserID(), ship);

        // Connect the ship's reached destination signal to the API's handler
        connect(ship.get(), &ShipNetSimCore::Ship::reachedDestination,
                this, &SimulatorAPI::handleShipReachedDestination);
    }

    // Connect the simulator's signal for simulation results to the API's slot
    connect(simulator.get(),
            &ShipNetSimCore::Simulator::simulationResultsAvailable,
            this, &SimulatorAPI::handleSimulationResults);
    connect(simulator.get(),
            &ShipNetSimCore::Simulator::simulationReachedReportingTime,
            this, &SimulatorAPI::handleSimulationReachedReportingTime);
}

ShipNetSimCore::Simulator& SimulatorAPI::getSimulator() {
    return *simulator;
}


void SimulatorAPI::
    handleSimulationReachedReportingTime(units::time::second_t simulationTime)
{
    emit simulationReachedReportingTime(simulationTime);
}

void SimulatorAPI::handleSimulationResults(ShipsResults &results)
{   
    emit simulationResultsAvailable(results);
}

void SimulatorAPI::initSimulation() {
    if (simulator) {
        simulator->initSimulation();
    }
}

void SimulatorAPI::pauseSimulation() {
    if (simulator) {
        simulator->pauseSimulation();
    }
}

void SimulatorAPI::resumeSimulation() {
    if (simulator) {
        simulator->resumeSimulation();
    }
}

void SimulatorAPI::endSimulation() {
    if (simulator) {
        simulator->stopSimulation();
    }

    if (apiThread && apiThread->isRunning()) {
        apiThread->terminate(); // Forcefully kill the thread
    }
}

void SimulatorAPI::runOneTimeStep() {
    if (simulator) {
        simulator->playShipsOneTimeStep();
    }
}

void SimulatorAPI::requestSimulationCurrentResults() {
    if (simulator) {
        // Request the simulator to provide current results
        simulator->exportSimulationResults(false);
    }
}

void SimulatorAPI::addShipToSimulation(
    const QMap<QString, std::any>& shipParameters) {

    if (simulator) {
        auto ship = std::make_shared<ShipNetSimCore::Ship>(shipParameters);
        ships.insert(ship->getUserID(), ship);
        simulator->addShipToSimulation(ship);

        // Connect the ship's reached destination signal to the API's handler
        connect(ship.get(), &ShipNetSimCore::Ship::reachedDestination,
                this, &SimulatorAPI::handleShipReachedDestination);
    }
}

std::shared_ptr<ShipNetSimCore::Ship>
SimulatorAPI::getShipByID(const QString& shipID) {
    return ships.value(shipID, nullptr);
}

QVector<std::shared_ptr<ShipNetSimCore::Ship>>
SimulatorAPI::getAllShips() const {
    return ships.values().toVector();
}


void SimulatorAPI::handleShipReachedDestination(const QJsonObject shipStatus) {
    emit shipReachedDestination(shipStatus);
}


// Implementation for InteractiveMode class

void SimulatorAPI::InteractiveMode::defineSimulator(
    ShipNetSimCore::OptimizedNetwork* network,
    QVector<std::shared_ptr<ShipNetSimCore::Ship>> shipList,
    units::time::second_t timeStep,
    bool runAsAServer)
{
    SimulatorAPI::getInstance().initializeSimulator(network, shipList,
                                                    timeStep, runAsAServer);
}

ShipNetSimCore::Simulator& SimulatorAPI::InteractiveMode::getSimulator() {
    return SimulatorAPI::getInstance().getSimulator();
}

void SimulatorAPI::InteractiveMode::initSimulation() {
    SimulatorAPI::getInstance().getSimulator().initSimulation();
}

void SimulatorAPI::InteractiveMode::runOneTimeStep() {
    SimulatorAPI::getInstance().getSimulator().playShipsOneTimeStep();
}

void SimulatorAPI::InteractiveMode::runSimulation(double byTimeSteps) {
    SimulatorAPI::getInstance().getSimulator().
        runBy(units::time::second_t(byTimeSteps));
}

void SimulatorAPI::InteractiveMode::endSimulation() {
    SimulatorAPI::getInstance().endSimulation();
}

void SimulatorAPI::InteractiveMode::requestSimulationCurrentResults() {
    SimulatorAPI::getInstance().requestSimulationCurrentResults();
}

std::shared_ptr<ShipNetSimCore::Ship>
SimulatorAPI::InteractiveMode::getShipByID(const QString& shipID) {
    return SimulatorAPI::getInstance().getShipByID(shipID);
}

QVector<std::shared_ptr<ShipNetSimCore::Ship>>
SimulatorAPI::InteractiveMode::getAllShips() {
    return SimulatorAPI::getInstance().getAllShips();
}


// Implementation for ContinuousMode class

void SimulatorAPI::ContinuousMode::defineSimulator(
    ShipNetSimCore::OptimizedNetwork* network,
    QVector<std::shared_ptr<ShipNetSimCore::Ship>> shipList,
    units::time::second_t timeStep,
    bool runAsAServer)
{
    SimulatorAPI::getInstance().initializeSimulator(network, shipList,
                                                    timeStep, runAsAServer);
}


ShipNetSimCore::Simulator& SimulatorAPI::ContinuousMode::getSimulator() {
    return SimulatorAPI::getInstance().getSimulator();
}

void SimulatorAPI::ContinuousMode::runSimulation() {
    SimulatorAPI::getInstance().getSimulator().runSimulation();
}

void SimulatorAPI::ContinuousMode::pauseSimulation() {
    SimulatorAPI::getInstance().pauseSimulation();
}

void SimulatorAPI::ContinuousMode::resumeSimulation() {
    SimulatorAPI::getInstance().resumeSimulation();
}

std::shared_ptr<ShipNetSimCore::Ship>
SimulatorAPI::ContinuousMode::getShipByID(const QString& shipID) {
    return SimulatorAPI::getInstance().getShipByID(shipID);
}

QVector<std::shared_ptr<ShipNetSimCore::Ship>>
SimulatorAPI::ContinuousMode::getAllShips() {
    return SimulatorAPI::getInstance().getAllShips();
}
