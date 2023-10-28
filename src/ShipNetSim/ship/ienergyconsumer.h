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
#include "IEnergySource.h"
#include <QString>
#include <any>

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
    ~IEnergyConsumer();

    /**
     * @brief Initialize the energy-consuming component.
     *
     * This method is called to initialize the energy-consuming
     * component with a reference to the ship it is part of,
     * an energy source that provides the energy it consumes, and any
     * additional parameters required for initialization.
     *
     * @param host A pointer to the ship the component is part of.
     * @param energySource A pointer to the energy source
     * that provides the energy the component consumes.
     * @param parameters A map of additional parameters
     * required for initialization.
     */
    virtual void initialize(Ship *host, IEnergySource *energySource,
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
     * @param energySource A pointer to the energy source that provides
     * the energy the component consumes.
     */
    void setEnergySource(IEnergySource *energySource);

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
    IEnergySource* getEnergySource() const;

    /**
     * @brief Calculate the amount of energy consumed by the component
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
    energyConsumed(units::time::second_t timeStep) = 0;

protected:
    Ship *mHost;  ///< A pointer to the host ship.
    IEnergySource *mEnergySource;  ///< A pointer to the energy source.
};

#endif // IENERGYCONSUMER_H
