#include "HoltropMethod.h"
#include "hydrology.h"
#include "ship.h"


static const QMap<Ship::ShipAppendage, double>& get_k_2_iMap() {
    static QMap<Ship::ShipAppendage, double> k_2_iMap = {
        {Ship::ShipAppendage::rudder_behind_skeg, (double)0.5},
        {Ship::ShipAppendage::rudder_behind_stern, (double)0.5},
        {Ship::ShipAppendage::slender_twin_screw_rudder, (double)1.5},
        {Ship::ShipAppendage::thick_twin_screw_rudder, (double)2.5},
        {Ship::ShipAppendage::shaft_brackets, (double)4.0},
        {Ship::ShipAppendage::skeg, (double)1.0},
        {Ship::ShipAppendage::strut_bossing, (double)3.0},
        {Ship::ShipAppendage::hull_bossing, (double)1.0},
        {Ship::ShipAppendage::
         exposed_shafts_10_degree_with_buttocks, (double)1.0},
        {Ship::ShipAppendage::
         exposed_shafts_20_degree_with_buttocks, (double)4.0},
        {Ship::ShipAppendage::stabilizer_fins, (double)1.8},
        {Ship::ShipAppendage::dome, (double)1.7},
        {Ship::ShipAppendage::bilge_keels, (double)0.4}
    };
    return k_2_iMap;
}

static const QMap<Ship::CStern, int>& get_C_SternMap() {
    static QMap<Ship::CStern, int> C_SternMap = {
        {Ship::CStern::pramWithGondola, (int)-25},
        {Ship::CStern::VShapedSections, (int)-10},
        {Ship::CStern::NormalSections, (int)0},
        {Ship::CStern::UShapedSections, (int)10}
    };
    return C_SternMap;
}

double HoltropMethod::calc_c_7(const Ship &ship)
{
    if ((ship.getBeam() / ship.getLengthInWaterline()) < 0.11)
    {
        return (double)0.229577 *
               pow(
                   (ship.getBeam() /
                    ship.getLengthInWaterline()),
                   (double)0.33333
                   );
    }
    else if ((ship.getBeam() /
              ship.getLengthInWaterline()).value() < (double)0.25)
    {
        return (ship.getBeam() / ship.getLengthInWaterline()).value();
    }
    else
    {
        return (
            (double)0.5 -
            (
                (double)0.0625 *
                (ship.getLengthInWaterline()
                 ) /
                ship.getBeam()
                )
            );
    }
}


double HoltropMethod::calc_c_1(const Ship &ship)
{
    return (
        (double)2223105.0
        * pow(get_C7(ship), (double)3.78613)
        * pow((ship.getMeanDraft() / ship.getBeam()), (double)1.07961)
        * pow(((double)90.0 -
               ship.getHalfWaterlineEntranceAngle().value()),
              (double)-1.37565)
        );
}

double HoltropMethod::calc_c_3(const Ship &ship)
{
    return (
        (double)0.56
        * pow(ship.getBulbousBowTransverseArea().value(), (double)1.5)
        / (
            ship.getBeam().value()
            * ship.getMeanDraft().value()
            * (
                (double)0.31 *
                    sqrt(ship.getBulbousBowTransverseArea().value()) +
                ship.getDraftAtForward().value() -
                ship.getBulbousBowTransverseAreaCenterHeight().value()
                )
            )
        );
}

double HoltropMethod::calc_c_2(const Ship &ship)
{
    return exp((double)-1.89 * sqrt(get_C3(ship)));
}

double HoltropMethod::calc_c_5(const Ship &ship)
{
    return (
        (double)1.0 -
        (double)0.8 * (
            ship.getImmersedTransomArea().value() /
            (
                ship.getBeam().value() *
                ship.getMeanDraft().value()
                * ship.getMidshipSectionCoef()
                )
            )
        );
}

