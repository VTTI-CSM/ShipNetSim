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
 * The interface provides methods to read power efficiency and RPM data from
 * files, get the current efficiency, brake power, and RPM of the engine,
 * as well as get the previous brake power of the engine.
 *
 * @author Ahmed Aredah
 * @date 10.10.2023
 */
#ifndef IENGINE_H
#define IENGINE_H

#include "ienergyconsumer.h"
#include "../utils/utils.h"

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
public:

    /**
     * @brief The EngineProperties class
     */
    struct EngineProperties {
        units::power::kilowatt_t breakPower;
        units::angular_velocity::revolutions_per_minute_t RPM;
        double efficiency;

        // Static member function for comparing by breakPower
        static bool compareByBreakPower(const EngineProperties& a,
                                        const EngineProperties& b) {
            return a.breakPower < b.breakPower;
        }

        // Static member function for comparing by RPM
        static bool compareByRPM(const EngineProperties& a,
                                 const EngineProperties& b) {
            return a.RPM < b.RPM;
        }

        // Static member function for comparing by efficiency
        static bool compareByEfficiency(const EngineProperties& a,
                                        const EngineProperties& b) {
            return a.efficiency < b.efficiency;
        }
    };

    /**
     * @brief The EngineOperationalLoad enum
     * @details
     * 1. Low Load Power: (Eq. to L4) is located at the lower end of the
     * engine layout curve. This setting is used for operations requiring
     * minimal power, such as maneuvering in port or idling. L4 helps in
     * situations where high power is unnecessary and could be inefficient
     * or even harmful to the engine if it causes it to operate below a
     * certain threshold for effective combustion.
     * 2. Economic or Cruising Power: (Eq. to L3). It corresponds to a
     * power setting where the engine operates efficiently for extended
     * periods. This power level balances fuel consumption and wear, which is
     * ideal for cruising. It is critical for operational planning as
     * it ensures economic fuel usage while maintaining adequate speed and
     * power.
     * 3. Reduced MCR: (Eq. to L2) is typically plotted slightly below L1 on
     * the engine layout curve. It represents a power level that is below the
     * maximum but is still high. It's used to provide a safety margin to
     * reduce the stress on the engine and extend its life, often used during
     * normal operations.
     * 4. Maximum Continuous Rating - MCR: (Eq. to L1) marks the highest
     * point on the engine layout curve that the engine can sustain
     * continuously without incurring damage or excessive wear. This is often
     * near the peak of the curve, representing the engine’s maximum power
     * output at a given RPM.
     */
    enum class EngineOperationalLoad{
        Low = 0,
        Economic = 1,
        ReducedMCR = 2,
        MCR = 3
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
    enum class EngineOperationalTier{
        tierII = 1,
        TierIII = 2,
    };

    /**
     * @brief Destructor for the IShipEngine interface.
     */
    virtual ~IShipEngine() = default;

    /**
     * @brief Get the current efficiency of the engine.
     *
     * This method is called to get the current efficiency of the engine.
     *
     * @return The current efficiency of the engine as a double.
     */
    virtual double getEfficiency() = 0;

    /**
     * @brief Get the current brake power of the engine.
     *
     * This method is called to get the current brake power of the engine.
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
    virtual units::angular_velocity::revolutions_per_minute_t getRPM() = 0;

    /**
     * @brief get RPM Range defined by the engine layout
     * @return vector of RPM deining the highest and lowest values.
     */
    virtual QVector<units::angular_velocity::revolutions_per_minute_t>
    getRPMRange() = 0;

    /**
     * @brief Get the previous brake power of the engine.
     *
     * This method is called to get the previous brake power of the engine.
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
     * @brief set the engine max power ratio representing the max engine
     * load it can reach, the default value is 1.0.
     *
     * @details The function sets the engine power ratio to the max
     * power of the engine. The default value is 1.0
     * @param setEngineMaxPowerLoad is the max ratio of engine load the
     * engine reaches.
     */
    virtual void setEngineMaxPowerLoad(double setEngineMaxPowerRatio) = 0;

    /**
     * @brief Get the engine max power ratio of the engine.
     * @return a ratio representing the max load the engine can reach.
     */
    virtual double getEngineMaxPowerRatio() = 0;

    virtual bool
    selectCurrentEnergySourceByFuelType(ShipFuel::FuelType fuelType) = 0;

    virtual void setEngineRPM(
        units::angular_velocity::revolutions_per_minute_t targetRPM) = 0;

    bool requestHigherEnginePower();

    bool requestLowerEnginePower();

    EngineOperationalLoad getCurrentOperationalLoad();

    void setCurrentEngineProperties(EngineOperationalLoad targetLoad);

    EngineProperties getEnginePropertiesAtPower(units::power::kilowatt_t p,
                                                EngineOperationalTier tier);

    bool changeTier(EngineOperationalTier targetTier);

private:
    void setEngineProperitesSetting(QVector<EngineProperties> engineSettings);

protected:

    double mMaxPowerRatio = 1.0;

    /// the engine operational power vector holds the engine power
    /// at the engine layout corners in the following order L4, L2, L3, L1
    QVector<units::power::kilowatt_t> mEngineOperationalPowerSettings;

    /// The engine properties vector holds the engine properties
    /// at the engine power-rpm-efficiency curves for Tier II
    QVector<EngineProperties> mEngineDefaultTierPropertiesPoints;

    /// The engine properties vector holds the engine properties
    /// at the engine power-rpm-efficiency curves for Tier III
    QVector<EngineProperties> mEngineNOxReducedTierPropertiesPoints;

    /// The max power the engine is current set at.
    units::power::kilowatt_t mCurrentOperationalPowerSetting;

    /// The current rpm of the engine
    units::angular_velocity::revolutions_per_minute_t
        mRPM;

    /// The tier that is currently the engine is running by.
    EngineOperationalTier mCurrentOperationalTier;

    /// The current operational load the engine is set to
    /// run at.
    EngineOperationalLoad mCurrentOperationalLoad;


private:
    /// Memorization
    QVector<double> mEnginePowerList = QVector<double>();
    QVector<double> mRPMList = QVector<double>();
    QVector<double> mEfficiencyList = QVector<double>();

};
};
#endif // IENGINE_H
