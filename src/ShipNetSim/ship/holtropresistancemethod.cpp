#include "holtropresistancemethod.h"
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
        {Ship::ShipAppendage::exposed_shafts_10_degree_with_buttocks, (double)1.0},
        {Ship::ShipAppendage::exposed_shafts_20_degree_with_buttocks, (double)4.0},
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

double HoltropResistanceMethod::calc_c_7(const Ship &ship)
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


double HoltropResistanceMethod::calc_c_1(const Ship &ship)
{
    return (
        (double)2223105.0
        * pow(calc_c_7(ship), (double)3.78613)
        * pow((ship.getMeanDraft() / ship.getBeam()), (double)1.07961)
        * pow(((double)90.0 -
               ship.getHalfWaterlineEntranceAngle().value()),
              (double)-1.37565)
        );
}

double HoltropResistanceMethod::calc_c_3(const Ship &ship)
{
    return (
        (double)0.56
        * pow(ship.getBulbousBowTransverseArea().value(), (double)1.5)
        / (
            ship.getBeam().value()
            * ship.getMeanDraft().value()
            * (
               (double)0.31 * sqrt(ship.getBulbousBowTransverseArea().value()) +
                ship.getDraftAtForward().value() -
                ship.getBulbousBowTransverseAreaCenterHeight().value()
                )
            )
        );
}

double HoltropResistanceMethod::calc_c_2(const Ship &ship)
{
    return exp((double)-1.89 * sqrt(calc_c_3(ship)));
}

double HoltropResistanceMethod::calc_c_5(const Ship &ship)
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

double HoltropResistanceMethod::calc_c_15(const Ship &ship)
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

double HoltropResistanceMethod::calc_c_16(const Ship &ship)
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

double HoltropResistanceMethod::calc_lambda(const Ship &ship)
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

double HoltropResistanceMethod::calc_m_1(const Ship &ship)
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
        - calc_c_16(ship)
        );
}

double HoltropResistanceMethod::calc_m_4(const Ship &ship)
{
    return (
        calc_c_15(ship)
        * (double)0.4
        * exp((double)-0.034 * pow(
                  hydrology::F_n(ship.getSpeed(),
                                 ship.getLengthInWaterline()), (double)-3.29))
        );
}

double HoltropResistanceMethod::calc_m_3(const Ship &ship)
{
    return (
        (double)-7.2035
        * pow((ship.getBeam() /
               ship.getLengthInWaterline()).value(), (double)0.326869)
        * pow((ship.getMeanDraft() /
               ship.getBeam()).value(), (double)0.605375)
        );
}

double HoltropResistanceMethod::calc_c_17(const Ship &ship)
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

units::force::kilonewton_t HoltropResistanceMethod::calc_R_Wa(
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
    return units::force::kilonewton_t(
        calc_c_1(ship) *
        calc_c_2(ship) *
        calc_c_5(ship) *
        hydrology::WATER_RHO *
        hydrology::G *
        ship.getVolumetricDisplacement() *
        exp(
            (
                (
                    calc_m_1(ship) *
                    pow(fn, d)
                    ) +
                (
                    calc_m_4(ship) *
                    cos(calc_lambda(ship) *
                        pow(fn, (double)-2.0)
                        )
                    )
                )
            )
        );
}

units::force::kilonewton_t HoltropResistanceMethod::calc_R_Wb(
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

    return units::force::kilonewton_t(
        calc_c_17(ship) *
        calc_c_2(ship) *
        calc_c_5(ship) *
        hydrology::WATER_RHO *
        hydrology::G *
        ship.getVolumetricDisplacement() *
        exp(calc_m_3(ship) *
                pow(fn, d) +
            calc_m_4(ship) *
                cos(calc_lambda(ship) *
                    pow(fn, -2.0)))
        );
}

double HoltropResistanceMethod::calc_C_F(const Ship &ship)
{
    return (double)0.075 /
           pow((log10(hydrology::R_n(ship.getSpeed(),
                                     ship.getLengthInWaterline()))
                        - (double)2.0), (double)2.0);
}

int getCStenByAftShape(const Ship &ship)
{
    return get_C_SternMap().value(ship.getSternShapeParam());
}

double HoltropResistanceMethod::calc_c_14(const Ship &ship)
{
    return (double)1.0 + (double)0.011 * (double)getCStenByAftShape(ship);
}

