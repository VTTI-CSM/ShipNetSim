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
#include "../network/gline.h"
#include "../network/galgebraicvector.h"

// Forward declaration of the cal water resistance Interface
class IShipCalmResistanceStrategy;
//Forward declaration of the dynamic resistance interface
class IShipDynamicResistanceStrategy;

/**
 * @class Ship
 * @brief Defines a ship and provides methods to
 * compute its properties and resistances.
 */
class Ship : public QObject
{
    Q_OBJECT
public:
    struct stopPointDefinition
    {
        qsizetype pointIndex;
        std::shared_ptr<GPoint> point;
    };

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


    Ship(const QMap<QString, std::any>& parameters,
         QObject* parent = nullptr);

    /**
     * @brief Destructor for the Ship class.
     *
     * Destroys the Ship object and any associated resources.
     */
    ~Ship();


    [[nodiscard]] QString getUserID();

    /**
     * @brief Calculates the total resistance experienced by the ship.
     * @param customSpeed   The custom speed to calculate resistance at.
     * @return Total resistance in kilonewtons.
     */
    [[nodiscard]] units::force::newton_t
    calculateTotalResistance(units::velocity::meters_per_second_t customSpeed =
                             units::velocity::meters_per_second_t(
                                 std::nan("unintialized"))) const;


    // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
    // %%%%%%%%%%%%%%%%%%%%% GETTERS & SETTERS %%%%%%%%%%%%%%%%%%%%%
    // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

    /**
     * @brief Retrieves the length of the ship at the waterline.
     * @return Length in meters.
     */
    [[nodiscard]] units::length::meter_t getLengthInWaterline() const;
    /**
     * @brief Updates the length of the ship at the waterline.
     * @param newL New length to set in meters.
     */
    void setLengthInWaterline(const units::length::meter_t &newL);

    /**
     * @brief Retrieves the length between perpendiculars of the ship.
     * @return Length in meters.
     */
    [[nodiscard]] units::length::meter_t getLengthBetweenPerpendiculars() const;
    /**
     * @brief Updates the length between perpendiculars of the ship.
     * @param newL New length to set in meters.
     */
    void setLengthBetweenPerpendiculars(const units::length::meter_t &newL);

    /**
     * @brief Retrieves the breadth (width) of the ship.
     * @return Breadth in meters.
     */
    [[nodiscard]] units::length::meter_t getBeam() const;
    /**
     * @brief Updates the breadth of the ship.
     * @param newB New breadth to set in meters.
     */
    void setBeam(const units::length::meter_t &newB);

    /**
     * @brief Retrieves the average depth of the ship submerged in water.
     * @return Mean draft in meters.
     */
    [[nodiscard]] units::length::meter_t getMeanDraft() const;
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
    [[nodiscard]] units::length::meter_t getDraftAtAft() const;
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
    [[nodiscard]] units::length::meter_t getDraftAtForward() const;
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
    [[nodiscard]] units::volume::cubic_meter_t
    getVolumetricDisplacement() const;
    /**
     * @brief Updates the volume of water displaced by the ship.
     * @param newNab New volumetric displacement to set in cubic meters.
     */
    void setVolumetricDisplacement(const units::volume::cubic_meter_t &newNab);

    /**
     * @brief Retrieves the block coefficient of the ship.
     * @return Block coefficient.
     */
    [[nodiscard]] double getBlockCoef() const;
    /**
     * @brief Updates the block coefficient of the ship.
     * @param newC_B New block coefficient to set.
     */
    void setBlockCoef(const double newC_B);

    /**
     * @brief Retrieves the prismatic coefficient of the ship.
     * @return Prismatic coefficient.
     */
    [[nodiscard]] double getPrismaticCoef() const;
    /**
     * @brief Updates the prismatic coefficient of the ship.
     * @param newC_P New prismatic coefficient value.
     */
    void setPrismaticCoef(const double newC_P);

    /**
     * @brief Retrieves the midship section coefficient of the ship.
     * @return Midship section coefficient.
     */
    [[nodiscard]] double getMidshipSectionCoef() const;
    /**
     * @brief Updates the midship section coefficient of the ship.
     * @param newC_M New midship section coefficient value.
     */
    void setMidshipSectionCoef(const double newC_M);

    /**
     * @brief Retrieves the waterplane area coefficient of the ship.
     * @return Waterplane area coefficient.
     */
    [[nodiscard]] double getWaterplaneAreaCoef() const;
    /**
     * @brief Updates the waterplane area coefficient of the ship.
     * @param newC_WP New waterplane area coefficient value.
     */
    void setWaterplaneAreaCoef(const double newC_WP);

