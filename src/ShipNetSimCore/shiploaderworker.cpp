#include "shiploaderworker.h"
#include "ship/shipsList.h"
#include "simulatorapi.h"


ShipLoaderWorker::ShipLoaderWorker(QObject *parent)
    : QObject{parent}
{}

void ShipLoaderWorker::loadShips(APIData& apiData,
                                 QString shipsFilePath,
                                 QString networkName)
{
    try {
        // Invoke your static method
        auto shipsData =
            ShipNetSimCore::ShipsList::readShipsFile(shipsFilePath,
                                                     apiData.network,
                                                     false);

        auto ships =
            ShipNetSimCore::ShipsList::loadShipsFromParameters(shipsData,
                                                               apiData.network,
                                                               false);

        // Emit the result
        emit shipsLoaded(ships);
    } catch (std::exception &e) {
        emit errorOccured(e.what());
    }

}

void ShipLoaderWorker::loadShips(APIData &apiData,
                                 QVector<QMap<QString, std::any> > ships,
                                 QString networkName)
{
    try {
        // Invoke your static method
        auto shipsVec =
            ShipNetSimCore::ShipsList::loadShipsFromParameters(ships,
                                                     apiData.network,
                                                     false);

        // Emit the result
        emit shipsLoaded(shipsVec);
    } catch (std::exception &e) {
        emit errorOccured(e.what());
    }
}

void ShipLoaderWorker::loadShips(APIData &apiData,
                                 QJsonObject &ships,
                                 QString networkName)
{
    try {
        // Invoke your static method
        auto shipsVec =
            ShipNetSimCore::ShipsList::loadShipsFromJson(ships,
                                                     apiData.network,
                                                     false);

        // Emit the result
        emit shipsLoaded(shipsVec);
    } catch (std::exception &e) {
        emit errorOccured(e.what());
    }
}

void ShipLoaderWorker::loadShips(APIData &apiData,
                                 QVector<QMap<QString, QString> > ships,
                                 QString networkName)
{
    try {
        // Invoke your static method
        auto shipsVec =
            ShipNetSimCore::ShipsList::loadShipsFromParameters(ships,
                                                               apiData.network,
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
