/**
 * @file Ship.h
 * @author Ahmed Aredah
 * @date 8/17/2023
 * @brief Represents a ship in the simulation, managing its properties,
 * movement, and energy consumption.
 *
 * The Ship class is responsible for calculating resistances, managing
 * propellers, and simulating the ship's movement along a predefined path.
 * It also handles the ship's energy consumption and other dynamic
 * properties such as acceleration and speed.
 */

#ifndef SHIP_H
#define SHIP_H

#include "../export.h"
#include <iostream>
#include <any>
#include "../../third_party/units/units.h"
#include "battery.h"
#include "ishippropeller.h"
#include "tank.h"
#include <QObject>
#include <QMap>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include "../network/gline.h"
#include "../network/galgebraicvector.h"

#ifdef BUILD_SERVER_ENABLED
#include "containermap.h"
#endif

namespace ShipNetSimCore
{

// Forward declaration of the cal water resistance Interface
class IShipCalmResistanceStrategy;
//Forward declaration of the dynamic resistance interface
class IShipDynamicResistanceStrategy;

/**
 * @class Ship
 * @brief Defines a ship and provides methods to
 * compute its properties and resistances.
 */
class SHIPNETSIM_EXPORT Ship : public QObject
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

    static QString toString(ShipAppendage appendage) {
        static const QMap<ShipAppendage, QString> appendageToString = {
            { ShipAppendage::rudder_behind_skeg, "Rudder Behind Skeg" },
            { ShipAppendage::rudder_behind_stern, "Rudder Behind Stern" },
            { ShipAppendage::slender_twin_screw_rudder,
             "Slender Twin Screw Rudder" },
            { ShipAppendage::thick_twin_screw_rudder,
             "Thick Twin Screw Rudder" },
            { ShipAppendage::shaft_brackets, "Shaft Brackets" },
            { ShipAppendage::skeg, "Skeg" },
            { ShipAppendage::strut_bossing, "Strut Bossing" },
            { ShipAppendage::hull_bossing, "Hull Bossing" },
            { ShipAppendage::exposed_shafts_10_degree_with_buttocks,
             "Exposed Shafts 10 Degree with Buttocks" },
            { ShipAppendage::exposed_shafts_20_degree_with_buttocks,
             "Exposed Shafts 20 Degree with Buttocks" },
            { ShipAppendage::stabilizer_fins, "Stabilizer Fins" },
            { ShipAppendage::dome, "Dome" },
            { ShipAppendage::bilge_keels, "Bilge Keels" }
        };
        return appendageToString.value(appendage, "Unknown");
    }

    static ShipAppendage toEnumShipAppendage(const QString &appendageString) {
        static const QMap<QString, ShipAppendage> stringToAppendage = {
            { "Rudder Behind Skeg", ShipAppendage::rudder_behind_skeg },
            { "Rudder Behind Stern", ShipAppendage::rudder_behind_stern },
            { "Slender Twin Screw Rudder",
             ShipAppendage::slender_twin_screw_rudder },
            { "Thick Twin Screw Rudder",
             ShipAppendage::thick_twin_screw_rudder },
            { "Shaft Brackets", ShipAppendage::shaft_brackets },
            { "Skeg", ShipAppendage::skeg },
            { "Strut Bossing", ShipAppendage::strut_bossing },
            { "Hull Bossing", ShipAppendage::hull_bossing },
            { "Exposed Shafts 10 Degree with Buttocks",
             ShipAppendage::exposed_shafts_10_degree_with_buttocks },
            { "Exposed Shafts 20 Degree with Buttocks",
             ShipAppendage::exposed_shafts_20_degree_with_buttocks },
            { "Stabilizer Fins", ShipAppendage::stabilizer_fins },
            { "Dome", ShipAppendage::dome },
            { "Bilge Keels", ShipAppendage::bilge_keels }
        };
        return stringToAppendage.value(appendageString,
                                       ShipAppendage::rudder_behind_skeg); // Default to rudder_behind_skeg if not found
    }

    static QStringList getAllAppendageTypes() {
        return QStringList{
            "Rudder Behind Skeg", "Rudder Behind Stern",
            "Slender Twin Screw Rudder",
            "Thick Twin Screw Rudder", "Shaft Brackets",
            "Skeg", "Strut Bossing",
            "Hull Bossing", "Exposed Shafts 10 Degree with Buttocks",
            "Exposed Shafts 20 Degree with Buttocks", "Stabilizer Fins",
            "Dome", "Bilge Keels"
        };
    }


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

