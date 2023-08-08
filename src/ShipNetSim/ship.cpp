#include "ship.h"
#include <cmath>

Ship::Ship()
{

}

units::area::square_meter_t Ship::get_wet_surface_area_to_holtrop()
{
    return (
        units::area::square_meter_t(
            this->L
            * (2.0 * this->T + this->B)
            * sqrt(this->C_M)
            * (
                0.453
                + 0.4425 * this->C_B
                - 0.2862 * this->C_M
                - 0.003467 * this->B / this->T
                + 0.3696 * this->C_WP
                )
        + 2.38 * this->A_BT / this->C_B
                                    )
        );
}

units::area::square_meter_t Ship::get_wet_surface_area_to_schenzle()
{
    double B = this->C_WP * this->B.value() / this->T.value();
    double C = this->L.value() / this->B.value() / this->C_M;
    double A1 = (1.0 + B / 2.0 - sqrt(1.0 + B * B / 4.0)) * 2.0 / B;
    double A2 = 1.0 + C - sqrt(1.0 + C * C);
    double CN1 = 0.8 + 0.2 * B;
    double CN2 = 1.15 + 0.2833 * C;
    double CPX = this->C_B / this->C_M;
    double CPZ = this->C_B / this->C_WP;
    double C1 = 1.0 - A1 * sqrt(1.0 - pow(2.0 * CPZ - 1.0, CN1));
    double C2 = 1.0 - A2 * sqrt(1.0 - pow(2.0 * CPX - 1.0, CN2));
    return (2.0 + C1 * B + 2.0 * C2 / C) * this->L * this->T;
}