    /**
     * @brief Retrieves the longitudinal buoyancy center of the ship.
     * @return Longitudinal buoyancy center.
     */
    [[nodiscard]] double getLongitudinalBuoyancyCenter() const;
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
    [[nodiscard]] units::area::square_meter_t getWettedHullSurface() const;
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
    [[nodiscard]] units::length::meter_t
    getBulbousBowTransverseAreaCenterHeight() const;

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
    [[nodiscard]] QMap<ShipAppendage, units::area::square_meter_t>
    getAppendagesWettedSurfaces() const;
    /**
     * @brief Retrieves the total wetted surface area of all ship appendages.
     * @return Total wetted surface area in square meters.
     */
    [[nodiscard]] units::area::square_meter_t
    getTotalAppendagesWettedSurfaces() const;
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
    [[nodiscard]] units::area::square_meter_t
    getBulbousBowTransverseArea() const;
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
    [[nodiscard]] units::angle::degree_t
    getHalfWaterlineEntranceAngle() const;
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
    [[nodiscard]] units::velocity::meters_per_second_t getSpeed() const;
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
    [[nodiscard]] units::area::square_meter_t
    getImmersedTransomArea() const;
    /**
     * @brief Updates the area of the immersed part of the transom.
     * @param newA_T New immersed transom area value in square meters.
     */
    void setImmersedTransomArea(const units::area::square_meter_t &newA_T);





    /**
     * @brief Sets the calm resistance strategy used by the
     *          ship for resistance calculations.
     * @param newStrategy Pointer to the new strategy to be used.
     */
    void setCalmResistanceStrategy(
        IShipCalmResistanceStrategy* newStrategy);
    /**
     * @brief Retrieves the current calm resistance strategy used by the ship.
     * @return Pointer to the current resistance strategy.
     */
    [[nodiscard]] IShipCalmResistanceStrategy
        *getCalmResistanceStrategy() const;

    /**
     * @brief Sets the dynamic resistance strategy used by the
     *          ship for resistance calculations.
     * @param newStrategy Pointer to the new strategy to be used.
     */
    void setDynamicResistanceStrategy(
        IShipDynamicResistanceStrategy* newStrategy);

    /**
     * @brief Retrieves the current dynamic resistance
     *          strategy used by the ship.
     * @return Pointer to the current resistance strategy.
     */
    [[nodiscard]] IShipDynamicResistanceStrategy
        *getDynamicResistanceStrategy() const;


    /**
     * @brief get the main tank capacity state of the ship.
     * @return the capacity state in percentage.
     */
    [[nodiscard]] double getMainTankCurrentCapacity();


   /**
    * @brief Retrieves the lengthwise projected area
    *          of the ship above the waterline.
    * @return Lengthwise projection area in square meters.
    */
   units::area::square_meter_t getLengthwiseProjectionArea() const;
   /**
    * @brief Sets the lengthwise projected area of the
    *          ship above the waterline.
    * @param newLengthwiseProjectionArea New lengthwise
    *          projection area value in square meters.
    */
   void setLengthwiseProjectionArea(
       const units::area::square_meter_t &newLengthwiseProjectionArea);

    /**
     * @brief Retrieves the surface roughness of the ship's hull.
     * @return Surface roughness in nanometers.
     */
    [[nodiscard]] units::length::nanometer_t getSurfaceRoughness() const;
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
    [[nodiscard]] CStern getSternShapeParam() const;

    /**
     * @brief Updates the parameter describing the ship's stern shape.
     * @param newC_Stern New stern shape parameter of type CStern.
     */
    void setSternShapeParam(CStern newC_Stern);

    /**
     * @brief Retrieves the run length of the ship.
     * @return Run length in meters.
     */
    [[nodiscard]] units::length::meter_t getRunLength() const;
    /**
     * @brief Sets the run length of the ship.
     * @param newRunLength New run length value in meters.
     */
    void setRunLength(const units::length::meter_t &newRunLength);

    [[nodiscard]] units::force::newton_t getTotalThrust() const;

    [[nodiscard]] units::mass::metric_ton_t getVesselWeight() const;
    void setVesselWeight(const units::mass::metric_ton_t &newVesselWeight);

    [[nodiscard]] units::mass::metric_ton_t getCargoWeight() const;
    void setCargoWeight(const units::mass::metric_ton_t &newCargoWeight);

    units::mass::metric_ton_t calc_SurgeAddedMass() const;

