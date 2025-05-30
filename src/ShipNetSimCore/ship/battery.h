/**
 * @file battery.h
 * @brief This file declares the Battery class.
 *        The Battery class represents a battery used in a ship.
 *        It stores information about the battery's capacity,
 *        charge level, state of charge, discharge rate, recharge
 * rate, depth of discharge, and other properties. The Battery class
 * provides methods for setting and retrieving battery properties,
 * consuming and recharging the battery, and checking battery status.
 *        Note: The implementation of the class is not provided
 *        in this declaration file.
 *              It should be implemented separately in a
 *              corresponding source file.
 * @author Ahmed Aredah
 * @date 3/20/2023
 */

#ifndef BATTERY_H
#define BATTERY_H

#include "../../third_party/units/units.h"
#include "../export.h"
#include "ienergysource.h"
#include <iostream>

namespace ShipNetSimCore
{
class SHIPNETSIM_EXPORT Battery : public IEnergySource
{
private:
    // Battery max capacity
    units::energy::kilowatt_hour_t batteryMaxCapacity;
    // Battery initial capacity
    units::energy::kilowatt_hour_t batteryInitialCharge;
    // Battery current charge
    units::energy::kilowatt_hour_t batteryCurrentCharge;
    // Battery State of Charge
    double batteryStateOfCharge;
    // The C-Rate value of the battery while discharging
    double batteryDischargeCRate;
    // The C-Rate value of the battery while recharging
    double batteryRechargeCRate;
    // The minimum battery depth of discharge
    double batteryDOD;
    // The depth of recharge at which the battery stops recharging
    double batteryRechargeSOCUpperBound;
    // The depth of discharge at which the battery requires a recharge
    double batteryRechargeSOCLowerBound;
    // Indicates whether the battery needs to recharge
    bool enableRecharge = false;
    // Cumulative energy consumed by the battery
    units::energy::kilowatt_hour_t batteryCumEnergyConsumed =
        units::energy::kilowatt_hour_t(0.0);
    // Cumulative energy regenerated by the battery
    units::energy::kilowatt_hour_t batteryCumEnergyRegenerated =
        units::energy::kilowatt_hour_t(0.0);
    // Cumulative net energy consumed by the battery
    units::energy::kilowatt_hour_t batteryCumNetEnergyConsumed =
        units::energy::kilowatt_hour_t(0.0);

public:
    /**
     * @brief Sets the battery characteristics.
     * @param maxCharge The maximum charge the battery can hold in
     * kWh.
     * @param initialChargePercentage The initial charge percentage
     *          of the battery.
     * @param depthOfDischarge The allowable depth of discharge
     *          for the battery.
     * @param batteryCRate The C-Rate of discharge for the battery.
     * @param maxRechargeSOC The maximum State of Charge the battery
     *          reaches when recharging (optional).
     * @param minRechargeSOC The minimum State of Charge the battery
     *          reaches when being drained (optional).
     */
    void setBatteryCharacterstics(
        units::energy::kilowatt_hour_t maxCharge,
        double initialChargePercentage, double depthOfDischarge,
        double batteryCRate, double maxRechargeSOC = 0.9,
        double minRechargeSOC = 0.5);

    /**
     * @brief Get the current state of charge.
     * @return The current state of charge in percentage.
     */
    double getCurrentCapacityState() override;
    /**
     * @brief Gets the maximum charge of the battery.
     * @return The maximum charge of the battery in kWh.
     */
    units::energy::kilowatt_hour_t getBatteryMaxCharge() const;

    /**
     * @brief Sets the maximum charge of the battery.
     * @param newMaxCharge The new maximum charge of the battery in
     * kWh.
     */
    void
    setBatteryMaxCharge(units::energy::kilowatt_hour_t newMaxCharge);

    /**
     * @brief Gets the initial charge of the battery.
     * @return The initial charge of the battery in kWh.
     */
    units::energy::kilowatt_hour_t getBatteryInitialCharge() const;

    /**
     * @brief Sets the initial charge of the battery.
     * @param newInitialCharge The new initial charge of the battery
     * in kWh.
     */
    void setBatteryInitialCharge(double newInitialCharge);

    /**
     * @brief Gets the current charge of the battery.
     * @return The current charge of the battery in kWh.
     */
    units::energy::kilowatt_hour_t getBatteryCurrentCharge() const;

    /**
     * @brief Consumes charge from the battery.
     * @param timeStep The time step in seconds.
     * @param consumedCharge The amount of charge to be consumed.
     * @return A pair containing a boolean indicating whether
     *         the consumption was successful
     *         and the actual amount of charge consumed.
     */
    EnergyConsumptionData
    consume(units::time::second_t          timeStep,
            units::energy::kilowatt_hour_t consumedCharge) override;

    /**
     * @brief Recharges the battery for hybrid vehicles.
     * @param timeStep The time step in seconds.
     * @param recharge The amount of charge to be recharged.
     * @return The actual amount of charge recharged.
     */
    units::energy::kilowatt_hour_t rechargeBatteryForHybrids(
        units::time::second_t          timeStep,
        units::energy::kilowatt_hour_t recharge);

    /**
     * @brief Recharges the battery by regenerated energy.
     * @param timeStep The time step in seconds.
     * @param recharge The amount of charge to be recharged.
     * @return The actual amount of charge recharged.
     */
    units::energy::kilowatt_hour_t rechargeBatteryByRegeneratedEnergy(
        units::time::second_t          timeStep,
        units::energy::kilowatt_hour_t recharge);

