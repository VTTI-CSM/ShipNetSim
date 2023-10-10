#ifndef IENERGYCONSUMER_H
#define IENERGYCONSUMER_H

#include "../../third_party/units/units.h"
#include "IEnergySource.h"
#include <QString>
#include <any>

class Ship;  // Forward declaration of the class ship


class IEnergyConsumer
{
public:
    IEnergyConsumer();
    ~IEnergyConsumer();

    virtual void initialize(Ship *host, IEnergySource *energySource,
                            QMap<QString, std::any>& parameters) = 0;
    void setHost(Ship *host);
    void setEnergySource(IEnergySource *energySource);
    virtual void setParameters(QMap<QString, std::any>& parameters) = 0;
    const Ship* getHost() const;
    const IEnergySource* getEnergySource() const;

    virtual EnergyConsumptionData
    energyConsumed(units::time::second_t timeStep) = 0;

protected:
    Ship *mHost;
    IEnergySource *mEnergySource;
};

#endif // IENERGYCONSUMER_H
