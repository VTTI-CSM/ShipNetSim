#ifndef IENGINE_H
#define IENGINE_H

#include "IEnergyConsumer.h"
class IShipEngine : public IEnergyConsumer
{
public:
    ~IShipEngine();
    virtual void readPowerEfficiency(QString filePath) = 0;
    virtual void readPowerRPM(QString filePath) = 0;

    virtual double getEfficiency() = 0;
\
    virtual units::power::kilowatt_t getBrakePower() = 0;

    virtual void
    setBrakePowerFractionToFullPower(double fractionToFullPower) = 0;

    virtual units::angular_velocity::revolutions_per_minute_t getRPM() = 0;

    virtual units::power::kilowatt_t getPreviousBrakePower() = 0;

};

#endif // IENGINE_H