double HoltropResistanceMethod::calc_k_1(const Ship &ship)
{
    return (double)0.93 +
           (double)0.487118 *
               calc_c_14(ship) *
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

double HoltropResistanceMethod::calc_c_4(const Ship &ship)
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

double HoltropResistanceMethod::calc_delta_c_A(const Ship &ship)
{
    if (ship.getSurfaceRoughness() == units::length::nanometer_t(150.0))
    {
        return (double)0.0;
    }
    else
    {
        return ((double)0.105 *
               pow(ship.getSurfaceRoughness().value(),
                            ((double)1.0/(double)3.0)) -
                (double)0.005579) /
               pow(ship.getLengthInWaterline().value(),
                     ((double)1.0/(double)3.0));
    }
}

double HoltropResistanceMethod::calc_c_A(const Ship &ship)
{
    return ((double)0.00546
                * pow(ship.getLengthInWaterline().value() +
                          (double)100.0, (double)-0.16)
            - (double)0.002
            + (double)0.003
                  * sqrt((ship.getLengthInWaterline().value()/(double)7.5))
                  * pow(ship.getBlockCoef(), (double)4.0)
                  * calc_c_2(ship)
                  * ((double)0.04 - calc_c_4(ship))
            );
}

double HoltropResistanceMethod::calc_F_n_T(const Ship &ship)
{
    return (ship.getSpeed().value() /
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

double HoltropResistanceMethod::calc_c_6(const Ship &ship)
{
    double FrT = calc_F_n_T(ship);
    if (FrT < 5.0){
        return (double)0.2 * ((double)1.0 - (double)0.2 * FrT);
    }
    else
    {
        return 0.0;
    }
}

units::length::meter_t HoltropResistanceMethod::calc_h_F(const Ship &ship)
{
    double fr = hydrology::F_n(ship.getSpeed(), ship.getLengthInWaterline());
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
    else
    {
        return hf;
    }
}

units::length::meter_t HoltropResistanceMethod::calc_h_W(const Ship &ship)
{
    units::length::meter_t hw = units::length::meter_t(
        ship.getHalfWaterlineEntranceAngle().value() *
        pow(ship.getSpeed().value(), (double)2.0) /
        ((double) 400.0 * hydrology::G.value()));
    if (hw > 0.01 * ship.getLengthInWaterline())
    {
        return 0.01 * ship.getLengthInWaterline();
    }
    else{
        return hw;
    }
}

double HoltropResistanceMethod::calc_F_n_i(const Ship &ship)
{
    return (ship.getSpeed().value() /
            (sqrt(hydrology::G.value() *
                  (ship.getDraftAtForward().value() -
                   ship.getBulbousBowTransverseAreaCenterHeight().value() -
                   ((double)0.25 * sqrt(ship.getBulbousBowTransverseArea().value())) +
                   calc_h_F(ship).value() +
                   calc_h_W(ship).value()))));
}

double HoltropResistanceMethod::calc_p_B(const Ship &ship)
{
    return (double)0.56 *
           (sqrt(ship.getBulbousBowTransverseArea().value()) /
            (ship.getDraftAtForward().value() -
             (double)1.5 *
                 (ship.getBulbousBowTransverseAreaCenterHeight().value()) +
             calc_h_F(ship).value()));
}

double get_k_2_i(Ship::ShipAppendage appendage)
{
    return get_k_2_iMap().value(appendage);
}

double HoltropResistanceMethod::calc_equivalentAppendageFormFactor(
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
        k_2_eq += (get_k_2_i(appendage) * value / sum);
    }

    return k_2_eq;
}

units::force::kilonewton_t
HoltropResistanceMethod::getfrictionalResistance(
    const Ship &ship)
{
    return units::force::kilonewton_t(
        calc_C_F(ship) *
        (double)0.5 *
        hydrology::WATER_RHO.value() *
        pow(ship.getSpeed().value(), (double)2.0) *
        ship.getWettedHullSurface().value() *
        calc_k_1(ship));
}


units::force::kilonewton_t
    HoltropResistanceMethod::getAppendageResistance(
        const Ship &ship)
{
    return units::force::kilonewton_t(
        (double)0.5 * hydrology::WATER_RHO.value() *
        pow(ship.getSpeed().value(), (double)2.0) *
        calc_equivalentAppendageFormFactor(ship) *
        ship.getTotalAppendagesWettedSurfaces().value() * calc_C_F(ship)
        );
}

units::force::kilonewton_t
HoltropResistanceMethod::getModelShipCorrelationResistance(
    const Ship &ship)
{
    return units::force::kilonewton_t(
        (double)0.5 *
        hydrology::WATER_RHO.value() *
        pow(ship.getSpeed().value(), (double)2.0) *
        (calc_c_A(ship) * calc_delta_c_A(ship)) *
        (ship.getWettedHullSurface() +
         ship.getTotalAppendagesWettedSurfaces()).value()
        );
}

units::force::kilonewton_t
HoltropResistanceMethod::getWaveResistance(
    const Ship &ship)
{
    double fn = hydrology::F_n(ship.getSpeed(),
                               ship.getLengthInWaterline());
    if (fn <= 0.4)
    {
        return calc_R_Wa(ship);
    }
    else if (fn > 0.55)
    {
        return calc_R_Wb(ship);
    }
    else{
        units::force::kilonewton_t Rwa =
            calc_R_Wa(ship, units::velocity::knot_t(0.4));
        units::force::kilonewton_t Rwb =
            calc_R_Wb(ship, units::velocity::knot_t(0.55));
        return Rwa + (((double)20 * fn - (double)8)/ (double)3.0) *
                         (Rwb - Rwa);
    }
}

units::force::kilonewton_t
HoltropResistanceMethod::getBulbousBowResistance(
const Ship &ship)
{
    double fri = calc_F_n_i(ship);
    return units::force::kilonewton_t(
        (double)0.11 *
        hydrology::WATER_RHO.value() *
        hydrology::G.value() *
        pow(sqrt(ship.getBulbousBowTransverseArea().value()),(double)3.0) *
        (pow(fri, (double)3.0)/((double)1.0 + pow(fri, (double)2.0)))
        * exp((double)-3.0 * pow(calc_p_B(ship),(double)-2.0))
        );
}

units::force::kilonewton_t
HoltropResistanceMethod::getImmersedTransomPressureResistance(
    const Ship &ship)
{
    return units::force::kilonewton_t(
        (double)0.5 * hydrology::WATER_RHO.value() *
        pow(ship.getSpeed().value(), (double)2.0) *
        ship.getImmersedTransomArea().value() * calc_c_6(ship)
        );
}

units::force::kilonewton_t
HoltropResistanceMethod::getAirResistance(const Ship &ship)
{
    return units::force::kilonewton_t((double)0.5 *
                                      hydrology::AIR_RHO.value() *
                                      hydrology::AIR_DRAG_COEF *
                                      ship.getLengthInWaterline().value() *
                                      pow(ship.getSpeed().value(),
                                                                                                                                                       (double)2.0));
}

std::string HoltropResistanceMethod::getMethodName()
{
    return "Holtrop and Mennen Resistance Prediction Method";
}

