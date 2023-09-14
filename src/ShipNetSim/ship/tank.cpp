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
}

units::volume::liter_t Tank::getTankCurrentCapacity() const
{
    return tankCurrentCapacity;
}

units::volume::liter_t Tank::consumeTank(
    units::volume::liter_t consumedAmount)
{
    if (! isTankDrainable(consumedAmount))
    {
        return consumedAmount;
    }
    tankCumConsumedFuel += consumedAmount;
    tankCurrentCapacity -= consumedAmount;
    tankStateOfCapacity = tankCurrentCapacity / tankMaxCapacity;
    return units::volume::liter_t(0.0);
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

bool Tank::tankHasFuel(){
    return tankStateOfCapacity > (1.0- tankDOD);
}

void Tank::SetTankCharacteristics(units::volume::liter_t maxCapacity,
                                  double initialCapacityPercentage,
                                  double depthOfDischarge)
{
    this->setTankMaxCapacity(maxCapacity);
    this->setTankInitialCapacity(initialCapacityPercentage);
    tankCurrentCapacity = tankInitialCapacity;

    this->tankStateOfCapacity = initialCapacityPercentage;
    this->setTankDOD(depthOfDischarge);
}

