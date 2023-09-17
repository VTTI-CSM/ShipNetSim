/**
 * @file Ship.h
 * @author Ahmed Aredah
 * @date 8/17/2023
 * @brief Provides an implementation of a ship with various methods to
 * calculate its properties and resistances.
 *
 * This class encompasses a wide range of ship properties, coefficients,
 * and resistance values, providing methods to calculate and manipulate them.
 * It allows users to define ships with diverse physical characteristics
 * and employ different resistance strategies.
 */

#ifndef SHIP_H
#define SHIP_H
#include <iostream>
#include <any>
#include "../../third_party/units/units.h"
#include "battery.h"
#include "ishippropeller.h"
#include "tank.h"
#include <QObject>
#include <QMap>
#include "../network/line.h"

class IShipResistancePropulsionStrategy; // Forward declaration of the Interface

/**
 * @class Ship
 * @brief Defines a ship and provides methods to
 * compute its properties and resistances.
 */
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
        None,
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
        None,
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
        None,
        U_Shape,
        Average_Section,
        V_Section,
        Tanker_Bulker,
        General_Cargo,
        Container
    };


    /**
     * @brief Defines the ship appendages
     *
     * @details
     * Appendages mostly affect the viscous resistance.
     *
     * @cite birk2019fundamentals
     */
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


    /**
     * @brief Defines the stern shape of the ship
     */
    enum class CStern {
        None,
        pramWithGondola,
        VShapedSections,
        NormalSections,
        UShapedSections
    };

    enum class ScrewVesselType {
        None,
        Single,
        Twin
    };

    // Declaration of a custom exception clas
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



    /**
     * @brief Constructs a Ship object with given parameters.
     *
     * Initializes a ship with the provided characteristics and sets
     * an initial strategy for resistance calculation.
     */
    Ship(units::length::meter_t lengthInWaterline,
         units::length::meter_t moldedBeam,
         double midshipCoef = std::nan("uninitialized"),
         double LongitudinalBuoyancyCenter = std::nan("uninitialized"),
         units::length::nanometer_t roughness =
         units::length::nanometer_t(std::nan("uninitialized")),
         units::area::square_meter_t bulbousBowTransverseArea =
         units::area::square_meter_t(std::nan("uninitialized")),
         units::length::meter_t bulbousBowTransverseAreaCenterHeight =
         units::length::meter_t(std::nan("uninitialized")),
         units::area::square_meter_t immersedTransomArea =
         units::area::square_meter_t(std::nan("uninitialized")),
         units::length::meter_t moldedMeanDraft =
         units::length::meter_t(std::nan("uninitialized")),
         units::length::meter_t moldedDraftAtAft =
         units::length::meter_t(std::nan("uninitialized")),
         units::length::meter_t moldedDraftAtForward =
         units::length::meter_t(std::nan("uninitialized")),
         double blockCoef = std::nan("uninitialized"),
         BlockCoefficientMethod blockCoefMethod =
         BlockCoefficientMethod::None,
         double prismaticCoef = std::nan("uninitialized"),
         units::length::meter_t runLength =
         units::length::meter_t(std::nan("uninitialized")),
         double waterplaneAreaCoef = std::nan("uninitialized"),
         WaterPlaneCoefficientMethod waterplaneCoefMethod =
         WaterPlaneCoefficientMethod::None,
         units::volume::cubic_meter_t volumetricDisplacement =
         units::volume::cubic_meter_t(std::nan("uninitialized")),
         units::area::square_meter_t wettedHullSurface =
         units::area::square_meter_t(std::nan("uninitialized")),
         WetSurfaceAreaCalculationMethod wetSurfaceAreaMethod =
         WetSurfaceAreaCalculationMethod::None,
         units::angle::degree_t halfWaterlineEntranceAngle =
         units::angle::degree_t(std::nan("uninitialized")),
         IShipResistancePropulsionStrategy* initialStrategy = nullptr);

    Ship(units::length::meter_t lengthInWaterline,
         units::length::meter_t moldedBeam,
         units::length::meter_t moldedMeanDraft);

    Ship(units::length::meter_t lengthInWaterline,
         units::length::meter_t moldedBeam,
         units::length::meter_t moldedDraftAtAft,
         units::length::meter_t moldedDraftAtForward);

    Ship(const QMap<QString, std::any>& parameters);

    /**
     * @brief Destructor for the Ship class.
     *
     * Destroys the Ship object and any associated resources.
     */
    ~Ship();


    /**
     * @brief Calculates the total resistance experienced by the ship.
     * @return Total resistance in kilonewtons.
     */
    units::force::newton_t calculateTotalResistance() const;


    // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
    // %%%%%%%%%%%%%%%%%%%%% GETTERS & SETTERS %%%%%%%%%%%%%%%%%%%%%
    // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

    /**
     * @brief Retrieves the length of the ship at the waterline.
     * @return Length in meters.
     */
    units::length::meter_t getLengthInWaterline() const;
    /**
     * @brief Updates the length of the ship at the waterline.
     * @param newL New length to set in meters.
     */
    void setLengthInWaterline(const units::length::meter_t &newL);

    /**
     * @brief Retrieves the breadth (width) of the ship.
     * @return Breadth in meters.
     */
    units::length::meter_t getBeam() const;
    /**
     * @brief Updates the breadth of the ship.
     * @param newB New breadth to set in meters.
     */
    void setBeam(const units::length::meter_t &newB);

    /**
     * @brief Retrieves the average depth of the ship submerged in water.
     * @return Mean draft in meters.
     */
    units::length::meter_t getMeanDraft() const;
    /**
     * @brief Updates the average depth of the ship submerged in water.
     * @param newT New draft to set in meters.
     */
    void setMeanDraft(const units::length::meter_t &newT);

    /**
     * @brief Updates the average depth of the ship submerged in water.
     * @param newT_A New draft at Aft of the ship in meters
     * @param newT_F New draft at the forward of the ship in meters
     */
    void setMeanDraft(const units::length::meter_t &newT_A,
                      const units::length::meter_t &newT_F);

    /**
     * @brief Retrieves the depth of the ship's aft submerged in water.
     * @return Draft at aft in meters.
     */
    units::length::meter_t getDraftAtAft() const;
    /**
     * @brief Updates the depth of the ship's aft submerged in water.
     * @param newT_A New draft to set at aft in meters.
     */
    void setDraftAtAft(const units::length::meter_t &newT_A);

    /**
     * @brief Retrieves the depth of the ship's forward
     *          section submerged in water.
     * @return Draft at forward in meters.
     */
    units::length::meter_t getDraftAtForward() const;
    /**
     * @brief Updates the depth of the ship's forward
     *          section submerged in water.
     * @param newT_F New draft to set at forward in meters.
     */
    void setDraftAtForward(const units::length::meter_t &newT_F);

    /**
     * @brief Retrieves the volume of water displaced by the ship.
     * @return Volumetric displacement in cubic meters.
     */
    units::volume::cubic_meter_t getVolumetricDisplacement() const;
    /**
     * @brief Updates the volume of water displaced by the ship.
     * @param newNab New volumetric displacement to set in cubic meters.
     */
    void setVolumetricDisplacement(const units::volume::cubic_meter_t &newNab);

    /**
     * @brief Retrieves the block coefficient of the ship.
     * @return Block coefficient.
     */
    double getBlockCoef() const;
    /**
     * @brief Updates the block coefficient of the ship.
     * @param newC_B New block coefficient to set.
     */
    void setBlockCoef(const double newC_B);

    /**
     * @brief Retrieves the prismatic coefficient of the ship.
     * @return Prismatic coefficient.
     */
    double getPrismaticCoef() const;
    /**
     * @brief Updates the prismatic coefficient of the ship.
     * @param newC_P New prismatic coefficient value.
     */
    void setPrismaticCoef(const double newC_P);

    /**
     * @brief Retrieves the midship section coefficient of the ship.
     * @return Midship section coefficient.
     */
    double getMidshipSectionCoef() const;
    /**
     * @brief Updates the midship section coefficient of the ship.
     * @param newC_M New midship section coefficient value.
     */
    void setMidshipSectionCoef(const double newC_M);

    /**
     * @brief Retrieves the waterplane area coefficient of the ship.
     * @return Waterplane area coefficient.
     */
    double getWaterplaneAreaCoef() const;
    /**
     * @brief Updates the waterplane area coefficient of the ship.
     * @param newC_WP New waterplane area coefficient value.
     */
    void setWaterplaneAreaCoef(const double newC_WP);

    /**
     * @brief Retrieves the longitudinal buoyancy center of the ship.
     * @return Longitudinal buoyancy center.
     */
    double getLongitudinalBuoyancyCenter() const;
    /**
     * @brief Updates the longitudinal buoyancy center of the ship.
     * @param newLcb New value for the longitudinal buoyancy center.
     */
    void setLongitudinalBuoyancyCenter(const double newLcb);