    /**
     * @brief Gets the state of charge of the battery.
     * @return The state of charge of the battery as a percentage.
     */
    double getBatteryStateOfCharge() const;

    /**
     * @brief Gets the depth of discharge of the battery.
     * @return The depth of discharge of the battery as a percentage.
     */
    double getBatteryDOD() const;

    /**
     * @brief Sets the depth of discharge of the battery.
     * @param newBatteryDOD The new depth of discharge of
     * the battery as a percentage.
     */
    void setBatteryDOD(double newBatteryDOD);

    /**
     * @brief Gets the C-Rate of discharge for the battery.
     * @return The C-Rate of discharge for the battery.
     */
    double getBatteryCRate() const;

    /**
     * @brief Sets the C-Rate of discharge for the battery.
     * @param newBatteryCRate The new C-Rate of discharge for the
     * battery.
     */
    void setBatteryCRate(double newBatteryCRate);

    /**
     * @brief Checks if the battery can be drained by the required
     * charge.
     * @param requiredCharge The required charge to be drained.
     * @return True if the battery can be drained, false otherwise.
     */
    bool
    isBatteryDrainable(units::energy::kilowatt_hour_t requiredCharge);

    /**
     * @brief Checks if the battery can be recharged.
     * @return True if the battery can be recharged, false otherwise.
     */
    bool isBatteryRechargable();

    /**
     * @brief Gets the maximum discharge of the battery.
     * @param timeStep The time step in seconds.
     * @return The maximum discharge of the battery in kWh.
     */
    units::energy::kilowatt_hour_t
    getBatteryMaxDischarge(units::time::second_t timeStep);

    /**
     * @brief Gets the maximum recharge of the battery.
     * @param timeStep The time step in seconds.
     * @return The maximum recharge of the battery in kWh.
     */
    units::energy::kilowatt_hour_t
    getBatteryMaxRecharge(units::time::second_t timeStep);

    /**
     * @brief Checks if the battery requires a recharge.
     * @return True if a recharge is required, false otherwise.
     */
    bool isRechargeRequired() const;

    /**
     * @brief Gets the upper bound of the battery's recharge state of
     * charge.
     * @return The upper bound of the recharge state of charge.
     */
    double getBatteryRechargeSOCUpperBound() const;

    /**
     * @brief Sets the upper bound of the battery's recharge state of
     * charge.
     * @param newBatteryMaxSOC The new upper bound of the recharge
     * state of charge.
     */
    void setBatteryRechargeSOCUpperBound(double newBatteryMaxSOC);

    /**
     * @brief Gets the lower bound of the battery's recharge state of
     * charge.
     * @return The lower bound of the recharge state of charge.
     */
    double getBatteryRechargeSOCLowerBound() const;

    /**
     * @brief Sets the lower bound of the battery's recharge state of
     * charge.
     * @param newBatteryRechargeSOCLowerBound The new lower bound
     * of the recharge state of charge.
     */
    void setBatteryRechargeSOCLowerBound(
        double newBatteryRechargeSOCLowerBound);

    /**
     * @brief Gets the cumulative energy consumption for this battery
     * only.
     * @return The cumulative energy consumption for this battery.
     */
    units::energy::kilowatt_hour_t getBatteryCumEnergyConsumption();

    /**
     * @brief Gets the cumulative energy regenerated for this battery
     * only.
     * @return The cumulative energy regenerated for this battery.
     */
    units::energy::kilowatt_hour_t getBatteryCumEnergyRegenerated();

    /**
     * @brief Gets the cumulative net energy consumption for this
     * battery.
     * @return The cumulative net energy consumption for this battery.
     */
    units::energy::kilowatt_hour_t
    getBatteryCumNetEnergyConsumption();

    /**
     * @brief Checks if the battery has enough charge.
     * @return True if the battery has enough charge, false otherwise.
     */
    bool batteryHasCharge();

    /**
     * @brief Checks if the battery exceeds certain thresholds.
     * @return True if the battery exceeds the thresholds, false
     * otherwise.
     */
    bool IsBatteryExceedingThresholds();

    // IEnergySource interface
    /**
     * @brief get the total energy consumed of the tank
     * @return total energy consumed
     */
    units::energy::kilowatt_hour_t getTotalEnergyConsumed() override;

    /**
     * @brief reset consumed energy and revert the
     * fuel level to initial condition
     */
    void reset() override;

    // IEnergySource interface
    void setCharacteristics(
        const QMap<QString, std::any> &parameters) override;

    /**
     * @brief Get current weight of the battery and its content.
     *
     * This function is not yet implemented.     *
     * This function is called to get the current updated tank total
     * weight.
     *
     * @return the current tank weight in kilogram.
     */
    units::mass::kilogram_t getCurrentWeight() override;

    /**
     * @brief Get the current fuel type stored in the energy source
     * container.
     *
     * This method is called to get the fuel type of the energy
     * source.
     *
     * @return ShipFuel::FuelType
     */
    ShipFuel::FuelType getFuelType() override;

    /**
     * @brief Set the current fuel type stored in the energy source
     * container.
     * @param fuelType The fuel type stored in the energy source
     * container.
     */
    void setFuelType(ShipFuel::FuelType fuelType) override;
};
}; // namespace ShipNetSimCore
#endif // BATTERY_H
