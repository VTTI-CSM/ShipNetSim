#include "simulationworker.h"
#include "ship/shipsList.h"

template<typename T>
SimulationWorker::SimulationWorker(
    QString waterBoundariesFile,
    QVector<QMap<QString, T>> shipsRecords,
    QString networkName,
    units::time::second_t endTime,
    units::time::second_t timeStep,
    double plotFrequency, QString exportDir,
    QString summaryFilename, bool exportInsta,
    QString instaFilename, bool exportAllTrainsSummary)
{

    try {
        this->net =
            new ShipNetSimCore::OptimizedNetwork(waterBoundariesFile,
                                                         networkName);


        auto ships =
            ShipNetSimCore::ShipsList::loadShipsFromParameters(shipsRecords);

        // check if the trainrecords is empty
        if (ships.size() < 1) {
            qDebug() << "No ships are added!";
            return;
        }

        this->sim = new ShipNetSimCore::Simulator(net, ships, timeStep);
        this->sim->setEndTime(endTime);
        this->sim->setTimeStep(timeStep);
        this->sim->setPlotFrequency(plotFrequency);
        this->sim->setOutputFolderLocation(exportDir);
        this->sim->setSummaryFilename(summaryFilename);
        if (instaFilename.size() > 1) {
            this->sim->setExportInstantaneousTrajectory(exportInsta,
                                                        instaFilename);
        }
        this->sim->setExportIndividualizedShipsSummary(exportAllTrainsSummary);

        connect(this->sim,
                &ShipNetSimCore::Simulator::simulationResultsAvailable, this,
                &SimulationWorker::onSimulationFinished);
        connect(this->sim, &ShipNetSimCore::Simulator::plotShipsUpdated, this,
                &::SimulationWorker::onShipsCoordinatesUpdated);
        connect(this->sim, &ShipNetSimCore::Simulator::progressUpdated, this,
                &SimulationWorker::onProgressUpdated);

    } catch (std::exception &e) {
        emit errorOccurred(QString("Error initializing SimulationWorker: %1")
                               .arg(e.what()));
        cleanup(); // Safely clean up allocated resources
        return;
    } catch (...) {
        emit errorOccurred("Unknown error occurred during "
                           "SimulationWorker initialization.");
        cleanup(); // Safely clean up allocated resources
        return;
    }
}

void SimulationWorker::cleanup() {
    if (sim) {
        delete sim;
        sim = nullptr;
    }
    if (net) {
        delete net;
        net = nullptr;
    }
}

void SimulationWorker::onProgressUpdated(int progressPercentage) {
    emit simulaionProgressUpdated(progressPercentage);
}

void SimulationWorker::onShipsCoordinatesUpdated(
    QVector<std::pair<QString, ShipNetSimCore::GPoint>> trainsStartEndPoints) {
    emit shipsCoordinatesUpdated(trainsStartEndPoints);
}

void SimulationWorker::onSimulationFinished(ShipsResults results) {
    emit simulationFinished(results);
}

void SimulationWorker::doWork() {
    try {
        if (!sim) {
            emit errorOccurred("No ships are added!");
            return;
        }
        this->sim->runSimulation();
    } catch (const std::exception& e) {
        emit errorOccurred(QString::fromUtf8(e.what()));
    }

}

SimulationWorker::~SimulationWorker() {
    delete this->net;
    delete this->sim;
}


// Explicit template instantiation
template SimulationWorker::SimulationWorker(
    QString, QVector<QMap<QString, QString>>, QString,
    units::time::second_t, units::time::second_t, double,
    QString, QString, bool, QString, bool);

template SimulationWorker::SimulationWorker(
    QString, QVector<QMap<QString, std::any>>, QString,
    units::time::second_t, units::time::second_t, double,
    QString, QString, bool, QString, bool);
