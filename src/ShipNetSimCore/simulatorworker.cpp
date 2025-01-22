#include "simulatorapi.h"
#include "simulatorworker.h"

SimulatorWorker::SimulatorWorker(QObject *parent)
    : QObject{parent}
{}

void SimulatorWorker::setupSimulator(
    APIData& apiData,
    QVector<std::shared_ptr<ShipNetSimCore::Ship>>& shipList,
    units::time::second_t timeStep, bool isExternallyControlled)
{
    qDebug() << "Creating simulator inside thread:"
             << QThread::currentThread();

    try {
    // Create Simulator inside the worker thread
    apiData.simulator = new ShipNetSimCore::Simulator(
        apiData.network, shipList, timeStep, isExternallyControlled);

    // Store the ship list in APIData for future reference
    apiData.ships.clear();
    for (const auto& ship : shipList) {
        apiData.ships[ship->getUserID()] = ship;
    }

    qDebug() << "Simulator successfully created inside thread: "
             << QThread::currentThread();
    }
    catch (std::exception &e) {
        emit errorOccurred(QString("Error: Error in setting the "
                                   "simulator!\n%1").arg(e.what()));
    }
}
