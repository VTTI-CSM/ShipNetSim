#ifndef SHIPFUEL_H
#define SHIPFUEL_H

#include <iostream>
#include <map>
#include "../../third_party/units/units.h"
class ShipFuel
{
public:
    enum class FuelType {
        Diesel,
        HFO, // Heavy Fuel Oil
        LNG, // Liquefied Natural Gas
        MDO, // Marine Diesel Oil
        MGO, // Marine Gas Oil
        Biofuel,
    };

    static units::mass::kilogram_t getWeight(units::volume::liter_t quantity,
                                             FuelType fuelType);
    static units::volume::liter_t convertKwhToLiters(
        units::energy::kilowatt_hour_t energy,
        FuelType fuelType);

    static units::energy::kilowatt_hour_t convertLitersToKwh(
        units::volume::liter_t volume,
        FuelType fuelType);

private:
    static std::map<FuelType, units::density::kilograms_per_liter_t>
        fuelDensities;
    static std::map<FuelType, units::energy::megajoule_t> calorificValues;

};

#endif // SHIPFUEL_H
