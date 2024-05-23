#ifndef HYDROLOGY_H
#define HYDROLOGY_H

#include "../../third_party/units/units.h"
#include <QDebug>


using namespace units;

namespace ShipNetSimCore
{
namespace hydrology{

static constexpr units::acceleration::meters_per_second_squared_t G =
    units::acceleration::meters_per_second_squared_t(9.80665);

// static constexpr units::velocity::meters_per_second_t NUE =
//     units::velocity::meters_per_second_t(1.1883e-6);

//!< water density at 15.0 degree c
// static constexpr units::density::kilograms_per_cubic_meter_t WATER_RHO =
//     units::density::kilograms_per_cubic_meter_t(1026.021);

static constexpr units::density::kilograms_per_cubic_meter_t AIR_RHO =
    units::density::kilograms_per_cubic_meter_t(1.225);

static constexpr double AIR_DRAG_COEF = (double)0.8;

inline units::density::kilograms_per_cubic_meter_t
get_waterDensity(units::concentration::pptd_t salinity,
                 units::temperature::celsius_t temperature)
{
    double S = salinity.value();
    double T = temperature.value();

    // Coefficients for EOS-80
    double a0 = 999.842594, a1 = 6.793952e-2, a2 = -9.095290e-3,
        a3 = 1.001685e-4, a4 = -1.120083e-6, a5 = 6.536332e-9;

    double b0 = 8.24493e-1, b1 = -4.0899e-3, b2 = 7.6438e-5,
        b3 = -8.2467e-7, b4 = 5.3875e-9;

    double c0 = -5.72466e-3, c1 = +1.0227e-4, c2 = -1.6546e-6;

    double d0 = 4.8314e-4;

    // Density of pure water at atmospheric pressure
    double rho0 = a0 + (a1 * T) + (a2 * std::pow(T, 2)) +
                  (a3 * std::pow(T, 3)) + (a4 * std::pow(T, 4)) +
                  (a5 * std::pow(T, 5));

    // Corrections for salinity
    double rhoS = rho0 + (b0 * S) + (b1 * std::pow(S, 1.5)) +
                  (b2 * std::pow(S, 2)) + (b3 * std::pow(S, 2.5)) +
                  (b4 * std::pow(S, 3)) + (c0 * T * S) +
                  (c1 * std::pow(T, 2) * S) + (c2 * std::pow(T, 3) * S) +
                  (d0 * std::pow(S, 2));

    return units::density::kilograms_per_cubic_meter_t(rhoS);
}

inline units::velocity::meters_per_second_t get_nue(
    double salinity, units::temperature::celsius_t temp)
{

    if (salinity < 0.0)
    {
        qCritical() << "salinity must be greater than 0%!";
        return units::velocity::meters_per_second_t(0);
    }
    if (salinity > 1.0)
    {
        qCritical() << "salinity must be less than 100%!";
        return units::velocity::meters_per_second_t(0);
    }
    if (temp.value() < 0.0)
    {
        qCritical() << "temperature must be greater than 0 celcuis!";
        return units::velocity::meters_per_second_t(0);
    }
    return units::velocity::meters_per_second_t(
        (double)0.000001 *
        ((double)0.014 * salinity +
         ((double)0.000645 * temp.value() - (double)0.0503)
             * temp.value() + (double)1.75)
        );
}

inline double F_n(units::velocity::meters_per_second_t ship_speed,
                  units::length::meter_t ship_length)
{
    if (ship_speed.value() < 0.0)
    {

        qCritical() << "Ship speed must be greater than 0 knots!";
        return 0.0;
    }
    if (ship_length.value() < 0.0)
    {
        qCritical() << "Ship Length must be greater than 0!";
        return 0.0;
    }

    return ship_speed.value() /
           sqrt(ship_length.value() * G.value());
}

inline double R_n(units::velocity::meters_per_second_t ship_speed,
                  units::length::meter_t ship_length,
                  double salinity = 0.035,
                  units::temperature::celsius_t temp =
                  units::temperature::celsius_t(15))
{
    if (ship_speed.value() < 0.0)
    {
        qCritical() << "Ship speed must be greater than 0 knots!";
        return 0.0;
    }
    if (ship_length.value() < 0.0)
    {
        qCritical() << "Ship Length must be greater than 0!";
        return 0.0;
    }
    if (salinity < 0.0)
    {
        qCritical() << "salinity must be greater than 0%!";
        return 0.0;
    }
    if (temp.value() < 0.0)
    {
        qCritical() << "temperature must be greater than 0 celcuis!";
        return 0.0;
    }

    return (ship_speed.value() * ship_length.value()) /
           get_nue(salinity, temp).value();
}

}
};
#endif // HYDROLOGY_H