    [[nodiscard]] units::mass::metric_ton_t getTotalVesselWeight() const;

    void addPropeller(IShipPropeller *newPropeller);

    [[nodiscard]] const QVector<IShipPropeller*> *getPropellers() const;

    [[nodiscard]] QVector<Ship*> *draggedVessels();

    [[nodiscard]] QVector<std::shared_ptr<GLine>>* getShipPathLines();
    [[nodiscard]] QVector<std::shared_ptr<GPoint>>* getShipPathPoints();
    void setPath(const QVector<std::shared_ptr<GPoint>> points,
                 const QVector<std::shared_ptr<GLine>> lines);


    [[nodiscard]] bool isReachedDestination() const;

    [[nodiscard]] bool isOutOfEnergy() const;

    [[nodiscard]] bool isLoaded() const;
    void load();
    void unload();

    [[nodiscard]] std::shared_ptr<GPoint> startPoint();
    void setStartPoint(std::shared_ptr<GPoint> startPoint);

    [[nodiscard]] std::shared_ptr<GPoint> endPoint();
    void setEndPoint(std::shared_ptr<GPoint> endPoint);

    [[nodiscard]] GPoint getCurrentPosition();
    [[nodiscard]] units::angle::degree_t getCurrentHeading() const;
    [[nodiscard]] GPoint getCurrentTarget();
    [[nodiscard]] AlgebraicVector::Environment getCurrentEnvironment() const;
    void setCurrentEnvironment(const AlgebraicVector::Environment newEnv);
//    void setCurrentPosition(const Point& point);

    [[nodiscard]] QVector<units::length::meter_t> getLinksCumLengths() const;
    [[nodiscard]] units::time::second_t getStartTime() const;
    void setStartTime(const units::time::second_t &newStartTime);


    void addToCummulativeConsumedEnergy(
        units::energy::kilowatt_hour_t consumedkWh);
    units::energy::kilowatt_hour_t getCumConsumedEnergy();

    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // ~~~~~~~~~~~~~~~~~~~~~~ Dynamics ~~~~~~~~~~~~~~~~~~~~~~~~~
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    void moveShip(units::time::second_t &timeStep,
                  units::velocity::meters_per_second_t &freeFlowSpeed,
                  QVector<units::length::meter_t> &gapToNextCriticalPoint,
                  QVector<bool> &isFollowingAnotherShip,
                  QVector<units::velocity::meters_per_second_t> &leaderSpeeds,
                  AlgebraicVector::Environment currentEnvironment);

    [[nodiscard]] units::length::meter_t getTraveledDistance() const;
    [[nodiscard]] units::length::meter_t getTotalPathLength() const;

    void reset();

    [[nodiscard]] size_t getPreviousPathPointIndex() const;
    [[nodiscard]] stopPointDefinition getNextStoppingPoint();
    units::length::meter_t distanceToFinishFromPathNodeIndex(qsizetype i);
    [[nodiscard]] units::length::meter_t
    distanceToNodePathIndexFromPathNodeIndex(qsizetype startIndex,
                                             qsizetype endIndex);
    [[nodiscard]] units::length::meter_t
    distanceFromCurrentPositionToNodePathIndex(qsizetype endIndex);

    [[nodiscard]] bool isShipOnCorrectPath();
    [[nodiscard]] double progress();

    // [[nodiscard]] units::velocity::meters_per_second_t getCurrentMaxSpeed();
    // [[nodiscard]] QHash<qsizetype, units::velocity::meters_per_second_t>
    // getAheadLowerSpeeds(qsizetype nextStopIndex);

    void kickForwardADistance(units::length::meter_t &distance,
                              units::time::second_t timeStep);

    [[nodiscard]] units::acceleration::meters_per_second_squared_t
    getAcceleration() const;

//    [[nodiscard]] units::velocity::meters_per_second_t getSpeed();
    [[nodiscard]] units::velocity::meters_per_second_t getPreviousSpeed() const;
    [[nodiscard]] units::velocity::meters_per_second_t getMaxSpeed() const;


private:

    //!< The ship ID
    QString mShipUserID;

    //!< Strategy used for calculating ship calm water resistance.
    IShipCalmResistanceStrategy* mCalmResistanceStrategy;

    IShipDynamicResistanceStrategy* mDynamicResistanceStrategy;

    //!< Length of the ship at the waterline.
    units::length::meter_t mWaterlineLength;

    //!< Length between the perpendiculars of the ship
    units::length::meter_t mLengthBetweenPerpendiculars;

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

