#include "langmaomethod.h"
#include "hydrology.h"
#include "ship.h"

LangMaoMethod::LangMaoMethod() {}

units::force::newton_t LangMaoMethod::getTotalResistance(const Ship &ship)
{
    return getWaveResistance(ship) +
           getWindResistance(ship);
}
units::force::newton_t LangMaoMethod::getWaveResistance(const Ship &ship)
{
    return getWaveReflectionResistance(ship) +
           getWaveMotionResistance(ship);
}

units::force::newton_t LangMaoMethod::getWindResistance(const Ship &ship)
{
    double c_d = getDragCoef();
    auto env = ship.getCurrentEnvironment();

    double relativeWindSpeed = [&]() -> double {

        double heading =
            ship.getCurrentHeading().convert<units::angle::radian>().value();

        return env.windSpeed_Eastward.value() * std::sin(heading) +
               env.windSpeed_Northward.value() * std::cos(heading);

    }();

    return units::force::newton_t(0.5 * c_d * relativeWindSpeed *
                                  ship.getLengthwiseProjectionArea().value());
}

units::force::newton_t LangMaoMethod::
    getWaveReflectionResistance(const Ship &ship)
{

    auto env = ship.getCurrentEnvironment();
    auto water_rho =
        hydrology::get_waterDensity(env.salinity, env.temperature);
    double waveOrientationToShip = 0.0;

    double bf = [&]() -> double {
        double angleValue = ship.getHalfWaterlineEntranceAngle().value();
        double sinSquared = std::pow(std::sin(angleValue * 2.0f), 2);
        return 2.25f * sinSquared;
    }();

    double alpha_T = [&]() -> double {
        double k = 1.0 / env.waveLength.value();
        double ohm = ship.getSpeed().value() *
                   env.waveFrequency.value() / hydrology::G.value();
        double ke =
            k * std::pow(1.0 + ohm * std::cos(waveOrientationToShip), 2);
        double term = -2.0 * ke * ship.getMeanDraft().value();
        return (double)1.0 - std::exp(term);
    }();

    double fn = hydrology::F_n(ship.getSpeed(), ship.getLengthInWaterline());

    double advanceCoef = [&]() {
        double C_u = std::max((double)-310.0 * bf + (double)68.0, (double)10.0);
        return 1.0 + C_u * fn;
    }();

    double term1 =
        env.waveLength.value() / ship.getLengthBetweenPerpendiculars().value();

    return units::force::newton_t((double)0.5 * water_rho.value() *
                                      hydrology::G.value() *
                                      std::pow(env.waveHeight.value(), 2) *
                                      ship.getBeam().value() * bf * alpha_T *
                                      advanceCoef *
                                      ((double)0.19 / ship.getBlockCoef()) *
                                      std::pow(term1, fn - (double)1.11));

}

units::force::newton_t LangMaoMethod::
    getWaveMotionResistance(const Ship &ship)
{
    double fr = hydrology::F_n(ship.getSpeed(), ship.getLengthInWaterline());
    double a1 =  ((double)60.3 * std::pow(ship.getBlockCoef(), (double)1.34) *
                std::pow(((double)1.0)/ship.getBlockCoef(), 1.0 + fr));
    // assume the ship's weight is uniformally distributed.
    // use the value indicated by the IITC.
    double k_yy = 0.25;

    double omega_delta = [&]() -> double {
        double c1 =
            (double)0.4567 * (ship.getBlockCoef()/k_yy) + (double)1.689;
        double term1 =
            std::sqrt(ship.getLengthBetweenPerpendiculars().value() /
                      hydrology::G.value());
        double term2 = std::pow(k_yy, ((double)1.0/c1));

        double dom = 1.09 + std::ceil(k_yy / 0.25) * 0.08;

        if (fr < 0.05)
        {
            return ((term1 * term2 * std::pow(0.05, 0.143)) / dom);
        }
        else
        {
            return ((term1 * term2 * std::pow(fr, 0.143)) / dom);
        }
    }();

    double a2 = [&]() -> double {
        if (fr < 0.12)
        {
            return 0.0072 + 0.24 * fr;
        }
        else
        {
            return std::pow(fr, -1.05 * ship.getBlockCoef() + 2.3) *
                   std::exp((-2.0 - std::ceil(k_yy / 0.25) -
                             std::floor(k_yy / 0.25)) * fr);
        }
    }();

    double b1 = [&]() -> double {
        if (omega_delta < 1.0 && ship.getBlockCoef() < 0.75)
        {
            return ((double)19.77 * (ship.getBlockCoef()/k_yy)
                    - (double)36.39) /
                   (std::ceil(k_yy / 0.25));
        }
        else if (omega_delta < 1.0 && ship.getBlockCoef() >= 0.75)
        {
            return ((11.0) / std::ceil(k_yy / 0.25));
        }
        else if (omega_delta >= 1.0 && ship.getBlockCoef() < 0.75)
        {
            return ((-12.5) / std::ceil(k_yy / 0.25));
        }
        // if (omega_delta >= 1.0 && ship.getBlockCoef() >= 0.75)
        else
        {
            return ((-5.5) / std::ceil(k_yy / 0.25));
        }
    }();

    double d1 = [&]() -> double {
        if (omega_delta < 1.0 && ship.getBlockCoef() < 0.75)
        {
            return 14.0;
        }
        else if (omega_delta < 1.0 && ship.getBlockCoef() >= 0.75)
        {
            return 566.0 *
                   std::pow(ship.getLengthBetweenPerpendiculars().value()
                                        / ship.getBeam().value(), -2.66) *
                   2.0;
        }
        else
        {
            return 566.0 *
                   std::pow(ship.getLengthBetweenPerpendiculars().value()
                                / ship.getBeam().value(), -2.66) *
                   6.0;
        }
    }();

    auto env = ship.getCurrentEnvironment();
    auto water_rho =
        hydrology::get_waterDensity(env.salinity, env.temperature);

    return units::force::newton_t(
               4.0 * water_rho.value() * hydrology::G.value() *
               std::pow(env.waveHeight.value(), 2) *
               (std::pow(ship.getBeam().value(), 2) /
                ship.getLengthBetweenPerpendiculars().value()) *
               std::pow(omega_delta, b1) *
               std::exp((b1/d1) * (1.0 - std::pow(omega_delta, d1))) * a1 * a2);
}


double LangMaoMethod::getDragCoef(units::angle::degree_t angleOfAttach)
{
    (void) angleOfAttach;
    // head portion of the wind only
    return 1.0;
}
