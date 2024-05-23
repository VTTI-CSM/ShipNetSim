/**
 * @file IShipPropeller.h
 *
 * @brief This file contains the declaration of the IShipPropeller interface,
 * which represents a propeller for a ship.
 *
 * This interface defines the methods that should be implemented by
 * any class that represents a propeller for a ship. The propeller
 * takes the mechanical power from the gearbox and converts it into
 * thrust to propel the ship. The interface provides methods to initialize
 * the propeller, set and get the ship and gearbox associated with the
 * propeller, set propeller parameters, and get the propeller's
 * efficiency, power, thrust, and other properties.
 *
 * @author Ahmed Aredah
 * @date 10.10.2023
 */

#ifndef I_SHIPPROPELLER_H
#define I_SHIPPROPELLER_H

#include "../../third_party/units/units.h"
#include "ishipengine.h"
#include "ishipgearbox.h"
#include <QMap>
#include <QString>
#include <any>

namespace ShipNetSimCore
{
class Ship;             ///< Forward declaration of ship class.
/**
 * @brief The IShipPropeller class
 * @class IShipPropeller
 *
 * Provides an interface to manage ships propellers
 */
class IShipPropeller
{
public:
    /**
     * @brief Constructor for the IShipPropeller interface.
     */
    IShipPropeller();

    /**
     * @brief Destructor for the IShipPropeller interface.
     */
    virtual ~IShipPropeller();

    /**
     * @brief Initialize the propeller with the associated ship, gearbox,
     * and parameters.
     *
     * @param ship The ship associated with the propeller.
     * @param gearbox The gearbox connected to the propeller.
     * @param parameters The parameters for setting up the propeller.
     */
    virtual void initialize(Ship *ship, IShipGearBox *gearbox,
                            const QMap<QString, std::any>& parameters) = 0;

    /**
     * @brief Set the ship associated with the propeller.
     *
     * @param ship The ship associated with the propeller.
     */
    void setHost(Ship *ship);

    /**
     * @brief Set the gearbox connected to the propeller.
     *
     * @param gearbox The gearbox connected to the propeller.
     */
    void setGearBox(IShipGearBox* gearbox);

    /**
     * @brief Get the ship associated with the propeller.
     *
     * @return The ship associated with the propeller.
     */
    const Ship *getHost() const;

    /**
     * @brief Get the gearbox connected to the propeller.
     *
     * @return The gearbox connected to the propeller.
     */
    const IShipGearBox *getGearBox() const;

    /**
     * @brief Set the parameters for setting up the propeller.
     *
     * @param parameters The parameters for setting up the propeller.
     */
    virtual void setParameters(const QMap<QString, std::any>& parameters) = 0;

    /**
     * @brief Get the diameter of the propeller.
     *
     * @return The diameter of the propeller.
     */
    [[nodiscard]] units::length::meter_t getPropellerDiameter() const;

    /**
     * @brief Set the diameter of the propeller.
     *
     * @param newPropellerDiameter The new diameter of the propeller.
     */
    void setPropellerDiameter(
        const units::length::meter_t &newPropellerDiameter);

    /**
     * @brief Get the Pitch of the propeller.
     *
     * @return The pitch of the propeller.
     */
    [[nodiscard]] units::length::meter_t getPropellerPitch() const;

    /**
     * @brief Set the pitch of the propeller.
     *
     * @param newPropellerPitch The new Pitch of the propeller.
     */
    void setPropellerPitch(const units::length::meter_t newPropellerPitch);

    /**
     * @brief Get the expanded blade area of the propeller.
     *
     * @return The expanded blade area of the propeller.
     */
    [[nodiscard]] units::area::square_meter_t
    getPropellerExpandedBladeArea() const;

    /**
     * @brief Set the expanded blade area of the propeller.
     *
     * @param newExpandedBladeArea The new expanded blade area
     * of the propeller.
     */
    void setPropellerExpandedBladeArea(
        const units::area::square_meter_t &newExpandedBladeArea);

    /**
     * @brief Get the disk area of the propeller.
     *
     * @return The disk area of the propeller.
     */
    [[nodiscard]] units::area::square_meter_t getPropellerDiskArea() const;

    /**
     * @brief Set the disk area of the propeller.
     *
     * @param newPropellerDiskArea The new disk area of the propeller.
     */
    void setPropellerDiskArea(
        const units::area::square_meter_t &newPropellerDiskArea);