    static QString toString(CStern stern) {
        static const QMap<CStern, QString> sternToString = {
            { CStern::None, "None" },
            { CStern::pramWithGondola, "Pram with Gondola" },
            { CStern::VShapedSections, "V-Shaped Sections" },
            { CStern::NormalSections, "Normal Sections" },
            { CStern::UShapedSections, "U-Shaped Sections" }
        };
        return sternToString.value(stern, "Unknown");
    }

    static CStern toEnum(const QString &sternString) {
        static const QMap<QString, CStern> stringToStern = {
            { "None", CStern::None },
            { "Pram with Gondola", CStern::pramWithGondola },
            { "V-Shaped Sections", CStern::VShapedSections },
            { "Normal Sections", CStern::NormalSections },
            { "U-Shaped Sections", CStern::UShapedSections }
        };
        return stringToStern.value(sternString, CStern::None); // Default to CStern::None if not found
    }

    static QStringList getAllSternTypes() {
        return QStringList{"Pram with Gondola",
                           "V-Shaped Sections",
                           "Normal Sections",
                           "U-Shaped Sections" };
    }

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
     * @brief Constructs a new Ship object with the given parameters.
     * @param parameters A map of parameters used to initialize the ship,
     * including strategies for calm and dynamic resistance.
     * @param parent The parent QObject.
     *
     * Initializes various properties of the ship such as resistance
     * strategies, dimensions, and energy sources.
     */
    Ship(const QMap<QString, std::any>& parameters,
         QObject* parent = nullptr);

    /**
     * @brief Destructor for the Ship class.
     *
     * Destroys the Ship object and any associated resources.
     */
    ~Ship();

    void moveObjectToThread(QThread *thread);


    /**
     * @brief Calculates the total resistance experienced by the ship
     * at a given speed.
     * @param customSpeed The speed at which to calculate the resistance.
     * @param totalResistance Optional pointer to store the total
     *                        calculated resistance.
     * @return The total resistance in newtons.
     *
     * This method calculates both calm water resistance and dynamic resistance
     * using the strategies assigned to the ship. It also accounts for any
     * dragged vessels that contribute to the overall resistance.
     */
    [[nodiscard]] units::force::newton_t
    calculateTotalResistance(units::velocity::meters_per_second_t customSpeed =
                             units::velocity::meters_per_second_t(
                                 std::nan("unintialized")),
                             units::force::newton_t* totalResistance = nullptr);

    // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
    // %%%%%%%%%%%%%%%%%%%%% GETTERS & SETTERS %%%%%%%%%%%%%%%%%%%%%
    // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

    /**
     * @brief Gets the ID that is defined by the user
     * @return ID as a string
     */
    [[nodiscard]] QString getUserID();

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
     * @brief Retrieves the length of entrance of the ship.
     * @return length in meters.
     */
    [[nodiscard]] units::length::meter_t getLengthOfEntrance() const;

    /**
     * @brief Updates the length of entrance of the ship.
     * @param newValue New length value in meters.
     */
    void setLengthOfEntrance(units::length::meter_t &newValue);

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
     * @brief Sets the calm water resistance strategy for the ship.
     * @param newStrategy A pointer to the new calm resistance strategy.
     *
     * Deletes the existing strategy (if any) and assigns the new strategy.
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
    [[nodiscard]] units::area::square_meter_t
    getLengthwiseProjectionArea() const;

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

    /**
     * @brief Calculate the total thrust the ship engine is producing.
     * @param totalThrust is an optional parameter to dump the calculated
     * value to.
     * @return total thrust in newton.
     */
    [[nodiscard]] units::force::newton_t CalculateTotalThrust(
        units::force::newton_t* totalThrust = nullptr);

    /**
     * @brief Get the total thrust the ship engine is producing.
     * @return total thrust in newton.
     */
    [[nodiscard]] units::force::newton_t getTotalThrust() const;

    /**
     * @brief Retrieves the vessel's own weight without cargo.
     *
     * @return The vessel's weight in metric tons.
     */
    [[nodiscard]] units::mass::metric_ton_t getVesselWeight() const;

