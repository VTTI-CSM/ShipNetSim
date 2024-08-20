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

#include "../export.h"

#include "ishipgearbox.h"
#include "ishippropeller.h"

namespace ShipNetSimCore
{
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
class SHIPNETSIM_EXPORT ShipPropeller : public IShipPropeller
{
public:
    // IShipPropeller interface

    // ~ShipPropeller();

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

    units::power::kilowatt_t getShaftPower();

    units::force::newton_t getShaftThrust();

    units::torque::newton_meter_t getShaftTorque();


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
    double getThrustCoefficient(
        units::angular_velocity::revolutions_per_minute_t rpm) override;

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
    double getTorqueCoefficient(
        units::angular_velocity::revolutions_per_minute_t rpm) override;

    /**
     * @brief Gets the theoritical ideal advance speed (V_a) of the ship.
     *
     * Returns the ideal advance speed of the ship considering
     * the propeller pitch and the number of revolutions when
     * there is no slipping.
     * The speed of advance is in m/s.
     *
     * @return the speed of advance in m/s.
     */
    units::velocity::meters_per_second_t getIdealAdvanceSpeed();

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
    double getAdvanceRatio(
        units::angular_velocity::revolutions_per_minute_t rpm) override;

    /**
     * @brief Gets the propeller slip value.
     *
     * Returns the slip of the propeller due to the difference between
     * the theoritical ideal speed of advance and the actual speed
     * of advance. the slip is dimensionless between [0, 1].
     *
     * @return The propeller slip.
     */
    double getPropellerSlip();

    /**
     * @brief Gets the driving engines of the propeller.
     *
     * Returns a vector containing pointers to the engines
     * that are driving the propeller.
     *
     * @return A vector of pointers to the driving engines.
     */
    const QVector<IShipEngine *> getDrivingEngines() const override;

    units::angular_velocity::revolutions_per_minute_t
    getRPMFromAdvanceRatioAndMaxShipSpeed(double advanceRatio) override;

    units::angular_velocity::revolutions_per_minute_t
    getRPMFromAdvanceRatioAndShipSpeed(
        double advanceRatio,
        units::velocity::meters_per_second_t speed) override;

    double getOptimumJ(units::velocity::meters_per_second_t speed) override;

    units::angular_velocity::revolutions_per_minute_t
    getOptimumRPM(units::velocity::meters_per_second_t speed) override;
private:
    struct KCoef
    {
    public: enum KType
        {
            Thrust,
            Torque
        };
        QVector<double> C;
        QVector<int> s;
        QVector<int> t;
        QVector<int> u;
        QVector<int> v;
        KType type;

        bool checkInputs(double PD, double AreaRatio, int Z);

        double getResult(double J, double PD,
                         double AreaRatio, double Z, double Rn);
    };

    KCoef KT = KCoef();
    KCoef KQ = KCoef();

    double mShaftEfficiency;  ///< Efficiency of the shaft.
    units::power::kilowatt_t
        mPreviousEffectivePower;  ///< Previous effective power.

    double mLastBestJ = 0.8; // Default starting point if no bestJ known

    // Private helper functions for calculating
    // various propeller-related parameters.

    double getOpenWaterEfficiency(double J = std::nan("undefined"),
                                  double KT = std::nan("undefined"),
                                  double KQ = std::nan("undefined"));
    double getRelativeEfficiency();
    double getHullEfficiency();

};
};
#endif // SHIPPROPELLER_H