double HoltropMethod::calc_c_15(const Ship &ship)
{
    if ((pow(ship.getLengthInWaterline().value(), (double)3.0) /
         ship.getVolumetricDisplacement().value()) < (double)512.0)
    {

        return (double)-1.69385;
    }
    else if ((pow(ship.getLengthInWaterline().value(), (double)3.0) /
              ship.getVolumetricDisplacement().value()) < (double)1726.91)
    {
        return (
            ((double)-1.69385) +
            (
                (
                    (
                        ship.getLengthInWaterline().value() /
                        pow(ship.getVolumetricDisplacement().value(),
                            ((double)1.0 / (double)3.0))
                        )
                    - (double)8.0) / (double)2.36
                )
            );
    }
    else
    {
        return (double)0.0;
    }
}

double HoltropMethod::calc_c_16(const Ship &ship)
{
    if (ship.getPrismaticCoef() < (double)0.8)
    {
        return (
            (double)8.07981 *
                ship.getPrismaticCoef()
            - (double)13.8673 * pow(ship.getPrismaticCoef(), (double)2.0)
            + (double)6.984388 * pow(ship.getPrismaticCoef(), (double)3.0)
            );
    }
    else{
        return (double)1.73014 - (double)0.7067 * ship.getPrismaticCoef();
    }
}

double HoltropMethod::calc_lambda(const Ship &ship)
{
    if ((ship.getLengthInWaterline() / ship.getBeam()).value() < 12.0)
    {
        return ((double)1.446 * ship.getPrismaticCoef() -
                (double)0.03 *
                    (ship.getLengthInWaterline() /
                     ship.getBeam())).value();
    }

    else{
        return ((double)1.446 * ship.getPrismaticCoef() - (double)0.36);
    }
}

double HoltropMethod::calc_m_1(const Ship &ship)
{
    return (
        (double)0.0140407 *
            (ship.getLengthInWaterline() / ship.getMeanDraft()).value()
        - (double)1.75254 *
              pow(ship.getVolumetricDisplacement().value(),
                  ((double)1.0 / (double)3.0)) /
              ship.getLengthInWaterline().value()
        - (double)4.79323 *
              (ship.getBeam() / ship.getLengthInWaterline()).value()
        - get_C16(ship)
        );
}

double HoltropMethod::calc_m_4(
    const Ship &ship,
    units::velocity::meters_per_second_t customSpeed)
{
    double fn = 0.0;
    if (std::isnan(customSpeed.value()))
    {
        fn = hydrology::F_n(ship.getSpeed(),
                            ship.getLengthInWaterline());
    }
    else
    {
        fn = hydrology::F_n(customSpeed,
                            ship.getLengthInWaterline());
    }
    return (
        get_C15(ship)
        * (double)0.4
        * exp((double)-0.034 * pow(fn, (double)-3.29))
        );
}

double HoltropMethod::calc_m_3(const Ship &ship)
{
    return (
        (double)-7.2035
        * pow((ship.getBeam() /
               ship.getLengthInWaterline()).value(), (double)0.326869)
        * pow((ship.getMeanDraft() /
               ship.getBeam()).value(), (double)0.605375)
        );
}

double HoltropMethod::calc_c_17(const Ship &ship)
{
    return (
        (double)6919.3
        * pow(ship.getMidshipSectionCoef(), (double)-1.3346)
        * pow((ship.getVolumetricDisplacement().value() /
               pow(ship.getLengthInWaterline().value(),
                   (double)3.0)),
              (double)2.00977)
        * pow((ship.getLengthInWaterline() /
               ship.getBeam()) - (double)2.0,
              (double)1.40692)
        );
}

units::force::newton_t HoltropMethod::calc_R_Wa(
    const Ship &ship,
    units::velocity::meters_per_second_t customSpeed)
{
    double fn = (double)0.0;

    if (std::isnan(customSpeed.value())) {
        fn = hydrology::F_n(ship.getSpeed(), ship.getLengthInWaterline());
    }
    else {
        fn = hydrology::F_n(customSpeed, ship.getLengthInWaterline());
    }
    return units::force::newton_t(
        get_C1(ship) *
        get_C2(ship) *
        get_C5(ship) *
        hydrology::WATER_RHO *
        hydrology::G *
        ship.getVolumetricDisplacement() *
        exp(
            (
                (
                    get_m1(ship) *
                    pow(fn, d)
                    ) +
                (
                    calc_m_4(ship, customSpeed) *
                    cos(get_lambda(ship) *
                        pow(fn, (double)-2.0)
                        )
                    )
                )
            )
        );
}

