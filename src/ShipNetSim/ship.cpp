#include "ship.h"
#include <cmath>
#include "hydrology.h"

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

units::area::square_meter_t Ship::get_wet_surface_area(
    WetSurfaceAreaCalculationMethod method)
{
    if (method == WetSurfaceAreaCalculationMethod::Holtrop)
    {
        return get_wet_surface_area_to_holtrop();
    }
    else if (method == WetSurfaceAreaCalculationMethod::Schenzle)
    {
        return get_wet_surface_area_to_schenzle();
    }
    else if (method == WetSurfaceAreaCalculationMethod::Cargo)
    {
        return units::area::square_meter_t(
            (Nab.value() / B.value()) * (1.7 / (C_B - (0.2 * (C_B - 0.65)))) +
            (B.value()/T.value()));
    }
    else if (method == WetSurfaceAreaCalculationMethod::Trawlers)
    {
        return units::area::square_meter_t(
            ((Nab.value() / B.value()) * (1.7/C_B)) +
            ((B.value()/T.value()) * (0.92 + (0.092/C_B))));
    }
    else
    {
        throw ShipException("Wrong method selected!");
    }
}

double Ship::get_C_B(units::velocity::meters_per_second_t &speed,
                     BlockCoefficientMethod method)
{
    double fn = F_n(speed, L);
    if (method == BlockCoefficientMethod::Ayre)
    {
        return (1.06 - 1.68 * fn);
    }
    else if (method == BlockCoefficientMethod::Jensen)
    {
        if (fn > 0.15 && fn < 0.32)
        {
            return (-4.22 + 27.8 * sqrt(fn) - 39.1 * fn + 46.6 * pow(fn, 3.0));
        }
        else
        {
            throw ShipException("Froud number is outside "
                                "the allowable range for Jensen Method");
        }
    }
    else if (method == BlockCoefficientMethod::Schneekluth)
    {
        if (fn > 0.14 && fn < 0.32)
        {
            if (fn > 0.3)
            {
                fn = 0.3;
            }
            double CB = (0.14 / fn) *
                        (((L.value() * B.value()) + 20.0) / 26.0);
            if (CB < 0.48)
            {
                return 0.48;
            }
            else if (CB > 0.85)
            {
                return 0.85;
            }
            else
            {
                return CB;
            }
        }
        else
        {
            throw ShipException("Froud number is outside "
                                "the allowable range for Schneekluth Method");
        }
    }
    else
    {
        throw ShipException("Wrong method selected!");
    }
}

double Ship::get_C_P()
{
    return C_B / C_M;
}

double Ship::get_Nab()
{
    return (L.value() * B.value() * T.value()) * C_B;
}

double Ship::get_C_WP(WaterPlaneCoefficientMethod method)
{
    if (method == WaterPlaneCoefficientMethod::U_Shape)
    {
        return 0.95 * C_P + 0.17 * pow((1.0 - C_P), (1.0 / 3.0));
    }
    else if (method == WaterPlaneCoefficientMethod::Average_Section)
    {
        return (1.0 + 2.0 * C_B) / 3.0;
    }
    else if (method == WaterPlaneCoefficientMethod::V_Section)
    {
        return sqrt(C_B) - 0.025;
    }
    else if (method == WaterPlaneCoefficientMethod::General_Cargo)
    {
        return 0.763 * (C_P + 0.34);
    }
    else if (method == WaterPlaneCoefficientMethod::Container)
    {
        return 3.226 * (C_P - 0.36);
    }
    else
    {
        throw ShipException("Wrong method selected!");
    }

}



