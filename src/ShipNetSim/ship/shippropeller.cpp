#include "shippropeller.h"
#include "ishipresistancepropulsionstrategy.h"
#include "ship.h"
#include "hydrology.h"
#include "../utils/utils.h"


void ShipPropeller::initialize(Ship *ship, IShipGearBox *gearbox,
                               QMap<QString, std::any> &parameters)
{
    mHost = ship;
    mGearBox = gearbox;
    setParameters(parameters);

}


void ShipPropeller::setParameters(QMap<QString, std::any> &parameters)
{
    // Process Shaft Efficiency
    auto it = parameters.find("shaftefficiency");
    if (it != parameters.end())
    {
        mShaftEfficiency = std::any_cast<double>(it.value());
    }

    // Process Open Water Efficiency
    it = parameters.find("openwaterefficiency");
    if (it != parameters.end())
    {
        mPropellerOpenWaterEfficiencyToJ =
            std::any_cast<QMap<double, double>>(it.value());
    }

    // Process Propeller Diameter & Disk Area
    it = parameters.find("propellerdiameter");
    if (it != parameters.end())
    {
        mPropellerDiameter =
            std::any_cast<units::length::meter_t>(it.value());
        mPropellerDiskArea =
            units::constants::pi *
                             units::math::pow<2>(mPropellerDiameter) / 4.0;
    }

    // Process Expanded Area Ratio & Blade Area
    it = parameters.find("propellerExpandedAreaRatio");
    if (it != parameters.end())
    {
        mPropellerExpandedAreaRatio =
            std::any_cast<double>(it.value());
        mExpandedBladeArea =
            mPropellerExpandedAreaRatio * mPropellerDiskArea;
    }
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
