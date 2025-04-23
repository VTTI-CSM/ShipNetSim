/**
 * @file IShipEngine.h
 *
 * @brief This file contains the declaration of the IShipEngine
 * interface, which represents a ship engine that consumes energy
 * and provides mechanical power.
 *
 * This interface defines the methods that should be implemented
 * by any class that represents a ship engine. A ship engine consumes
 * energy from an energy source and converts it into mechanical power.
 * The interface provides methods to read power efficiency and RPM
 * data from files, get the current efficiency, brake power, and RPM
 * of the engine, as well as get the previous brake power of the
 * engine.
 *
 * @author Ahmed Aredah
 * @date 10.10.2023
 */
#ifndef IENGINE_H
#define IENGINE_H

#include "../utils/utils.h"
#include "ienergyconsumer.h"
#include <QObject>

namespace ShipNetSimCore
{
/**
 * @class IShipEngine
 *
 * @brief The IShipEngine interface represents a ship engine
 * that consumes energy and provides mechanical power.
 *
 * This interface inherits from the IEnergyConsumer interface
 * and adds methods specific to a ship engine.
 * A ship engine consumes energy from an energy source and
 * converts it into mechanical power.
 */
class IShipEngine : public IEnergyConsumer
{
    Q_OBJECT

public:
    /**
     * @brief The EngineProperties class
     */
    struct EngineProperties
    {
        units::power::kilowatt_t                          breakPower;
        units::angular_velocity::revolutions_per_minute_t RPM;
        double                                            efficiency;

        // Static member function for comparing by breakPower
        static bool compareByBreakPower(const EngineProperties &a,
                                        const EngineProperties &b,
                                        const bool ascending = true)
        {
            if (ascending)
                return a.breakPower < b.breakPower;
            return a.breakPower > b.breakPower;
        }

        // Static member function for comparing by RPM
        static bool compareByRPM(const EngineProperties &a,
                                 const EngineProperties &b,
                                 const bool ascending = true)
        {
            if (ascending)
                return a.RPM < b.RPM;
            return a.RPM > b.RPM;
        }

        // Static member function for comparing by efficiency
        static bool compareByEfficiency(const EngineProperties &a,
                                        const EngineProperties &b,
                                        const bool ascending = true)
        {
            if (ascending)
                return a.efficiency < b.efficiency;
            return a.efficiency > b.efficiency;
        }

        // Equality operator
        bool operator==(const EngineProperties &other) const
        {
            return breakPower == other.breakPower && RPM == other.RPM
                   && efficiency == other.efficiency;
        }

        // Inequality operator
        bool operator!=(const EngineProperties &other) const
        {
            return !(*this == other);
        }

        // Overload the << operator for QTextStream
        friend QTextStream &operator<<(QTextStream            &stream,
                                       const EngineProperties &ep)
        {
            stream << "EngineProperties("
                   << "Break Power(kW): " << ep.breakPower.value()
                   << "; "
                   << "RPM: " << ep.RPM.value() << "; "
                   << "Efficiency: " << ep.efficiency << ")";
            return stream;
        }
    };

    /**
     * @brief The EngineOperationalLoad enum
     * @details
     * 1. Low Load Power: (Eq. to L4) is located at the lower end of
     * the engine layout curve. This setting is used for operations
     * requiring minimal power, such as maneuvering in port or idling.
     * L4 helps in situations where high power is unnecessary and
     * could be inefficient or even harmful to the engine if it causes
     * it to operate below a certain threshold for effective
     * combustion.
     * 2. Economic or Cruising Power: (Eq. to L3). It corresponds to a
     * power setting where the engine operates efficiently for
     * extended periods. This power level balances fuel consumption
     * and wear, which is ideal for cruising. It is critical for
     * operational planning as it ensures economic fuel usage while
     * maintaining adequate speed and power.
     * 3. Reduced MCR: (Eq. to L2) is typically plotted slightly below
     * L1 on the engine layout curve. It represents a power level that
     * is below the maximum but is still high. It's used to provide a
     * safety margin to reduce the stress on the engine and extend its
     * life, often used during normal operations.
     * 4. Maximum Continuous Rating - MCR: (Eq. to L1) marks the
     * highest point on the engine layout curve that the engine can
     * sustain continuously without incurring damage or excessive
     * wear. This is often near the peak of the curve, representing
     * the engine’s maximum power output at a given RPM.
     */
    enum class EngineOperationalLoad
    {
        Low         = 0,
        Economic    = 1,
        ReducedMCR  = 2,
        MCR         = 3,
        Default     = 4,
        UserDefined = 5
    };

