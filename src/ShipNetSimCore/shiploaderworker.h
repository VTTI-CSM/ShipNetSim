#ifndef SHIPLOADERWORKER_H
#define SHIPLOADERWORKER_H

#include <QObject>
#include <any>

// Forward declare APIData and ShipNetSimCore::Ship
struct APIData;
namespace ShipNetSimCore {
class Ship;
class OptimizedNetwork;
}

class ShipLoaderWorker : public QObject
{
    Q_OBJECT
public:
    explicit ShipLoaderWorker(QObject *parent = nullptr);
    void loadShips(APIData& apiData,
                   QString shipsFilePath,
                   QString networkName);
    void loadShips(APIData& apiData,
                   QVector<QMap<QString, std::any> > ships,
                   QString networkName);
    void loadShips(APIData& apiData,
                   QJsonObject &ships,
                   QString networkName);
    void loadShips(APIData& apiData,
                   QVector<QMap<QString, QString> > ships,
                   QString networkName);
    void loadShips(QJsonObject ships,
                   ShipNetSimCore::OptimizedNetwork* network);


signals:
    void shipsLoaded(QVector<std::shared_ptr<ShipNetSimCore::Ship>> ships);
    void errorOccured(QString error);
};

#endif // SHIPLOADERWORKER_H
