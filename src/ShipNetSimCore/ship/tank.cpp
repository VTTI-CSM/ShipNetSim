#include "tank.h"
#include "../utils/utils.h"
#include "ship.h"
#include <iostream>

namespace ShipNetSimCore
{
double Tank::getCurrentCapacityState()
{
    return tankStateOfCapacity * 100.0;
}
// Define the methods for the Tank class:
// Getter for the maximum tank capacity.
units::volume::liter_t Tank::getTankMaxCapacity() const
{
    // Return the maximum capacity of the tank.
    return this->tankMaxCapacity;
}

// Setter for the maximum tank capacity.
void Tank::setTankMaxCapacity(units::volume::liter_t newMaxCapacity)
{
    // Set the tank's maximum capacity to the new value provided.
    this->tankMaxCapacity = newMaxCapacity;
}

// Getter for the initial tank capacity.
units::volume::liter_t Tank::getTankInitialCapacity() const
{
    // Return the initial capacity of the tank.
    return tankInitialCapacity;
}

// Setter for the initial tank capacity.
void Tank::setTankInitialCapacity(double newInitialCapacityPercentage)
{
    // Calculate the initial capacity based on
    // the provided percentage and the maximum capacity.
    tankInitialCapacity =
        tankMaxCapacity * newInitialCapacityPercentage;

    // Update the fuel weight based on the new initial capacity.
    mFuelWeight = ShipFuel::getWeight(tankInitialCapacity, mFuelType);
}

// Getter for the current tank capacity.
units::volume::liter_t Tank::getTankCurrentCapacity() const
{
    // Return the current capacity of the tank.
    return tankCurrentCapacity;
}

// Consumes fuel based on provided energy usage.
// Updates tank state and returns consumption results.
EnergyConsumptionData
Tank::consume(units::time::second_t          timeStep,
              units::energy::kilowatt_hour_t consumedkWh)
{
    // ignore the time step parameter
    (void)timeStep;

    // Instantiate a result object to capture energy consumption
    // details.
    EnergyConsumptionData result;

    // Convert the amount of energy in kWh to the equivalent volume in
    // liters.
    auto consumedAmount =
        ShipFuel::convertKwhToLiters(consumedkWh, mFuelType);

    // Check if the tank has enough fuel to satisfy the consumption
    // request.
    if (!isTankDrainable(consumedAmount))
    {
        // If not, set the result to indicate unsuccessful energy
        // supply.
        result.isEnergySupplied = false;
        result.energyConsumed   = units::energy::kilowatt_hour_t(0.0);
        result.energyNotConsumed = consumedkWh;
        result.fuelConsumed      = {mFuelType,
                                    units::volume::liter_t(0.0)};
        return result;
    }
    // Update tank state after fuel consumption.
    tankCumConsumedFuel += consumedAmount;
    tankCurrentCapacity -= consumedAmount;
    mFuelWeight = ShipFuel::getWeight(tankCurrentCapacity, mFuelType);
    tankStateOfCapacity = tankCurrentCapacity / tankMaxCapacity;

    // Update the result to indicate successful energy supply.
    result.isEnergySupplied  = true;
    result.energyConsumed    = consumedkWh;
    result.energyNotConsumed = units::energy::kilowatt_hour_t(0.0);
    auto l = ShipFuel::convertKwhToLiters(consumedkWh, mFuelType);
    result.fuelConsumed = {mFuelType, l};
    // mHost->addToCummulativeConsumedEnergy(consumedkWh);
    return result;
}

// Getter for the state of the tank's capacity.
double Tank::getTankStateOfCapacity() const
{
    // Return the current state of capacity as a percentage.
    return tankStateOfCapacity;
}

// Checks if it's possible to drain the specified
// amount of fuel from the tank.
bool Tank::isTankDrainable(units::volume::liter_t consumedAmount)
{
    // Ensure that the desired amount is less than or
    // equal to the current capacity and the state of
    // capacity exceeds the Depth of Discharge limit.
    return (consumedAmount <= this->tankCurrentCapacity
            && tankStateOfCapacity > (1.0 - tankDOD));
}

// Getter for the depth of discharge.
double Tank::getTankDOD() const
{
    // Return the tank's depth of discharge value.
    return tankDOD;
}

// Setter for the depth of discharge.
void Tank::setTankDOD(double newTankDOD)
{
    // Ensure the provided depth of
    // discharge value is within valid range.
    if (newTankDOD <= 1.0 && newTankDOD >= 0.0)
    {
        tankDOD = newTankDOD;
    }
    else
    {
        // If not, throw an exception with an explanatory message.
        throw std::invalid_argument(
            "the Depth of Discharge must be between 0.0 and "
            "1.0. 0.0: no discharge is allowed, 1.0: full "
            "discharge is allowed");
    }
}

// Getter for the cumulative consumed fuel.
units::volume::liter_t Tank::getTankCumConsumedFuel() const
{
    // Return the cumulative amount of fuel consumed from the tank.
    return tankCumConsumedFuel;
}

// Method to retrieve the total energy consumed in the tank.
units::energy::kilowatt_hour_t Tank::getTotalEnergyConsumed()
{
    // Convert the cumulative consumed fuel
    // volume to the equivalent energy in kWh.
    return ShipFuel::convertLitersToKwh(tankCumConsumedFuel,
                                        mFuelType);
}

// Getter for the type of fuel stored in the tank.
ShipFuel::FuelType Tank::getFuelType()
{
    // Return the tank's fuel type.
    return mFuelType;
}
// Setter for the fuel type stored in the tank
void Tank::setFuelType(ShipFuel::FuelType fuelType)
{
    mFuelType   = fuelType;
    mFuelWeight = ShipFuel::getWeight(tankCurrentCapacity, mFuelType);
}

// Check if the tank has any fuel left.
bool Tank::tankHasFuel()
{
    // Return true if the state of
    // capacity exceeds the Depth of Discharge limit.
    return tankStateOfCapacity > (1.0 - tankDOD);
}

void Tank::setCharacteristics(
    const QMap<QString, std::any> &parameters)
{
    this->mFuelType = Utils::getValueFromMap<ShipFuel::FuelType>(
        parameters, "FuelType", ShipFuel::FuelType::HFO);
    units::volume::liter_t maxCapacity =
        Utils::getValueFromMap<units::volume::liter_t>(
            parameters, "MaxCapacity", units::volume::liter_t(-1.0));
    if (maxCapacity.value() < 0.0)
    {
        throw std::runtime_error("Tank max capacity is not defined!");
    }

    double initialCapacityPercentage = Utils::getValueFromMap<double>(
        parameters, "TankInitialCapacityPercentage", -1.0);
    if (initialCapacityPercentage < 0.0)
    {
        throw std::runtime_error("Tank initial capacity percentage "
                                 "is not defined!");
    }

    double depthOfDischarge = Utils::getValueFromMap<double>(
        parameters, "TankDepthOfDischage", -1.0);
    if (depthOfDischarge < 0.0)
    {
        qWarning() << "Tank depth of charge is not defined!"
                      "Set to default '0.9'!";
        depthOfDischarge = 0.9;
    }

    SetTankCharacteristics(mFuelType, maxCapacity,
                           initialCapacityPercentage,
                           depthOfDischarge);
}

units::mass::kilogram_t Tank::getCurrentWeight()
{
    return this->mFuelWeight;
}

// Method to set initial properties of the tank.
void Tank::SetTankCharacteristics(ShipFuel::FuelType storedFuelType,
                                  units::volume::liter_t maxCapacity,
                                  double initialCapacityPercentage,
                                  double depthOfDischarge)
{
    // Set the tank's fuel type.
    this->mFuelType = storedFuelType;

    // Set the tank's maximum capacity.
    this->setTankMaxCapacity(maxCapacity);

    // Set the tank's initial capacity.
    this->setTankInitialCapacity(initialCapacityPercentage);
    tankCurrentCapacity = tankInitialCapacity;

    // Update the state of capacity based on
    // the initial percentage provided.
    this->tankStateOfCapacity = initialCapacityPercentage;

    // Set the tank's depth of discharge.
    this->setTankDOD(depthOfDischarge);
}

// Reset the tank to its initial state.
void Tank::reset()
{
    // Reset the cumulative consumed fuel to zero.
    tankCumConsumedFuel = units::volume::liter_t(0.0);

    // Reset the current capacity to its initial value.
    tankCurrentCapacity = tankInitialCapacity;

    // Recalculate the state of capacity.
    tankStateOfCapacity =
        (tankCurrentCapacity / tankMaxCapacity).value();
}

}; // namespace ShipNetSimCore