    /**
     * @brief The EngineOperationalTier enum
     *
     * @details
     * Tier II and Tier III are emission standards set by the
     * International Maritime Organization (IMO) under the MARPOL
     * Annex VI regulations to control pollution from ship exhaust.
     * These standards specifically target the reduction of nitrogen
     * oxides (NOx), which are harmful pollutants that contribute to
     * environmental problems like acid rain and smog.
     *
     * The current release does only support Tier II operations and
     * reduction in NOx or COx are controlled by the fuel type only.
     */
    enum class EngineOperationalTier
    {
        tierII  = 1,
        TierIII = 2,
    };

    /**
     * @brief Destructor for the IShipEngine interface.
     */
    virtual ~IShipEngine() = default;

    /**
     * @brief Get the current efficiency of the engine.
     *
     * This method is called to get the current efficiency of the
     * engine.
     *
     * @return The current efficiency of the engine as a double.
     */
    virtual double getEfficiency() = 0;

    /**
     * @brief Get the current brake power of the engine.
     *
     * This method is called to get the current brake power of the
     * engine.
     *
     * @return The current brake power of the engine in kilowatts.
     */
    virtual units::power::kilowatt_t getBrakePower() = 0;

    /**
     * @brief Get the current torque equivalent to the brake power.
     *
     * This method is called to get the current torque at the
     * current RPM and brake power of the engine.
     *
     * @return The current Engine Torque.
     */
    virtual units::torque::newton_meter_t getBrakeTorque() = 0;

    /**
     * @brief Get the current RPM of the engine.
     *
     * This method is called to get the current RPM
     * (revolutions per minute) of the engine.
     *
     * @return The current RPM of the engine.
     */
    virtual units::angular_velocity::revolutions_per_minute_t
    getRPM() = 0;

    /**
     * @brief get RPM Range defined by the engine layout
     * @return vector of RPM deining the highest and lowest values.
     */
    virtual QPair<units::angular_velocity::revolutions_per_minute_t,
                  units::angular_velocity::revolutions_per_minute_t>
    getRPMRange() = 0;

    /**
     * @brief Get the previous brake power of the engine.
     *
     * This method is called to get the previous brake power of the
     * engine.
     *
     * @return The previous brake power of the engine in kilowatts.
     */
    virtual units::power::kilowatt_t getPreviousBrakePower() = 0;

    /**
     * @brief Get the id of the engine
     * @return engine id as integer value
     */
    virtual int getEngineID() = 0;

    /**
     * @brief Check if the engine is still working
     * @return true if the engine is still working, false o.w.
     */
    virtual bool isEngineWorking() = 0;

    /**
     * @brief set the engine max power ratio representing the max
     * engine load it can reach, the default value is 1.0.
     *
     * @details The function sets the engine power ratio to the max
     * power of the engine. The default value is 1.0
     * @param setEngineMaxPowerLoad is the max ratio of engine load
     * the engine reaches.
     */
    virtual void
    setEngineMaxPowerLoad(double setEngineMaxPowerRatio) = 0;

    /**
     * @brief Get the engine max power ratio of the engine.
     * @return a ratio representing the max load the engine can reach.
     */
    virtual double getEngineMaxPowerRatio() = 0;

    virtual bool selectCurrentEnergySourceByFuelType(
        ShipFuel::FuelType fuelType) = 0;

    virtual QVector<EngineProperties> estimateEnginePowerCurve() = 0;

    IShipEngine::EngineProperties getEngineRatingProperties();

    /**
     * @brief Updates the current step of the engine's operation.
     */
    virtual void updateEngineOperationalState() = 0;

    bool requestHigherEnginePower();

    bool requestLowerEnginePower();