//    /**
//     * @brief Retrieves the depth 'D' of the ship.
//     * @return Depth in meters.
//     */
//    units::length::meter_t getD() const;
//    /**
//     * @brief Updates the depth 'D' of the ship.
//     * @param newD New depth value in meters.
//     */
//    void setD(const units::length::meter_t &newD);


    /**
     * @brief Retrieves the wetted surface of the ship's hull.
     * @return Wetted surface area in square meters.
     */
    units::area::square_meter_t getWettedHullSurface() const;
    /**
     * @brief Updates the wetted surface of the ship's hull.
     * @param newS New wetted surface area in square meters.
     */
    void setWettedHullSurface(const units::area::square_meter_t &newS);

    /**
     * @brief Retrieves the center height of the transverse
     *          area of the bulbous bow.
     * @return Center height in meters.
     */
    units::length::meter_t getBulbousBowTransverseAreaCenterHeight() const;
    /**
     * @brief Updates the center height of the transverse
     *          area of the bulbous bow.
     * @param newH_b New center height value in meters.
     */
    void setBulbousBowTransverseAreaCenterHeight(
        const units::length::meter_t &newH_b);

    /**
     * @brief Retrieves a map containing wetted surfaces of
     *          each ship appendage.
     * @return QMap with ship appendage type as the key
     *          and wetted surface area as the value.
     */
    QMap<ShipAppendage, units::area::square_meter_t>
    getAppendagesWettedSurfaces() const;
    /**
     * @brief Retrieves the total wetted surface area of all ship appendages.
     * @return Total wetted surface area in square meters.
     */
    units::area::square_meter_t getTotalAppendagesWettedSurfaces() const;
    /**
     * @brief Sets the wetted surface area for each appendage
     *          provided in the list.
     * @param newS_appList QMap containing ship appendage type
     *          and corresponding wetted surface area.
     */
    void setAppendagesWettedSurfaces(
        const QMap<ShipAppendage, units::area::square_meter_t> &newS_appList);
    /**
     * @brief Adds or updates the wetted surface area for
     *          a specific appendage.
     * @param entry A pair containing the ship appendage type
     *          and its wetted surface area.
     */
    void addAppendagesWettedSurface(
        const QPair<ShipAppendage, units::area::square_meter_t> &entry);

    /**
     * @brief Retrieves the transverse area of the bulbous bow.
     * @return Transverse area in square meters.
     */
    units::area::square_meter_t getBulbousBowTransverseArea() const;
    /**
     * @brief Updates the transverse area of the bulbous bow.
     * @param newA_BT New transverse area value in square meters.
     */
    void setBulbousBowTransverseArea(
        const units::area::square_meter_t &newA_BT);

    /**
     * @brief Retrieves the half waterline entrance angle of the ship.
     * @return Angle in degrees.
     */
    units::angle::degree_t getHalfWaterlineEntranceAngle() const;
    /**
     * @brief Updates the half waterline entrance angle of the ship.
     * @param newHalfWaterlineEntranceAnlge New angle value in degrees.
     */
    void setHalfWaterlineEntranceAngle(
        const units::angle::degree_t &newHalfWaterlineEntranceAnlge);

    /**
     * @brief Retrieves the speed of the ship in knots.
     * @return Speed in knots.
     */
    units::velocity::meters_per_second_t getSpeed() const;
    /**
     * @brief Updates the speed of the ship using a value in knots.
     * @param $newSpeed New speed value in knots.
     */
    void setSpeed(const units::velocity::knot_t $newSpeed);
    /**
     * @brief Updates the speed of the ship using a value in meters per second.
     * @param newSpeed New speed value in meters per second.
     */
    void setSpeed(const units::velocity::meters_per_second_t &newSpeed);




    /**
     * @brief Retrieves the area of the immersed part of the transom.
     * @return Immersed transom area in square meters.
     */
    units::area::square_meter_t getImmersedTransomArea() const;
    /**
     * @brief Updates the area of the immersed part of the transom.
     * @param newA_T New immersed transom area value in square meters.
     */
    void setImmersedTransomArea(const units::area::square_meter_t &newA_T);





    /**
     * @brief Sets the resistance strategy used by the
     *          ship for resistance calculations.
     * @param newStrategy Pointer to the new strategy to be used.
     */
    void setResistancePropulsionStrategy(IShipResistancePropulsionStrategy* newStrategy);
    /**
     * @brief Retrieves the current resistance strategy used by the ship.
     * @return Pointer to the current resistance strategy.
     */
    IShipResistancePropulsionStrategy *getResistanceStrategy() const;






