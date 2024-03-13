/**
 * @file IEnergySource.h
 *
 * @brief This file contains the declaration of the IEnergySource
 * interface, which represents an energy source for the ship.
 *
 * This interface defines the methods that should be implemented
 * by any class that represents an energy source for a ship.
 * The energy source provides energy to various components of the
 * ship such as engines, shields, or other systems. The interface
 * provides methods to set the characteristics of the energy source,
 * consume energy, get the total energy consumed, and
 * reset the energy source.
 *
 * @author Ahmed Aredah
 * @date 10.10.2023
 */

#ifndef IENERGYSOURCE_H
#define IENERGYSOURCE_H

#include "../../third_party/units/units.h"
#include <QString>
#include <any>

class Ship;  // Forward declaration of the class ship

/**
 * @struct EnergyConsumptionData
 *
 * @brief Represents the data for energy consumption.
 *
 * This struct holds information about whether energy was supplied,
 * the amount of energy consumed, and the amount of energy that
 * was not consumed.
 */
struct EnergyConsumptionData
{
    bool isEnergySupplied;
    units::energy::kilowatt_hour_t energyConsumed;
    units::energy::kilowatt_hour_t energyNotConsumed;
};

/**
 * @class IEnergySource
 *
 * @brief The IEnergySource interface represents an energy source for a ship.
 *
 * This interface defines the methods that should be implemented by
 * any class that represents an energy source for a ship. The energy
 * source provides energy to various components of the
 * ship such as engines, shields, or other systems.
 */
class IEnergySource
{
public:
    /**
     * @brief Virtual destructor for the IEnergySource interface.
     */
    ~IEnergySource()
    {
        mHost = nullptr;
    }

    /**
     * @brief Initialize the energy source.
     * @param host
     */
    void initialize(Ship *host)
    {
        mHost = host;
    }

    /**
     * @brief Set the characteristics of the energy source.
     *
     * This method is called to set the characteristics of the
     * energy source using a map of parameters. The parameters
     * could include properties like capacity, efficiency, etc.
     *
     * @param parameters A map of parameters for setting the
     * characteristics of the energy source.
     */
    virtual void setCharacteristics(
        const QMap<QString, std::any>& parameters) = 0;

    /**
     * @brief Consume energy from the energy source.
     *
     * This method is called to consume energy from the energy
     * source over a specified time step.
     * The method returns a struct containing information about
     * the energy consumed.
     *
     * @param timeStep The time step for which to consume energy.
     * @param consumedkWh The amount of energy to be consumed in
     * kilowatt-hours.
     * @return A struct containing information about the energy consumed.
     */
    virtual EnergyConsumptionData consume(
        units::time::second_t timeStep,
        units::energy::kilowatt_hour_t consumedkWh) = 0;

    /**
     * @brief Get the total energy consumed from the energy source.
     *
     * This method is called to get the total amount of energy consumed
     * from the energy source.
     *
     * @return The total amount of energy consumed in kilowatt-hours.
     */
    virtual units::energy::kilowatt_hour_t getTotalEnergyConsumed() = 0;

    virtual double getCurrentCapacityState() = 0;

    /**
     * @brief Get the updated current weight of the energy source.
     *
     * This method is called to get the total current weight in kilograms
     * of the energy source and its content.
     *
     * @return The total current current weight in klograms.
     */
    virtual units::mass::kilogram_t getCurrentWeight() = 0;

    /**
     * @brief Reset the energy source.
     *
     * This method is called to reset the energy source, setting
     * its state back to its initial state.
     */
    virtual void reset() = 0;

protected:
     Ship *mHost;  ///< A pointer to the host ship.
};

#endif // IENERGYSOURCE_H
