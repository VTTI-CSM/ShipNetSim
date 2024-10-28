#include "./simulationworker.h"
#include "ship/shipsList.h"
#include "network/networkdefaults.h"
#include "utils/utils.h"
#include "simulatorapi.h"

SimulationWorker::SimulationWorker(QObject *parent)
    : QObject(parent) {}


void SimulationWorker::requestSimulationCurrentResults() {
    if (simulator) {
        simulator->exportSimulationResults(false);
        emit simulationResultsRequested();
        emit workerReady();
    } else {
        emit errorOccurred("Simulator not initialized.");
        emit workerReady();
    }
}

void SimulationWorker::addShipsToSimulation(QJsonObject shipParams)
{
    if (simulator) {
        try {
            auto ship =
                ShipNetSimCore::ShipsList::
                loadShipFromParameters(shipParams, network.get());

            // Check dublicate ship
            if (ships.contains(ship->getUserID())) {
                QString errMsg = "Duplicate ship ID: " +
                                 ship->getUserID() + "\n";
                emit errorOccurred(errMsg);
                return;
            }

            // Check if the ship start time already passed
            if (ship->getStartTime() <= simulator->getCurrentSimulatorTime()) {
                ship->setStartTime(simulator->getCurrentSimulatorTime() +
                                   simulator->getSimulatorTimeStep());
            }

            // Add the ship to the simulator and keep track of it using its ID
            ships.insert(ship->getUserID(), ship);
            simulator->addShipToSimulation(ship);

            connect(ship.get(), &ShipNetSimCore::Ship::reachedDestination,
                    this, &SimulationWorker::shipReachedDestination);

            emit shipAddedToSimulator(ship->getUserID());
        } catch (std::exception &e) {
            QString errMsg = "Error during adding ship to simulation: " +
                             QString(e.what()) + "\n";
            emit errorOccurred(errMsg);
            emit workerReady();
            return;
        }

        emit workerReady();
    } else {
        emit errorOccurred("Simulator not initialized.");
        emit workerReady();
    }
}

void SimulationWorker::requestShipCurrentState(QString shipID)
{
    if (ships.contains(shipID)) {
        try {
            auto sjson = ships[shipID]->getCurrentStateAsJson();
            emit shipCurrentStateRequested(sjson);
        } catch (std::exception &e) {
            QString errMsg = "Error during getting ship's current state: " +
                             QString(e.what()) + "\n";
            emit errorOccurred(errMsg);
            return;
        }
    } else {
        QString errMsg = "Error during getting ship's current state: ShipID "
                         + shipID + "does not exist.\n";
        emit errorOccurred(errMsg);
        return;
    }
}

void SimulationWorker::requestSimulatorStatus()
{
    if (simulator) {
        try {
            auto sjson = simulator->getCurrentStateAsJson();
            emit simulatorCurrentStateRequested(sjson);
        } catch (std::exception &e) {
            QString errMsg = "Error during getting simulator's current state: " +
                             QString(e.what()) + "\n";
            emit errorOccurred(errMsg);
            return;
        }
    }
}


// -----------------------------------------------------------------------------
// Step-by-step simulation worker implementation
// -----------------------------------------------------------------------------
StepSimulationWorker::StepSimulationWorker(QObject *parent)
    : SimulationWorker(parent) {}

void StepSimulationWorker::defineSimulator(double timeStep_sec,
                                       QJsonObject shipsParams,
                                       QMetaMethod endOfInitsignalToEmit) {
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
            emit simulatorNetworkLoaded();
            // Write a message to the console
            std::cout << "Network has been successfully loaded.";
        }

        ships.clear();  // make sure its empty
        QVector<std::shared_ptr<ShipNetSimCore::Ship>> initShipsList;
        if (!shipsParams.empty()) {
            initShipsList =
                ShipNetSimCore::ShipsList::loadShipsFromJson(
                    shipsParams, network.get());
            for (auto& ship: initShipsList) {
                ships.insert(ship->getUserID(), ship);
            }
        }
        else {
            initShipsList = QVector<std::shared_ptr<ShipNetSimCore::Ship>>();
        }


        SimulatorAPI::InteractiveMode::defineSimulator(network.get(),
                                                       initShipsList, timeStep,
                                                       true);
        simulator = &SimulatorAPI::InteractiveMode::getSimulator();


        connect(simulator,
                &ShipNetSimCore::Simulator::simulationResultsAvailable,
                this, &SimulationWorker::simulationResultsAvailable);

        SimulatorAPI::InteractiveMode::initSimulation();

        // Emit the signal passed as parameter, or the default one
        if (endOfInitsignalToEmit.isValid()) {
            endOfInitsignalToEmit.invoke(this);
        }

        emit workerReady();
    } catch (const std::exception &e) {
        emit errorOccurred(QString::fromStdString(e.what()));
        emit workerReady();
    }
}

