#ifndef SHIP_H
#define SHIP_H

#include "../third_party/units/units.h"
#include <QObject>

class Ship : public QObject
{
    Q_OBJECT
public:
    Ship();

    units::area::square_meter_t get_wet_surface_area_to_holtrop();
    units::area::square_meter_t get_wet_surface_area_to_schenzle();

private:
    units::length::meter_t L;
    units::length::meter_t B;
    units::length::meter_t T;
    units::length::meter_t D;
    units::length::meter_t T_F;
    units::length::meter_t T_A;
    units::area::square_meter_t S;
    units::length::meter_t h_b;
    units::volume::cubic_meter_t Nab;
    units::area::square_meter_t S_app;
    units::area::square_meter_t A_BT;
    units::angle::degree_t i_E;
    units::area::square_meter_t A_T;
    units::area::square_meter_t A_WP;
    units::length::meter_t L_WP;
    units::length::meter_t B_WP;
    units::velocity::meters_per_second_t v_Probe;
    units::force::kilonewton_t R_F;
    units::force::kilonewton_t R_app;
    units::force::kilonewton_t R_W;
    units::force::kilonewton_t R_B;
    units::force::kilonewton_t R_TR;
    units::force::kilonewton_t R_A;
    units::force::kilonewton_t R;



    double k_1;
    QVector<double> k_2;
    QVector<QPair<units::area::square_meter_t, double>> App;
    double lcb;
    int C_Stern;
    double C_A;
    double C_M;
    double C_WP;
    double C_P;
    double C_B;
    double eta_R;
    double w;
    double t;
    double A_E_0;
    double c_P_D;
    double eta_0;






};

#endif // SHIP_H
