#include "battery.h"
#include <iostream>
#include "../utils/utils.h"

// Getter method for the maximum charge of the battery.
units::energy::kilowatt_hour_t Battery::getBatteryMaxCharge() const
{
    // Simply return the max capacity value of the battery.
    return this->batteryMaxCapacity;
}

// Setter method to update the maximum charge of the battery.
void Battery::setBatteryMaxCharge(
    units::energy::kilowatt_hour_t newMaxCharge)
{
    // Update the max capacity with the provided value.
    batteryMaxCapacity = newMaxCharge;
}

// Getter method for the initial charge of the battery.
units::energy::kilowatt_hour_t Battery::getBatteryInitialCharge() const
{
    // Return the initial charge of the battery.
    return this->batteryInitialCharge;
}

// Setter method for the initial charge of the battery.
void Battery::setBatteryInitialCharge(double newInitialChargePercentage)
{
    // Calculate and set the initial charge based on the
    // provided percentage and the battery's max capacity.
    this->batteryInitialCharge = batteryMaxCapacity *
                                 newInitialChargePercentage;
}

// Getter method for the current charge of the battery.
units::energy::kilowatt_hour_t Battery::getBatteryCurrentCharge() const
{
    // Return the current charge of the battery.
    return this->batteryCurrentCharge;
}

// Method to consume energy from the battery.
EnergyConsumptionData Battery::consume(
    units::time::second_t timeStep,
    units::energy::kilowatt_hour_t consumedCharge)
{
    EnergyConsumptionData result; // To store consumption details.

    // Check if the desired energy amount is drainable from the battery.
    if (! isBatteryDrainable(consumedCharge))
    {
        // If not, set the result to indicate unsuccessful energy supply.
        result.isEnergySupplied = false;
        result.energyConsumed = units::energy::kilowatt_hour_t(0.0);
        result.energyNotConsumed = consumedCharge;
        return result;
    }
    // Calculate the maximum energy the
    // battery can supply in the given time step.
    units::energy::kilowatt_hour_t batteryMax_kwh =
        getBatteryMaxDischarge(timeStep);

    // If the requested energy exceeds the maximum the battery can supply.
    if (consumedCharge > batteryMax_kwh)
    {
        units::energy::kilowatt_hour_t EC_extra_kwh =
            consumedCharge - batteryMax_kwh;

        // Update battery state after partial consumption.
        this->batteryCumEnergyConsumed += batteryMax_kwh;
        this->batteryCumNetEnergyConsumed += batteryMax_kwh;
        batteryCurrentCharge -= batteryMax_kwh;
        batteryStateOfCharge = batteryCurrentCharge / batteryMaxCapacity;

        // Set the result details.
        result.isEnergySupplied = true;
        result.energyConsumed = batteryMax_kwh;
        result.energyNotConsumed = EC_extra_kwh;
        return result;
    }

    // If the requested energy is within the limits.
    this->batteryCumEnergyConsumed += consumedCharge;
    this->batteryCumNetEnergyConsumed += consumedCharge;
    batteryCurrentCharge -= consumedCharge;
    batteryStateOfCharge = batteryCurrentCharge / batteryMaxCapacity;

    // Set the result details.
    result.isEnergySupplied = true;
    result.energyConsumed = consumedCharge;
    result.energyNotConsumed = units::energy::kilowatt_hour_t(0.0);
    return result;
}

// Recharge the battery with energy specifically for hybrid systems.
units::energy::kilowatt_hour_t Battery::rechargeBatteryForHybrids(
    units::time::second_t timeStep,
    units::energy::kilowatt_hour_t recharge)
{
    // Check if the battery is in a state where it can be recharged.
    if (! isBatteryRechargable())
    {
        return units::energy::kilowatt_hour_t(0.0);
    }
    // Calculate the maximum energy the
    // battery can accept in the given time step.
    units::energy::kilowatt_hour_t batteryMax_kwh =
        getBatteryMaxRecharge(timeStep);

    // If the provided recharge amount
    // exceeds the maximum the battery can accept.
    if (recharge > batteryMax_kwh)
    {
        // Update the battery state after partial recharge.
        this->batteryCumEnergyConsumed -= batteryMax_kwh;
        this->batteryCumNetEnergyConsumed -= batteryMax_kwh;
        batteryCurrentCharge += batteryMax_kwh;
        batteryStateOfCharge = batteryCurrentCharge / batteryMaxCapacity;

        // Return the actual recharge amount.
        return batteryMax_kwh;
    }

    // If the recharge amount is within the limits.
    this->batteryCumEnergyConsumed -= recharge;
    this->batteryCumNetEnergyConsumed -= recharge;
    batteryCurrentCharge += recharge;
    batteryStateOfCharge = batteryCurrentCharge / batteryMaxCapacity;

    // Return the provided recharge amount.
    return recharge;
}

