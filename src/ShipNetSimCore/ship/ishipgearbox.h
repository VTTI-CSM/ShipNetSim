/**
 * @file IShipGearBox.h
 *
 * @brief This file contains the declaration of the IShipGearBox
 * interface, which represents a gearbox for a ship.
 *
 * This interface defines the methods that should be implemented
 * by any class that represents a gearbox for a ship. The gearbox
 * takes the mechanical power from the engines and transmits
 * it to the ship's propulsion system. The interface provides
 * methods to initialize the gearbox, set and get the ship and
 * engines associated with the gearbox, set gearbox parameters,
 * and get the output RPM and power of the gearbox.
 *
 * @author Ahmed Aredah
 * @date 10.10.2023
 */

#ifndef ISHIPGEARBOX_H
#define ISHIPGEARBOX_H

#include "../../third_party/units/units.h"
#include "ishipengine.h"
#include <QMap>
#include <QObject>
#include <any>

namespace ShipNetSimCore
{
// class IShipEngine;  ///< Forward declaration of engine.
class Ship; ///< Forward declaration of ship class.

/**
 * @class IShipGearBox
 *
 * @brief The IShipGearBox interface represents a gearbox for a ship.
 *
 * This interface defines the methods that should be implemented
 * by any class that represents a gearbox for a ship. The gearbox
 * takes the mechanical power from the engines and transmits
 * it to the ship's propulsion system.
 */
class IShipGearBox : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Constructor for the IShipGearBox interface.
     */
    IShipGearBox();

    /**
     * @brief Destructor for the IShipGearBox interface.
     */
    virtual ~IShipGearBox();

    void moveObjectToThread(QThread *thread);

    /**
     * @brief Initialize the gearbox with the associated ship,
     * engines, and parameters.
     *
     * @param host The ship associated with the gearbox.
     * @param engines The engines connected to the gearbox.
     * @param parameters The parameters for setting up the gearbox.
     */
    virtual void
    initialize(Ship *host, QVector<IShipEngine *> engines,
               const QMap<QString, std::any> &parameters) = 0;

    /**
     * @brief Set the ship associated with the gearbox.
     *
     * @param host The ship associated with the gearbox.
     */
    void setHost(Ship *host);

    /**
     * @brief Set the engines connected to the gearbox.
     *
     * @param engines The engines connected to the gearbox.
     */
    void setEngines(QVector<IShipEngine *> engines);

    /**
     * @brief Get the engines connected to the gearbox.
     *
     * @return A vector of engines connected to the gearbox.
     */
    const QVector<IShipEngine *> getEngines() const;

    /**
     * @brief Get the ship associated with the gearbox.
     *
     * @return The ship associated with the gearbox.
     */
    const Ship *getHost() const;

    /**
     * @brief Set the parameters for setting up the gearbox.
     *
     * @param parameters The parameters for setting up the gearbox.
     */
    virtual void
    setParameters(const QMap<QString, std::any> &parameters) = 0;

    /**
     * @brief Get the output revolutions per minute (RPM) of the
     * gearbox.
     *
     * @return The output RPM of the gearbox.
     */
    virtual units::angular_velocity::revolutions_per_minute_t
    getOutputRPM() const = 0;

    /**
     * @brief get the output RPM Range of the gearbox
     * defined by the engine layout.
     * @return vector of RPM deining the lowest and highest values.
     */
    virtual QPair<units::angular_velocity::revolutions_per_minute_t,
                  units::angular_velocity::revolutions_per_minute_t>
    getOutputRPMRange() const = 0;

    /**
     * @brief Get the output power of the gearbox in kilowatts.
     *
     * @return The output power of the gearbox in kilowatts.
     */
    virtual units::power::kilowatt_t getOutputPower() = 0;

    /**
     * @brief Get the current torque equivalent to the output power.
     *
     * This method is called to get the current torque at the
     * current RPM and output power of the gearbox.
     *
     * @return The current Torque.
     */
    virtual units::torque::newton_meter_t getOutputTorque() = 0;

    /**
     * @brief Get the previous output power of the gearbox in
     * kilowatts.
     *
     * @return The previous output power of the gearbox in kilowatts.
     */
    virtual units::power::kilowatt_t
    getPreviousOutputPower() const = 0;

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
    virtual void
    setEngineTargetState(IShipEngine::EngineProperties newState) = 0;

    virtual void setEngineDefaultTargetState(
        IShipEngine::EngineProperties newState) = 0;

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
    virtual void setEngineMaxPowerLoad(double targetPowerLoad) = 0;

    virtual void updateGearboxOperationalState() = 0;

    virtual IShipEngine::EngineProperties
    getEngineOperationalPropertiesAtRPM(
        units::angular_velocity::revolutions_per_minute_t rpm) = 0;
    virtual IShipEngine::EngineProperties
    getGearboxOperationalPropertiesAtRPM(
        units::angular_velocity::revolutions_per_minute_t rpm) = 0;

    QVector<IShipEngine::EngineProperties>
    getEnginesRatingProperties();

    IShipEngine::EngineOperationalLoad getCurrentOperationalLoad();

    bool requestHigherEnginePower();

    bool requestLowerEnginePower();

protected:
    Ship *mHost; ///< Pointer to the ship associated with the gearbox.
    QVector<IShipEngine *>
        mEngines; ///< Vector of engines connected to the gearbox.
};
}; // namespace ShipNetSimCore
#endif // ISHIPGEARBOX_H