void StepSimulationWorker::restartSimulator(double timeStep_sec,
                                            QJsonObject shipsParams) {
    try {
        // Clean up the existing simulator and network
        if (simulator) {
            simulator->stopSimulation();  // Ensure the simulation is stopped
            delete simulator;             // Delete the simulator object
            simulator = nullptr;          // Set the pointer to nullptr
        }

        if (network) {
            network.reset();  // Use reset() for std::unique_ptr
        }

        ships.clear();

        // Reinitialize the network and simulator
        QMetaMethod restartSgnl =
            QMetaMethod::fromSignal(&SimulationWorker::simulatorRestarted);
        defineSimulator(timeStep_sec, shipsParams, restartSgnl);

        emit workerReady();

    } catch (const std::exception &e) {
        emit errorOccurred(QString::fromStdString(e.what()));
        emit workerReady();
    }
}

void StepSimulationWorker::runSimulator(double byTimeSteps) {
    if (simulator) {
        try {
            simulator->runBy(units::time::second_t(byTimeSteps));
            double currentSimTime = simulator->getCurrentSimulatorTime().value();
            emit simulationAdvanced(currentSimTime);
            emit workerReady();
        } catch (std::exception &e) {
            emit errorOccurred(e.what());
            emit workerReady();
        }

    } else {
        emit errorOccurred("Simulator not initialized.");
        emit workerReady();
    }
}

void StepSimulationWorker::endSimulator() {
    if (simulator) {
        simulator->endSimulation();
        emit simulatorEnded();
        emit workerReady();
    } else {
        emit errorOccurred("Simulator not initialized.");
        emit workerReady();
    }
}

// -----------------------------------------------------------------------------
// Continuous simulation worker implementation
// -----------------------------------------------------------------------------
ContinuousSimulationWorker::ContinuousSimulationWorker(QObject *parent)
    : SimulationWorker(parent) {}

void ContinuousSimulationWorker::defineSimulator(
    double timeStep_sec,
    QJsonObject shipsParams,
    QMetaMethod endOfInitsignalToEmit)
{
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
            emit simulatorNetworkLoaded();
            // Write a message to the console
            std::cout << "Network has been successfully loaded.";
        }

        ships.clear();  // make sure its empty
        QVector<std::shared_ptr<ShipNetSimCore::Ship>> initShipsList;
        if (!shipsParams.empty()) {
            initShipsList =
                ShipNetSimCore::ShipsList::loadShipsFromJson(
                    shipsParams, network.get());

            for (auto& ship: initShipsList) {
                ships.insert(ship->getUserID(), ship);
            }
        }
        else {
            initShipsList = QVector<std::shared_ptr<ShipNetSimCore::Ship>>();
        }


        SimulatorAPI::ContinuousMode::defineSimulator(network.get(),
                                                      initShipsList, timeStep,
                                                      true);
        simulator = &SimulatorAPI::ContinuousMode::getSimulator();


        connect(simulator,
                &ShipNetSimCore::Simulator::simulationResultsAvailable,
                this, &SimulationWorker::simulationResultsAvailable);

        // Emit the signal passed as parameter, or the default one
        if (endOfInitsignalToEmit.isValid()) {
            endOfInitsignalToEmit.invoke(this);
        }

        emit workerReady();

    } catch (const std::exception &e) {
        emit errorOccurred(QString::fromStdString(e.what()));
    }
}

void ContinuousSimulationWorker::restartSimulator(double timeStep_sec,
                                            QJsonObject shipsParams) {
    try {
        // Clean up the existing simulator and network
        if (simulator) {
            simulator->stopSimulation();  // Ensure the simulation is stopped
            delete simulator;             // Delete the simulator object
            simulator = nullptr;          // Set the pointer to nullptr
        }

        if (network) {
            network.reset();  // Use reset() for std::unique_ptr
        }

        ships.clear();

        // Reinitialize the network and simulator
        QMetaMethod restartSgnl =
            QMetaMethod::fromSignal(&SimulationWorker::simulatorRestarted);
        defineSimulator(timeStep_sec, shipsParams, restartSgnl);

        emit workerReady();

    } catch (const std::exception &e) {
        emit errorOccurred(QString::fromStdString(e.what()));
        emit workerReady();
    }
}

void ContinuousSimulationWorker::runSimulation() {
    if (simulator) {
        try {
            connect(simulator, &ShipNetSimCore::Simulator::progressUpdated,
                    this, [this](int progressPercentage)
                    {
                        emit progressUpdated(progressPercentage);
                    });

            simulator->runSimulation();

            emit workerReady();
        } catch (std::exception &e) {
            emit errorOccurred(e.what());
            emit workerReady();
        }
    } else {
        emit errorOccurred("Simulator not initialized.");
        emit workerReady();
    }
}

void ContinuousSimulationWorker::pauseSimulator() {
    if (simulator) {
        simulator->pauseSimulation();
        emit simulationPaused();
        emit workerReady();
    } else {
        emit errorOccurred("Simulator not initialized.");
        emit workerReady();
    }
}

void ContinuousSimulationWorker::resumeSimulator() {
    if (simulator) {
        simulator->resumeSimulation();
        emit simulationResumed();
        emit workerReady();
    } else {
        emit errorOccurred("Simulator not initialized.");
        emit workerReady();
    }
}


void ContinuousSimulationWorker::endSimulation() {
    if (simulator) {
        simulator->stopSimulation();
        emit simulatorEnded();
        emit workerReady();
    } else {
        emit errorOccurred("Simulator not initialized.");
        emit workerReady();
    }
}