    EngineOperationalTier getCurrentOperationalTier();
    EngineOperationalLoad getCurrentOperationalLoad();

    void setEngineOperationalLoad(EngineOperationalLoad targetLoad);

    IShipEngine::EngineProperties getEnginePropertiesAtRPM(
        units::angular_velocity::revolutions_per_minute_t rpm);

    EngineProperties
    getEnginePropertiesAtPower(units::power::kilowatt_t p,
                               EngineOperationalTier    tier);

    units::torque::newton_meter_t getEngineTorqueByRPM(
        units::angular_velocity::revolutions_per_minute_t rpm);

    bool setEngineOperationalTier(EngineOperationalTier targetTier);

    static QVector<EngineOperationalLoad> getEngineOperationalLoads();

    static QVector<EngineOperationalTier> getEngineOperationalTiers();

    virtual void setEngineTargetState(EngineProperties newState);

    void setEngineTierIICurve(QVector<EngineProperties> newCurve);
    void setEngineTierIIICurve(QVector<EngineProperties> newCurve);

    void setEngineDefaultTargetState(EngineProperties newState);
    EngineProperties getEngineDefaultTargetState();
    EngineProperties getEngineTargetState();
    void setEnginePreviousState(EngineProperties newState);
    EngineProperties getEnginePreviousState();
    void             setEngineCurrentState(EngineProperties newState);
    EngineProperties getEngineCurrentState();

    bool isRPMWithinOperationalRange(
        units::angular_velocity::revolutions_per_minute_t rpm);

    bool
    isPowerWithinOperationalRange(units::power::kilowatt_t power);

    virtual void turnOffEngine() = 0;
    virtual void turnOnEngine()  = 0;

protected:
    double mMaxPowerRatio = 1.0;

    // the current power curve data the engine is following
    QVector<EngineProperties> mEngineCurve;
    QVector<EngineProperties> mUserEngineCurveInDefaultTier;
    QVector<EngineProperties> mUserEngineCurveInNOxReducedTier;

    /// The engine properties vector holds the engine properties
    /// at the engine power-rpm-efficiency curves for Tier II
    QVector<EngineProperties> mEngineDefaultTierPropertiesPoints;

    /// The engine properties vector holds the engine properties
    /// at the engine power-rpm-efficiency curves for Tier III
    QVector<EngineProperties> mEngineNOxReducedTierPropertiesPoints;

    void setEngineProperitesSetting(
        QVector<EngineProperties> engineSettings);

private:
    /// Memorization
    QVector<double> mEnginePowerList = QVector<double>();
    QVector<double> mRPMList         = QVector<double>();
    QVector<double> mEfficiencyList  = QVector<double>();

    /// The target state for the default operational load (load is not
    /// set to a specific zone point L1, L2, L3, L4) but set to a
    /// specific point in between.
    EngineProperties mEngineDefaultTargetState;

    /// The target state (max point properties of the engine) the
    /// engine is current set at.
    EngineProperties mEngineTargetState;

    /// The previous engine state of power, rpm, efficiency
    EngineProperties mEnginePreviousState;

    /// The current engine state of power, rpm, efficiency
    EngineProperties mEngineCurrentState;

    /// The tier that is currently the engine is running by.
    EngineOperationalTier mCurrentOperationalTier =
        EngineOperationalTier::tierII;

    /// The current operational load the engine is set to
    /// run at.
    EngineOperationalLoad mCurrentOperationalLoad =
        EngineOperationalLoad::Default;

    static QVector<EngineOperationalLoad> mEngineOperationalLoad;

    static QVector<EngineOperationalTier> mEngineOperationalTier;

signals:
    // Signal to emit when the current state gets changed
    void engineTargetStateChanged(EngineProperties targetState);

    // Signal to emit when the max target point of engine change
    void engineCurrentStateChanged(EngineProperties newState);

    // Signal to emit when the operational load changes
    void operationalLoadChanged(EngineOperationalLoad newLoad);

    // Signal to emit when the tier changes
    void engineOperationalTierChanged(EngineOperationalTier newTier);
};
}; // namespace ShipNetSimCore
#endif // IENGINE_H
