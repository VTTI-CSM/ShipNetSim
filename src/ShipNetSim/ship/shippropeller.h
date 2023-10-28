/**
 * @file ShipPropeller.h
 * @brief This file contains the declaration of the ShipPropeller class.
 *
 * The ShipPropeller class is an implementation of the IShipPropeller
 * interface and represents the propeller of a ship. It handles the
 * calculation of various propeller-related parameters, such as
 * thrust, torque, RPM, etc., based on the ship's current state and
 * the gearbox's output.
 *
 * @author Ahmed Aredah
 * @date 10.12.2023
 */
#ifndef SHIPPROPELLER_H
#define SHIPPROPELLER_H

#include "ishipgearbox.h"
#include "ishippropeller.h"

class Ship;  // Forward declaration of the class ship

/**
 * @class ShipPropeller
 * @brief Represents a ship's propeller and its associated properties.
 *
 * The ShipPropeller class implements the IShipPropeller interface and
 * is responsible for calculating various propeller-related parameters
 * such as thrust, torque, RPM, etc. It uses the output from the gearbox
 * and various propeller and shaft efficiencies to calculate these
 * parameters.
 */
class ShipPropeller : public IShipPropeller
{
public:
    // IShipPropeller interface

    /**
     * @brief Initializes the propeller with the given parameters.
     *
     * Initializes the propeller with a reference to the ship it is
     * part of, a reference to the gearbox connected to it, and a
     * map of additional parameters.
     *
     * @param ship A pointer to the ship that the propeller is part of.
     * @param gearbox A pointer to the gearbox connected to the propeller.
     * @param parameters A map of additional parameters for initialization.
     */
    void initialize(Ship *ship, IShipGearBox *gearbox,
                    const QMap<QString, std::any> &parameters) override;

    /**
     * @brief Sets the propeller's parameters.
     *
     * Updates the propeller's parameters based on the values provided
     * in the parameter map.
     *
     * @param parameters A map of parameters to update.
     */
    void setParameters(const QMap<QString, std::any> &parameters) override;

    // Getters and setters for various propeller-related parameters.
    /**
     * @brief Gets the shaft efficiency.
     *
     * Returns the efficiency of the shaft, which is a value
     * between 0 and 1 that represents how efficiently the shaft
     * transmits power to the propeller.
     *
     * @return The shaft efficiency.
     */
    double getShaftEfficiency() override;

    /**
     * @brief Sets the shaft efficiency.
     *
     * Sets the efficiency of the shaft, which should be a value
     * between 0 and 1.
     *
     * @param newShaftEfficiency The new shaft efficiency.
     */
    void setShaftEfficiency(double newShaftEfficiency) override;

    /**
     * @brief Gets the propeller efficiency.
     *
     * Returns the open water efficiency of the propeller,
     * which is a measure of how efficiently the propeller
     * converts rotational energy into thrust.
     *
     * @return The propeller efficiency.
     */
    double getPropellerEfficiency() override;

    /**
     * @brief Sets the propeller open water efficiencies.
     *
     * Sets a map of open water efficiencies for the propeller.
     * The map should contain values representing how efficiently
     * the propeller converts rotational energy into thrust at
     * different speeds.
     *
     * @param newPropellerOpenWaterEfficiencies The new map of
     * open water efficiencies.
     */
    void setPropellerOpenWaterEfficiencies(
        QMap<double, double> newPropellerOpenWaterEfficiencies) override;

    /**
     * @brief Gets the effective power of the propeller.
     *
     * Returns the effective power of the propeller, which is
     * the power actually being used to produce thrust,
     * taking into account various losses and inefficiencies.
     *
     * @return The effective power of the propeller.
     */
    units::power::kilowatt_t getEffectivePower() override;

    /**
     * @brief Gets the previous effective power of the propeller.
     *
     * Returns the effective power of the propeller from the
     * previous time step.
     *
     * @return The previous effective power of the propeller.
     */
    units::power::kilowatt_t getPreviousEffectivePower() override;

    /**
     * @brief Gets the thrust produced by the propeller.
     *
     * Returns the thrust produced by the propeller, which is
     * the force exerted by the propeller in the direction of
     * the ship's movement.
     *
     * @return The thrust produced by the propeller.
     */
    units::force::newton_t getThrust() override;

    /**
     * @brief Gets the revolutions per minute (RPM) of the propeller.
     *
     * Returns the revolutions per minute (RPM) of the propeller,
     * which is a measure of how fast the propeller is spinning.
     *
     * @return The RPM of the propeller.
     */
    units::angular_velocity::revolutions_per_minute_t getRPM() override;

    /**
     * @brief Gets the torque produced by the propeller.
     *
     * Returns the torque produced by the propeller, which is
     * a measure of the rotational force exerted by the propeller.
     *
     * @return The torque produced by the propeller.
     */
    units::torque::newton_meter_t getTorque() override;

    /**
     * @brief Gets the thrust coefficient of the propeller.
     *
     * Returns the thrust coefficient of the propeller, which is
     * a dimensionless number that represents the ratio of the
     * propeller's thrust to the product of the fluid density,
     * the propeller's diameter squared, and the velocity squared.
     *
     * @return The thrust coefficient of the propeller.
     */
    double getThrustCoefficient() override;

    /**
     * @brief Gets the torque coefficient of the propeller.
     *
     * Returns the torque coefficient of the propeller, which is
     * a dimensionless number that represents the ratio of the
     * propeller's torque to the product of the fluid density,
     * the propeller's diameter squared, and the velocity squared.
     *
     * @return The torque coefficient of the propeller.
     */
    double getTorqueCoefficient() override;

    /**
     * @brief Gets the advance ratio of the propeller.
     *
     * Returns the advance ratio of the propeller, which is
     * a dimensionless number that represents the ratio of the
     * ship's speed to the product of the propeller's diameter
     * and its rotational speed.
     *
     * @return The advance ratio of the propeller.
     */
    double getAdvancedRatio() override;

    /**
     * @brief Gets the driving engines of the propeller.
     *
     * Returns a vector containing pointers to the engines
     * that are driving the propeller.
     *
     * @return A vector of pointers to the driving engines.
     */
    const QVector<IShipEngine *> getDrivingEngines() const override;

private:
    double mShaftEfficiency;  ///< Efficiency of the shaft.
    QMap<double, double>
        mPropellerOpenWaterEfficiencyToJ; ///< Map of propeller open water eff.
    units::power::kilowatt_t
        mPreviousEffectivePower;  ///< Previous effective power.

    // Private helper functions for calculating
    // various propeller-related parameters.

    double getOpenWaterEfficiency();
    double getRelativeEfficiency();
    double getHullEfficiency();

};

#endif // SHIPPROPELLER_H
