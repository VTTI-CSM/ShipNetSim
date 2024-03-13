/**
 * @file IShipResistance.h
 * @brief This file defines the IShipResistancePropulsionStrategy
 * interface, which provides methods for calculating different
 * types of resistance experienced by a ship, including frictional,
 * appendage, wave, and air resistance, as well as the total resistance.
 *
 * This interface allows for the implementation of different
 * resistance prediction methods, and includes functions to
 * calculate the speed of advance, hull efficiency, and propeller
 * rotation efficiency of the ship.
 *
 * @author Ahmed Aredah
 * @date 09/12/2023
 */

#ifndef ISHIPRESISTANCE_H
#define ISHIPRESISTANCE_H

#include "../../third_party/units/units.h"


class Ship;  // Forward declaration of the class ship

/**
 * @brief The IShipResistanceStrategy class
 * @class IShipResistanceStrategy
 *
 * Provides an interface to manage multiple resistance prediction methods
 */
class IShipCalmResistanceStrategy
{

public:
    virtual ~IShipCalmResistanceStrategy() = default;

    /**
     * @brief Calculates the frictional resistance of the ship.
     *
     * This function determines the resistance caused by the viscous drag
     * between the ship's hull and the surrounding fluid.
     *
     * @param ship A constant reference to the Ship object.
     * @return Frictional resistance in kilonewtons.
     */
    virtual units::force::newton_t
    getfrictionalResistance(
        const Ship &ship,
        units::velocity::meters_per_second_t customSpeed =
        units::velocity::meters_per_second_t(
            std::nan("Unintialized"))) = 0;

    /**
     * @brief Calculates the appendage resistance of the ship.
     *
     * Appendages, like rudders and shafts, introduce additional
     * resistance to a ship's motion. This function calculates the
     * total resistance caused by all appendages on the ship.
     *
     * @param ship A constant reference to the Ship object.
     * @return Appendage resistance in kilonewtons.
     */
    virtual units::force::newton_t
    getAppendageResistance(
        const Ship &ship,
        units::velocity::meters_per_second_t customSpeed =
        units::velocity::meters_per_second_t(
            std::nan("Unintialized"))) = 0;

    /**
     * @brief Calculates the wave resistance of the ship.
     *
     * As a ship moves through water, it creates waves that result
     * in additional resistance. This function computes the resistance
     * caused by these waves.
     *
     * @param ship A constant reference to the Ship object.
     * @return Wave resistance in kilonewtons.
     */
    virtual units::force::newton_t
    getWaveResistance(
        const Ship &ship,
        units::velocity::meters_per_second_t customSpeed =
        units::velocity::meters_per_second_t(
            std::nan("Unintialized"))) = 0;


    /**
     * @brief Calculates the bulbous bow resistance of the ship.
     *
     * Ships with a bulbous bow design may experience additional
     * resistance as the bulb interacts with waves.
     * This function determines the resistance arising from
     * this interaction.
     *
     * @param ship A constant reference to the Ship object.
     * @return Bulbous bow resistance in kilonewtons.
     */
    virtual units::force::newton_t
    getBulbousBowResistance(
        const Ship &ship,
        units::velocity::meters_per_second_t customSpeed =
        units::velocity::meters_per_second_t(
            std::nan("Unintialized"))) = 0;

    /**
     * @brief Calculates the resistance due to pressure changes
     * at the ship's immersed transom.
     *
     * The ship's immersed transom can lead to abrupt changes in
     * water pressure, which can introduce additional resistance.
     * This function calculates this resistance component.
     *
     * @param ship A constant reference to the Ship object.
     * @return Resistance due to immersed transom pressure in kilonewtons.
     */
    virtual units::force::newton_t
    getImmersedTransomPressureResistance(
        const Ship &ship,
        units::velocity::meters_per_second_t customSpeed =
        units::velocity::meters_per_second_t(
            std::nan("Unintialized"))) = 0;