// Recharge the battery using regenerated energy.
units::energy::kilowatt_hour_t Battery::rechargeBatteryByRegeneratedEnergy(
    units::time::second_t timeStep,
    units::energy::kilowatt_hour_t recharge)
{
    // Check if the battery is in a state where it can be recharged.
    if (! isBatteryRechargable())
    {
        return units::energy::kilowatt_hour_t(0.0);
    }

    // Calculate the maximum energy the
    // battery can accept in the given time step.
    units::energy::kilowatt_hour_t batteryMax_kwh =
        getBatteryMaxRecharge(timeStep);

    // If the provided recharge amount
    // exceeds the maximum the battery can accept.
    if (recharge > batteryMax_kwh)
    {
        // Update the battery state after
        // partial recharge using regenerated energy.
        this->batteryCumEnergyRegenerated += batteryMax_kwh;
        this->batteryCumNetEnergyConsumed -= batteryMax_kwh;
        batteryCurrentCharge += batteryMax_kwh;
        batteryStateOfCharge = batteryCurrentCharge / batteryMaxCapacity;

        // Return the actual recharge amount.
        return batteryMax_kwh;
    }

    // If the recharge amount is within the limits.
    this->batteryCumEnergyRegenerated += recharge;
    this->batteryCumNetEnergyConsumed -= recharge;
    batteryCurrentCharge += recharge;
    batteryStateOfCharge = batteryCurrentCharge / batteryMaxCapacity;

    // Return the provided recharge amount.
    return recharge;
}

// Method to get the battery's state of charge.
double Battery::getBatteryStateOfCharge() const
{
    // Return the current state of charge as a percentage.
    return batteryStateOfCharge;
}

// Getter method for the Depth of Discharge (DOD) of the battery.
double Battery::getBatteryDOD() const
{
    // Return the current Depth of Discharge.
    return batteryDOD;
}

// Setter method for the Depth of Discharge (DOD) of the battery.
void Battery::setBatteryDOD(double newBatteryDOD)
{
    // Ensure the provided DOD is within a valid range.
    if (newBatteryDOD<=1 && newBatteryDOD>0.0)
    {
        batteryDOD = newBatteryDOD;
    }
    else
    {
        // If not, throw an exception indicating an invalid DOD value.
        throw std::invalid_argument(
            "the Depth of Discharge must be between 0.0 and "
            "1.0. 0.0: no discharge is allowed, 1.0: full "
            "discharge is allowed");
    }
}

// Method to retrieve the current discharge C-rate.
double Battery::getBatteryCRate() const
{
    // Return the discharge C-rate.
    return batteryDischargeCRate;
}

