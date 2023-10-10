#include "tank.h"
#include <iostream>

units::volume::liter_t Tank::getTankMaxCapacity() const
{
    return this->tankMaxCapacity;
}

void Tank::setTankMaxCapacity(units::volume::liter_t newMaxCapacity)
{
    this->tankMaxCapacity = newMaxCapacity;
}

units::volume::liter_t Tank::getTankInitialCapacity() const
{
    return tankInitialCapacity;
}

void Tank::setTankInitialCapacity(double newInitialCapacityPercentage)
{
    tankInitialCapacity = tankMaxCapacity * newInitialCapacityPercentage;
    fuelWeight = ShipFuel::getWeight(tankInitialCapacity, fuelType);
}

units::volume::liter_t Tank::getTankCurrentCapacity() const
{
    return tankCurrentCapacity;
}

EnergyConsumptionData Tank::consume(
    units::time::second_t timeStep,
    units::energy::kilowatt_hour_t consumedkWh)
{
    EnergyConsumptionData result;

    (void) timeStep;
    auto consumedAmount =
        ShipFuel::convertKwhToLiters(consumedkWh, fuelType);
    if (! isTankDrainable(consumedAmount) &&
        consumedAmount > tankCurrentCapacity)
    {
        result.isEnergySupplied = false;
        result.energyConsumed = units::energy::kilowatt_hour_t(0.0);
        result.energyNotConsumed = consumedkWh;
        return result;
    }
    tankCumConsumedFuel += consumedAmount;
    tankCurrentCapacity -= consumedAmount;
    fuelWeight = ShipFuel::getWeight(tankCurrentCapacity, fuelType);
    tankStateOfCapacity = tankCurrentCapacity / tankMaxCapacity;

    result.isEnergySupplied = true;
    result.energyConsumed = consumedkWh;
    result.energyNotConsumed = units::energy::kilowatt_hour_t(0.0);
    return result;
}

double Tank::getTankStateOfCapacity() const
{
    return tankStateOfCapacity;
}

bool Tank::isTankDrainable(units::volume::liter_t consumedAmount) {
    return (consumedAmount <= this->tankCurrentCapacity &&
            tankStateOfCapacity > (1.0- tankDOD));
}

double Tank::getTankDOD() const {
    return tankDOD;
}

void Tank::setTankDOD(double newTankDOD) {
    if (newTankDOD<=1 && newTankDOD>0.0){
        tankDOD = newTankDOD;
    }
    else {
        throw std::invalid_argument(
            "the Depth of Discharge must be between 0.0 and "
            "1.0. 0.0: no discharge is allowed, 1.0: full "
            "discharge is allowed");
    }
}

units::volume::liter_t Tank::getTankCumConsumedFuel() const
{
    return tankCumConsumedFuel;
}

units::energy::kilowatt_hour_t Tank::getTotalEnergyConsumed()
{
    return ShipFuel::convertLitersToKwh(tankCumConsumedFuel, fuelType);
}

ShipFuel::FuelType Tank::getFuelType()
{
    return fuelType;
}

bool Tank::tankHasFuel(){
    return tankStateOfCapacity > (1.0- tankDOD);
}

void Tank::SetTankCharacteristics(ShipFuel::FuelType storedFuelType,
                                  units::volume::liter_t maxCapacity,
                                  double initialCapacityPercentage,
                                  double depthOfDischarge)
{
    this->fuelType = storedFuelType;
    this->setTankMaxCapacity(maxCapacity);
    this->setTankInitialCapacity(initialCapacityPercentage);
    tankCurrentCapacity = tankInitialCapacity;

    this->tankStateOfCapacity = initialCapacityPercentage;
    this->setTankDOD(depthOfDischarge);
}

