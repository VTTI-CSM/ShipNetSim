#ifndef SHIPLOADERWORKER_H
#define SHIPLOADERWORKER_H

#include "network/optimizednetwork.h"
#include <QObject>
#include <any>

// Forward declare APIData and ShipNetSimCore::Ship
struct APIData;
namespace ShipNetSimCore
{
class Ship;
}

class ShipLoaderWorker : public QObject
{
    Q_OBJECT
public:
    explicit ShipLoaderWorker(QObject *parent = nullptr);
    void loadShips(ShipNetSimCore::OptimizedNetwork *network,
                   QString shipsFilePath, QString networkName);
    void loadShips(ShipNetSimCore::OptimizedNetwork *network,
                   QVector<QMap<QString, std::any>>  ships,
                   QString                           networkName);
    void loadShips(ShipNetSimCore::OptimizedNetwork *network,
                   QJsonObject &ships, QString networkName);
    void loadShips(ShipNetSimCore::OptimizedNetwork *network,
                   QVector<QMap<QString, QString>>   ships,
                   QString                           networkName);
    void loadShips(QJsonObject                       ships,
                   ShipNetSimCore::OptimizedNetwork *network);

signals:
    void
    shipsLoaded(QVector<std::shared_ptr<ShipNetSimCore::Ship>> ships);
    void errorOccured(QString error);
};

#endif // SHIPLOADERWORKER_H