units::force::newton_t HoltropMethod::calc_R_Wb(
    const Ship &ship,
    units::velocity::meters_per_second_t customSpeed)
{
    double fn = (double)0.0;

    if (std::isnan(customSpeed.value())) {
        fn = hydrology::F_n(ship.getSpeed(), ship.getLengthInWaterline());
    }
    else {
        fn = hydrology::F_n(customSpeed, ship.getLengthInWaterline());
    }

    return units::force::newton_t(
        get_C1(ship) *
        get_C2(ship) *
        get_C5(ship) *
        hydrology::WATER_RHO *
        hydrology::G *
        ship.getVolumetricDisplacement() *
        exp(get_m3(ship) *
                pow(fn, d) +
            calc_m_4(ship) *
                cos(get_lambda(ship) *
                    pow(fn, -2.0)))
        );
}

double HoltropMethod::calc_C_F(
    const Ship &ship,
    units::velocity::meters_per_second_t customSpeed)
{
    double Rn = 0.0;
    if (std::isnan(customSpeed.value()))
    {
        Rn = hydrology::R_n(ship.getSpeed(),
                            ship.getLengthInWaterline());
    }
    else
    {
        Rn = hydrology::R_n(customSpeed,
                            ship.getLengthInWaterline());
    }
    return (double)0.075 /
           pow((log10(Rn)
                - (double)2.0), (double)2.0);
}

int getCStenByAftShape(const Ship &ship)
{
    return get_C_SternMap().value(ship.getSternShapeParam());
}

double HoltropMethod::calc_c_14(const Ship &ship)
{
    return (double)1.0 + (double)0.011 * (double)getCStenByAftShape(ship);
}

double HoltropMethod::calc_k_1(const Ship &ship)
{
    return (double)0.93 +
           (double)0.487118 *
               get_C14(ship) *
               pow(ship.getBeam().value() /
                       ship.getLengthInWaterline().value(), (double)1.06806) *
               pow(ship.getMeanDraft().value() /
                       ship.getLengthInWaterline().value(), (double)0.46106) *
               pow(ship.getLengthInWaterline().value() /
                       ship.getRunLength().value(), (double)0.121563) *
               pow(pow(ship.getLengthInWaterline().value(),(double)3.0) /
                       ship.getVolumetricDisplacement().value(),
                   (double)0.36486) *
               pow((double)1.0 - ship.getPrismaticCoef(), (double)-0.604247);
}

double HoltropMethod::calc_c_4(const Ship &ship)
{
    if (ship.getDraftAtForward()/ship.getLengthInWaterline() <= 0.04)
    {
        return (ship.getDraftAtForward() /
                ship.getLengthInWaterline()).value();
    }
    else
    {
        return (double)0.04;
    }
}

double HoltropMethod::calc_delta_c_A(const Ship &ship)
{
    if (ship.getSurfaceRoughness() == units::length::nanometer_t(150.0))
    {
        return (double)0.0;
    }
    else
    {
        auto sR = ship.getSurfaceRoughness().
                  convert<units::length::meter>().value(); //It has to be in m
        return ((double)0.105 *
                    pow(sR,
                        ((double)1.0/(double)3.0)) -
                (double)0.005579) /
               pow(ship.getLengthInWaterline().value(),
                   ((double)1.0/(double)3.0));
    }
}

double HoltropMethod::calc_c_A(const Ship &ship)
{
    return ((double)0.00546
                * pow(ship.getLengthInWaterline().value() +
                          (double)100.0, (double)-0.16)
            - (double)0.002
            + (double)0.003
                  * sqrt((ship.getLengthInWaterline().value()/(double)7.5))
                  * pow(ship.getBlockCoef(), (double)4.0)
                  * get_C2(ship)
                  * ((double)0.04 - get_C4(ship))
            );
}

double HoltropMethod::calc_F_n_T(
    const Ship &ship,
    units::velocity::meters_per_second_t customSpeed)
{
    units::velocity::meters_per_second_t s;

    if (std::isnan(customSpeed.value()))
    {
        s = ship.getSpeed();
    }
    else
    {
        s = customSpeed;
    }

    return (s.value() /
            sqrt(
                (double)2.0 *
                hydrology::G.value() *
                (
                    ship.getDraftAtForward().value() /
                    (
                        ship.getBeam().value() +
                        (
                            ship.getBeam().value() *
                            ship.getWaterplaneAreaCoef()
                            )
                        )
                    )
                )
            );
}

