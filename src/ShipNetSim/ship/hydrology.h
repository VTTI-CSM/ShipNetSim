#ifndef HYDROLOGY_H
#define HYDROLOGY_H

#include "../../third_party/units/units.h"
#include <QDebug>


using namespace units;

namespace hydrology{

static constexpr units::acceleration::meters_per_second_squared_t G =
    units::acceleration::meters_per_second_squared_t(9.81);

static constexpr units::velocity::meters_per_second_t NUE =
    units::velocity::meters_per_second_t(1.1883e-6);

static constexpr units::density::kilograms_per_cubic_meter_t WATER_RHO =
    units::density::kilograms_per_cubic_meter_t(1025.0);

static constexpr units::density::kilograms_per_cubic_meter_t AIR_RHO =
    units::density::kilograms_per_cubic_meter_t(1.225);

static constexpr double AIR_DRAG_COEF = (double)0.8;

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
#endif // HYDROLOGY_H
