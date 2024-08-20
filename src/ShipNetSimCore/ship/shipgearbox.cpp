#include "shipgearbox.h"
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
        qFatal("Gearbox ratio is not defined. "
               "it should be a value of double in range [0,inf]!");
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

QVector<units::angular_velocity::revolutions_per_minute_t>
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
        QVector<units::angular_velocity::revolutions_per_minute_t> result;

        for (auto& v : mEngines.front()->getRPMRange())
        {
            result.push_back(v/mGearRationTo1);
        }
        return result;
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
        weightedLowRPM = rpmRange[0] * power;
        weightedHighRPM = rpmRange[1] * power;
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
ShipGearBox::setEngineRPM(
    units::angular_velocity::revolutions_per_minute_t targetRPM)
{
    auto rpmOut = getOutputRPMRange();
    if (targetRPM < rpmOut[0] || targetRPM > rpmOut[1]) {
        qFatal("RPM required value is outside the engine range!");
    }

    // Return 0 if there are no engines to avoid division by zero
    if(mEngines.empty())
    {
        return;
    }

    // if only one engines is connected, return its rpm
    if (mEngines.size() == 1)
    {
        mEngines.front()->setEngineRPM(targetRPM * mGearRationTo1);
        return;
    }


    // if multiple engines are connected, returned weighted average rpm
    double totalPower = 0; // To store the sum of the power of all engines.

    for(auto const& engine: mEngines) {
        double power = engine->getBrakePower().value();
        auto rpmRange = engine->getRPMRange();
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

        engine->setEngineRPM(weightedRPM);
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
};