    /**
     * @brief Sets the vessel's own weight.
     *
     * @param newVesselWeight The new weight of the vessel in metric tons.
     */
    void setVesselWeight(const units::mass::metric_ton_t &newVesselWeight);

    /**
     * @brief Retrieves the weight of the cargo on the vessel.
     *
     * @return The cargo weight in metric tons.
     */
    [[nodiscard]] units::mass::metric_ton_t getCargoWeight() const;

    /**
     * @brief Sets the weight of the cargo on the vessel.
     *
     * @param newCargoWeight The new weight of the cargo in metric tons.
     */
    void setCargoWeight(const units::mass::metric_ton_t &newCargoWeight);

    /**
     * @brief Calculates the surge added mass of the vessel.
     *
     * @note Surge added mass is an additional mass that must be accelerated
     * when the vessel accelerates due to the added resistance of the
     * surrounding water.
     *
     * @return The calculated surge added mass in metric tons.
     */
    [[nodiscard]] units::mass::metric_ton_t calc_SurgeAddedMass() const;

    /**
     * @brief Calculates the total static weight of the vessel, including
     * both vessel weight and cargo weight.
     *
     * @return The total static weight of the vessel in metric tons.
     */
    [[nodiscard]] units::mass::metric_ton_t getTotalVesselStaticWeight() const;

    /**
     * @brief Calculates the total dynamic weight of the vessel, including
     * static weight and surge added mass.
     *
     * @return The total dynamic weight of the vessel in metric tons.
     */
    [[nodiscard]] units::mass::metric_ton_t getTotalVesselDynamicWeight() const;

    /**
     * @brief Adds a propeller to the vessel.
     *
     * @param newPropeller Pointer to the propeller object to be added
     *                     to the vessel.
     */
    void addPropeller(IShipPropeller *newPropeller);

    /**
     * @brief Retrieves the list of propellers attached to the vessel.
     *
     * @return A pointer to a vector containing pointers to the propeller
     * objects.
     */
    [[nodiscard]] const QVector<IShipPropeller*> *getPropellers() const;

    /**
     * @brief Retrieves the list of vessels being dragged by this vessel.
     *
     * @return A pointer to a vector containing pointers to the dragged
     * vessel objects.
     */
    [[nodiscard]] QVector<Ship*> *draggedVessels();

    /**
     * @brief Retrieves the ship's path lines.
     *
     * @return A pointer to a vector of shared pointers to the GLine
     * objects representing the ship's path.
     */
    [[nodiscard]] QVector<std::shared_ptr<GLine>>* getShipPathLines();

    /**
     * @brief Retrieves the ship's path points.
     *
     * @return A pointer to a vector of shared pointers to the GPoint
     * objects representing the ship's path.
     */
    [[nodiscard]] QVector<std::shared_ptr<GPoint>>* getShipPathPoints();

    /**
     * @brief Sets the path that the ship will follow.
     *
     * @param points A vector of shared pointers to GPoint objects
     *               representing the path points.
     * @param lines A vector of shared pointers to GLine objects
     *              representing the path lines.
     *
     * @warning The path must be defined before the ship starts moving.
     *          It cannot be changed mid-journey.
     */
    void setPath(const QVector<std::shared_ptr<GPoint>> points,
                 const QVector<std::shared_ptr<GLine>> lines);


    /**
     * @brief Checks if the vessel has reached its destination.
     *
     * @return `true` if the vessel has reached its destination,
     * `false` otherwise.
     */
    [[nodiscard]] bool isReachedDestination() const;

    /**
     * @brief Checks if the vessel has run out of energy.
     *
     * @return `true` if the vessel is out of energy, `false` otherwise.
     */
    [[nodiscard]] bool isOutOfEnergy() const;

    /**
     * @brief Checks if the vessel is loaded with cargo.
     *
     * @return `true` if the vessel is loaded, `false` otherwise.
     */
    [[nodiscard]] bool isLoaded() const;

    /**
     * @brief Marks the vessel as loaded with cargo.
     *
     * This method should be called once the vessel has been fully
     * loaded and is ready for operation.
     */
    void load();

    /**
     * @brief Marks the vessel as unloaded.
     *
     * This method should be called when the vessel has been
     * completely unloaded.
     */
    void unload();