    /**
     * @brief Calculates the resistance due to correlations between
     * model ship tests and actual ship performance.
     *
     * Empirical correlations from model ship testing can be used
     * to predict the performance of actual ships.
     * This function provides an estimate of the resistance based
     * on these correlations.
     *
     * @param ship A constant reference to the Ship object.
     * @return Model ship correlation resistance in kilonewtons.
     */
    virtual units::force::newton_t
    getModelShipCorrelationResistance(
        const Ship &ship,
        units::velocity::meters_per_second_t customSpeed =
        units::velocity::meters_per_second_t(
            std::nan("Unintialized"))) = 0;

    /**
     * @brief Calculates the air resistance experienced by the ship.
     *
     * While the majority of a ship's resistance comes from water,
     * some resistance can arise from the interaction of the ship's
     * superstructure with the air.
     * This function calculates this air resistance component.
     *
     * @param ship A constant reference to the Ship object.
     * @return Air resistance in kilonewtons.
     */
    virtual units::force::newton_t
    getAirResistance(
        const Ship &ship,
        units::velocity::meters_per_second_t customSpeed =
        units::velocity::meters_per_second_t(
            std::nan("Unintialized"))) = 0;

    /**
     * @brief Calculates the total resistance of the ship.
     *
     * This function aggregates all the individual resistance
     * components to provide a comprehensive estimate of the
     * total resistance experienced by the ship.
     *
     * @param ship A constant reference to the Ship object.
     * @return Total resistance in kilonewtons.
     */
    virtual units::force::newton_t
    getTotalResistance(
        const Ship &ship,
        units::velocity::meters_per_second_t customSpeed =
        units::velocity::meters_per_second_t(
            std::nan("Unintialized"))) = 0;

    /**
     * @brief Calculates the speed of advance of the ship.
     *
     * The speed of advance is the effective speed of the ship through water,
     * accounting for factors like current and wind.
     *
     * @param ship A constant reference to the Ship object.
     * @param customSpeed Optional custom speed parameter.
     * @return The speed of advance in meters per second.
     */
    virtual units::velocity::meters_per_second_t
    calc_SpeedOfAdvance(
        const Ship &ship,
        units::velocity::meters_per_second_t customSpeed =
        units::velocity::meters_per_second_t(
            std::nan("Unintialized"))) = 0;

    /**
     * @brief Get the coefficient of total resistance
     * @param ship A constant reference to the Ship object.
     * @param customSpeed Optional custom speed parameter.
     * @return The coefficient of resistance of the ship at the current speed.
     */
    virtual double getCoefficientOfResistance(
        const Ship& ship,
        units::velocity::meters_per_second_t customSpeed =
        units::velocity::meters_per_second_t(
            std::nan("Unintialized"))) = 0;

    /**
     * @brief Calculates the hull efficiency of the ship.
     *
     * Hull efficiency is a measure of how effectively the ship's hull
     * moves through water,
     * with higher values indicating better performance.
     *
     * @param ship A constant reference to the Ship object.
     * @return The hull efficiency, a dimensionless ratio.
     */
    virtual double getHullEffeciency(const Ship &ship) = 0;

    /**
     * @brief Calculates the propeller rotation efficiency of the ship.
     *
     * Propeller rotation efficiency is a measure of how effectively
     * the ship's propeller converts rotational energy into forward
     * motion, with higher values indicating better performance.
     *
     * @param ship A constant reference to the Ship object.
     * @return The propeller rotation efficiency, a dimensionless ratio.
     */
    virtual double getPropellerRotationEfficiency(const Ship &ship) = 0;

    /**
     * @brief Gets the thrust deduction fraction due to the hull interaction.
     * @param ship A constant reference to the Ship object.
     * @return The thrust deduction fraction, a dimensionless ratio.
     */
    virtual double getThrustDeductionFraction(const Ship &ship) = 0;


    virtual double calc_F_n_i(
        const Ship &ship,
        units::velocity::meters_per_second_t customSpeed =
        units::velocity::meters_per_second_t(
            std::nan("Unintialized"))) = 0;
    /**
     * @brief Retrieves the name of the resistance prediction method.
     *
     * This function returns the name of the resistance prediction
     * method being used, in this case, "Holtrop and Mennen".
     *
     * @return A string containing the name of the resistance
     * prediction method.
     */
    virtual std::string getMethodName() = 0;
};

#endif // ISHIPRESISTANCE_H
