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
    units::density::kilograms_per_cubic_meter_t(1025);

static constexpr units::density::kilograms_per_cubic_meter_t AIR_RHO =
    units::density::kilograms_per_cubic_meter_t(1.225);

static constexpr double AIR_DRAG_COEF = (double)0.8;

inline units::velocity::meters_per_second_t get_nue(
    double salinity, units::temperature::celsius_t temp)
{
    qDebug() << "Temp is :" << temp.value();
    qDebug() << "Salin is :" << salinity;

    if (salinity < 0.0)
    {
        qDebug() << "salinity must be greater than 0%!";
        return units::velocity::meters_per_second_t(0);
    }
    if (temp.value() < 0.0)
    {
        qDebug() << "temperature must be greater than 0 celcuis!";
        return units::velocity::meters_per_second_t(0);
    }
    return units::velocity::meters_per_second_t(
        (double)0.000001 *
        ((double)0.014 * salinity +
         ((double)0.000645 * temp.value() - (double)0.0503)
             * temp.value() + (double)1.75)
        );
}

inline double F_n(units::velocity::knot_t ship_speed,
                  units::length::meter_t ship_length)
{
    if (ship_speed.value() < 0.0)
    {
        qDebug() << "Ship speed must be greater than 0 knots!";
        return 0.0;
    }
    if (ship_length.value() < 0.0)
    {
        qDebug() << "Ship Length must be greater than 0!";
        return 0.0;
    }
    units::velocity::meters_per_second_t speed =
        ship_speed.convert<velocity::meters_per_second>();
    qDebug() << "Ship Speed is :" << QString::number(speed.value(), 'f', 10);
    qDebug() << "G Value is :" << QString::number(G.value(), 'f', 10);
    qDebug() << "Ship Length is (m): " <<
        QString::number(ship_length.value(), 'f', 10);
    return speed.value() /
           sqrt(ship_length.value() * G.value());
}

inline double R_n(units::velocity::knot_t ship_speed,
                  units::length::meter_t ship_length,
                  double salinity = 0.035,
                  units::temperature::celsius_t temp =
                  units::temperature::celsius_t(15))
{
    if (ship_speed.value() < 0.0)
    {
        qDebug() << "Ship speed must be greater than 0 knots!";
        return 0.0;
    }
    if (ship_length.value() < 0.0)
    {
        qDebug() << "Ship Length must be greater than 0!";
        return 0.0;
    }
    if (salinity < 0.0)
    {
        qDebug() << "salinity must be greater than 0%!";
        return 0.0;
    }
    if (temp.value() < 0.0)
    {
        qDebug() << "temperature must be greater than 0 celcuis!";
        return 0.0;
    }
    units::velocity::meters_per_second_t speed =
        ship_speed.convert<velocity::meters_per_second>();

    qDebug() << "Ship Speed is :" << QString::number(speed.value(), 'f', 10);
    qDebug() << "Ship Length is (m): " <<
        QString::number(ship_length.value(), 'f', 10);
    qDebug() << "nue value is: " <<
        QString::number(get_nue(salinity, temp).value(), 'f', 10);

    return (speed.value() * ship_length.value()) /
           get_nue(salinity, temp).value();
}

inline double C_F(units::velocity::knot_t ship_speed,
                  units::length::meter_t ship_length)
{

    if (ship_speed.value() < 0.0)
    {
        qDebug() << "Ship speed must be greater than 0 knots!";
        return 0.0;
    }
    if (ship_length.value() < 0.0)
    {
        qDebug() << "Ship Length must be greater than 0!";
        return 0.0;
    }

    qDebug() << "Ship Speed is (knots):" <<
        QString::number(ship_speed.value(), 'f', 10);
    qDebug() << "Ship Length is (m): " <<
        QString::number(ship_length.value(), 'f', 10);

    double dom_value = log10(
                           R_n(ship_speed, ship_length)) - 2.0;

    return (double)0.075 / (dom_value * dom_value);
}
}
#endif // HYDROLOGY_H