    /**
     * @brief Retrieves the starting point of the vessel's path.
     *
     * @return A shared pointer to the GPoint object representing the
     * start point of the vessel's path.
     */
    [[nodiscard]] std::shared_ptr<GPoint> startPoint();

    /**
     * @brief Sets the starting point of the vessel's path.
     *
     * @param startPoint A shared pointer to the GPoint object
     * representing the start point of the vessel's path.
     *
     * @warning This should only be set at the beginning of the journey.
     */
    void setStartPoint(std::shared_ptr<GPoint> startPoint);

    /**
     * @brief Retrieves the endpoint of the vessel's path.
     *
     * @return A shared pointer to the GPoint object representing the
     * end point of the vessel's path.
     */
    [[nodiscard]] std::shared_ptr<GPoint> endPoint();

    /**
     * @brief Sets the endpoint of the vessel's path.
     *
     * @param endPoint A shared pointer to the GPoint object representing
     * the end point of the vessel's path.
     */
    void setEndPoint(std::shared_ptr<GPoint> endPoint);

    void restoreLatestGPSCorrectPosition();

    /**
     * @brief Retrieves the current position of the vessel.
     *
     * @return The GPoint object representing the vessel's current position.
     */
    [[nodiscard]] GPoint getCurrentPosition();

    /**
     * @brief Set current Position of the ship in case of a cyber attack.
     * @param newPosition new position of the ship.
     */
    void setCurrentPosition(GPoint newPosition);

    /**
     *  @brief Disable the ship communications with surrounding ships
     *  and GPS updates.
     */
    void disableCommunications();

    /**
     *  @brief Enable the ship communications with surrounding ships
     *  and GPS updates.
     */
    void enableCommunications();

    /**
     * @brief Retrieves the current heading (orientation) of the vessel.
     *
     * @return The current heading of the vessel in degrees.
     */
    [[nodiscard]] units::angle::degree_t getCurrentHeading() const;

    /**
     * @brief Retrieves the current target point that the vessel is
     * navigating towards.
     *
     * @return The GPoint object representing the current target point.
     */
    [[nodiscard]] GPoint getCurrentTarget();

    /**
     * @brief Retrieves the current environmental conditions affecting
     * the vessel.
     *
     * @return The current environment as an AlgebraicVector::Environment
     * object.
     */
    [[nodiscard]] AlgebraicVector::Environment getCurrentEnvironment() const;

    /**
     * @brief Sets the current environmental conditions affecting the vessel.
     *
     * @param newEnv The new environmental conditions as an
     * AlgebraicVector::Environment object.
     */
    void setCurrentEnvironment(const AlgebraicVector::Environment newEnv);


//    void setCurrentPosition(const Point& point);

    /**
     * @brief Retrieves the cumulative lengths of the path segments (links).
     *
     * @return A QVector containing the cumulative lengths of the path
     * segments in meters.
     */
    [[nodiscard]] QVector<units::length::meter_t> getLinksCumLengths() const;

    /**
     * @brief Retrieves the start time of the vessel's trip.
     *
     * @return The start time of the trip as a units::time::second_t object.
     */
    [[nodiscard]] units::time::second_t getStartTime() const;

    /**
     * @brief Sets the start time of the vessel's trip.
     *
     * @param newStartTime The new start time of the trip as a
     * units::time::second_t object.
     */
    void setStartTime(const units::time::second_t &newStartTime);

    /**
     * @brief Adds to the cumulative consumed energy of the vessel.
     *
     * @param consumedkWh The amount of energy consumed in kilowatt-hours
     * to be added to the cumulative total.
     */
    void addToCummulativeConsumedEnergy(
        units::energy::kilowatt_hour_t consumedkWh);

    /**
     * @brief Retrieves the cumulative consumed energy of the vessel.
     *
     * @return The cumulative consumed energy in kilowatt-hours.
     */
    [[nodiscard]] units::energy::kilowatt_hour_t getCumConsumedEnergy() const;

    /**
     * @brief Retrieves the cumulative fuel consumption of the vessel.
     *
     * @return A map where the key is the fuel type (ShipFuel::FuelType)
     * and the value is the volume consumed in liters.
     */
    [[nodiscard]] std::map<ShipFuel::FuelType, units::volume::liter_t>
    getCumConsumedFuel() const;

