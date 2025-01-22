#include "shiploaderworker.h"
#include "ship/shipsList.h"
#include "network/optimizednetwork.h"

ShipLoaderWorker::ShipLoaderWorker(QObject *parent)
    : QObject{parent}
{}

void ShipLoaderWorker::loadShips(ShipNetSimCore::OptimizedNetwork *network,
                                 QString shipsFilePath,
                                 QString networkName)
{
    try {
        // Invoke your static method
        auto shipsData =
            ShipNetSimCore::ShipsList::readShipsFile(shipsFilePath,
                                                     network,
                                                     false);

        auto ships =
            ShipNetSimCore::ShipsList::loadShipsFromParameters(shipsData,
                                                               network,
                                                               false);

        // Emit the result
        emit shipsLoaded(ships);
    } catch (std::exception &e) {
        emit errorOccured(e.what());
    }

}

void ShipLoaderWorker::loadShips(ShipNetSimCore::OptimizedNetwork *network,
                                 QVector<QMap<QString, std::any> > ships,
                                 QString networkName)
{
    try {
        // Invoke your static method
        auto shipsVec =
            ShipNetSimCore::ShipsList::loadShipsFromParameters(ships,
                                                               network,
                                                               false);

        // Emit the result
        emit shipsLoaded(shipsVec);
    } catch (std::exception &e) {
        emit errorOccured(e.what());
    }
}

void ShipLoaderWorker::loadShips(ShipNetSimCore::OptimizedNetwork *network,
                                 QJsonObject &ships,
                                 QString networkName)
{
    try {
        // Invoke your static method
        auto shipsVec =
            ShipNetSimCore::ShipsList::loadShipsFromJson(ships,
                                                         network,
                                                         false);

        // Emit the result
        emit shipsLoaded(shipsVec);
    } catch (std::exception &e) {
        emit errorOccured(e.what());
    }
}

void ShipLoaderWorker::loadShips(ShipNetSimCore::OptimizedNetwork *network,
                                 QVector<QMap<QString, QString> > ships,
                                 QString networkName)
{
    try {
        // Invoke your static method
        auto shipsVec =
            ShipNetSimCore::ShipsList::loadShipsFromParameters(ships,
                                                               network,
                                                               false);

        // Emit the result
        emit shipsLoaded(shipsVec);
    } catch (std::exception &e) {
        emit errorOccured(e.what());
    }
}

void ShipLoaderWorker::loadShips(QJsonObject ships,
                                 ShipNetSimCore::OptimizedNetwork *network)
{
    try {
        // Invoke your static method
        auto shipsVec =
            ShipNetSimCore::ShipsList::loadShipsFromJson(ships,
                                                         network,
                                                         false);

        // Emit the result
        emit shipsLoaded(shipsVec);
    } catch (std::exception &e) {
        emit errorOccured(e.what());
    }
}