//    /**
//     * @brief Retrieves the lengthwise projected area
//     *          of the ship above the waterline.
//     * @return Lengthwise projection area in square meters.
//     */
//    units::area::square_meter_t getLengthwiseProjectionArea() const;
//    /**
//     * @brief Sets the lengthwise projected area of the
//     *          ship above the waterline.
//     * @param newLengthwiseProjectionArea New lengthwise
//     *          projection area value in square meters.
//     */
//    void setLengthwiseProjectionArea(
//        const units::area::square_meter_t &newLengthwiseProjectionArea);

    /**
     * @brief Retrieves the surface roughness of the ship's hull.
     * @return Surface roughness in nanometers.
     */
    units::length::nanometer_t getSurfaceRoughness() const;
    /**
     * @brief Updates the surface roughness of the ship's hull.
     * @param newSurfaceRoughness New surface roughness value in nanometers.
     */
    void setSurfaceRoughness(
        const units::length::nanometer_t &newSurfaceRoughness);

    /**
     * @brief Retrieves the parameter describing the ship's stern shape.
     * @return Stern shape parameter of type CStern.
     */
    CStern getSternShapeParam() const;

    /**
     * @brief Updates the parameter describing the ship's stern shape.
     * @param newC_Stern New stern shape parameter of type CStern.
     */
    void setSternShapeParam(CStern newC_Stern);

    /**
     * @brief Retrieves the run length of the ship.
     * @return Run length in meters.
     */
    units::length::meter_t getRunLength() const;
    /**
     * @brief Sets the run length of the ship.
     * @param newRunLength New run length value in meters.
     */
    void setRunLength(const units::length::meter_t &newRunLength);

    units::length::meter_t getPropellerDiameter() const;
    void setPropellerDiameter(
        const units::length::meter_t &newPropellerDiameter);

    units::area::square_meter_t getExpandedBladeArea() const;
    void setExpandedBladeArea(
        const units::area::square_meter_t &newExpandedBladeArea);

    units::area::square_meter_t getPropellerDiskArea() const;
    void setPropellerDiskArea(
        const units::area::square_meter_t &newPropellerDiskArea);

    double getPropellerExpandedAreaRatio() const;
    void setPropellerExpandedAreaRation(double newPropellerExpandedAreaRation);

    ScrewVesselType getScrewVesselType() const;
    void setScrewVesselType(ScrewVesselType newScrewVesselType);

    units::force::newton_t getTotalThrust() const;

    units::mass::metric_ton_t getVesselWeight() const;
    void setVesselWeight(const units::mass::metric_ton_t &newVesselWeight);

    units::mass::metric_ton_t getCargoWeight() const;
    void setCargoWeight(const units::mass::metric_ton_t &newCargoWeight);

    units::mass::metric_ton_t getTotalVesselWeight() const;

    void addPropeller(IShipPropeller *newPropeller);

    QVector<IShipPropeller*> *propellers();

    QVector<Ship*> *draggedVessels();

    std::vector<std::shared_ptr<Line>> shipPath();
    void setShipPath(std::vector<std::shared_ptr<Line> > &path);

    bool reachedDestination() const;
    void setReachedDestination(bool newReachedDestination);

    bool outOfEnergy() const;
    void setOutOfEnergy(bool newOutOfEnergy);

    bool loaded() const;
    void setLoaded(bool newLoaded);

    std::vector<units::length::meter_t> linksCumLengths() const;
    void setLinksCumLengths(
        const std::vector<units::length::meter_t> &newLinksCumLengths);

    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // ~~~~~~~~~~~~~~~~~~~~~~ Dynamics ~~~~~~~~~~~~~~~~~~~~~~~~~
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    void moveShip(units::time::second_t &timeStep,
                  units::velocity::meters_per_second_t &freeFlowSpeed,
                  QVector<units::length::meter_t> *gapToNextCriticalPoint,
                  QVector<bool> *gapToNextCrticalPointType,
                  QVector<units::velocity::meters_per_second_t> *leaderSpeeds);
