#include "shipgearbox.h"
#include "ishipcalmresistancestrategy.h"
#include "ship.h"
#include "../utils/utils.h"
#include "ishipengine.h"

namespace ShipNetSimCore
{
ShipGearBox::ShipGearBox()
{
    mHost = nullptr;
}
void ShipGearBox::initialize(Ship *host, QVector<IShipEngine *> engines,
                             const QMap<QString, std::any> &parameters)
{
    mHost = host;
    mEngines = engines;
    setParameters(parameters);
}

void ShipGearBox::setParameters(const QMap<QString, std::any> &parameters)
{
    mGearRationTo1 =
        Utils::getValueFromMap<double>(parameters, "GearboxRatio", -1.0);
    if (mGearRationTo1 < 0)
    {
        throw std::runtime_error("Gearbox ratio is not defined. "
                                 "it should be a value of double in "
                                 "range [0,inf]!");
    }
    mEfficiency =
        Utils::getValueFromMap<double>(parameters, "gearboxEfficiency", -1.0);
    if (mEfficiency < 0)
    {
        mEfficiency = 1.0;

        qWarning() << "Gearbox efficiency is not defined. "
                      "Set to default '1.0'!";
    }
}

units::angular_velocity::revolutions_per_minute_t ShipGearBox::getOutputRPM() const
{
     // Return 0 if there are no engines to avoid division by zero
    if(mEngines.empty())
    {
        return units::angular_velocity::revolutions_per_minute_t(0.0);
    }

    // if only one engines is connected, return its rpm
    if (mEngines.size() == 1)
    {
        return mEngines.front()->getRPM() / mGearRationTo1;
    }

    // if multiple engines are connected, returned weighted average rpm
    double totalPower = 0; // To store the sum of the power of all engines.

    // To store the weighted sum of RPM and Power.
    units::angular_velocity::revolutions_per_minute_t weightedRPM =
        units::angular_velocity::revolutions_per_minute_t(0.0);

    for(auto const& engine: mEngines) {
        double power = engine->getBrakePower().value();
        units::angular_velocity::revolutions_per_minute_t rpm =
            engine->getRPM();

        totalPower += power; // Sum up the power of all engines.
        // Sum up the product of RPM and power of all engines.
        weightedRPM += rpm * power;
    }

    // Return 0 if the total power is zero to avoid division by zero.
    if(totalPower == 0)
    {
        return units::angular_velocity::revolutions_per_minute_t(0);
    }

    // Calculate the weighted average of RPM.
    units::angular_velocity::revolutions_per_minute_t outputRPM =
        weightedRPM / totalPower;
     // Divide by gear ratio to get the output RPM.
    return outputRPM / mGearRationTo1;
}

QPair<units::angular_velocity::revolutions_per_minute_t,
      units::angular_velocity::revolutions_per_minute_t>
ShipGearBox::getOutputRPMRange() const
{
    // Return 0 if there are no engines to avoid division by zero
    if(mEngines.empty())
    {
        return {units::angular_velocity::revolutions_per_minute_t(0.0),
                units::angular_velocity::revolutions_per_minute_t(0.0)};
    }

    // if only one engines is connected, return its rpm
    if (mEngines.size() == 1)
    {
        auto r = mEngines.front()->getRPMRange();
        return {r.first / mGearRationTo1, r.second / mGearRationTo1};
    }


    // if multiple engines are connected, returned weighted average rpm
    double totalPower = 0; // To store the sum of the power of all engines.

    // To store the weighted sum of RPM and Power.
    units::angular_velocity::revolutions_per_minute_t weightedLowRPM =
        units::angular_velocity::revolutions_per_minute_t(0.0);
    units::angular_velocity::revolutions_per_minute_t weightedHighRPM =
        units::angular_velocity::revolutions_per_minute_t(0.0);

    for(auto const& engine: mEngines) {
        double power = engine->getBrakePower().value();
        auto rpmRange = engine->getRPMRange();
        totalPower += power; // Sum up the power of all engines.
        // Sum up the product of RPM and power of all engines.
        weightedLowRPM = rpmRange.first * power;
        weightedHighRPM = rpmRange.second * power;
    }

    // Return 0 if the total power is zero to avoid division by zero.
    if(totalPower == 0)
    {
        return {units::angular_velocity::revolutions_per_minute_t(0),
                units::angular_velocity::revolutions_per_minute_t(0)};
    }

    // Calculate the weighted average of RPM.
    units::angular_velocity::revolutions_per_minute_t outputLowRPM =
        weightedLowRPM / totalPower;
    units::angular_velocity::revolutions_per_minute_t outputHighRPM =
        weightedHighRPM / totalPower;
    // Divide by gear ratio to get the output RPM.
    return {outputLowRPM / mGearRationTo1, outputHighRPM / mGearRationTo1};
}

units::power::kilowatt_t ShipGearBox::getOutputPower()
{
    units::power::kilowatt_t totalPower = units::power::kilowatt_t(0.0);

    for (auto& engine: mEngines)
    {
        // Accumulate power from all engines.
        totalPower += engine->getBrakePower();
    }

    // Multiply by efficiency to get the output power.
    mOutputPower = totalPower * mEfficiency;
    return mOutputPower;
}

units::torque::newton_meter_t ShipGearBox::getOutputTorque()
{
    units::torque::newton_meter_t result =
        units::torque::newton_meter_t(
            (getOutputPower().value() * 1000.0) /  // convert to watt
            getOutputRPM().
            convert<
                units::angular_velocity::radians_per_second>().value());

    return result;
}

void
ShipGearBox::setEngineTargetState(IShipEngine::EngineProperties newState)
{
    if (mEngines.front()->getCurrentOperationalLoad() ==
        IShipEngine::EngineOperationalLoad::Default ||
        mEngines.front()->getCurrentOperationalLoad() ==
            IShipEngine::EngineOperationalLoad::UserDefined) {

        auto targetRPM = newState.RPM;
        auto rpmOut = getOutputRPMRange();
        if (targetRPM < rpmOut.first || targetRPM > rpmOut.second) {
            throw std::runtime_error("RPM required value is outside "
                                     "the engine range!");
        }

        // Return 0 if there are no engines to avoid division by zero
        if(mEngines.empty())
        {
            return;
        }

        // if only one engines is connected, return its rpm
        if (mEngines.size() == 1)
        {
            newState.RPM = targetRPM * mGearRationTo1;
            auto ns =
                mEngines.front()->getEnginePropertiesAtPower(
                    newState.breakPower,
                    mEngines.front()->getCurrentOperationalTier());
            mEngines.front()->setEngineTargetState(ns);

            return;
        }


        // if multiple engines are connected, returned weighted average rpm
        double totalPower = 0; // To store the sum of the power of all engines.

        for(auto const& engine: mEngines) {
            double power = engine->getBrakePower().value();
            totalPower += power; // Sum up the power of all engines.
        }

        // Return 0 if the total power is zero to avoid division by zero.
        if(totalPower == 0)
        {
            return;
        }

        for(auto const& engine: mEngines) {
            // Adjust target RPM by the gear ratio and weight it
            // by the engine's brake power.
            units::angular_velocity::revolutions_per_minute_t weightedRPM =
                targetRPM * mGearRationTo1 *
                (engine->getBrakePower().value() / totalPower);

            newState.RPM = weightedRPM;
            auto ns = mEngines.front()->getEnginePropertiesAtPower(
                newState.breakPower,
                mEngines.front()->getCurrentOperationalTier());
            engine->setEngineTargetState(ns);
        }

    }
}

void ShipGearBox::setEngineDefaultTargetState(
    IShipEngine::EngineProperties newState)
{
    for(auto const& engine: mEngines) {
        engine->setEngineDefaultTargetState(newState);
    }
}

void ShipGearBox::setEngineMaxPowerLoad(double targetPowerLoad)
{
    for(auto const& engine: mEngines) {
        engine->setEngineMaxPowerLoad(targetPowerLoad);
    }
}

units::power::kilowatt_t ShipGearBox::getPreviousOutputPower() const
{
    return mOutputPower;
}

void ShipGearBox::updateGearboxOperationalState()
{
    for (auto & engine : mEngines) {
        engine->updateEngineOperationalState();
    }
}

IShipEngine::EngineProperties
ShipGearBox::getEngineOperationalPropertiesAtRPM(
    units::angular_velocity::revolutions_per_minute_t rpm)
{
    IShipEngine::EngineProperties combinedProperties;

    // Return default properties if there are no engines
    if (mEngines.size() == 0) {
        return combinedProperties;
    }

    // Adjust RPM for gearbox reduction ratio
    // (this is the effective RPM for engines)
    units::angular_velocity::revolutions_per_minute_t adjustedRPM =
        rpm / mGearRationTo1;

    // If there's only one engine, return its properties
    // adjusted by the gearbox
    if (mEngines.size() == 1) {
        combinedProperties =
            mEngines.at(0)->getEnginePropertiesAtRPM(adjustedRPM);
        return combinedProperties;
    }

    // Initialize variables for summing power and
    // calculating weighted efficiency
    units::power::kilowatt_t totalBreakPower(0);
    double totalWeightedEfficiency = 0.0;

    // Loop through each engine and aggregate properties
    for (const auto& engine : mEngines) {
        IShipEngine::EngineProperties engineProperties =
            engine->getEnginePropertiesAtRPM(adjustedRPM);

        // Sum the break power of all engines
        totalBreakPower += engineProperties.breakPower;

        // Weight efficiency by the engine's break power
        totalWeightedEfficiency +=
            engineProperties.efficiency * engineProperties.breakPower.value();
    }

    // Combine the properties
    combinedProperties.breakPower = totalBreakPower;

    // Calculate the overall efficiency as the weighted average based on power
    if (totalBreakPower.value() > 0) {
        combinedProperties.efficiency =
            totalWeightedEfficiency / totalBreakPower.value();
    } else {
        combinedProperties.efficiency = 0.0;  // Set efficiency to zero if no power
    }

    return combinedProperties;
}

IShipEngine::EngineProperties
ShipGearBox::getGearboxOperationalPropertiesAtRPM(
    units::angular_velocity::revolutions_per_minute_t rpm)
{

    IShipEngine::EngineProperties combinedProperties =
        getEngineOperationalPropertiesAtRPM(rpm);

    // Set the combined RPM to the gearbox output RPM
    // (i.e., the input RPM to this function)
    combinedProperties.RPM = rpm;  // This is the output RPM after the gearbox's reduction

    return combinedProperties;
}


};
