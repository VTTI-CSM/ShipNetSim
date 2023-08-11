#ifndef SHIP_H
#define SHIP_H

#include "../third_party/units/units.h"
#include <QObject>

class Ship : public QObject
{
    Q_OBJECT
public:


    /**
     * @brief defines the Wet Surface Area Calculation Method
     *
     * @details
     * Method 'Holtrop' predicts the wetted surface of the hull at rest
     * if it is not knows from hydrostatic analysis.
     * @cite  birk2019fundamentals <br>
     * Method '':
     */
    enum class WetSurfaceAreaCalculationMethod {
        Holtrop,
        Schenzle,
        Cargo,
        Trawlers
    };

    /**
     * @brief defines the Block Coefficient Methods
     *
     * @details
     * Method 'Ayre': $C_B=C-1.68 F_n$ where often C = 1.06 is used. <br>
     * Method 'Jensen': is recommended for modern ship hulls:
     * $C_B=-4.22+27.8 \cdot \sqrt{F_n}-39.1 \cdot F_n+46.6 \cdot F_n^3$
     * for $0.15<F_n<0.32$ <br>
     * Method 'Schneekluth': is optimized aiming to reduce 'production costs’
     * for specified deadweight and speed. The formula is valid for
     * $0:48 <= CB <= 0:85$ and $0:14 <= Fn <= 0:32$. However, for actual
     * $Fn >= 0.3$ only Fn = 0.30 is used.
     * @cite schneekluth1998ship
     */
    enum class BlockCoefficientMethod {
        Ayre,
        Jensen,
        Schneekluth
    };

    /**
     * @brief defines the Water Plane Coefficient Methods
     *
     * @details
     * Methods 'U_Shape', 'Average_Section', and 'V_Section' are only
     * applicable to ships with cruiser sterns and ‘cut-away cruiser sterns’.
     * As these formulae are not applicable to vessels with submerged
     * transom sterns, they should be tested on a ‘similar ship’
     * @cite schneekluth1998ship   <br>
     * ---<br>
     * Methods 'General_Cargo' and 'Container' gives only initial estimates.
     * @cite birk2019fundamentals
     */
    enum class WaterPlaneCoefficientMethod {
        U_Shape,
        Average_Section,
        V_Section,
        Tanker_Bulker,
        General_Cargo,
        Container
    };

    // Declaration of a custom exception class within MyClass
    class ShipException : public std::exception {
    public:
        ShipException(
            const std::string& message) : msg_(message) {}
        const char* what() const noexcept override {
            return msg_.c_str();
        }
    private:
        std::string msg_;
    };




    Ship();


    units::area::square_meter_t get_wet_surface_area(
        WetSurfaceAreaCalculationMethod method =
        WetSurfaceAreaCalculationMethod::Holtrop
        );

    double get_C_B(units::velocity::meters_per_second_t &speed,
                   BlockCoefficientMethod method =
                   BlockCoefficientMethod::Ayre);

    double get_C_P();

    double get_Nab();


    double get_C_WP(WaterPlaneCoefficientMethod method =
                    WaterPlaneCoefficientMethod::General_Cargo);


private:
    units::length::meter_t L;
    units::length::meter_t B;
    units::length::meter_t T;
    units::length::meter_t D;
    units::length::meter_t T_F;
    units::length::meter_t T_A;
    units::area::square_meter_t S;
    units::length::meter_t h_b;
    units::volume::cubic_meter_t Nab; // volumetric displacement
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



    units::area::square_meter_t get_wet_surface_area_to_holtrop();
    units::area::square_meter_t get_wet_surface_area_to_schenzle();

};

#endif // SHIP_H