    /**
     * @brief Retrieves the total cargo ton-kilometers traveled by the vessel.
     *
     * @return The total cargo ton-kilometers as a unit_t of mass in metric
     * tons and distance in kilometers.
     */
    [[nodiscard]] units::unit_t<units::compound_unit<units::mass::metric_tons,
                                                     units::length::kilometers>>
    getTotalCargoTonKm() const;

    /**
     * @brief Retrieves the total time elapsed since the start of
     * the vessel's trip.
     *
     * @return The total trip time as a units::time::second_t object.
     */
    [[nodiscard]] units::time::second_t getTripTime();

    /**
     * @brief Calculates the running average for a given time step data.
     *
     * @param previousAverage The previous running average.
     * @param currentTimeStepData The current time step data to be
     * included in the average.
     * @param timeStep The time step duration in seconds.
     * @return The updated running average value.
     */
    [[nodiscard]] double calculateRunningAverage(
        double previousAverage, double currentTimeStepData, double timeStep);

    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // ~~~~~~~~~~~~~~~~~~~~~~ Dynamics ~~~~~~~~~~~~~~~~~~~~~~~~~
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    /**
     * @brief Simulates the ship's sailing behavior for a given time step.
     *
     * This method calculates the ship's movement, speed, acceleration,
     * and energy consumption during the given time step. It accounts for
     * the free flow speed, the gap to the next critical point, whether
     * the ship is following another ship, and the leader's speed. The ship's
     * current environment, including water conditions, is also considered.
     *
     * @param timeStep The time step duration over
     *                 which to simulate the sailing.
     * @param freeFlowSpeed The free flow speed of the ship,
     *                      typically its target cruising speed.
     * @param gapToNextCriticalPoint A vector of distances to the
     *                               next critical points along the
     *                               ship's path.
     * @param isFollowingAnotherShip A vector indicating whether
     *                               the ship is following another
     *                               ship at each critical point.
     * @param leaderSpeeds A vector of speeds of the ships ahead
     *                     (leaders) at each critical point.
     * @param currentEnvironment The current environmental conditions
     *                           affecting the ship, such as water
     *                           density and salinity.
     *
     * This method updates the ship's position, speed, acceleration,
     * and energy consumption. It also handles the turning and stopping
     * behaviors based on the ship's path and whether it is following
     * another vessel.
     */
    void sail(units::time::second_t &timeStep,
              units::velocity::meters_per_second_t &freeFlowSpeed,
              QVector<units::length::meter_t> &gapToNextCriticalPoint,
              QVector<bool> &isFollowingAnotherShip,
              QVector<units::velocity::meters_per_second_t> &leaderSpeeds,
              AlgebraicVector::Environment currentEnvironment);

    /**
     * @brief Calculates general statistics for the ship's trip based
     * on the given time step.
     *
     * This function updates the running averages for speed and acceleration,
     * as well as the total trip time, using the provided time step.
     *
     * @param timeStep The time step duration in seconds for which the
     *                 statistics should be calculated.
     */
    void calculateGeneralStats(units::time::second_t timeStep);

    /**
     * @brief Retrieves the total distance traveled by the ship.
     *
     * @return The total traveled distance in meters.
     */
    [[nodiscard]] units::length::meter_t getTraveledDistance() const;

    /**
     * @brief Retrieves the total length of the ship's path.
     *
     * @return The total path length in meters.
     */
    [[nodiscard]] units::length::meter_t getTotalPathLength() const;

    /**
     * @brief Resets the ship's internal state, including speed, acceleration,
     * and traveled distance.
     *
     * This function resets various ship parameters to their initial states,
     * preparing the ship for a new trip or simulation run.
     */
    void reset();

    /**
     * @brief Retrieves the index of the previous path point that the
     * ship has passed.
     *
     * @return The index of the previous path point.
     */
    [[nodiscard]] size_t getPreviousPathPointIndex() const;

    /**
     * @brief Retrieves information about the next stopping point on
     * the ship's path.
     *
     * @return A stopPointDefinition object containing details of the
     * next stopping point.
     */
    [[nodiscard]] stopPointDefinition getNextStoppingPoint();

    /**
     * @brief Calculates the distance from a specified path node index
     * to the finish line.
     *
     * @param i The index of the path node from which the distance to the
     * finish is calculated.
     * @return The distance from the specified path node index to the
     * finish in meters.
     */
    units::length::meter_t distanceToFinishFromPathNodeIndex(qsizetype i);

