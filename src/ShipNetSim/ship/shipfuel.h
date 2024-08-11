/**
 * @file ShipFuel.h
 * @brief This file contains the definition of the ShipFuel class,
 *        which is responsible for managing the ship's fuel operations.
 *
 * @author Ahmed Aredah
 * @date 10.12.2023
 */
#ifndef SHIPFUEL_H
#define SHIPFUEL_H

#include <iostream>
#include <map>
#include <QVector>
#include "../../third_party/units/units.h"

namespace ShipNetSimCore
{

/**
 * @class ShipFuel
 * @brief The ShipFuel class manages the ship's fuel operations.
 */
class ShipFuel
{


public:

    /**
     * @enum FuelType
     * @brief Represents the type of fuel used.
     *
     * The FuelType enumeration provides a list of various types of
     * fuels that can be used in a ship's engine. Each fuel type
     * has different properties and efficiencies, which are considered
     * in the calculations for energy consumption and emissions.
     *
     * The available fuel types are:
     * - Diesel: A type of fuel derived from petroleum, commonly used
     *   in engines.
     * - HFO (Heavy Fuel Oil): A thick, residual oil used in engines
     *   and boilers.
     * - LNG (Liquefied Natural Gas): A cleaner alternative to other
     *   fossil fuels, used in engines designed to handle gas fuels.
     * - MDO (Marine Diesel Oil): A blend of gasoil and heavy fuel oil,
     *   used in ship engines.
     * - MGO (Marine Gas Oil): A type of fuel oil used in ship engines,
     *   derived from distilling crude oil.
     * - Biofuel: A renewable energy source made from organic materials,
     *   used as an alternative to fossil fuels in engines.
     */
    enum class FuelType {
        Diesel,
        HFO, // Heavy Fuel Oil
        LNG, // Liquefied Natural Gas
        MDO, // Marine Diesel Oil
        MGO, // Marine Gas Oil
        Biofuel,
        Electric
    };

    /**
     * @brief Gets the weight of the fuel.
     * @param quantity The quantity of the fuel.
     * @param fuelType The type of the fuel.
     * @return The weight of the fuel.
     */
    static units::mass::kilogram_t getWeight(units::volume::liter_t quantity,
                                             FuelType fuelType);

    /**
     * @brief Converts kilowatt hours to liters of fuel.
     * @param energy The energy in kilowatt hours.
     * @param fuelType The type of the fuel.
     * @return The equivalent liters of fuel.
     */
    static units::volume::liter_t convertKwhToLiters(
        units::energy::kilowatt_hour_t energy,
        FuelType fuelType);

    /**
     * @brief Converts liters of fuel to kilowatt hours.
     * @param volume The volume of the fuel in liters.
     * @param fuelType The type of the fuel.
     * @return The equivalent kilowatt hours of energy.
     */
    static units::energy::kilowatt_hour_t convertLitersToKwh(
        units::volume::liter_t volume,
        FuelType fuelType);

    /**
     * @brief convert liters to carbon dioxide in kg.
     * @param volume The volume of the fuel in liters.
     * @param fuelType The type of the fuel.
     * @return The equivalent CO2 content.
     */
    static units::mass::kilogram_t convertLitersToCarbonDioxide(
        units::volume::liter_t volume,
        FuelType fuelType);

    /**
     * @brief convert liters to sulfur dioxide in kg.
     * @param volume The volume of the fuel in liters.
     * @param fuelType The type of the fuel.
     * @return The equivalent SO2 content.
     */
    static units::mass::kilogram_t convertLitersToSulfurDioxide(
        units::volume::liter_t volume,
        FuelType fuelType);

    /**
     * @brief get fuel types supported in the simulator as a QStringList.
     * @return a list of fuel types.
     */
    static QStringList getFuelTypeList();

    /**
     * @brief get fuel types supported in the simulator as a vector.
     * @return a vector of fuel types.
     */
    static QVector<ShipFuel::FuelType> getFuelTypes();

    /**
     * @brief convert fuel type to string.
     * @param fuelType The type of the fuel.
     * @return a string representation of the fuel type.
     */
    static QString convertFuelTypeToString(ShipFuel::FuelType fuelType);

    struct FuelProperties {
        units::density::kilograms_per_liter_t density;
        units::energy::megajoule_t calorificValue;
        double carbonContent;
        double sulfureContent;
    };
private:

    /**
     * @brief Map of fuel types to their properties.
     */
    static std::map<FuelType, FuelProperties> fuelDetails;

    /**
     * @brief A list of all supported fuel types in the simulator.
     */
    static QVector<FuelType> mFuelTypes;

};
};
#endif // SHIPFUEL_H
