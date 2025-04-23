/**
 * @file ShipGearbox.h
 * @brief File containing the declaration of the ShipGearbox class.
 *
 * This file contains the declaration of the ShipGearbox class, which
 * is a concrete implementation of the IShipGearbox interface.
 *
 * @author Ahmed Aredah
 * @date 10.12.2023
 */
#ifndef SHIPGEARBOX_H
#define SHIPGEARBOX_H

#include "../export.h"

#include "ishipgearbox.h"

namespace ShipNetSimCore
{
/**
 * @class ShipGearBox
 * @brief Implementation of the IShipGearBox interface for ship
 * gearboxes.
 *
 * The ShipGearbox class is a concrete implementation of the
 * IShipGearBox interface, which defines the behavior and properties
 * of a gearbox in a ship's propulsion system. The gearbox is
 * responsible for transmitting power from the ship's engine to its
 * propellers, while also adjusting the rotation speed of the
 * propellers to match the desired speed of the ship.
 *
 * The ShipGearBox class stores the gearbox's efficiency, gear ratio,
 * and output power, and provides methods to initialize the gearbox,
 * set its parameters, and retrieve its output rotation speed and
 * power.
 */
class SHIPNETSIM_EXPORT ShipGearBox : public IShipGearBox
{
public:
    /**
     * @brief Default constructor for the ShipGearbox class.
     *
     * Initializes the gearbox with default values.
     */
    ShipGearBox();

    // IShipGearBox interface

    /**
     * @brief Initializes the gearbox with the given parameters.
     *
     * This method is used to initialize the gearbox with a reference
     * to the ship it is part of, a list of engines connected to it,
     * and a map of parameters that define its behavior and
     * properties.
     *
     * @param host Pointer to the ship that the gearbox is part of.
     * @param engines List of engines connected to the gearbox.
     * @param parameters Map of parameters defining the gearbox's
     * behavior and properties.
     */
    void
    initialize(Ship *host, QVector<IShipEngine *> engines,
               const QMap<QString, std::any> &parameters) override;

    /**
     * @brief Sets the gearbox's parameters.
     *
     * This method is used to update the gearbox's parameters with the
     * values provided in the given map.
     *
     * @param parameters Map of parameters to update.
     */
    void
    setParameters(const QMap<QString, std::any> &parameters) override;

    /**
     * @brief Retrieves the gearbox's output rotation speed.
     *
     * This method returns the output rotation speed of the gearbox in
     * revolutions per minute.
     *
     * @return The output rotation speed in revolutions per minute.
     */
    units::angular_velocity::revolutions_per_minute_t
    getOutputRPM() const override;

    /**
     * @brief get the output RPM Range of the gearbox
     * defined by the engine layout.
     * @return vector of RPM deining the lowest and highest values.
     */
    QPair<units::angular_velocity::revolutions_per_minute_t,
          units::angular_velocity::revolutions_per_minute_t>
    getOutputRPMRange() const override;

    /**
     * @brief Retrieves the gearbox's output power.
     *
     * This method returns the output power of the gearbox in
     * kilowatts.
     *
     * @return The output power in kilowatts.
     */
    units::power::kilowatt_t getOutputPower() override;

    /**
     * @brief Get the current torque equivalent to the output power.
     *
     * This method is called to get the current torque at the
     * current RPM and output power of the gearbox.
     *
     * @return The current Torque.
     */
    units::torque::newton_meter_t getOutputTorque() override;

    /**
     * @brief set the engine new target state (max state) could be L1
     *
     * @details This function sets the engine target state. The
     * function is mainly designed to set the engine target state that
     * equates the engine power curve to the propeller curve.
     *
     * @param newState the target state that the engine should abide
     * by.
     */
    void setEngineTargetState(
        IShipEngine::EngineProperties newState) override;

    void setEngineDefaultTargetState(
        IShipEngine::EngineProperties newState) override;

    /**
     * @brief set the engine max power load.
     *
     * @details This function sets the engine max power load. power
     * load is the max power / max power the engine can reach. the
     * power load is a fraction between 0 and 1.
     *
     * @param targetPowerLoad the target power load the engine should
     * reach.
     */
    void setEngineMaxPowerLoad(double targetPowerLoad) override;

    /**
     * @brief Retrieves the gearbox's output power in the previous
     * step.
     *
     * This method returns the output power of the gearbox in the
     * previous time step, in kilowatts.
     *
     * @return The output power in the previous time step, in
     * kilowatts.
     */
    units::power::kilowatt_t getPreviousOutputPower() const override;

    void updateGearboxOperationalState() override;

    IShipEngine::EngineProperties getEngineOperationalPropertiesAtRPM(
        units::angular_velocity::revolutions_per_minute_t rpm)
        override;

    IShipEngine::EngineProperties
    getGearboxOperationalPropertiesAtRPM(
        units::angular_velocity::revolutions_per_minute_t rpm)
        override;

private:
    double mEfficiency;    // Gearbox efficiency in the range [0, 1].
    double mGearRationTo1; // Gear ratio of the gearbox.
    units::power::kilowatt_t
        mOutputPower; // Output power of the gearbox.
};
}; // namespace ShipNetSimCore
#endif // SHIPGEARBOX_H
