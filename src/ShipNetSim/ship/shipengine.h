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


#include "ishipengine.h"
#include "IEnergySource.h"
#include <QMap>

/**
 * @class ShipEngine
 * @brief The ShipEngine class manages the ship's engine operations.
 */
class ShipEngine : public IShipEngine
{
public:

    /**
     * @brief Constructor for the ShipEngine class.
     */
    ShipEngine();

    // IEnergyConsumer interface

    /**
     * @brief Initializes the ship engine with the necessary parameters.
     * @param host The ship that owns the engine.
     * @param energySource The energy source for the engine.
     * @param parameters The parameters required for initialization.
     */
    void initialize(Ship *host, IEnergySource *energySource,
                    const QMap<QString, std::any> &parameters) override;

    /**
     * @brief Sets the parameters for the ship engine.
     * @param parameters The parameters to be set.
     */
    void setParameters(const QMap<QString, std::any> &parameters) override;

    /**
     * @brief Calculates the energy consumed by the ship engine.
     * @param timeStep The time step for the calculation.
     * @return The energy consumption data.
     */
    EnergyConsumptionData
    energyConsumed(units::time::second_t timeStep) override;

    // IEngine interface

    /**
     * @brief Reads the power efficiency from a file.
     * @param filePath The file path to read from.
     */
    void readPowerEfficiency(QString filePath) override;

    /**
     * @brief Reads the power RPM from a file.
     * @param filePath The file path to read from.
     */
    void readPowerRPM(QString filePath) override;

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
     * @brief Gets the current RPM of the engine.
     * @return The RPM value.
     */
    units::angular_velocity::revolutions_per_minute_t getRPM() override;

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
        units::velocity::meters_per_second_t ShipSpeed);


private:
    unsigned int mId; ///< ID of the engine
    bool mIsWorking; ///< a boolean to indicate the engine is working

    // for interpolation
    QMap<units::power::kilowatt_t,
         units::angular_velocity::revolutions_per_minute_t>
        mBrakePowerToRPMMap; ///< RPM to power curve from the manufacturer

    QMap<units::power::kilowatt_t, double>
        mBrakePowerToEfficiencyMap; ///< eff to power curve from manufacturer

    unsigned int counter = 0; ///< keep track of new ids

    double mEfficiency; ///< current efficiency of the engine
    units::angular_velocity::revolutions_per_minute_t
        mRPM; ///< current rpm of the engine

    units::power::kilowatt_t mRawPower; ///< power without considering eff
    units::power::kilowatt_t mCurrentOutputPower; ///< power with eff
    units::power::kilowatt_t mPreviousOutputPower; ///< previous power with eff

    /**
     * @brief Updates the current step of the engine's operation.
     */
    void updateCurrentStep();

};

#endif // SHIPENGINE_H
