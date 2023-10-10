#ifndef SHIPPROPELLER_H
#define SHIPPROPELLER_H

#include "ishipgearbox.h"
#include "ishippropeller.h"

class Ship;  // Forward declaration of the class ship

class ShipPropeller : IShipPropeller
{
public:
    // IShipPropeller interface
    void initialize(Ship *ship, IShipGearBox *gearbox,
                    QMap<QString, std::any> &parameters) override;

    void setParameters(QMap<QString, std::any>& parameters) override;

//    double getGearEfficiency() override;
//    void setGearEfficiency(double newGearEfficiency) override;
    double getShaftEfficiency() override;
    void setShaftEfficiency(double newShaftEfficiency) override;
    double getPropellerEfficiency() override;
    void setPropellerOpenWaterEfficiencies(
        QMap<double, double> newPropellerOpenWaterEfficiencies) override;
//    double getHullEfficiency() override;
//    void setHullEfficiency(double newHullEfficiency) override;


//    units::power::kilowatt_t getDeliveredPower() override;
//    units::power::kilowatt_t getThrustPower() override;
    units::power::kilowatt_t getEffectivePower() override;
    units::power::kilowatt_t getPreviousEffectivePower() override;
    units::force::newton_t getThrust() override;
    units::angular_velocity::revolutions_per_minute_t getRPM() override;
    units::torque::newton_meter_t getTorque() override;
    double getThrustCoefficient() override;
    double getTorqueCoefficient() override;
    double getAdvancedRatio() override;

    const QVector<IShipEngine *> getDrivingEngines() const override;

private:
    double mShaftEfficiency;
    QMap<double, double> mPropellerOpenWaterEfficiencyToJ;

    units::power::kilowatt_t mPreviousEffectivePower;
    double getOpenWaterEfficiency();
    double getRelativeEfficiency();
    double getHullEfficiency();

};

#endif // SHIPPROPELLER_H