double HoltropMethod::calc_c_6(
    const Ship &ship,
    units::velocity::meters_per_second_t customSpeed)
{
    double FrT = calc_F_n_T(ship, customSpeed);
    if (FrT < 5.0){
        return (double)0.2 * ((double)1.0 - (double)0.2 * FrT);
    }
    else
    {
        return 0.0;
    }
}

units::length::meter_t HoltropMethod::calc_h_F(
    const Ship &ship,
    units::velocity::meters_per_second_t customSpeed)
{
    double fr = 0.0;
    if (std::isnan(customSpeed.value()))
    {
        fr = hydrology::F_n(ship.getSpeed(), ship.getLengthInWaterline());
    }
    else
    {
        fr = hydrology::F_n(customSpeed, ship.getLengthInWaterline());
    }

    units::length::meter_t hf =  ship.getPrismaticCoef() *
                                ship.getMidshipSectionCoef() *
                                ((ship.getBeam() * ship.getMeanDraft()) /
                                 (ship.getLengthInWaterline()))
                                * ((double)136.0 - (double)316.3 * fr) *
                                pow(fr, (double)3.0);
    if (hf < (double)-0.01 * ship.getLengthInWaterline())
    {
        return (double)-0.01 * ship.getLengthInWaterline();
    }

    return hf;
}

units::length::meter_t HoltropMethod::calc_h_W(
    const Ship &ship,
    units::velocity::meters_per_second_t customSpeed)
{
    double s = 0.0L;
    if (std::isnan(customSpeed.value()))
    {
        s = ship.getSpeed().value();
    }
    else
    {
        s = customSpeed.value();
    }

    units::length::meter_t hw = units::length::meter_t(
        ship.getHalfWaterlineEntranceAngle().value() *
        pow(s, (double)2.0) /
        ((double) 400.0 * hydrology::G.value()));

    if (hw > 0.01 * ship.getLengthInWaterline())
    {
        return 0.01 * ship.getLengthInWaterline();
    }
    else{
        return hw;
    }
}

double HoltropMethod::calc_F_n_i(
    const Ship &ship,
    units::velocity::meters_per_second_t customSpeed)
{
    units::velocity::meters_per_second_t s;
    if (std::isnan(customSpeed.value()))
    {
        s = ship.getSpeed();
    }
    else
    {
        s = customSpeed;
    }

    return (s.value() /
            (sqrt(hydrology::G.value() *
                  (ship.getDraftAtForward().value() -
                   ship.getBulbousBowTransverseAreaCenterHeight().value() -
                   ((double)0.25 *
                    sqrt(ship.getBulbousBowTransverseArea().value())) +
                   calc_h_F(ship, s).value() +
                   calc_h_W(ship, s).value()))));
}

double HoltropMethod::calc_p_B(
    const Ship &ship,
    units::velocity::meters_per_second_t customSpeed)
{
    return (double)0.56 *
           (sqrt(ship.getBulbousBowTransverseArea().value()) /
            (ship.getDraftAtForward().value() -
             (double)1.5 *
                 (ship.getBulbousBowTransverseAreaCenterHeight().value()) +
             calc_h_F(ship, customSpeed).value()));
}


double HoltropMethod::calc_c_8(const Ship &ship)
{
    if ((ship.getBeam() / ship.getDraftAtAft()).value() <= (double)5.0)
    {
        return ((ship.getWettedHullSurface() /
                 ship.getLengthInWaterline() * ship.getPropellerDiameter()) *
                (ship.getBeam() / ship.getDraftAtAft())).value();
    }
    else
    {
        return ((ship.getWettedHullSurface() *
                 ((double)7.0 *
                      (ship.getBeam()/ship.getDraftAtAft()) - (double)25.0))
                /
                (ship.getLengthInWaterline() * ship.getPropellerDiameter() *
                 ((ship.getBeam()/ship.getDraftAtAft()) - (double)3.0)));

    }

}

