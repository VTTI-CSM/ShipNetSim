#ifndef IENERGYSOURCE_H
#define IENERGYSOURCE_H

#include "../../third_party/units/units.h"

struct EnergyConsumptionData
{
    bool isEnergySupplied;
    units::energy::kilowatt_hour_t energyConsumed;
    units::energy::kilowatt_hour_t energyNotConsumed;
};

class IEnergySource
{
public:
    virtual ~IEnergySource() = default; // Virtual destructor

    virtual EnergyConsumptionData consume(
        units::time::second_t timeStep,
        units::energy::kilowatt_hour_t consumedkWh) = 0;

    virtual units::energy::kilowatt_hour_t getTotalEnergyConsumed() = 0;
};

#endif // IENERGYSOURCE_H
