#include "./simulationworker.h"
#include "ship/shipsList.h"
#include "network/networkdefaults.h"
#include "utils/utils.h"
#include "simulatorapi.h"

SimulationWorker::SimulationWorker(QObject *parent)
    : QObject(parent) {}

void SimulationWorker::initializeSimulator(double timeStep_sec) {
    try {
        units::time::second_t timeStep = units::time::second_t(timeStep_sec);

        QString networkFilePath =
            ShipNetSimCore::Utils::getFirstExistingPathFromList(
                ShipNetSimCore::NetworkDefaults::worldNetworkLocation(),
                QVector<QString>{"shp"});

        std::cout << networkFilePath.toStdString();

        network =
            std::make_unique<ShipNetSimCore::OptimizedNetwork>(networkFilePath);

        if (network) {
            // Manually emit the signal
            emit simulationNetworkLoaded();
            // Write a message to the console
            std::cout << "Network has been successfully loaded.";
        }

        QVector<std::shared_ptr<ShipNetSimCore::Ship>> emptyShipList;
        SimulatorAPI& api = SimulatorAPI::getInstance();
        api.initializeSimulator(network.get(), emptyShipList, timeStep);
        simulator = &api.getSimulator();
        ships.clear();

        connect(simulator,
                &ShipNetSimCore::Simulator::simulationResultsAvailable,
                this, &SimulationWorker::simulationResultsAvailable);

        simulator->initSimulation();

        emit simulationInitialized();

    } catch (const std::exception &e) {
        emit errorOccurred(QString::fromStdString(e.what()));
    }
}

void SimulationWorker::pauseSimulation() {
    if (simulator) {
        simulator->pauseSimulation();
        emit simulationPaused();
    } else {
        emit errorOccurred("Simulator not initialized.");
    }
}

void SimulationWorker::resumeSimulation() {
    if (simulator) {
        simulator->resumeSimulation();
        emit simulationResumed();
    } else {
        emit errorOccurred("Simulator not initialized.");
    }
}

void SimulationWorker::endSimulation() {
    if (simulator) {
        simulator->stopSimulation();
        emit simulationEnded();
    } else {
        emit errorOccurred("Simulator not initialized.");
    }
}

void SimulationWorker::runOneTimeStep() {
    if (simulator) {
        simulator->playShipsOneTimeStep();
        double currentSimTime = simulator->getCurrentSimulatorTime().value();
        emit simulationAdvanced(currentSimTime);
    } else {
        emit errorOccurred("Simulator not initialized.");
    }
}

void SimulationWorker::requestSimulationCurrentResults() {
    if (simulator) {
        simulator->exportSimulationResults(false);
        emit simulationResultsRetrieved();
    } else {
        emit errorOccurred("Simulator not initialized.");
    }
}

void SimulationWorker::addShipToSimulation(
    const QMap<QString, QString> &shipParameters) {
    if (simulator) {
        auto ship =
            ShipNetSimCore::ShipsList::loadShipFromParameters(shipParameters);
        ships.insert(ship->getUserID(), ship);
        simulator->addShipToSimulation(ship);

        connect(ship.get(), &ShipNetSimCore::Ship::reachedDestination,
                this, &SimulationWorker::shipReachedDestination);
        emit shipAddedToSimulator();
    } else {
        emit errorOccurred("Simulator not initialized.");
    }
}