double HoltropMethod::calc_c_9(const Ship &ship)
{
    double c8 = get_C8(ship);
    if (c8 <= (double)28.0)
    {
        return c8;
    }
    else
    {
        return ((double)32.0 - (((double)16.0) / (c8 - (double)24.0)));
    }
}

double HoltropMethod::calc_c_11(const Ship &ship)
{
    if ((ship.getDraftAtAft() /
         ship.getPropellerDiameter()).value() <= (double)2.0)
    {
        return ((ship.getDraftAtAft() / ship.getPropellerDiameter()).value());
    }
    else
    {
        return ((double)0.0833333 *
                    pow((ship.getDraftAtAft() /
                         ship.getPropellerDiameter()).value(), (double)3.0) +
                (double)1.33333);
    }
}

double HoltropMethod::calc_c_19(const Ship &ship)
{
    if (ship.getPrismaticCoef() <= 0.7)
    {
        return (((double)0.12997)/((double)0.95 - ship.getBlockCoef()) -
                ((double)0.11056)/(((double)0.95 - ship.getPrismaticCoef())));
    }
    else
    {
        return ((((double)0.18567)/
                 ((double)1.3571 - ship.getMidshipSectionCoef())) -
                (double)0.71276 + (double)0.38648 * ship.getPrismaticCoef());
    }
}

double HoltropMethod::calc_c_20(const Ship &ship)
{
    return ((double)1.0 + (double)0.015 *
                               get_C_SternMap().
                               value(ship.getSternShapeParam()));
}

double HoltropMethod::calc_c_p1(const Ship &ship)
{
    return ((double)1.45 * ship.getPrismaticCoef() -
            (double)0.315 -
            (double)0.0225 * ship.getLongitudinalBuoyancyCenter()
            );
}

double HoltropMethod::calc_C_V(
    const Ship &ship,
    units::velocity::meters_per_second_t customSpeed)
{
    double s = 0.0L;
    if (std::isnan(customSpeed.value()))
    {
        s = ship.getSpeed().value();
    }
    else
    {
        s = customSpeed.value();
    }
    return (
        (
            getfrictionalResistance(ship).value() *
            getAppendageResistance(ship).value() *
            getModelShipCorrelationResistance(ship).value()
            ) /
        (
            (double)0.5 *
            hydrology::WATER_RHO.value() *
            pow(s, (double)2.0) *
            (ship.getWettedHullSurface().value() +
             ship.getTotalAppendagesWettedSurfaces().value())
            )
        );
}

double HoltropMethod::calc_w_s(
    const Ship &ship,
    units::velocity::meters_per_second_t customSpeed)
{
    if (ship.getScrewVesselType() == Ship::ScrewVesselType::Single)
    {
        double CV = calc_C_V(ship, customSpeed);

        return (get_C9(ship) * get_C20(ship) * CV *
                    (
                        ship.getLengthInWaterline().value() /
                        ship.getDraftAtAft().value()
                        ) *
                    (
                        (double)0.050776 +
                        (double)0.93405 *
                            ((get_C11(ship) * CV) / ((double)1.0 - get_CP1(ship)))
                        ) +
                (
                    (double)0.27915 * get_C20(ship) *
                    sqrt(
                        (
                            (ship.getBeam().value()) /
                            (
                                ship.getLengthInWaterline().value() *
                                ((double)1.0 - get_CP1(ship))
                                )
                            )
                        )
                    ) +
                (get_C19(ship) * get_C20(ship))
                );
    }

    else if (ship.getScrewVesselType() == Ship::ScrewVesselType::Twin)
    {
        return ((double)0.3095 * ship.getBlockCoef() +
                (double)10.0 * calc_C_V(ship,customSpeed) *
                    ship.getBlockCoef() -
                (double)0.23 * (ship.getPropellerDiameter().value() /
                                 sqrt(ship.getBeam().value() *
                                      ship.getMeanDraft().value())));
    }
    else {
        throw Ship::ShipException("Wrong Screw Type!");
    }


}


