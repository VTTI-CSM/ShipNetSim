#ifndef SHIPPROPELLER_H
#define SHIPPROPELLER_H

#include "ishippropeller.h"

class Ship;  // Forward declaration of the class ship

class ShipPropeller : IShipPropeller
{
public:
    // IShipPropeller interface
    ShipPropeller();
    void initialize(Ship *ship, QMap<QString, std::any> &parameters) override;
    ~ShipPropeller() override;

    void setHost(Ship *ship) override;
    void setParameters(QMap<QString, std::any>& parameters) override;

    double getGearEfficiency() override;
    void setGearEfficiency(double newGearEfficiency) override;
    double getShaftEfficiency() override;
    void setShaftEfficiency(double newShaftEfficiency) override;
    double getPropellerEfficiency() override;
    void setPropellerEfficiency(double newPropellerEfficiency) override;
    double getHullEfficiency() override;
    void setHullEfficiency(double newHullEfficiency) override;

    units::power::kilowatt_t getBreakPower() override;
    void setBreakPower(units::power::kilowatt_t newPower) override;
    units::power::kilowatt_t getShaftPower() override;
    units::power::kilowatt_t getDeliveredPower() override;
    units::power::kilowatt_t getThrustPower() override;
    units::power::kilowatt_t getEffectivePower() override;
    units::force::newton_t getThrust() override;
    double getRPM() override;
    units::torque::newton_meter_t getTorque() override;
    double getThrustCoefficient() override;
    double getTorqueCoefficient() override;
    double getAdvancedRatio() override;

private:
    Ship *mHost;

    units::power::kilowatt_t mBreakPower;

    double mGearEfficiency;

    double mShaftEfficiency;

    double mPropellerEfficiency;

    double mHullEfficiency;

    void initializeDefaults();


};

#endif // SHIPPROPELLER_H
