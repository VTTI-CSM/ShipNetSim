/**
 * @file IShipEngine.h
 *
 * @brief This file contains the declaration of the IShipEngine
 * interface, which represents a ship engine that consumes energy
 * and provides mechanical power.
 *
 * This interface defines the methods that should be implemented
 * by any class that represents a ship engine. A ship engine consumes
 * energy from an energy source and converts it into mechanical power.
 * The interface provides methods to read power efficiency and RPM data from
 * files, get the current efficiency, brake power, and RPM of the engine,
 * as well as get the previous brake power of the engine.
 *
 * @author Ahmed Aredah
 * @date 10.10.2023
 */
#ifndef IENGINE_H
#define IENGINE_H

#include "IEnergyConsumer.h"

/**
 * @class IShipEngine
 *
 * @brief The IShipEngine interface represents a ship engine
 * that consumes energy and provides mechanical power.
 *
 * This interface inherits from the IEnergyConsumer interface
 * and adds methods specific to a ship engine.
 * A ship engine consumes energy from an energy source and
 * converts it into mechanical power.
 */
class IShipEngine : public IEnergyConsumer
{
public:
    /**
     * @brief Destructor for the IShipEngine interface.
     */
    ~IShipEngine();

    /**
     * @brief Read power efficiency data from a file.
     *
     * This method is called to read power efficiency data from
     * a file and store it for later use.
     *
     * @param filePath The path to the file containing the
     * power efficiency data.
     */
    virtual void readPowerEfficiency(QString filePath) = 0;

    /**
     * @brief Read power RPM data from a file.
     *
     * This method is called to read power RPM data from a file
     * and store it for later use.
     *
     * @param filePath The path to the file containing the power RPM data.
     */
    virtual void readPowerRPM(QString filePath) = 0;

    /**
     * @brief Get the current efficiency of the engine.
     *
     * This method is called to get the current efficiency of the engine.
     *
     * @return The current efficiency of the engine as a double.
     */
    virtual double getEfficiency() = 0;

    /**
     * @brief Get the current brake power of the engine.
     *
     * This method is called to get the current brake power of the engine.
     *
     * @return The current brake power of the engine in kilowatts.
     */
    virtual units::power::kilowatt_t getBrakePower() = 0;

    /**
     * @brief Get the current RPM of the engine.
     *
     * This method is called to get the current RPM
     * (revolutions per minute) of the engine.
     *
     * @return The current RPM of the engine.
     */
    virtual units::angular_velocity::revolutions_per_minute_t getRPM() = 0;

    /**
     * @brief Get the previous brake power of the engine.
     *
     * This method is called to get the previous brake power of the engine.
     *
     * @return The previous brake power of the engine in kilowatts.
     */
    virtual units::power::kilowatt_t getPreviousBrakePower() = 0;

};

#endif // IENGINE_H
