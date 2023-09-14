#include "shippropeller.h"
#include "ishipresistancepropulsionstrategy.h"
#include "ship.h"
#include "hydrology.h"
#include "../utils/utils.h"


ShipPropeller::ShipPropeller()
{
    mHost = nullptr;
}

void ShipPropeller::initialize(Ship *ship,
                               QMap<QString, std::any> &parameters)
{
    mHost = ship;

    mBreakPower =
        Utils::getValueFromMap<units::power::kilowatt_t>(
            parameters, "breakPower",
            units::power::kilowatt_t(std::nan("uninitialized")));

    mGearEfficiency =
        Utils::getValueFromMap<double>(
            parameters, "gearEfficiency", std::nan("uninitialized"));

    mShaftEfficiency =
        Utils::getValueFromMap<double>(
            parameters, "shaftEfficiency", std::nan("uninitialized"));

    mPropellerEfficiency =
        Utils::getValueFromMap<double>(
            parameters, "propellerEfficiency", std::nan("uninitialized"));

    mHullEfficiency =
        Utils::getValueFromMap<double>(
            parameters, "hullEfficiency", std::nan("uninitialized"));

    initializeDefaults();

}

void ShipPropeller::setHost(Ship *ship)
{
    mHost = ship;
}

void ShipPropeller::setParameters(QMap<QString, std::any> &parameters)
{
    if (parameters.contains("breakPower"))
    {
        mBreakPower =
            std::any_cast<units::power::kilowatt_t>(parameters["breakPower"]);
    }
    if (parameters.contains("gearEfficiency"))
    {
        mGearEfficiency =
            std::any_cast<double>(parameters["gearEfficiency"]);
    }
    if (parameters.contains("shaftEfficiency"))
    {
        mShaftEfficiency =
            std::any_cast<double>(parameters["shaftEfficiency"]);
    }
    if (parameters.contains("propellerEfficiency"))
    {
        mPropellerEfficiency =
            std::any_cast<double>(parameters["propellerEfficiency"]);
    }
    if (parameters.contains("hullEfficiency"))
    {
        mHullEfficiency =
            std::any_cast<double>(parameters["hullEfficiency"]);
    }
}

ShipPropeller::~ShipPropeller()
{
    if (mHost)
    {
        delete mHost;
    }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~ Getters and Setters ~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
double ShipPropeller::getGearEfficiency()
{
    return mGearEfficiency;
}
void ShipPropeller::setGearEfficiency(double newGearEfficiency)
{
    mGearEfficiency = newGearEfficiency;
}

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
    return mPropellerEfficiency;
}

void ShipPropeller::setPropellerEfficiency(double newPropellerEfficiency)
{
    mPropellerEfficiency = newPropellerEfficiency;
}

double ShipPropeller::getHullEfficiency()
{
    return mHullEfficiency;
}

void ShipPropeller::setHullEfficiency(double newHullEfficiency)
{
    mHullEfficiency = newHullEfficiency;
}


units::power::kilowatt_t ShipPropeller::getBreakPower()
{
    return mBreakPower;
}

void ShipPropeller::setBreakPower(units::power::kilowatt_t newPower)
{
    mBreakPower = newPower;
}

units::power::kilowatt_t ShipPropeller::getShaftPower()
{
    return getBreakPower() * mGearEfficiency;
}


units::power::kilowatt_t ShipPropeller::getDeliveredPower()
{
    return getShaftPower() * mShaftEfficiency;
}

units::power::kilowatt_t ShipPropeller::getThrustPower()
{
    return getDeliveredPower() * mPropellerEfficiency;
}

units::power::kilowatt_t ShipPropeller::getEffectivePower()
{
    return getThrustPower() * mHullEfficiency;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~ Calculations ~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

units::force::newton_t ShipPropeller::getThrust()
{
    return units::math::min(
        (getEffectivePower() /
         units::math::max(
             mHost->getResistanceStrategy()->calc_SpeedOfAdvance(*mHost),
             units::velocity::meters_per_second_t(0.0001))).
        convert<units::force::newton>(),

        units::force::kilonewton_t(
            (double)2.0 * hydrology::WATER_RHO.value() *
            mHost->getPropellerDiskArea().value() *
            pow(getDeliveredPower().convert<units::power::watts>().value(),
                (double)2.0) / (double)1000.0
            ).convert<units::force::newton>());
}

double ShipPropeller::getRPM()
{
    return pow(pow(getShaftPower().
                   convert<units::power::horsepower>().value(),
                   (double)0.2) * ((double)632.7/
                      mHost->getPropellerDiameter().
                      convert<units::length::inch>().value()),
               ((double)1 / (double)0.6));
}

units::torque::newton_meter_t ShipPropeller::getTorque()
{
    return units::torque::newton_meter_t(
        getDeliveredPower().convert<units::power::watt>().value() /
        ((double)2.0 * units::constants::pi.value() * getRPM()));

    //    return units::torque::foot_pound_t(
    //               getEffectivePower().convert<units::power::horsepower>().value() /
    //                                       (getRPM() / (double)5252.0)).
    //        convert<units::torque::newton_meter>();
}

//units::force::newton_t ShipPropeller::calc_Torque()
//{
//    return units::force::kilonewton_t(
//               cbrt((double)2.0 *
//                    hydrology::WATER_RHO.value() *
//                    mHost->getPropellerDiskArea().value() *
//                    pow(getDeliveredPower().
//                        convert<units::power::watts>().value(), (double)2.0)
//                    )/(double)1000.0).convert<units::force::newton>();
//}

double ShipPropeller::getThrustCoefficient()
{
    return (getThrust().value() /
            (hydrology::WATER_RHO.value() *
             pow(getRPM(), (double)2.0) *
             pow(mHost->getPropellerDiameter().value(),(double)4.0)));
}

double ShipPropeller::getTorqueCoefficient()
{
    return (getTorque().value() /
            (hydrology::WATER_RHO.value() *
             pow(getRPM(), (double)2.0) *
             pow(mHost->getPropellerDiameter().value(),(double)5.0)));
}

double ShipPropeller::getAdvancedRatio()
{
    return (mHost->getResistanceStrategy()->
            calc_SpeedOfAdvance(*mHost).value() /
            (getRPM() * mHost->getPropellerDiameter().value()));
}

void ShipPropeller::initializeDefaults()
{
    if (std::isnan(mGearEfficiency))
    {
        mGearEfficiency = (double)0.99;
    }

    if (std::isnan(mShaftEfficiency))
    {
        mShaftEfficiency = (double)0.00;
    }

    if (std::isnan(mPropellerEfficiency))
    {
        mPropellerEfficiency = (double)0.75;
    }

    if (std::isnan(mHullEfficiency))
    {
        mHullEfficiency = 0.99;
    }

}