private:

    //!< The ship ID
    QString mShipUserID;

    //!< Strategy used for calculating ship resistance.
    IShipResistancePropulsionStrategy* mStrategy;

    //!< Length of the ship at the waterline.
    units::length::meter_t mWaterlineLength;

    //!< Breadth or width of the ship at its widest point.
    units::length::meter_t mBeam;

    /// Average vertical distance between the waterline
    /// and the bottom of the hull.
    units::length::meter_t mMeanDraft;

    /// Vertical distance between the waterline and
    /// the bottom of the hull at the forward part of the ship.
    units::length::meter_t mDraftAtForward;

    /// Vertical distance between the waterline and the bottom
    /// of the hull at the aft or rear part of the ship.
    units::length::meter_t mDraftAtAft;

    //!< Total volume of water displaced by the hull.
    units::volume::cubic_meter_t mVolumetricDisplacement;

    //!< Total surface area of the ship's hull that is submerged.
    units::area::square_meter_t mWettedHullSurface;
    /// The method used for calculating the wet surface area.
    WetSurfaceAreaCalculationMethod mWetSurfaceAreaMethod;

//    /// The area projected by the ship onto a vertical
//    /// plane in the direction of travel.
//    units::area::square_meter_t mLengthwiseProjectionArea;

    /// Vertical position of the center of the bulbous bow's
    /// transverse area, also known as h_B.
    units::length::meter_t mBulbousBowTransverseAreaCenterHeight; //h_B

    /// Map containing the wetted surface areas of various
    /// appendages of the ship.
    QMap<ShipAppendage, units::area::square_meter_t> mAppendagesWettedSurfaces;

    //!< Transverse area of the bulbous bow of the ship.
    units::area::square_meter_t mBulbousBowTransverseArea;

    //!< Area of the ship's transom that is submerged.
    units::area::square_meter_t mImmersedTransomArea;

    //!< Angle at which the waterline meets the ship's hull at the bow.
    units::angle::degree_t mHalfWaterlineEntranceAngle;

    //!< Roughness of the ship's submerged surface.
    units::length::nanometer_t mSurfaceRoughness =
        units::length::nanometer_t(150.0);

    //!< Length of the Run
    units::length::meter_t mRunLength;

    //!< Map of Ship's appendages Areas.
    QVector<QPair<units::area::square_meter_t, double>> App;

    //!< Position along the ship's length where its buoyancy force acts.
    double mLongitudinalBuoyancyCenter;

    //!< Parameter describing the shape of the ship's stern.
    CStern mSternShapeParam;

    /// Coefficient representing the cross-sectional area
    /// of the ship's midship section.
    double mMidshipSectionCoef;

    //!< Coefficient representing the area of the ship's waterline plane.
    double mWaterplaneAreaCoef;
    /// Method of calculating the waterplanecoef
    WaterPlaneCoefficientMethod mWaterplaneCoefMethod;

    //!< Coefficient representing the form or shape of the ship's hull.
    double mPrismaticCoef;

    //!< Coefficient representing the volume efficiency of the ship's hull.
    double mBlockCoef;

    /// Method of calculating the block coef
    BlockCoefficientMethod mBlockCoefMethod;

    /// Resistance due to friction between the ship's
    /// hull and the surrounding water.
    units::force::newton_t mFrictionalResistance;

    //!< Resistance due to ship appendages like rudders, bilge keels, etc.
    units::force::newton_t mAppendageResistance;

    //!< Resistance due to wave formation around the ship.
    units::force::newton_t mWaveResistance;

    //!< Resistance due to the bulbous bow of the ship.
    units::force::newton_t mBulbousBowResistance;

    //!< Resistance due to the submerged part of the ship's transom.
    units::force::newton_t mTransomResistance;

    /// Additional resistance that accounts for discrepancies
    /// in theoretical and actual resistances.
    units::force::newton_t mCorrelationAllowanceResistance;

    //!< Resistance due to the ship's superstructure interaction with air.
    units::force::newton_t mAirResistance;

    units::length::meter_t mPropellerDiameter;

    units::area::square_meter_t mExpandedBladeArea;

    units::area::square_meter_t mPropellerDiskArea;

    double mPropellerExpandedAreaRation;

    ScrewVesselType mScrewVesselType;

    //!< Total resistance faced by the ship.
    units::force::newton_t mTotalResistance;

    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // ~~~~~~~~~~~~~~~~ Dynamics Variables ~~~~~~~~~~~~~~~~~~~~
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    units::jerk::meters_per_second_cubed_t mMaxJerk =
        units::jerk::meters_per_second_cubed_t(2.0);

    units::acceleration::meters_per_second_squared_t mAcceleration;

    units::acceleration::meters_per_second_squared_t mPreviousAcceleration;

    units::velocity::meters_per_second_t mMaxSpeed;
    //!< Current speed of the ship.
    units::velocity::meters_per_second_t mSpeed;

    units::velocity::meters_per_second_t mPreviousSpeed;

    //!< The ship traveled distance
    units::length::meter_t mTraveledDistance;

    units::mass::metric_ton_t mVesselWeight;

    units::mass::metric_ton_t mCargoWeight;

    units::mass::metric_ton_t mAddedWeight;

    bool mStopIfNoEnergy;

    bool mIsOn;

    bool mOffLoaded;

    bool mReachedDestination;

    bool mOutOfEnergy;

    bool mLoaded;
    std::vector<units::length::meter_t> mLinksCumLengths;
    Point mCurrentCoordinates;
    Point mStartCoordinates;


    QVector<IShipPropeller*> mPropellers;
    QVector<Ship*> mDraggedVessels;
    Battery *mBattery;
    Tank *mTank;
    std::vector<std::shared_ptr<Line>> mPath;
    std::shared_ptr<Line> currentLink;

    bool mShowNoPowerMessage;
    units::time::second_t mT_s = units::time::second_t(2.0);

    units::acceleration::meters_per_second_squared_t mD_des =
        units::acceleration::meters_per_second_squared_t(0.2);





    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // ~~~~~~~~~~~~~~~~~~~ Ship Statistics ~~~~~~~~~~~~~~~~~~~~~
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    //!< The ship total trip distance since loaded
    units::time::second_t mTripTime;



    /**
     * @brief Calculates the wet surface area of the ship.
     *
     * @param method The method to use for calculation. Default is Holtrop.
     * @return The calculated wet surface area.
     */
    units::area::square_meter_t calc_WetSurfaceArea(
        WetSurfaceAreaCalculationMethod method =
        WetSurfaceAreaCalculationMethod::Holtrop
        ) const;

    /**
     * @brief Calculates the block coefficient of the
     * ship given the volumetric displacement.
     * @return The block coefficient.
     */
    double calc_BlockCoefFromVolumetricDisplacement() const;

    /**
     * @brief Calculates the block coefficient of the ship.
     * @param method The specific calculation method to use;
     *              defaults to Ayre's method.
     * @return The block coefficient.
     */
    double calc_BlockCoef(BlockCoefficientMethod method =
                    BlockCoefficientMethod::Ayre) const;

    /**
     * @brief calculates  the Midshi section coefficient
     * if block coef and prismatic coef are defined
     *
     * @return The midship section coefficient
     */
    double calc_MidshiSectionCoef();

    /**
     * @brief Calculates the Length of the Run of the Ship.
     * @return The run length of the ship.
     */
    units::length::meter_t calc_RunLength() const;

    /**
     * @brief Calculates the entrance half-angle at the ship's waterline.
     * @return The half entrance angle.
     */
    units::angle::degree_t calc_i_E() const;

    /**
     * @brief Calculates the wetted surface area of the
     *          ship using Holtrop's method.
     * @return The wetted surface area.
     */
    units::area::square_meter_t calc_WetSurfaceAreaToHoltrop() const;

    /**
     * @brief Calculates the wetted surface area of the
     *          ship using Schenzle's method.
     * @return The wetted surface area.
     */
    units::area::square_meter_t calc_WetSurfaceAreaToSchenzle() const;

    /**
     * @brief Calculates the total volume of water displaced
     *          by the ship's weight.
     * @return The volumetric displacement of the ship.
     */
    units::volume::cubic_meter_t calc_VolumetricDisplacementByWeight() const;

    /**
     * @brief Calculates the total volume of water displaced
     *          by the ship's hull.
     * @return The volumetric displacement of the ship.
     */
    units::volume::cubic_meter_t calc_VolumetricDisplacement() const;

    /**
     * @brief Calculates the waterplane area coefficient of the ship.
     * @param method The specific calculation method to use; defaults
     *          to the General Cargo method.
     * @return The waterplane area coefficient.
     */
    double calc_WaterplaneAreaCoef(WaterPlaneCoefficientMethod method =
                     WaterPlaneCoefficientMethod::General_Cargo) const;

    /**
     * @brief Calculates the prismatic coefficient of the ship.
     * @return The prismatic coefficient.
     */
    double calc_PrismaticCoef() const;

    units::mass::metric_ton_t calc_addedWeight() const;



    /**
     * @brief Checks the assumptions of the selected resistance strategy.
     *
     * Validates if the current ship's properties align with the assumptions
     * made by the selected resistance strategy.
     *
     * @param strategy The resistance strategy to check against.
     * @return true if the strategy's assumptions are met, false otherwise.
     */
    bool checkSelectedMethodAssumptions(
        IShipResistancePropulsionStrategy* strategy);

    void initializeDefaults();

    units::force::newton_t calc_Torque();



    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // ~~~~~~~~~~~~~~~~~~~~ Dynamics ~~~~~~~~~~~~~~~~~~~~~
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    units::acceleration::meters_per_second_squared_t
    calc_maxAcceleration() const;

    units::acceleration::meters_per_second_squared_t
    calc_decelerationAtSpeed(
        const units::velocity::meters_per_second_t customSpeed) const;

    double getHyperbolicThrottleCoef(
        const units::velocity::meters_per_second_t &shipSpeed) const;

    units::length::meter_t getSafeGap(
        const units::length::meter_t initialGap,
        const units::velocity::meters_per_second_t speed,
        const units::velocity::meters_per_second_t freeFlowSpeed,
        const units::time::second_t T_s, bool estimate);

    units::velocity::meters_per_second_t
    getNextTimeStepSpeed(
        const units::length::meter_t gap,
        const units::length::meter_t minGap,
        const units::velocity::meters_per_second_t speed,
        const units::velocity::meters_per_second_t freeFlowSpeed,
        const units::acceleration::meters_per_second_squared_t aMax,
        const units::time::second_t T_s,
        const units::time::second_t deltaT);

    units::time::second_t getTimeToCollision(
        const units::length::meter_t gap,
        const units::length::meter_t minGap,
        const units::velocity::meters_per_second_t speed,
        const units::velocity::meters_per_second_t leaderSpeed);

    units::acceleration::meters_per_second_squared_t
    get_acceleration_an11(
        const units::velocity::meters_per_second_t u_hat,
        const units::velocity::meters_per_second_t speed,
        const units::time::second_t TTC_s);

    units::acceleration::meters_per_second_squared_t
    get_acceleration_an12(
        const units::velocity::meters_per_second_t &u_hat,
        const units::velocity::meters_per_second_t &speed,
        const units::time::second_t &T_s,
        const units::acceleration::meters_per_second_squared_t &amax);

    double get_beta1(
        const units::acceleration::meters_per_second_squared_t &an11);

    units::acceleration::meters_per_second_squared_t
    get_acceleration_an13(double beta1,
        const units::acceleration::meters_per_second_squared_t &an11,
        const units::acceleration::meters_per_second_squared_t &an12);

    units::acceleration::meters_per_second_squared_t
    get_acceleration_an14(
        const units::velocity::meters_per_second_t &speed,
        const units::velocity::meters_per_second_t &leaderSpeed,
        const units::time::second_t &T_s,
        const units::acceleration::meters_per_second_squared_t &amax);

    double get_beta2();

    units::acceleration::meters_per_second_squared_t
    get_acceleration_an1(double beta2,
        const units::acceleration::meters_per_second_squared_t &an13,
        const units::acceleration::meters_per_second_squared_t &an14);

    double get_gamma(
        const units::velocity::meters_per_second_t &speedDiff);

    units::acceleration::meters_per_second_squared_t
    get_acceleration_an2(
        const units::length::meter_t &gap,
        const units::length::meter_t &minGap,
        const units::velocity::meters_per_second_t &speed,
        const units::velocity::meters_per_second_t &leaderSpeed,
        const units::time::second_t &T_s);

    units::acceleration::meters_per_second_squared_t
    accelerate(
        const units::length::meter_t &gap,
        const units::length::meter_t &mingap,
        const units::velocity::meters_per_second_t &speed,
        const units::acceleration::meters_per_second_squared_t &acceleration,
        const units::velocity::meters_per_second_t &leaderSpeed,
        const units::velocity::meters_per_second_t &freeFlowSpeed,
        const units::time::second_t &deltaT);

    units::acceleration::meters_per_second_squared_t
    getStepAcceleration(units::time::second_t &timeStep,
        units::velocity::meters_per_second_t &freeFlowSpeed,
        QVector<units::length::meter_t> *gapToNextCriticalPoint,
        QVector<bool> *gapToNextCrticalPointType,
        QVector<units::velocity::meters_per_second_t> *leaderSpeeds);

    units::acceleration::meters_per_second_squared_t
    accelerateConsideringJerk(units::acceleration::meters_per_second_squared_t &acceleration,
        units::acceleration::meters_per_second_squared_t &previousAcceleration,
        units::jerk::meters_per_second_cubed_t &jerk,
        units::time::second_t &deltaT );

    units::acceleration::meters_per_second_squared_t
    smoothAccelerate(units::acceleration::meters_per_second_squared_t &acceleration,
        units::acceleration::meters_per_second_squared_t &previousAccelerationValue,
        double &alpha);

    units::velocity::meters_per_second_t
    speedUpDown(units::velocity::meters_per_second_t &previousSpeed,
        units::acceleration::meters_per_second_squared_t &acceleration,
        units::time::second_t &deltaT,
        units::velocity::meters_per_second_t &freeFlowSpeed);

    units::acceleration::meters_per_second_squared_t
    adjustAcceleration(units::velocity::meters_per_second_t &speed,
        units::velocity::meters_per_second_t &previousSpeed,
        units::time::second_t &deltaT);

    bool checkSuddenAccChange(units::acceleration::meters_per_second_squared_t &previousAcceleration,
        units::acceleration::meters_per_second_squared_t &currentAcceleration,
        units::time::second_t &deltaT);

    void immediateStop(units::time::second_t &timestep);

    void kickForwardADistance(units::length::meter_t &distance);

signals:

    /**
     * @brief report a sudden acceleration.
     * @details this is emitted when the train's acceleration is larger
     * than the jerk
     * @param msg is the warning message
     */
    void suddenAccelerationOccurred(std::string msg);

    /**
     * @brief report the ship is very slow or stopped
     * @details this is emitted when the ship's speed is very
     * slow either because the resistance is high or because the
     * speed of the ship is very small
     * @param msg
     */
    void slowSpeedOrStopped(std::string msg);
};

#endif // SHIP_H
