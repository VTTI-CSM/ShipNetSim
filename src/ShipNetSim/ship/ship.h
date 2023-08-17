#ifndef SHIP_H
#define SHIP_H

#include "../../third_party/units/units.h"
#include "holtropresistancemethod.h"
#include <QObject>
#include <QMap>

class IShipResistanceStrategy;

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


    enum class ShipAppendage {
        rudder_behind_skeg,
        rudder_behind_stern,
        slender_twin_screw_rudder,
        thick_twin_screw_rudder,
        shaft_brackets,
        skeg,
        strut_bossing,
        hull_bossing,
        exposed_shafts_10_degree_with_buttocks,
        exposed_shafts_20_degree_with_buttocks,
        stabilizer_fins,
        dome,
        bilge_keels
    };


    enum class CStern {
        pramWithGondola,
        VShapedSections,
        NormalSections,
        UShapedSections
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




    Ship(units::length::meter_t lengthInWaterline,
         units::length::meter_t moldedBeam,
         units::length::meter_t moldedMeanDraft =
            units::length::meter_t(std::nan("no value")),
         units::length::meter_t moldedDraftAtAft =
            units::length::meter_t(std::nan("no value")),
         units::length::meter_t moldedDraftAtForward =
            units::length::meter_t(std::nan("no value")),
         units::volume::cubic_meter_t volumetricDisplacement =
            units::volume::cubic_meter_t(std::nan("no value")),
         units::area::square_meter_t wettedSurface =
            units::area::square_meter_t(std::nan("no value")),
         IShipResistanceStrategy* initialStrategy =
            new HoltropResistanceMethod);

    ~Ship();



    units::force::kilonewton_t calculateTotalResistance() const;



    units::length::meter_t getLengthInWaterline() const;
    void setLengthInWaterline(const units::length::meter_t &newL);

    units::length::meter_t getBeam() const;
    void setBeam(const units::length::meter_t &newB);

    units::length::meter_t getMeanDraft() const;
    void setMeanDraft(const units::length::meter_t &newT);

    units::length::meter_t getDraftAtAft() const;
    void setDraftAtAft(const units::length::meter_t &newT_A);

    units::length::meter_t getDraftAtForward() const;
    void setDraftAtForward(const units::length::meter_t &newT_F);

    units::volume::cubic_meter_t getVolumetricDisplacement() const;
    void setVolumetricDisplacement(const units::volume::cubic_meter_t &newNab);

    double getBlockCoef() const;
    void setBlockCoef(double newC_B);

    double getPrismaticCoef() const;
    void setPrismaticCoef(double newC_P);

    double getMidshipSectionCoef() const;
    void setMidshipSectionCoef(double newC_M);

    double getWaterplaneAreaCoef() const;
    void setWaterplaneAreaCoef(double newC_WP);

    double getLongitudinalBuoyancyCenter() const;
    void setLongitudinalBuoyancyCenter(double newLcb);





    units::length::meter_t getD() const;
    void setD(const units::length::meter_t &newD);


    units::area::square_meter_t getWettedHullSurface() const;
    void setWettedHullSurface(const units::area::square_meter_t &newS);

    units::length::meter_t getBulbousBowTransverseAreaCenterHeight() const;
    void setBulbousBowTransverseAreaCenterHeight(
        const units::length::meter_t &newH_b);

    QMap<ShipAppendage, units::area::square_meter_t>
    getAppendagesWettedSurfaces() const;
    units::area::square_meter_t getTotalAppendagesWettedSurfaces() const;
    void setAppendagesWettedSurfaces(
        const QMap<ShipAppendage, units::area::square_meter_t> &newS_appList);
    void addAppendagesWettedSurface(
        const QPair<ShipAppendage, units::area::square_meter_t> &entry);

    units::area::square_meter_t getBulbousBowTransverseArea() const;
    void setBulbousBowTransverseArea(
        const units::area::square_meter_t &newA_BT);

    units::angle::degree_t getHalfWaterlineEntranceAngle() const;
    void setHalfWaterlineEntranceAngle(
        const units::angle::degree_t &newHalfWaterlineEntranceAnlge);

    //units::velocity::meters_per_second_t getSpeed() const;
    units::velocity::knot_t getSpeed() const;
    void setSpeed(const units::velocity::knot_t $newSpeed);
    void setSpeed(const units::velocity::meters_per_second_t &newSpeed);





    units::area::square_meter_t getImmersedTransomArea() const;
    void setImmersedTransomArea(const units::area::square_meter_t &newA_T);





    void setResistanceStrategy(IShipResistanceStrategy* newStrategy);
    IShipResistanceStrategy *getResistanceStrategy() const;



    units::area::square_meter_t getLengthwiseProjectionArea() const;
    void setLengthwiseProjectionArea(
        const units::area::square_meter_t &newLengthwiseProjectionArea);

    units::length::nanometer_t getSurfaceRoughness() const;
    void setSurfaceRoughness(
        const units::length::nanometer_t &newSurfaceRoughness);


    CStern getSternShapeParam() const;
    void setSternShapeParam(CStern newC_Stern);

    units::length::meter_t getRunLength() const;
    void setRunLength(const units::length::meter_t &newRunLength);

private:
    IShipResistanceStrategy* mResistanceStrategy;

    units::length::meter_t mWaterlineLength;
    units::length::meter_t mBeam;
    units::length::meter_t mMeanDraft;
    units::length::meter_t mDraftAtForward;
    units::length::meter_t mDraftAtAft;
    units::volume::cubic_meter_t mVolumetricDisplacement;
    units::area::square_meter_t mWettedHullSurface;
    units::area::square_meter_t mLengthwiseProjectionArea;
    units::length::meter_t mBulbousBowTransverseAreaCenterHeight; //h_B

    QMap<ShipAppendage, units::area::square_meter_t> mAppendagesWettedSurfaces;
    units::area::square_meter_t mBulbousBowTransverseArea;
    units::area::square_meter_t mImmersedTransomArea;

    units::force::kilonewton_t R_F;
    units::force::kilonewton_t R_app;
    units::force::kilonewton_t R_W;
    units::force::kilonewton_t R_B;
    units::force::kilonewton_t R_TR;
    units::force::kilonewton_t R_A;
    units::force::kilonewton_t R;

    units::angle::degree_t mHalfWaterlineEntranceAngle;
    units::velocity::meters_per_second_t mSpeed;
    units::length::nanometer_t mSurfaceRoughness =
        units::length::nanometer_t(150.0);
    units::length::meter_t mRunLength;

    QVector<QPair<units::area::square_meter_t, double>> App;
    double mLongitudinalBuoyancyCenter;
    CStern mSternShapeParam;
    double mMidshipSectionCoef;
    double mWaterplaneAreaCoef;
    double mPrismaticCoef;
    double mBlockCoef;


    units::area::square_meter_t calc_WetSurfaceArea(
        WetSurfaceAreaCalculationMethod method =
        WetSurfaceAreaCalculationMethod::Holtrop
        );

    double calc_BlockCoef(units::velocity::meters_per_second_t &speed,
                    BlockCoefficientMethod method =
                    BlockCoefficientMethod::Ayre);

    units::length::meter_t calc_RunLength();
    units::angle::degree_t calc_i_E();
    units::area::square_meter_t calc_WetSurfaceAreaToHoltrop();
    units::area::square_meter_t calc_WetSurfaceAreaToSchenzle();
    units::volume::cubic_meter_t calc_VolumetricDisplacement();
    double calc_WaterplaneAreaCoef(WaterPlaneCoefficientMethod method =
                     WaterPlaneCoefficientMethod::General_Cargo);

    double calc_PrismaticCoef();

    bool checkSelectedMethodAssumptions(IShipResistanceStrategy* strategy);
};

#endif // SHIP_H