double HoltropMethod::calc_t(const Ship &ship)
{
    if (ship.getScrewVesselType() == Ship::ScrewVesselType::Single)
    {
        return ((((double)0.25014 *
                  pow(((ship.getBeam()) /
                       (ship.getLengthInWaterline())),
                      (double)0.28956) *
                  pow(((sqrt(ship.getBeam().value() *
                             ship.getMeanDraft().value())) /
                       (ship.getPropellerDiameter().value())),
                      (double)0.2624)) /
                 pow(((double)1.0 - ship.getPrismaticCoef() +
                      (double)0.0225 * ship.getLongitudinalBuoyancyCenter()),
                     (double)0.01762)) +
                (double)0.0015 *
                    get_C_SternMap().value(ship.getSternShapeParam()));
    }
    else if (ship.getScrewVesselType() == Ship::ScrewVesselType::Twin)
    {
        return ((double)0.325 * ship.getBlockCoef() -
                (double)0.1885 *
                    (ship.getPropellerDiameter().value() /
                     (ship.getBeam().value() * ship.getMeanDraft().value())));
    }
    return 0.0;
}


double get_k_2_i(const Ship &ship,
                 Ship::ShipAppendage appendage)
{
    return get_k_2_iMap().value(appendage);
}

double HoltropMethod::calc_equivalentAppendageFormFactor(
    const Ship &ship)
{
    double sum = ship.getTotalAppendagesWettedSurfaces().value();

    if (sum <= 0)
    {
        return 0.0;
    }

    double k_2_eq = 0;
    const auto& appsMap = ship.getAppendagesWettedSurfaces();
    for (auto it = appsMap.constBegin(); it != appsMap.constEnd(); ++it)
    {
        const Ship::ShipAppendage &appendage = it.key();
        double value = it.value().value();
        k_2_eq += (((double)1.0 + get_k_2_i(ship, appendage)) * value);
    }

    return k_2_eq / sum;
}

double HoltropMethod::get_C1(const Ship &ship)
{
    if (std::isnan(c1))
    {
        c1 = calc_c_1(ship);
    }
    return c1;
}

double HoltropMethod::get_C2(const Ship &ship)
{
    if (std::isnan(c2))
    {
        c2 = calc_c_2(ship);
    }
    return c2;
}

double HoltropMethod::get_C3(const Ship &ship)
{
    if (std::isnan(c3))
    {
        c3 = calc_c_3(ship);
    }
    return c3;
}

double HoltropMethod::get_C4(const Ship &ship)
{
    if (std::isnan(c4))
    {
        c4 = calc_c_4(ship);
    }
    return c4;
}

double HoltropMethod::get_C5(const Ship &ship)
{
    if (std::isnan(c5))
    {
        c5 = calc_c_5(ship);
    }
    return c5;
}

double HoltropMethod::get_C7(const Ship &ship)
{
    if (std::isnan(c7))
    {
        c7 = calc_c_7(ship);
    }
    return c7;
}

double HoltropMethod::get_C14(const Ship &ship)
{
    if (std::isnan(c14))
    {
        c14 = calc_c_14(ship);
    }
    return c14;
}

double HoltropMethod::get_C15(const Ship &ship)
{
    if (std::isnan(c15))
    {
        c15 = calc_c_15(ship);
    }
    return c15;
}

double HoltropMethod::get_C16(const Ship &ship)
{
    if (std::isnan(c16))
    {
        c16 = calc_c_16(ship);
    }
    return c16;
}

double HoltropMethod::get_C17(const Ship &ship)
{
    if (std::isnan(c16))
    {
        c17 = calc_c_17(ship);
    }
    return c17;
}

double HoltropMethod::get_lambda(const Ship &ship)
{
    if (std::isnan(lambda))
    {
        lambda = calc_lambda(ship);
    }
    return lambda;
}

double HoltropMethod::get_m1(const Ship &ship)
{
    if (std::isnan(m1))
    {
        m1 = calc_m_1(ship);
    }
    return m1;
}

double HoltropMethod::get_m3(const Ship &ship)
{
    if (std::isnan(m3))
    {
        m3 = calc_m_3(ship);
    }
    return m3;
}

double HoltropMethod::get_PB(
    const Ship &ship,
    units::velocity::meters_per_second_t customSpeed)
{
    if (std::isnan(PB))
    {
        PB = calc_p_B(ship, customSpeed);
    }
    return PB;
}

