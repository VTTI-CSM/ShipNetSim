#include "battery.h"
#include <iostream>

units::energy::kilowatt_hour_t Battery::getBatteryMaxCharge() const
{
    return this->batteryMaxCapacity;
}

void Battery::setBatteryMaxCharge(
    units::energy::kilowatt_hour_t newMaxCharge)
{
    batteryMaxCapacity = newMaxCharge;
}

units::energy::kilowatt_hour_t Battery::getBatteryInitialCharge() const
{
    return this->batteryInitialCharge;
}

void Battery::setBatteryInitialCharge(double newInitialChargePercentage)
{
    this->batteryInitialCharge = batteryMaxCapacity *
                                 newInitialChargePercentage;
}

units::energy::kilowatt_hour_t Battery::getBatteryCurrentCharge() const
{
    return this->batteryCurrentCharge;
}

EnergyConsumptionData Battery::consume(
    units::time::second_t timeStep,
    units::energy::kilowatt_hour_t consumedCharge)
{
    EnergyConsumptionData result;
    // check if the battery is drainable,
    if (! isBatteryDrainable(consumedCharge))
    {
        result.isEnergySupplied = false;
        result.energyConsumed = units::energy::kilowatt_hour_t(0.0);
        result.energyNotConsumed = consumedCharge;
        return result;
    }
    units::energy::kilowatt_hour_t batteryMax_kwh =
        getBatteryMaxDischarge(timeStep);
    if (consumedCharge > batteryMax_kwh)
    {
        units::energy::kilowatt_hour_t EC_extra_kwh =
            consumedCharge - batteryMax_kwh;
        this->batteryCumEnergyConsumed += batteryMax_kwh;
        this->batteryCumNetEnergyConsumed += batteryMax_kwh;
        batteryCurrentCharge -= batteryMax_kwh;
        batteryStateOfCharge = batteryCurrentCharge / batteryMaxCapacity;
        result.isEnergySupplied = true;
        result.energyConsumed = batteryMax_kwh;
        result.energyNotConsumed = EC_extra_kwh;
        return result;
    }
    this->batteryCumEnergyConsumed += consumedCharge;
    this->batteryCumNetEnergyConsumed += consumedCharge;
    batteryCurrentCharge -= consumedCharge;
    batteryStateOfCharge = batteryCurrentCharge / batteryMaxCapacity;
    result.isEnergySupplied = true;
    result.energyConsumed = consumedCharge;
    result.energyNotConsumed = units::energy::kilowatt_hour_t(0.0);
    return result;
}

units::energy::kilowatt_hour_t Battery::rechargeBatteryForHybrids(
    units::time::second_t timeStep,
    units::energy::kilowatt_hour_t recharge)
{
    if (! isBatteryRechargable()) {
        return units::energy::kilowatt_hour_t(0.0);
    }
    units::energy::kilowatt_hour_t batteryMax_kwh =
        getBatteryMaxRecharge(timeStep);
    if (recharge > batteryMax_kwh)
    {
        this->batteryCumEnergyConsumed -= batteryMax_kwh;
        this->batteryCumNetEnergyConsumed -= batteryMax_kwh;
        batteryCurrentCharge += batteryMax_kwh;
        batteryStateOfCharge = batteryCurrentCharge / batteryMaxCapacity;
        return batteryMax_kwh;
    }
    this->batteryCumEnergyConsumed -= recharge;
    this->batteryCumNetEnergyConsumed -= recharge;
    batteryCurrentCharge += recharge;
    batteryStateOfCharge = batteryCurrentCharge / batteryMaxCapacity;
    return recharge;
}

units::energy::kilowatt_hour_t Battery::rechargeBatteryByRegeneratedEnergy(
    units::time::second_t timeStep,
    units::energy::kilowatt_hour_t recharge)
{
    if (! isBatteryRechargable())
    {
        return units::energy::kilowatt_hour_t(0.0);
    }
    units::energy::kilowatt_hour_t batteryMax_kwh =
        getBatteryMaxRecharge(timeStep);
    if (recharge > batteryMax_kwh)
    {
        this->batteryCumEnergyRegenerated += batteryMax_kwh;
        this->batteryCumNetEnergyConsumed -= batteryMax_kwh;
        batteryCurrentCharge += batteryMax_kwh;
        batteryStateOfCharge = batteryCurrentCharge / batteryMaxCapacity;
        return batteryMax_kwh;
    }
    this->batteryCumEnergyRegenerated += recharge;
    this->batteryCumNetEnergyConsumed -= recharge;
    batteryCurrentCharge += recharge;
    batteryStateOfCharge = batteryCurrentCharge / batteryMaxCapacity;
    return recharge;
}

double Battery::getBatteryStateOfCharge() const
{
    return batteryStateOfCharge;
}

double Battery::getBatteryDOD() const
{
    return batteryDOD;
}

