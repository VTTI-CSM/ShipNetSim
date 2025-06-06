#ifndef SIMULATORWORKER_H
#define SIMULATORWORKER_H

#include "../third_party/units/units.h"
#include <QObject>

// Forward declare APIData and ShipNetSimCore::Ship
struct APIData;
namespace ShipNetSimCore
{
class Ship;
}

class SimulatorWorker : public QObject
{
    Q_OBJECT
public:
    explicit SimulatorWorker(QObject *parent = nullptr);
    void setupSimulator(
        APIData                                        &apiData,
        QVector<std::shared_ptr<ShipNetSimCore::Ship>> &shipList,
        units::time::second_t timeStep, bool isExternallyControlled);

signals:
    void errorOccurred(QString error);
};

#endif // SIMULATORWORKER_H
