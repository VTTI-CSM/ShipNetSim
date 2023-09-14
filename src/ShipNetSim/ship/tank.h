/**
 * @file tank.h
 * @brief This file declares the Tank class.
 *        The Tank class represents a fuel tank used in a ship.
 *        It stores information about the tank's capacity,
 *        current fuel level, state of capacity,
 *        depth of discharge, and other properties.
 *        The Tank class provides methods for setting and
 *        retrieving tank properties,
 *        consuming fuel from the tank, and checking tank status.
 *        Note: The implementation of the class is not
 *        provided in this declaration file.
 *              It should be implemented separately in a
 *              corresponding source file.
 * @author Ahmed Aredah
 * @date 3/20/2023
 */

#ifndef TANK_H
#define TANK_H

#include "../../third_party/units/units.h"

class Tank {
private:
    /** Fuel cell variables if other fuel types and
     *  battery tender max capacity */

    // Maximum capacity of the tank in liters
    units::volume::liter_t tankMaxCapacity;
    // Initial capacity of the tank in liters
    units::volume::liter_t tankInitialCapacity;
    // Current capacity of the tank in liters
    units::volume::liter_t tankCurrentCapacity;
    // State of capacity of the tank
    double tankStateOfCapacity;
    // Depth of discharge
    double tankDOD;
    // Total consumed amount of fuel in liters
    units::volume::liter_t tankCumConsumedFuel =
        units::volume::liter_t(0.0);

public:
    /**
     * @brief Set the main properties of the tank (initialize the tank)
     *
     * @param maxCapacity   The maximum capacity the tank can hold in liters
     * @param initialCapacityPercentage
     *                      The initial capacity percentage that the tank
     *                      holds once the train is loaded onto the network.
     * @param depthOfDischarge
     *                      The allowable depth of discharge, the tank
     *                      can drain to.
     */
    void SetTankCharacteristics(units::volume::liter_t maxCapacity,
                                double initialCapacityPercentage,
                                double depthOfDischarge);

    /**
     * Gets the maximum capacity of the tank
     *
     * @returns The tank's maximum capacity in liters.
     */
    units::volume::liter_t getTankMaxCapacity() const;

    /**
     * Sets the maximum capacity of the tank
     *
     * @param newMaxCapacity The new maximum capacity of the tank in liters.
     */
    void setTankMaxCapacity(units::volume::liter_t newMaxCapacity);

    /**
     * Gets the initial capacity of the tank
     *
     * @returns The tank's initial capacity in liters.
     */
    units::volume::liter_t getTankInitialCapacity() const;

    /**
     * Sets the initial capacity of the tank
     *
     * @param newInitialCapacityPercentage The new
     *          initial capacity percentage of the tank.
     */
    void setTankInitialCapacity(double newInitialCapacityPercentage);

    /**
     * Gets the current capacity of the tank
     *
     * @returns The tank's current capacity in liters.
     */
    units::volume::liter_t getTankCurrentCapacity() const;

    /**
     * Consumes fuel from the tank
     *
     * @param consumedAmount The amount of fuel to consume
     *                          from the tank in liters.
     * @returns The actual amount of fuel consumed from the tank in liters.
     */
    units::volume::liter_t consumeTank(units::volume::liter_t consumedAmount);

    /**
     * Gets the state of capacity of the tank
     *
     * @returns The tank's state of capacity.
     */
    double getTankStateOfCapacity() const;

    /**
     * Checks if the specified amount of fuel is drainable from the tank
     *
     * @param consumedAmount The amount of fuel to be
     *                          drained from the tank in liters.
     * @returns True if the tank is drainable (has enough fuel),
     *              false otherwise.
     */
    bool isTankDrainable(units::volume::liter_t consumedAmount);

    /**
     * Gets the depth of discharge of the tank
     *
     * @returns The tank's depth of discharge.
     */
    double getTankDOD() const;

    /**
     * Sets the depth of discharge of the tank
     *
     * @param newTankDOD The new depth of discharge of the tank.
     */
    void setTankDOD(double newTankDOD);

    /**
     * Checks if the tank has fuel (current capacity > 0)
     *
     * @returns True if the tank has fuel, false otherwise.
     */
    bool tankHasFuel();

    /**
     * Gets the total amount of fuel consumed from the tank
     *
     * @returns The cumulative consumed fuel from the tank in liters.
     */
    units::volume::liter_t getTankCumConsumedFuel() const;
};

#endif // TANK_H