// Setter to update the discharge C-rate.
void Battery::setBatteryCRate(double newBatteryCRate)
{
    // Set the discharge C-rate.
    batteryDischargeCRate = newBatteryCRate;

    // Update the recharge C-rate as half of the discharge C-rate.
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

// Check if the battery can drain the required charge.
bool Battery::isBatteryRechargable() {
    // check if the battery reaches the max level of charge,
    // disable the recharge request
    this->IsBatteryExceedingThresholds();
    return (batteryStateOfCharge < batteryRechargeSOCUpperBound);
}

// Helper function to check if the battery's state
// of charge is within certain thresholds.
bool Battery::IsBatteryExceedingThresholds(){
    if (this->batteryStateOfCharge >=
        this->batteryRechargeSOCUpperBound)
    {
        // If battery is charged above upper bound, disable recharging.
        this->enableRecharge = false;
    }
    else if (this->batteryStateOfCharge <
               this->batteryRechargeSOCLowerBound)
    {
        // If battery is charged below lower bound, enable recharging.
        this->enableRecharge = true;
    }
    return this->enableRecharge;
}

// Getter method to retrieve the total energy consumed from the battery.
units::energy::kilowatt_hour_t Battery::getTotalEnergyConsumed()
{
    return batteryCumEnergyConsumed;
}

// Method to reset the battery to its initial state.
void Battery::reset()
{
    // Reset all tracking values for energy consumed,
    // regenerated, and net consumption.
    batteryCumEnergyConsumed =
        units::energy::kilowatt_hour_t(0.0);
    batteryCumEnergyRegenerated =
        units::energy::kilowatt_hour_t(0.0);
    batteryCumNetEnergyConsumed =
        units::energy::kilowatt_hour_t(0.0);

    // Reset the battery's current charge to its initial charge.
    batteryCurrentCharge =
        batteryInitialCharge;

    // Recalculate the battery's state of charge.
    batteryStateOfCharge =
        (batteryInitialCharge/batteryMaxCapacity).value();
}

// Helper function to calculate the maximum discharge energy
// for the battery based on a time step.
units::energy::kilowatt_hour_t Battery::getBatteryMaxDischarge(
    units::time::second_t timeStep) {
    // returns the max discharge in kWh
    return (batteryMaxCapacity / batteryDischargeCRate) *
           timeStep.convert<units::time::hour>().value();
}

// Helper function to calculate the maximum recharge energy
// for the battery based on a time step.
units::energy::kilowatt_hour_t Battery::getBatteryMaxRecharge(
    units::time::second_t timeStep){
    // returns the max recharge in kWh
    return (batteryMaxCapacity /
            batteryRechargeCRate) *
           timeStep.convert<units::time::hour>().value();
}

// Method to determine if the battery requires recharging.
bool Battery::isRechargeRequired() const
{
    // Returns the current status of the recharge flag.
    return enableRecharge;
}

// Get the upper bound for the battery's State of Charge
// (SoC) during recharging.
double Battery::getBatteryRechargeSOCUpperBound() const
{
    return batteryRechargeSOCUpperBound;
}

// Set a new upper bound for the battery's State of Charge during recharging.
void Battery::setBatteryRechargeSOCUpperBound(
    double newBatteryRechargeSOCUpperBound)
{
    // Check the new upper bound against limits and adjust if necessary.
    // Ensure it's within acceptable discharge levels.
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

    // Ensure that the upper bound isn't set
    // lower than the current lower bound.
    if (batteryRechargeSOCUpperBound <
        batteryRechargeSOCLowerBound)
    {
        batteryRechargeSOCUpperBound =
            batteryRechargeSOCLowerBound;
    }
}

// Get the lower bound for the battery's State of Charge during recharging.
double Battery::getBatteryRechargeSOCLowerBound() const
{
    return batteryRechargeSOCLowerBound;
}

// Set a new lower bound for the battery's State of Charge during recharging.
void Battery::setBatteryRechargeSOCLowerBound(
    double newBatteryRechargeSOCLowerBound)
{
    // Ensure the new lower bound lies within the acceptable discharge levels.
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

// Retrieve the total energy consumption from the battery.
units::energy::kilowatt_hour_t Battery::getBatteryCumEnergyConsumption()
{
    return this->batteryCumEnergyConsumed;
}

// Retrieve the total energy regenerated back into the battery.
units::energy::kilowatt_hour_t Battery::getBatteryCumEnergyRegenerated()
{
    return this->batteryCumEnergyRegenerated;
}

// Calculate and return the net energy consumption (consumed - regenerated).
units::energy::kilowatt_hour_t Battery::getBatteryCumNetEnergyConsumption()
{
    return this->batteryCumNetEnergyConsumed;
}

// Check if the battery still has a charge based on its
// state of charge and the depth of discharge.
bool Battery::batteryHasCharge()
{
    return batteryStateOfCharge > (1.0- batteryDOD);
}

void Battery::setCharacteristics(const QMap<QString, std::any> &parameters)
{
    units::energy::kilowatt_hour_t maxCharge =
        Utils::getValueFromMap<units::energy::kilowatt_hour_t>(
        parameters, "MaxCharge", units::energy::kilowatt_hour_t(-1.0));
    if (maxCharge.value() < 0.0)
    {
        qFatal("Battery max charge is not defined!");
    }

    double initialChargePercentage =
        Utils::getValueFromMap<double>(parameters,
                                       "InitialChargePercentage",
                                       -1.0);
    if (initialChargePercentage < 0.0) {
        qFatal("Battery initial charge percentage is not defined!");
    }

    double depthOfDischarge =
        Utils::getValueFromMap<double>(parameters,
                                       "DepthOfDischarge",
                                       -1.0);
    if (depthOfDischarge < 0.0) {
        qFatal("Battery depth of charge is not defined!");
    }

    double batteryCRate =
        Utils::getValueFromMap<double>(parameters,
                                       "CRate",
                                       -1.0);
    if (batteryCRate < 0.0) {
        qFatal("Battery c-rate is not defined!");
    }

    double minRechargeSOC =
        Utils::getValueFromMap<double>(parameters,
                                       "MinRechargeSOC",
                                       -1.0);
    if (minRechargeSOC < 0.0) {
        qFatal("Battery min recharge State of Charge is not defined!");
    }

    double maxRechargeSOC =
        Utils::getValueFromMap<double>(parameters,
                                       "MaxRechargeSOC",
                                       -1.0);
    if (maxRechargeSOC < 0.0) {
        qFatal("Battery max recharge State of Charge is not defined!");
    }

    setBatteryCharacterstics(maxCharge, initialChargePercentage,
                             depthOfDischarge, batteryCRate, maxRechargeSOC,
                             minRechargeSOC);
}

// Set the battery's main characteristics and initial conditions.
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