void Battery::setBatteryDOD(double newBatteryDOD)
{
    if (newBatteryDOD<=1 && newBatteryDOD>0.0)
    {
        batteryDOD = newBatteryDOD;
    }
    else
    {
        throw std::invalid_argument(
            "the Depth of Discharge must be between 0.0 and "
            "1.0. 0.0: no discharge is allowed, 1.0: full "
            "discharge is allowed");
    }
}

double Battery::getBatteryCRate() const
{
    return batteryDischargeCRate;
}

void Battery::setBatteryCRate(double newBatteryCRate)
{
    batteryDischargeCRate = newBatteryCRate;
    batteryRechargeCRate = 0.5 * newBatteryCRate;
}

bool Battery::isBatteryDrainable(
    units::energy::kilowatt_hour_t requiredCharge)
{
    // check if the battery reaches the low level of charge,
    // enable the recharge request
    this->IsBatteryExceedingThresholds();
    return (requiredCharge <= this->batteryCurrentCharge &&
            batteryStateOfCharge > (1.0- batteryDOD));
}

bool Battery::isBatteryRechargable() {
    // check if the battery reaches the max level of charge,
    // disable the recharge request
    this->IsBatteryExceedingThresholds();
    return (batteryStateOfCharge < batteryRechargeSOCUpperBound);
}


bool Battery::IsBatteryExceedingThresholds(){
    if (this->batteryStateOfCharge >=
        this->batteryRechargeSOCUpperBound)
    {
        this->enableRecharge = false;
    }
    else if (this->batteryStateOfCharge <
               this->batteryRechargeSOCLowerBound)
    { this->enableRecharge = true; }
    return this->enableRecharge;
}


units::energy::kilowatt_hour_t Battery::getBatteryMaxDischarge(
    units::time::second_t timeStep) {
    // returns the max discharge in kWh
    return (batteryMaxCapacity / batteryDischargeCRate) *
           timeStep.convert<units::time::hour>().value();
}

units::energy::kilowatt_hour_t Battery::getBatteryMaxRecharge(
    units::time::second_t timeStep){
    // returns the max recharge in kWh
    return (batteryMaxCapacity /
            batteryRechargeCRate) *
           timeStep.convert<units::time::hour>().value();
}
bool Battery::isRechargeRequired() const
{
    return enableRecharge;
}

double Battery::getBatteryRechargeSOCUpperBound() const
{
    return batteryRechargeSOCUpperBound;
}

void Battery::setBatteryRechargeSOCUpperBound(
    double newBatteryRechargeSOCUpperBound)
{
    if (newBatteryRechargeSOCUpperBound < (1 - batteryDOD))
    {
        batteryRechargeSOCUpperBound = 1 - batteryDOD;
    }
    else if (newBatteryRechargeSOCUpperBound > batteryDOD)
    {
        batteryRechargeSOCUpperBound = batteryDOD;
    }
    else{
        batteryRechargeSOCUpperBound =
            newBatteryRechargeSOCUpperBound;
    }

    if (batteryRechargeSOCUpperBound <
        batteryRechargeSOCLowerBound)
    {
        batteryRechargeSOCUpperBound =
            batteryRechargeSOCLowerBound;
    }
}

double Battery::getBatteryRechargeSOCLowerBound() const
{
    return batteryRechargeSOCLowerBound;
}

void Battery::setBatteryRechargeSOCLowerBound(
    double newBatteryRechargeSOCLowerBound)
{
    if (newBatteryRechargeSOCLowerBound < (1 - batteryDOD))
    {
        batteryRechargeSOCLowerBound = 1 - batteryDOD;
    }
    else if (newBatteryRechargeSOCLowerBound > batteryDOD)
    {
        batteryRechargeSOCLowerBound = batteryDOD;
    }
    else
    {
        batteryRechargeSOCLowerBound =
            newBatteryRechargeSOCLowerBound;
    }
}

units::energy::kilowatt_hour_t Battery::getBatteryCumEnergyConsumption()
{
    return this->batteryCumEnergyConsumed;
}

units::energy::kilowatt_hour_t Battery::getBatteryCumEnergyRegenerated()
{
    return this->batteryCumEnergyRegenerated;
}

units::energy::kilowatt_hour_t Battery::getBatteryCumNetEnergyConsumption()
{
    return this->batteryCumNetEnergyConsumed;
}

bool Battery::batteryHasCharge()
{
    return batteryStateOfCharge > (1.0- batteryDOD);
}

void Battery::setBatteryCharacterstics(
    units::energy::kilowatt_hour_t maxCharge,
    double initialChargePercentage,
    double depthOfDischarge,
    double batteryCRate,
    double maxRechargeSOC,
    double minRechargeSOC)
{
    this->setBatteryMaxCharge(maxCharge);
    this->setBatteryInitialCharge(initialChargePercentage);
    this->batteryCurrentCharge = this->batteryInitialCharge;
    this->batteryStateOfCharge = initialChargePercentage;
    this->setBatteryDOD(depthOfDischarge);
    this->setBatteryCRate(batteryCRate);
    this->setBatteryRechargeSOCLowerBound(minRechargeSOC);
    this->setBatteryRechargeSOCUpperBound(maxRechargeSOC);
}