   //!< The area projected by the ship onto a vertical
   //! plane in the direction of travel.
   units::area::square_meter_t mLengthwiseProjectionArea;

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
    QVector<QPair<units::area::square_meter_t, double>> App; //TODO: init

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
    mutable BlockCoefficientMethod mBlockCoefMethod;

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

    //!< Total resistance faced by the ship.
    units::force::newton_t mTotalResistance;

    //!< the start time this vessel enters the simulation
    units::time::second_t mStartTime;

    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // ~~~~~~~~~~~~~~~~ Dynamics Variables ~~~~~~~~~~~~~~~~~~~~
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    //!< The max jerk the vessel can change in acceleration
    units::jerk::meters_per_second_cubed_t mMaxJerk =
        units::jerk::meters_per_second_cubed_t(2.0);

    //!< The current acceleration of the vessel
    units::acceleration::meters_per_second_squared_t mAcceleration;

    //!< The previous acceleration of the vessel
    units::acceleration::meters_per_second_squared_t mPreviousAcceleration;

    //!< The max speed the vessel can go by.
    //! this is required for the model to work.
    //! if the vessel is a cargo ship, 25 knots is a typical value.
    units::velocity::meters_per_second_t mMaxSpeed;

    //!< Current speed of the ship.
    units::velocity::meters_per_second_t mSpeed;

    //!< The previous speed of the vessel.
    units::velocity::meters_per_second_t mPreviousSpeed;

    //!< The ship traveled distance.
    units::length::meter_t mTraveledDistance;

    //!< The lightship weight (LWT) of the vessel.
    units::mass::metric_ton_t mVesselWeight;

    //!< The weight of the cargo only on the ship.
    units::mass::metric_ton_t mCargoWeight;

    // //!< Any extra weight that is not counted in either
    // //! the VesselWeight or the CargoWeight.
    // units::mass::metric_ton_t mAddedWeight;

    //!< A bool to indicate the ship should stop
    //! completely when it has no power.
    bool mStopIfNoEnergy = false;

    //!< A bool to indicate the ship is still running.
    //! The ship stops running when there is no power.
    bool mIsOn = true;

    //!< A bool to indicate the ship has been offloaded
    //! from the simulator.
    bool mOffLoaded = false;

    //!< A bool to indicate the ship has reached its destination.
    bool mReachedDestination = false;

    //!< A bool to indicate the ship is out of energy.
    bool mOutOfEnergy = false;

    //!< A bool to indicate the ship is loaded to the simulator.
    bool mLoaded = false;

    //!< A vector to hold cum lengths of the links.
    //! This variable is used to find the location of
    //! the ship (macroscopically).
    QVector<units::length::meter_t> mLinksCumLengths;

    //!< Holds the total length the ship is estimated to traverse.
    units::length::meter_t mTotalPathLength;

    //!< Holds the current position and course
    GAlgebraicVector mCurrentState;

    //!< the initial start point of the ship on its path
    std::shared_ptr<GPoint> mStartCoordinates;

    //!< the last point on the ship path
    std::shared_ptr<GPoint> mEndCoordinates;

    //!< The main energy source that is feeding the main
    //! engine for the propulsion system.
    std::shared_ptr<IEnergySource> mMainEnergySource;

    //!< Holds all the propellers the ship has
    QVector<IShipPropeller*> mPropellers;

    //!< Holds all ships the current ship is dragging.
    //! If the dragged ships have propulsion system active,
    //! they will move forward but not directed.
    QVector<Ship*> mDraggedVessels;

    //!< The Lines connecting path points.
    QVector<std::shared_ptr<GLine>> mPathLines;

    //!< The points on the path that define the path of the vessel.
    QVector<std::shared_ptr<GPoint>> mPathPoints;

    ///* a vector that has indices of port points
    QVector<qsizetype> mStoppingPointIndices;

    //!< Holds the current link.
    std::shared_ptr<GLine> mCurrentLink;

    //!< A boolean to disable showing warning messages for no power.
    bool mShowNoPowerMessage;

    //!< Holds the reaction time.
    units::time::second_t mT_s = units::time::second_t(10.0);

    //!< Holds the max deceleration level.
    //! This is the upper bound of the decceleration.
    //! Refer to calc_decelerationAtSpeed().
    units::acceleration::meters_per_second_squared_t mD_des =
        units::acceleration::meters_per_second_squared_t(0.2);

    //!< The max angle the rudder can turn by
    units::angle::degree_t mRudderAngle = units::angle::degree_t(25.0);