double HoltropMethod::get_C8(const Ship &ship)
{
    if (std::isnan(C8))
    {
        C8 = calc_c_8(ship);
    }
    return C8;
}

double HoltropMethod::get_C9(const Ship &ship)
{
    if (std::isnan(C9))
    {
        C9 = calc_c_9(ship);
    }
    return C9;
}

double HoltropMethod::get_C11(const Ship &ship)
{
    if (std::isnan(C11))
    {
        C11 = calc_c_11(ship);
    }
    return C11;
}

double HoltropMethod::get_C19(const Ship &ship)
{
    if (std::isnan(C19))
    {
        C19 = calc_c_19(ship);
    }
    return C19;
}

double HoltropMethod::get_C20(const Ship &ship)
{
    if (std::isnan(C20))
    {
        C20 = calc_c_20(ship);
    }
    return C20;
}

double HoltropMethod::get_CP1(const Ship &ship)
{
    if (std::isnan(CP1))
    {
        CP1 = calc_c_p1(ship);
    }
    return CP1;
}


units::force::newton_t
HoltropMethod::getfrictionalResistance(
    const Ship &ship,
    units::velocity::meters_per_second_t customSpeed)
{
    units::velocity::meters_per_second_t s;
    if (std::isnan(customSpeed.value()))
    {
        s = ship.getSpeed();
    }
    else
    {
        s = customSpeed;
    }

    return units::force::newton_t(
        calc_C_F(ship, s) *
        (double)0.5 *
        hydrology::WATER_RHO.value() *
        pow(s.value(), (double)2.0) *
        ship.getWettedHullSurface().value() *
        calc_k_1(ship));
}


units::force::newton_t
HoltropMethod::getAppendageResistance(
    const Ship &ship,
    units::velocity::meters_per_second_t customSpeed)
{
    units::velocity::meters_per_second_t s;
    if (std::isnan(customSpeed.value()))
    {
        s = ship.getSpeed();
    }
    else
    {
        s = customSpeed;
    }
    return units::force::newton_t(
        (double)0.5 * hydrology::WATER_RHO.value() *
        pow(s.value(), (double)2.0) *
        calc_equivalentAppendageFormFactor(ship) *
        ship.getTotalAppendagesWettedSurfaces().value() *
        calc_C_F(ship, s)
        );
}

units::force::newton_t
HoltropMethod::getModelShipCorrelationResistance(
    const Ship &ship,
    units::velocity::meters_per_second_t customSpeed)
{
    units::velocity::meters_per_second_t s;
    if (std::isnan(customSpeed.value()))
    {
        s = ship.getSpeed();
    }
    else
    {
        s = customSpeed;
    }
    return units::force::newton_t(
        (double)0.5 *
        hydrology::WATER_RHO.value() *
        pow(s.value(), (double)2.0) *
        (calc_c_A(ship) + calc_delta_c_A(ship)) *
        (ship.getWettedHullSurface() +
         ship.getTotalAppendagesWettedSurfaces()).value()
        );
}

units::force::newton_t
HoltropMethod::getWaveResistance(
    const Ship &ship,
    units::velocity::meters_per_second_t customSpeed)
{
    double fn =  0.0;

    if (std::isnan(customSpeed.value()))
    {
        fn = hydrology::F_n(ship.getSpeed(),
                            ship.getLengthInWaterline());
    }
    else
    {
        fn = hydrology::F_n(customSpeed,
                            ship.getLengthInWaterline());
    }
    if (fn <= 0.4)
    {
        return calc_R_Wa(ship, customSpeed);
    }
    else if (fn > 0.55)
    {
        return calc_R_Wb(ship, customSpeed);
    }
    else{
        units::force::newton_t Rwa =
            calc_R_Wa(ship, units::velocity::knot_t(0.4));
        units::force::newton_t Rwb =
            calc_R_Wb(ship, units::velocity::knot_t(0.55));
        return Rwa + (((double)20 * fn - (double)8)/ (double)3.0) *
                         (Rwb - Rwa);
    }
}