    /**
     * @brief Calculates the distance between two specified path node indices.
     *
     * @param startIndex The index of the starting path node.
     * @param endIndex The index of the ending path node.
     * @return The distance between the two specified path node indices in
     * meters.
     */
    [[nodiscard]] units::length::meter_t
    distanceToNodePathIndexFromPathNodeIndex(qsizetype startIndex,
                                             qsizetype endIndex);

    /**
     * @brief Calculates the distance from the ship's current position
     * to a specified path node index.
     *
     * @param endIndex The index of the ending path node.
     * @return The distance from the ship's current position to the
     * specified path node index in meters.
     */
    [[nodiscard]] units::length::meter_t
    distanceFromCurrentPositionToNodePathIndex(qsizetype endIndex);

    /**
     * @brief Checks whether the ship is on the correct path.
     *
     * @return True if the ship is on the correct path, false otherwise.
     */
    [[nodiscard]] bool isShipOnCorrectPath();

    [[nodiscard]] bool isShipStillMoving();

    /**
     * @brief Calculates the progress of the ship along its path as a percentage.
     *
     * @return The progress of the ship along its path, expressed as a
     * value between 0.0 and 1.0.
     */
    [[nodiscard]] double progress();

    [[nodiscard]] bool isExperiencingHighResistance();

    // [[nodiscard]] units::velocity::meters_per_second_t getCurrentMaxSpeed();
    // [[nodiscard]] QHash<qsizetype, units::velocity::meters_per_second_t>
    // getAheadLowerSpeeds(qsizetype nextStopIndex);

    /**
     * @brief Moves the ship forward by a specified distance.
     *
     * This function updates the ship's position by moving it forward by
     * the given distance over the specified time step.
     *
     * @param distance The distance in meters by which to move the ship
     *                 forward.
     * @param timeStep The time step duration in seconds over which the
     *                 movement occurs.
     */
    void kickForwardADistance(units::length::meter_t &distance,
                              units::time::second_t timeStep);

    /**
     * @brief Retrieves the ship's current acceleration.
     *
     * @return The current acceleration of the ship in meters per
     * second squared.
     */
    [[nodiscard]] units::acceleration::meters_per_second_squared_t
    getAcceleration() const;

//    [[nodiscard]] units::velocity::meters_per_second_t getSpeed();

    /**
     * @brief Retrieves the ship's previous speed.
     *
     * @return The previous speed of the ship in meters per second.
     */
    [[nodiscard]] units::velocity::meters_per_second_t getPreviousSpeed() const;

    /**
     * @brief Retrieves the ship's maximum speed.
     *
     * @return The maximum speed of the ship in meters per second.
     */
    [[nodiscard]] units::velocity::meters_per_second_t getMaxSpeed() const;

    /**
     * @brief Calculates and retrieves the ship's maximum acceleration.
     *
     * @return The maximum acceleration of the ship in meters per
     * second squared.
     */
    [[nodiscard]] units::acceleration::meters_per_second_squared_t
    getMaxAcceleration();

    /**
     * @brief Retrieves the running average of the ship's acceleration
     * over the trip.
     *
     * @return The running average acceleration of the ship in meters
     * per second squared.
     */
    [[nodiscard]] units::acceleration::meters_per_second_squared_t
    getTripRunningAverageAcceleration() const;

    /**
     * @brief Retrieves the running average of the ship's speed over the trip.
     *
     * @return The running average speed of the ship in meters per second.
     */
    [[nodiscard]] units::velocity::meters_per_second_t
    getTripRunningAvergageSpeed() const;

    /**
     * @brief Calculates and retrieves the energy consumption per
     * ton of cargo.
     *
     * @return The energy consumption per ton of cargo in kilowatt-hours
     * per metric ton.
     */
    [[nodiscard]] units::unit_t<
        units::compound_unit<units::energy::kilowatt_hour,
                             units::inverse<units::mass::metric_ton>>>
    getEnergyConsumptionPerTon() const;

    /**
     * @brief Calculates and retrieves the energy consumption per
     * ton-kilometer of cargo.
     *
     * @return The energy consumption per ton-kilometer of cargo
     * in kilowatt-hours per metric ton-kilometer.
     */
    [[nodiscard]] units::unit_t<
        units::compound_unit<units::energy::kilowatt_hour,
                             units::inverse<
                                 units::compound_unit<
                                     units::length::meter, units::mass::metric_ton>>>>
    getEnergyConsumptionPerTonKM() const;

