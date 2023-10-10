#include "shipgearbox.h"
#include "ship.h"
#include "../utils/utils.h"
#include "ishipengine.h"

ShipGearBox::ShipGearBox()
{
    mHost = nullptr;
}
void ShipGearBox::initialize(Ship *host, QVector<IShipEngine *> engines,
                             QMap<QString, std::any> &parameters)
{
    mHost = host;
    mEngines = engines;
    setParameters(parameters);
}

void ShipGearBox::setParameters(QMap<QString, std::any> &parameters)
{
    for (auto pv: parameters.keys())
    {
        if (pv.toLower() == "gearratiotoone")
        {
            mGearRationTo1 =
                Utils::getValueFromMap<double>(parameters, "gearratio", 1.0);
        }
        if (pv.toLower() == "efficiency")
        {
            mEfficiency =
                Utils::getValueFromMap<double>(parameters, "efficiency", 1.0);
        }
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

units::power::kilowatt_t ShipGearBox::getPreviousOutputPower() const
{
    return mOutputPower;
}
