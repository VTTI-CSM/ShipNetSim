/**
 * @file ShipEngine.h
 * @brief This file contains the definition of the ShipEngine class,
 *        which is responsible for managing the ship's engine operations.
 *
 * @author Ahmed Aredah
 * @date 10.12.2023
 */
#ifndef SHIPENGINE_H
#define SHIPENGINE_H

#include "../export.h"
#include "ishipengine.h"
#include "ienergysource.h"
#include <QMap>

namespace ShipNetSimCore
{
/**
 * @class ShipEngine
 * @brief The ShipEngine class manages the ship's engine operations.
 */
class SHIPNETSIM_EXPORT ShipEngine : public IShipEngine
{
    Q_OBJECT

public:

    /**
     * @brief Constructor for the ShipEngine class.
     */
    ShipEngine();

    ~ShipEngine() override;

    void moveObjectToThread(QThread *thread) override;

    /**
     * @brief Initializes the ship engine with the necessary parameters.
     * @param host The ship that owns the engine.
     * @param energySources The energy sources for the engine.
     * @param parameters The parameters required for initialization.
     */
    void initialize(Ship *host, QVector<IEnergySource*> energySources,
                    const QMap<QString, std::any> &parameters) override;

    /**
     * @brief Sets the parameters for the ship engine.
     * @param parameters The parameters to be set.
     */
    void setParameters(const QMap<QString, std::any> &parameters) override;

    /**
     * @brief Calculates and consume the energy used by the ship engine.
     * @param timeStep The time step for the calculation.
     * @param fuelType The fuel type to be consumed.
     * @return The energy consumption data.
     */
    EnergyConsumptionData
    consumeUsedEnergy(units::time::second_t timeStep) override;

    bool
    selectCurrentEnergySourceByFuelType(ShipFuel::FuelType fuelType) override;

    // IEngine interface

    /**
     * @brief Gets the current efficiency of the engine.
     * @return The efficiency value.
     */
    double getEfficiency() override;

    /**
     * @brief Gets the current brake power of the engine.
     * @return The brake power value.
     */
    units::power::kilowatt_t getBrakePower() override;

    /**
     * @brief Get the current torque equivalent to the brake power.
     *
     * @return The current Engine Torque.
     */
    units::torque::newton_meter_t getBrakeTorque() override;

    /**
     * @brief Gets the current RPM of the engine.
     * @return The RPM value.
     */
    units::angular_velocity::revolutions_per_minute_t getRPM() override;

    /**
     * @brief Get RPM Range defined by the engine layout
     * @return vector of RPM deining the highest and lowest values.
     */
    QPair<units::angular_velocity::revolutions_per_minute_t,
          units::angular_velocity::revolutions_per_minute_t>
    getRPMRange() override;

    /**
     * @brief Gets the previous brake power of the engine.
     * @return The previous brake power value.
     */
    units::power::kilowatt_t getPreviousBrakePower() override;

    /**
     * @brief Calculates the hyperbolic throttle coefficient for
     *        the given ship speed.
     * @param ShipSpeed The speed of the ship.
     * @return The hyperbolic throttle coefficient.
     */
    double getHyperbolicThrottleCoef(
        units::velocity::meters_per_second_t ShipSpeed,
        bool isExperiencingHighResistance);

    /**
     * @brief Get engine unique ID
     * @return The unique ID of the engine
     */
    int getEngineID() override;

    /**
     * @brief Gets the current status of the engine.
     * @return True if the engine is running, false otherwise.
     */
    bool isEngineWorking() override;

    /**
     * @brief set the engine max power ratio, the default value is 1.0.
     *
     * @details The function sets the engine power ratio to the max
     * power of the engine. The default value is 1.0
     * @param setEngineMaxPowerLoad is the max ratio of engine load the engine
     * reaches.
     */
    void setEngineMaxPowerLoad(double targetRatio) override;

    /**
     * @brief Get the engine max power ratio of the engine.
     * @return a ratio representing the max load the engine can reach.
     */
    double getEngineMaxPowerRatio() override;


    units::energy::kilowatt_hour_t getCumEnergyConsumption() override;

    void setEngineTargetState(EngineProperties newState) override;

    QVector<IShipEngine::EngineProperties> estimateEnginePowerCurve() override;

    /**
     * @brief Updates the current step of the engine's operation.
     */
    void updateEngineOperationalState() override;

    void turnOffEngine();
    void turnOnEngine();

private:
    unsigned int mId; ///< ID of the engine
    bool mIsWorking = true; ///< a boolean to indicate the engine is working

    unsigned int counter = 0; ///< keep track of new ids

    /** If the ship experienced high resistance, its speed will be reduced
     *  and hence the power.
        This will holds the lamda when the ship is not experiencing high
        resistance to keep the engine working with high power even in high
        resistance.  */
    double mNormalLambda = 1.0;



private slots:
    void handleTargetStateChange();

    void handleOperationalDetailsChange();
};
};
#endif // SHIPENGINE_H