    /**
     * @brief Retrieves the cumulative fuel consumption of the ship in liters.
     *
     * @return The overall cumulative fuel consumption of the ship in liters.
     */
    [[nodiscard]] units::volume::liter_t getOverallCumFuelConsumption() const;

    /**
     * @brief Calculates and retrieves the overall fuel consumption
     * per ton of cargo.
     *
     * @return The overall fuel consumption per ton of cargo in
     * liters per metric ton.
     */
    [[nodiscard]] units::unit_t<
        units::compound_unit<units::volume::liter,
                             units::inverse<units::mass::metric_ton>>>
    getOverallCumFuelConsumptionPerTon() const;

    /**
     * @brief Calculates and retrieves the overall fuel consumption
     * per ton-kilometer of cargo.
     *
     * @return The overall fuel consumption per ton-kilometer of cargo
     * in liters per metric ton-kilometer.
     */
    [[nodiscard]] units::unit_t<
        units::compound_unit<units::volume::liter,
                             units::inverse<
                                 units::compound_unit<
                                     units::length::meter, units::mass::metric_ton>>>>
    getOverallCumFuelConsumptionPerTonKM() const;

    QJsonObject getCurrentStateAsJson() const;

#ifdef BUILD_SERVER_ENABLED
    QVector<ContainerCore::Container*> getLoadedContainers() const;
    void addContainer(ContainerCore::Container *container);
    void addContainers(QJsonObject json);
#endif


private:

    //!< The ship ID
    QString mShipUserID;

    bool mIsCommunicationActive = true;


    //!< Counter to track how many steps the ship has not moved.
    int mInactivityStepCount = 0;

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

    //!< The length of waterline entrance
    units::length::meter_t mLengthOfEntrance;

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

    //!< Total thrust forces generated by the ship.
    units::force::newton_t mTotalThrust;

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

    //!< the max acceleration of the vessel in the current time step.
    units::acceleration::meters_per_second_squared_t mMaxAcceleration;

    //!< The average running acceleration through out the trip.
    units::acceleration::meters_per_second_squared_t mRunningAvrAcceleration;

    //!< The max speed the vessel can go by.
    //! this is required for the model to work.
    //! if the vessel is a cargo ship, 25 knots is a typical value.
    units::velocity::meters_per_second_t mMaxSpeed;

    //!< Current speed of the ship.
    units::velocity::meters_per_second_t mSpeed;

    //!< The previous speed of the vessel.
    units::velocity::meters_per_second_t mPreviousSpeed;

    //!< The average running speed through out the trip.
    units::velocity::meters_per_second_t mRunningAvrSpeed;

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

    //!< The energy sources that is feeding the
    //! engines for the propulsion system.
    QVector<std::shared_ptr<IEnergySource>> mEnergySources;

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
    bool mShowNoPowerMessage = false;

    //!< Holds the reaction time.
    units::time::second_t mT_s = units::time::second_t(10.0);

    //!< Holds the max deceleration level.
    //! This is the upper bound of the decceleration.
    //! Refer to calc_decelerationAtSpeed().
    // units::acceleration::meters_per_second_squared_t mD_des =
    //     units::acceleration::meters_per_second_squared_t(0.2);

    //!< The max angle the rudder can turn by
    units::angle::degree_t mRudderAngle = units::angle::degree_t(25.0);

    bool mBrakingThrustAvailable = true;

    bool mHighResistanceOccuring = false;

    bool mIsShipMoving = true;

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
    std::map<ShipFuel::FuelType, units::volume::liter_t> mCumConsumedFuel;

    units::unit_t<units::compound_unit<units::mass::metric_tons,
                                       units::length::kilometers>>
        mTotalCargoTonKm =
        units::unit_t<units::compound_unit<units::mass::metric_tons,
                                           units::length::kilometers>>(0.0);


#ifdef BUILD_SERVER_ENABLED
    ContainerCore::ContainerMap mLoadedContainers;
#endif
    /**
     * @brief Calculates the wet surface area of the ship.
     *
     * @param method The method to use for calculation. Default is Holtrop.
     * @return The calculated wet surface area.
     */
    units::area::square_meter_t calc_WetSurfaceArea(
        WetSurfaceAreaCalculationMethod method =
        WetSurfaceAreaCalculationMethod::Cargo
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

    double calc_blockCoefByVolumetricDisplacement() const;

    double calc_blockCoefByMidShipAndPrismaticCoefs() const;

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
     * @brief Calculates the length of entrance in meters.
     * @return The length of entrance.
     */
    units::length::meter_t calc_LE() const;

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
        IShipCalmResistanceStrategy* strategy);

