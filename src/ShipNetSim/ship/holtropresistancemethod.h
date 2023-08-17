/**
 * @brief Defines the HoltropResistanceMethod class for calculating
 * ship resistance.
 *
 * This file contains the HoltropResistanceMethod class that
 * implements various resistance calculations for ships based on
 * the Holtrop and Mennen resistance prediction model.
 *
 * @author Ahmed Aredah
 * @date 8/15/2023
 */

#ifndef RESISTANCESTRATEGY_H
#define RESISTANCESTRATEGY_H

#include "ishipresistancestrategy.h"


/**
 * @class HoltropResistanceMethod
 * @brief Implements the Holtrop and Mennen resistance prediction
 * model for ships.
 *
 * Provides a concrete strategy for calculating various resistance
 * components of a ship using the Holtrop and Mennen method.
 * It derives from the IShipResistanceStrategy interface.
 *
 * The methods provide calculations for frictional resistance,
 * appendage resistance, wave resistance, bulbous bow resistance,
 * and other resistance types.
 */
class HoltropResistanceMethod : public IShipResistanceStrategy
{
public:

    /**
     * @copydoc IShipResistanceStrategy::getfrictionalResistance()
     */
    units::force::kilonewton_t
    getfrictionalResistance(const Ship &ship) override;

    /**
     * @copydoc IShipResistanceStrategy::getAppendageResistance()
     */
    units::force::kilonewton_t
    getAppendageResistance(const Ship &ship) override;

    /**
     * @copydoc IShipResistanceStrategy::getWaveResistance()
     */
    units::force::kilonewton_t
    getWaveResistance(const Ship &ship) override;

    /**
     * @copydoc IShipResistanceStrategy::getBulbousBowResistance()
     */
    units::force::kilonewton_t
    getBulbousBowResistance(const Ship &ship) override;

    /**
     * @copydoc IShipResistanceStrategy::getImmersedTransomPressureResistance()
     */
    units::force::kilonewton_t
    getImmersedTransomPressureResistance(const Ship &ship) override;

    /**
     * @copydoc IShipResistanceStrategy::getModelShipCorrelationResistance()
     */
    units::force::kilonewton_t
    getModelShipCorrelationResistance(const Ship &ship) override;

    /**
     * @copydoc IShipResistanceStrategy::getAirResistance()
     */
    units::force::kilonewton_t
    getAirResistance(const Ship &ship) override;

    /**
     * @copydoc IShipResistanceStrategy::getTotalResistance()
     */
    units::force::kilonewton_t
    getTotalResistance(const Ship &ship) override;

    /**
     * @copydoc IShipResistanceStrategy::getMethodName()
     */
    std::string getMethodName() override;


private:

    /**
     * @brief Computes the c_7 coefficient.
     * @param ship Ship object reference.
     * @return c_7 value.
     */
    double calc_c_7(const Ship &ship);

    /**
     * @brief Computes the c_1 coefficient.
     * @param ship Ship object reference.
     * @return c_1 value.
     */
    double calc_c_1(const Ship &ship);

    /**
     * @brief Computes the c_3 coefficient.
     * @param ship Ship object reference.
     * @return c_3 value.
     */
    double calc_c_3(const Ship &ship);

    /**
     * @brief Computes the c_2 coefficient.
     * @param ship Ship object reference.
     * @return c_2 value.
     */
    double calc_c_2(const Ship &ship);

    /**
     * @brief Computes the c_5 coefficient.
     * @param ship Ship object reference.
     * @return c_5 value.
     */
    double calc_c_5(const Ship &ship);

    /**
     * @brief Computes the c_15 coefficient.
     * @param ship Ship object reference.
     * @return c_15 value.
     */
    double calc_c_15(const Ship &ship);

    /**
     * @brief Computes the c_16 coefficient.
     * @param ship Ship object reference.
     * @return c_16 value.
     */
    double calc_c_16(const Ship &ship);