    bool mBrakingThrustAvailable = true;

    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // ~~~~~~~~~~~~~~~~~~ Ship Memorization ~~~~~~~~~~~~~~~~~~~
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    //!< Holds the previous traversed point index.
    //! This is important as a memorization variable
    //! to make the search faster.
    qsizetype mPreviousPathPointIndex;

    //!< Hold the node index of the point (and its lower speed)
    //!  where the following link has lower speed than the current
    //! link speed of the ship.
    QVector<QVector<QHash<qsizetype,
                          units::velocity::meters_per_second_t>>>
        mLowerSpeedLinkIndex;

    QMap<units::velocity::meters_per_second_t,
         units::length::meter_t> mGapCache;

    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // ~~~~~~~~~~~~~~~~~~~ Ship Statistics ~~~~~~~~~~~~~~~~~~~~~
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    //!< The ship total trip distance since loaded
    units::time::second_t mTripTime;

    //!< The cum energy consuption to current time.
    units::energy::kilowatt_hour_t mCumConsumedEnergy =
        units::energy::kilowatt_hour_t(0.0);



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

    double calc_SimpleBlockCoef() const;

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
        IShipCalmResistanceStrategy* strategy);

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
    double calc_frictionCoef(
        const units::velocity::meters_per_second_t customSpeed) const;

    units::length::meter_t getSafeGap(
        const units::length::meter_t initialGap,
        const units::velocity::meters_per_second_t speed,
        const units::velocity::meters_per_second_t freeFlowSpeed,
        const units::time::second_t T_s,
        const units::time::second_t timeStep, bool estimate);

    /**
     * @brief Get the next time step prediction of the speed
     * @param gap is the gap between the front tip of the ship
     *              and the next object or back of ship
     * @param minGap is the clearance between the two objects if
     *              the ships to stop behind the leading object
     * @param speed is the speed of the ship
     * @param freeFlowSpeed is the max speed allowed
     * @param aMax is the maximum acceleration
     * @param T_s is the reaction perception time
     * @param deltaT is the simulator time step
     * @return
     */
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
        QVector<units::length::meter_t> &gapToNextCriticalPoint,
        QVector<bool> &isFollowingAnotherShip,
        QVector<units::velocity::meters_per_second_t> &leaderSpeeds);

    units::acceleration::meters_per_second_squared_t
    accelerateConsideringJerk(
        units::acceleration::meters_per_second_squared_t &acceleration,
        units::acceleration::meters_per_second_squared_t &previousAcceleration,
        units::jerk::meters_per_second_cubed_t &jerk,
        units::time::second_t &deltaT );

    units::acceleration::meters_per_second_squared_t
    smoothAccelerate(
        units::acceleration::meters_per_second_squared_t &acceleration,
        units::acceleration::
        meters_per_second_squared_t &previousAccelerationValue,
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

    bool checkSuddenAccChange(
        units::acceleration::meters_per_second_squared_t &previousAcceleration,
        units::acceleration::meters_per_second_squared_t &currentAcceleration,
        units::time::second_t &deltaT);

    void immediateStop(units::time::second_t &timestep);



    QVector<units::length::meter_t> generateCumLinesLengths();

    void setStepTravelledDistance(units::length::meter_t distance,
                                  units::time::second_t timeStep);
    // Point getPositionByTravelledDistance(
    //     units::length::meter_t newTotalDistance);

    units::length::meter_t calcTurningRadius();
    units::angle::degree_t calcMaxROT(units::length::meter_t turnRaduis);

    units::velocity::meters_per_second_t
    calc_shallowWaterSpeedReduction(
            units::velocity::meters_per_second_t speed);
    void computeStoppingPointIndices();

signals:

    /**
     * @brief report a sudden acceleration.
     * @details this is emitted when the ship's acceleration is larger
     * than the jerk
     * @param msg is the warning message
     */
    void suddenAccelerationOccurred(QString msg);

    /**
     * @brief report the ship is very slow or stopped
     * @details this is emitted when the ship's speed is very
     * slow either because the resistance is high or because the
     * speed of the ship is very small
     * @param msg
     */
    void slowSpeedOrStopped(QString msg);

private: signals:
    void stepDistanceChanged(units::length::meter_t newDistance,
                             units::time::second_t timeStep);
    void pathDeviation(QString msg);

private slots:
    void handleStepDistanceChanged(units::length::meter_t newTotalDistance,
                                   units::time::second_t timeStep);
};

#endif // SHIP_H
