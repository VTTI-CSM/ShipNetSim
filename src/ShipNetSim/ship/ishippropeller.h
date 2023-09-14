#ifndef I_SHIPPROPELLER_H
#define I_SHIPPROPELLER_H

#include "../../third_party/units/units.h"
#include <QString>
#include <any>

class Ship;  // Forward declaration of the class ship

/**
 * @brief The IShipPropeller class
 * @class IShipPropeller
 *
 * Provides an interface to manage ships propellers
 */
class IShipPropeller
{
public:
    virtual void initialize(Ship *ship,
                            QMap<QString, std::any>& parameters) = 0;
    virtual ~IShipPropeller() = 0;
    virtual void setHost(Ship *ship) = 0;
    virtual void setParameters(QMap<QString, std::any>& parameters) = 0;

    virtual units::power::kilowatt_t getBreakPower() = 0;
    virtual void setBreakPower(units::power::kilowatt_t newPower) = 0;
    virtual double getGearEfficiency() = 0;
    virtual void setGearEfficiency(double newGearEfficiency) = 0;
    virtual double getShaftEfficiency() = 0;
    virtual void setShaftEfficiency(double newShaftEfficiency) = 0;
    virtual double getPropellerEfficiency() = 0;
    virtual void setPropellerEfficiency(double newPropellerEfficiency) = 0;
    virtual double getHullEfficiency() = 0;
    virtual void setHullEfficiency(double newHullEfficiency) = 0;




    virtual units::power::kilowatt_t getShaftPower() = 0 ;
    virtual units::power::kilowatt_t getDeliveredPower() = 0;
    virtual units::power::kilowatt_t getThrustPower() = 0;
    virtual units::power::kilowatt_t getEffectivePower() = 0;
    virtual units::force::newton_t getThrust() = 0;
    virtual double getRPM() = 0;
    virtual units::torque::newton_meter_t getTorque() = 0;
    virtual double getThrustCoefficient() = 0;
    virtual double getTorqueCoefficient() = 0;
    virtual double getAdvancedRatio() = 0;

};

#endif //I_SHIPPROPELLER_H