    /**
     * @brief Computes the lambda value.
     * @param ship Ship object reference.
     * @return lambda value.
     */
    double calc_lambda(const Ship &ship);

    /**
     * @brief Computes the m_1 value.
     * @param ship Ship object reference.
     * @return m_1 value.
     */
    double calc_m_1(const Ship &ship);
    /**
     * @brief Computes the m_4 value.
     * @param ship Ship object reference.
     * @return m_4 value.
     */
    double calc_m_4(const Ship &ship);

    /**
     * @brief Computes the c_17 coefficient.
     * @param ship Ship object reference.
     * @return c_17 value.
     */
    double calc_c_17(const Ship &ship);

    /**
     * @brief Computes the m_3 value.
     * @param ship Ship object reference.
     * @return m_3 value.
     */
    double calc_m_3(const Ship &ship);

    /**
     * @brief Computes the c_14 coefficient.
     * @param ship Ship object reference.
     * @return c_14 value.
     */
    double calc_c_14(const Ship &ship);

    /**
     * @brief Computes the k_1 value.
     * @param ship Ship object reference.
     * @return k_1 value.
     */
    double calc_k_1(const Ship &ship);

    /**
     * @brief Computes the c_4 coefficient.
     * @param ship Ship object reference.
     * @return c_4 value.
     */
    double calc_c_4(const Ship &ship);

    /**
     * @brief Computes the delta c_A value.
     * @param ship Ship object reference.
     * @return delta c_A value.
     */
    double calc_delta_c_A(const Ship &ship);

    /**
     * @brief Computes the c_A coefficient.
     * @param ship Ship object reference.
     * @return c_A value.
     */
    double calc_c_A(const Ship &ship);

    /**
     * @brief Computes the F_n_T value.
     * @param ship Ship object reference.
     * @return F_n_T value.
     */
    double calc_F_n_T(const Ship &ship);

    /**
     * @brief Computes the c_6 coefficient.
     * @param ship Ship object reference.
     * @return c_6 value.
     */
    double calc_c_6(const Ship &ship);

    /**
     * @brief Computes the h_F value.
     * @param ship Ship object reference.
     * @return h_F value in meters.
     */
    units::length::meter_t calc_h_F(const Ship &ship);

    /**
     * @brief Computes the h_W value.
     * @param ship Ship object reference.
     * @return h_W value in meters.
     */
    units::length::meter_t calc_h_W(const Ship &ship);

    /**
     * @brief Computes the F_n_i value.
     * @param ship Ship object reference.
     * @return F_n_i value.
     */
    double calc_F_n_i(const Ship &ship);

    /**
     * @brief Computes the p_B value.
     * @param ship Ship object reference.
     * @return p_B value.
     */
    double calc_p_B(const Ship &ship);

    /**
     * @brief Computes the equivalent appendage form factor.
     * @param ship Ship object reference.
     * @return Equivalent appendage form factor value.
     */
    double calc_equivalentAppendageFormFactor(const Ship &ship);

    /**
     * @brief Computes wave resistance first component.
     * @param ship Ship object reference.
     * @param customSpeed Optional custom speed value.
     * @return Resistance 'a' component.
     */
    units::force::kilonewton_t calc_R_Wa(
        const Ship &ship,
        units::velocity::meters_per_second_t customSpeed =
        units::velocity::meters_per_second_t(std::nan("no value")));

    /**
     * @brief Computes wave resistance 'b' component.
     * @param ship Ship object reference.
     * @param customSpeed Optional custom speed value.
     * @return Resistance 'b' component.
     */
    units::force::kilonewton_t calc_R_Wb(
        const Ship &ship,
        units::velocity::meters_per_second_t customSpeed =
        units::velocity::meters_per_second_t(std::nan("no value")));

    /**
     * @brief Computes the C_F coefficient.
     * @param ship Ship object reference.
     * @return C_F value.
     */
    double calc_C_F(const Ship &ship);


    const double d = -0.9;

};

#endif // RESISTANCESTRATEGY_H
