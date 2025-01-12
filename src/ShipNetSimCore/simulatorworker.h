#ifndef SIMULATORWORKER_H
#define SIMULATORWORKER_H

#include <QObject>
#include "../third_party/units/units.h"

// Forward declare APIData and ShipNetSimCore::Ship
struct APIData;
namespace ShipNetSimCore {
    class Ship;
}

class SimulatorWorker : public QObject
{
    Q_OBJECT
public:
    explicit SimulatorWorker(QObject *parent = nullptr);
    void setupSimulator(
        APIData& apiData,
        QVector<std::shared_ptr<ShipNetSimCore::Ship>>& shipList,
        units::time::second_t timeStep, bool isExternallyControlled);

signals:
};

#endif // SIMULATORWORKER_H
