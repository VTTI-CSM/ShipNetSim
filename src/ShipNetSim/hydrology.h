#ifndef HYDROLOGY_H
#define HYDROLOGY_H

#include "../third_party/units/units.h"

using namespace units;

namespace {


static constexpr units::acceleration::meters_per_second_squared_t g =
    units::acceleration::meters_per_second_squared_t(9.81);

static constexpr units::velocity::meters_per_second_t nue =
    units::velocity::meters_per_second_t(1.1883e-6);

static constexpr units::density::kilograms_per_cubic_meter_t rho =
    units::density::kilograms_per_cubic_meter_t(1025);

inline units::velocity::meters_per_second_t get_nue(
    double salin, units::temperature::celsius_t temp)
{
    return units::velocity::meters_per_second_t(
        1.0e-6 *
        (0.014 * salin +
                  (0.000645 * temp.value() - 0.0503)
                                      * temp.value() + 1.75)
        );
}

inline double F_n(units::velocity::meters_per_second_t ship_speed,
                  units::length::meter_t ship_length)
{
    return ship_speed / units::math::sqrt(ship_length * g);
}

inline double R_n(units::velocity::meters_per_second_t ship_speed,
                  units::length::meter_t ship_length)
{
    return (ship_speed.value() * ship_length.value()) / nue.value();
}

inline double C_F(units::velocity::meters_per_second_t ship_speed,
                  units::length::meter_t ship_length)
{
    double dom_value = log10(
                           R_n(ship_speed, ship_length)) - 2.0;

    return 0.075 / (dom_value * dom_value);
}

//inline units::force::kilonewton_t R_F(
//    units::velocity::meters_per_second_t ship_speed,
//    units::length::meter_t ship_length,
//    units::area::square_meter_t ship_wet_surface_area)
//{
//    return C_F(ship_speed, ship_length) * 0.5 *
//           rho *
//           (ship_speed * ship_speed)*
//           ship_wet_surface_area;
//}
}
#endif // HYDROLOGY_H