units::force::newton_t
HoltropMethod::getBulbousBowResistance(
    const Ship &ship,
    units::velocity::meters_per_second_t customSpeed)
{
    double fri = calc_F_n_i(ship, customSpeed);
    return units::force::newton_t(
        (double)0.11 *
        hydrology::WATER_RHO.value() *
        hydrology::G.value() *
        pow(sqrt(ship.getBulbousBowTransverseArea().value()),(double)3.0) *
        (pow(fri, (double)3.0)/((double)1.0 + pow(fri, (double)2.0)))
        * exp((double)-3.0 * pow(get_PB(ship, customSpeed),(double)-2.0))
        );
}

units::force::newton_t
HoltropMethod::getImmersedTransomPressureResistance(
    const Ship &ship,
    units::velocity::meters_per_second_t customSpeed)
{
    units::velocity::meters_per_second_t s =
        units::velocity::meters_per_second_t(0.0L);
    if (std::isnan(customSpeed.value()))
    {
        s = ship.getSpeed();
    }
    else
    {
        s = customSpeed;
    }
    return units::force::newton_t(
        (double)0.5 * hydrology::WATER_RHO.value() *
        pow(s.value(), (double)2.0) *
        ship.getImmersedTransomArea().value() * calc_c_6(ship, s)
        );
}

units::force::newton_t
HoltropMethod::getAirResistance(
    const Ship &ship,
    units::velocity::meters_per_second_t customSpeed)
{
    double s = 0.0L;
    if (std::isnan(customSpeed.value()))
    {
        s = ship.getSpeed().value();
    }
    else
    {
        s = customSpeed.value();
    }
    return units::force::newton_t((double)0.5 *
                                  hydrology::AIR_RHO.value() *
                                  hydrology::AIR_DRAG_COEF *
                                  ship.getLengthInWaterline().value() *
                                  pow(s,
                                      (double)2.0));
}

units::force::newton_t
HoltropMethod::getTotalResistance(
    const Ship &ship,
    units::velocity::meters_per_second_t customSpeed)
{
    return getfrictionalResistance(ship, customSpeed) +
           getAppendageResistance(ship, customSpeed) +
           getModelShipCorrelationResistance(ship, customSpeed) +
           getWaveResistance(ship, customSpeed) +
           getBulbousBowResistance(ship, customSpeed) +
           getAirResistance(ship, customSpeed);
}


//units::force::newton_t HoltropMethod::calc_minthrust(const Ship &ship)
//{
//    return (getTotalResistance(ship) / ((double)1.0 - calc_t(ship)));
//}

units::velocity::meters_per_second_t HoltropMethod::
    calc_SpeedOfAdvance(
        const Ship &ship,
        units::velocity::meters_per_second_t customSpeed)
{
    units::velocity::meters_per_second_t s;
    if (std::isnan(customSpeed.value()))
    {
        s = ship.getSpeed();
    }
    else
    {
        s = customSpeed;
    }
    return ((double)1.0 - calc_w_s(ship, s)) * s;
}

//units::force::newton_t HoltropMethod::getThrust(const Ship &ship)
//{
//    return (ship.getThrustPower() / calc_SpeedOfAdvance(ship));
//}

//double HoltropMethod::calc_rotEff(Ship &ship)
//{
//    if (ship.getScrewVesselType() == Ship::ScrewVesselType::Single)
//    {
//        return ((double)0.9922 -
//                (double)0.05908 * (ship.getPropellerExpandedAreaRatio()) +
//                (double)0.07424 * (
//                    ship.getPrismaticCoef() -
//                    (double)0.02225 * ship.getLongitudinalBuoyancyCenter())
//                );
//    }
//    else if (ship.getScrewVesselType() == Ship::ScrewVesselType::Twin)
//    {
//        return ((double)0.9737  +
//                (double)0.111 * (
//                    ship.getPrismaticCoef() -
//                    (double)0.02225 * ship.getLongitudinalBuoyancyCenter()) -
//                (double)0.06325 * (power / ship.getPropellerDiameter()));
//    }
//}

//double HoltropMethod::calc_hullEff(const Ship &ship)
//{
//    return (((double)1.0 - calc_t(ship))/ ((double)1.0 - calc_w_s(ship)));
//}



std::string HoltropMethod::getMethodName()
{
    return "Holtrop and Mennen Resistance Prediction Method";
}

