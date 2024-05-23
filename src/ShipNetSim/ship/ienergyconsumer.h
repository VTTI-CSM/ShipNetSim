/**
 * @file IEnergyConsumer.h
 *
 * @brief This file contains the declaration of the IEnergyConsumer
 * interface, which represents an energy-consuming component of a ship.
 *
 * This interface defines the methods that should be implemented by
 * any class that represents a component of a ship that consumes
 * energy, such as engines, shields, or other systems.
 * The energy-consuming component is initialized with a reference
 * to the ship it is a part of and an energy source that provides
 * the energy it consumes.
 *
 * @author Ahmed Aredah
 * @date 10.10.2023
 */
#ifndef IENERGYCONSUMER_H
#define IENERGYCONSUMER_H

#include "../../third_party/units/units.h"
#include "ienergysource.h"
#include <QString>
#include <QVector>
#include <any>
#include "../../third_party/units/units.h"

namespace ShipNetSimCore
{
class Ship;  // Forward declaration of the class ship

/**
 * @class IEnergyConsumer
 *
 * @brief The IEnergyConsumer interface represents an energy-consuming
 * component of a ship.
 *
 * This interface defines the methods that should be implemented by
 * any class that represents a component of a ship that consumes energy,
 * such as engines, shields, or other systems.
 * The energy-consuming component is initialized with a reference to
 * the ship it is a part of and an energy source that provides the
 * energy it consumes.
 */
class IEnergyConsumer
{
public:
    /**
     * @brief Constructor for the IEnergyConsumer interface.
     */
    IEnergyConsumer();

    /**
     * @brief Destructor for the IEnergyConsumer interface.
     */
    virtual ~IEnergyConsumer();

    /**
     * @brief Initialize the energy-consuming component.
     *
     * This method is called to initialize the energy-consuming
     * component with a reference to the ship it is part of,
     * an energy source that provides the energy it consumes, and any
     * additional parameters required for initialization.
     *
     * @param host A pointer to the ship the component is part of.
     * @param energySource QVector of pointers to the energy source
     * that provides the energy the component consumes.
     * @param parameters A map of additional parameters
     * required for initialization.
     */
    virtual void initialize(Ship *host, QVector<IEnergySource*> energySource,
                            const QMap<QString, std::any>& parameters) = 0;

    /**
     * @brief Set the host ship for the energy-consuming component.
     *
     * @param host A pointer to the ship the component is part of.
     */
    void setHost(Ship *host);

    /**
     * @brief Set the energy source for the energy-consuming component.
     *
     * @param energySources Pointers to the energy sources that provides
     * the energy the component consumes.
     */
    void setEnergySources(QVector<IEnergySource*> energySources);

    /**
     * @brief Set the additional parameters required for initialization.
     *
     * @param parameters A map of additional parameters required for
     * initialization.
     */
    virtual void setParameters(const QMap<QString, std::any>& parameters) = 0;

    /**
     * @brief Get the host ship for the energy-consuming component.
     *
     * @return A const pointer to the ship the component is part of.
     */
    const Ship* getHost() const;

    /**
     * @brief Get the energy source for the energy-consuming component.
     *
     * @return A pointer to the energy source that provides the energy
     * the component consumes.
     */
    IEnergySource* getCurrentEnergySource() const;

    /**
     * @brief Calculate and consume the amount of energy used by the component
     * in a given time step.
     *
     * This method should return an instance of EnergyConsumptionData
     * that contains the amount of energy consumed by the component
     * in the given time step.
     *
     * @param timeStep The time step for which to calculate the
     * energy consumption.
     * @return An instance of EnergyConsumptionData that contains
     * the amount of energy consumed by the component.
     */
    virtual EnergyConsumptionData
    consumeUsedEnergy(units::time::second_t timeStep) = 0;

    /**
     * @brief gets the cumulative energy used and consumed by the component
     * during simulation time.
     * @return An instance of EnergyConsumptionData that contains
     * the amount of energy consumed by the component.
     */
    virtual units::energy::kilowatt_hour_t getCumEnergyConsumption() = 0;

private:
    static std::map<ShipFuel::FuelType, units::volume::liter_t>
    initializeFuelConsumption() {
        std::map<ShipFuel::FuelType, units::volume::liter_t> map;
        for (auto& ft: ShipFuel::getFuelTypes()) {
            map.insert({ft, units::volume::liter_t(0.0)});
        }
        return map;  // Make sure to return the map
    }

protected:
    Ship *mHost;  ///< A pointer to the host ship.
    QVector<IEnergySource*> mEnergySources;  ///< Pointers to the energy sources.
    IEnergySource* mCurrentEnergySource; ///< Current E.S to draw energy from.
    units::energy::kilowatt_hour_t mCumEnergyConsumption =
        units::energy::kilowatt_hour_t(0.0);
    std::map<ShipFuel::FuelType, units::volume::liter_t> mCumFuelConsumption;
};
};
#endif // IENERGYCONSUMER_H
