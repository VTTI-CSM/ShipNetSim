#include "shippropeller.h"
#include "ishipresistancepropulsionstrategy.h"
#include "ship.h"
#include "hydrology.h"
#include "../utils/utils.h"


void ShipPropeller::initialize(Ship *ship, IShipGearBox *gearbox,
                               const QMap<QString, std::any> &parameters)
{
    mHost = ship;
    mGearBox = gearbox;
    setParameters(parameters);

}


void ShipPropeller::setParameters(const QMap<QString, std::any> &parameters)
{
    // Process Shaft Efficiency
    mShaftEfficiency =
        Utils::getValueFromMap<double>(parameters, "ShaftEfficiency", -1.0);
    if (mShaftEfficiency < 0.0)
    {
        qFatal("Shaft efficiency is not defined!");
    }

    // Process Open Water Efficiency
    mPropellerOpenWaterEfficiencyToJ =
        Utils::getValueFromMap<QMap<double, double>>(
        parameters, "OpenWaterPropellerEfficiency", QMap<double, double>());
    if (mPropellerOpenWaterEfficiencyToJ.isEmpty()) {
        qFatal("Propeller open water efficiency to j is not defined!");
    }

    // Process Propeller Diameter & Disk Area
    mPropellerDiameter =
        Utils::getValueFromMap<units::length::meter_t>(
        parameters, "PropellerDiameter", units::length::meter_t(-1.0));
    if (mPropellerDiameter.value() < 0.0)
    {
        qFatal("Propeller diameter is not defined!");
    }

    mPropellerDiskArea =
        units::constants::pi *
        units::math::pow<2>(mPropellerDiameter) / 4.0;

    // Process Expanded Area Ratio & Blade Area
    mPropellerExpandedAreaRatio =
        Utils::getValueFromMap<double>(parameters,
                                       "PropellerExpandedAreaRatio", -1.0);
    if (mPropellerExpandedAreaRatio < 0.0)
    {
        qFatal("Propeller expanded area ratio is not defined!");
    }
    mExpandedBladeArea =
        mPropellerExpandedAreaRatio * mPropellerDiskArea;

}


// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~ Getters and Setters ~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


double ShipPropeller::getShaftEfficiency()
{
    return mShaftEfficiency;
}

void ShipPropeller::setShaftEfficiency(double newShaftEfficiency)
{
    mShaftEfficiency = newShaftEfficiency;
}

double ShipPropeller::getPropellerEfficiency()
{
    return getOpenWaterEfficiency() * getRelativeEfficiency();
}

void ShipPropeller::setPropellerOpenWaterEfficiencies(
    QMap<double, double> newPropellerOpenWaterEfficiencies)
{
    mPropellerOpenWaterEfficiencyToJ = newPropellerOpenWaterEfficiencies;
}

units::power::kilowatt_t ShipPropeller::getEffectivePower()
{
    mPreviousEffectivePower = mGearBox->getOutputPower() *
           getPropellerEfficiency() *
           mShaftEfficiency *
           getHullEfficiency();
    return mPreviousEffectivePower;
}

units::power::kilowatt_t ShipPropeller::getPreviousEffectivePower()
{
    return mPreviousEffectivePower;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~ Calculations ~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

units::force::newton_t ShipPropeller::getThrust()
{
    // 70% of the raduis only provide thrust
    return getEffectivePower()/mHost->getResistanceStrategy()->
                                 calc_SpeedOfAdvance(*mHost);
}

units::angular_velocity::revolutions_per_minute_t ShipPropeller::getRPM()
{
    return mGearBox->getOutputRPM();
}

units::torque::newton_meter_t ShipPropeller::getTorque()
{
    return units::torque::newton_meter_t(
        getEffectivePower().convert<units::power::watt>().value() /
        getRPM().convert<units::angular_velocity::
                         radians_per_second>().value());
}

double ShipPropeller::getThrustCoefficient()
{
    return (getThrust().value() /
            (hydrology::WATER_RHO.value() *
             pow(getRPM().value(), (double)2.0) *
             pow(getPropellerDiameter().value(),(double)4.0)));
}

double ShipPropeller::getTorqueCoefficient()
{
    return (getTorque().value() /
            (hydrology::WATER_RHO.value() *
             pow(getRPM().value(), (double)2.0) *
             pow(getPropellerDiameter().value(),(double)5.0)));
}

double ShipPropeller::getAdvancedRatio()
{
    return (mHost->getResistanceStrategy()->
            calc_SpeedOfAdvance(*mHost).value() /
            (getRPM().value() * getPropellerDiameter().value()));
}

const QVector<IShipEngine *> ShipPropeller::getDrivingEngines() const
{
    return mGearBox->getEngines();
}

double ShipPropeller::getHullEfficiency()
{
    return mHost->getResistanceStrategy()->getHullEffeciency(*mHost);
}

double ShipPropeller::getRelativeEfficiency()
{
    return mHost->getResistanceStrategy()->
        getPropellerRotationEfficiency(*mHost);

}

double ShipPropeller::getOpenWaterEfficiency()
{
    return Utils::interpolate(mPropellerOpenWaterEfficiencyToJ,
                              mHost->getResistanceStrategy()->
                              calc_SpeedOfAdvance(*mHost).value());
}