    void initializeDefaults();

    units::force::newton_t calc_Torque();

    [[nodiscard]] units::force::newton_t getTotalResistance();

    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // ~~~~~~~~~~~~~~~~~~~~ Dynamics ~~~~~~~~~~~~~~~~~~~~~
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    units::acceleration::meters_per_second_squared_t
    calc_maxAcceleration(
        units::acceleration::meters_per_second_squared_t* maxAccel = nullptr,
        units::force::newton_t* totalThrust = nullptr,
        units::force::newton_t* totalResistance = nullptr);

    units::acceleration::meters_per_second_squared_t
    calc_decelerationAtSpeed(
        const units::velocity::meters_per_second_t customSpeed);
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
        const units::time::second_t &deltaT,
        units::acceleration::meters_per_second_squared_t* maxAccel = nullptr,
        units::force::newton_t* totalThrust = nullptr,
        units::force::newton_t* totalResistance = nullptr);

    units::acceleration::meters_per_second_squared_t
    getStepAcceleration(
        units::time::second_t &timeStep,
        units::velocity::meters_per_second_t &freeFlowSpeed,
        QVector<units::length::meter_t> &gapToNextCriticalPoint,
        QVector<bool> &isFollowingAnotherShip,
        QVector<units::velocity::meters_per_second_t> &leaderSpeeds,
        units::acceleration::meters_per_second_squared_t* maxAccel = nullptr,
        units::force::newton_t* totalThrust = nullptr,
        units::force::newton_t* totalResistance = nullptr);

    units::acceleration::meters_per_second_squared_t
    accelerateConsideringJerk(
        units::acceleration::meters_per_second_squared_t &acceleration,
        units::acceleration::meters_per_second_squared_t &previousAcceleration,
        units::jerk::meters_per_second_cubed_t &jerk,
        units::time::second_t &deltaT );

    units::acceleration::meters_per_second_squared_t
    smoothAccelerate(units::acceleration::meters_per_second_squared_t &acceleration,
                     units::acceleration::
                     meters_per_second_squared_t &previousAccelerationValue,
                     double &alpha,
                     units::acceleration::meters_per_second_squared_t &maxAcceleration);

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


public:

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

    /**
     * @brief Signal emitted when the ship reaches its destination.
     *
     * This signal is emitted when the ship has successfully reached
     * its final destination point along its path.
     *
     * @param shipDetails a json object that has all state details of the ship.
     */
    void reachedDestination(const QJsonObject shipDetails);

private: signals:
    /**
     * @brief Signal emitted when the ship's traveled distance changes
     * during a time step.
     *
     * This signal is emitted whenever the ship's traveled distance is
     * updated, typically after moving along its path.
     *
     * @param newDistance The new distance traveled by the ship in
     *                    the current time step, measured in meters.
     * @param timeStep The duration of the current time step in seconds.
     */
    void stepDistanceChanged(units::length::meter_t newDistance,
                             units::time::second_t timeStep);

    /**
     * @brief Signal emitted when the ship deviates from its intended path.
     *
     * This signal is emitted when the ship is detected to be off-course
     * from its designated path, possibly due to navigation issues.
     *
     * @param msg A message describing the nature of the path deviation.
     */
    void pathDeviation(QString msg);

private slots:
    /**
     * @brief Slot to handle changes in the ship's traveled distance
     * during a time step.
     *
     * This slot is connected to the `stepDistanceChanged` signal and
     * processes the ship's movement, updating its position along the path.
     *
     * @param newTotalDistance The total distance traveled by the ship
     * after the current time step, measured in meters.
     * @param timeStep The duration of the current time step in seconds.
     */
    void handleStepDistanceChanged(units::length::meter_t newTotalDistance,
                                   units::time::second_t timeStep);
};
};
#endif // SHIP_H
