#include "simulatorapi.h"

void SimulatorAPI::initializeSimulator(
    ShipNetSimCore::OptimizedNetwork* network,
    QVector<std::shared_ptr<ShipNetSimCore::Ship>> shipList,
    units::time::second_t timeStep)
{

    simulator = std::make_unique<ShipNetSimCore::Simulator>(network, shipList, timeStep);
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
}

ShipNetSimCore::Simulator& SimulatorAPI::getSimulator() {
    return *simulator;
}


void SimulatorAPI::handleSimulationResults(
    const QVector<std::pair<QString, QString>>& summaryData,
    const QString& trajectoryFile) 
{   
    emit simulationResultsAvailable(summaryData, trajectoryFile);
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


void SimulatorAPI::handleShipReachedDestination(const QString& shipID) {
    emit shipReachedDestination(shipID);
}