    /**
     * @brief Get the expanded area ratio of the propeller.
     *
     * @return The expanded area ratio of the propeller.
     */
    [[nodiscard]] double getPropellerExpandedAreaRatio() const;

    /**
     * @brief Set the expanded area ratio of the propeller.
     *
     * @param newPropellerExpandedAreaRation The new expanded
     * area ratio of the propeller.
     */
    void setPropellerExpandedAreaRatio(double newPropellerExpandedAreaRatio);

    [[nodiscard]] int getPropellerBladesCount() const;

    void setPropellerBladesCount(int newPropellerBladesCount);

    /**
     * @brief Get the shaft efficiency of the propeller.
     *
     * @return The shaft efficiency of the propeller.
     */
    virtual double getShaftEfficiency() = 0;

    /**
     * @brief Set the shaft efficiency of the propeller.
     *
     * @param newShaftEfficiency The new shaft efficiency of the propeller.
     */
    virtual void setShaftEfficiency(double newShaftEfficiency) = 0;

    /**
     * @brief Get the propeller efficiency.
     *
     * @return The propeller efficiency.
     */
    virtual double getPropellerEfficiency() = 0;


    /**
     * @brief Get the effective power of the propeller.
     *
     * @return The effective power of the propeller.
     */
    virtual units::power::kilowatt_t getEffectivePower() = 0;

    /**
     * @brief Get the previous effective power of the propeller.
     *
     * @return The previous effective power of the propeller.
     */
    virtual units::power::kilowatt_t getPreviousEffectivePower() = 0;

    /**
     * @brief Get the torque of the propeller.
     *
     * @return The torque of the propeller.
     */
    virtual units::torque::newton_meter_t getTorque() = 0;

    /**
     * @brief Get the thrust of the propeller.
     *
     * @return The thrust of the propeller.
     */
    virtual units::force::newton_t getThrust() = 0;

    /**
     * @brief Get the revolutions per minute (RPM) of the propeller.
     *
     * @return The revolutions per minute (RPM) of the propeller.
     */
    virtual units::angular_velocity::revolutions_per_minute_t getRPM() = 0;

    /**
     * @brief Get the thrust coefficient of the propeller.
     *
     * @return The thrust coefficient of the propeller.
     */
    virtual double getThrustCoefficient(
        units::angular_velocity::revolutions_per_minute_t rpm) = 0;

    /**
     * @brief Get the torque coefficient of the propeller.
     *
     * @return The torque coefficient of the propeller.
     */
    virtual double getTorqueCoefficient(
        units::angular_velocity::revolutions_per_minute_t rpm) = 0;

    /**
     * @brief Get the advanced ratio of the propeller.
     *
     * @return The advanced ratio of the propeller.
     */
    virtual double getAdvanceRatio(
        units::angular_velocity::revolutions_per_minute_t rpm) = 0;

    /**
     * @brief Get the driving engines of the propeller.
     *
     * @return The driving engines of the propeller.
     */
    virtual const QVector<IShipEngine*> getDrivingEngines() const = 0;

    virtual units::angular_velocity::revolutions_per_minute_t
    getRPMFromAdvanceRatioAndMaxShipSpeed(double advanceRatio) = 0;

    virtual units::angular_velocity::revolutions_per_minute_t
    getRPMFromAdvanceRatioAndShipSpeed(
        double advanceRatio, units::velocity::meters_per_second_t speed) = 0;

    virtual double getOptimumJ(units::velocity::meters_per_second_t speed) = 0;

    virtual units::angular_velocity::revolutions_per_minute_t
    getOptimumRPM(units::velocity::meters_per_second_t speed) = 0;

protected:
    Ship *mHost;            /**< The ship associated with the propeller. */
    IShipGearBox* mGearBox; /**< The gearbox connected to the propeller. */
    units::length::meter_t
        mPropellerDiameter; /**< The diameter of the propeller. */
    units::length::meter_t
        mPropellerPitch;    /**< The pitch of the propeller. */
    units::area::square_meter_t
        mExpandedBladeArea; /**< The expanded blade area of the propeller. */
    units::area::square_meter_t
        mPropellerDiskArea; /**< The disk area of the propeller. */
    int mNumberOfblades; /**< Number of blades in the propeller. */

    /**< The expanded area ratio of the propeller. */
    double
        mPropellerExpandedAreaRatio;
};
};
#endif //I_SHIPPROPELLER_H
