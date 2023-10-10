#include "ShipFuel.h"

std::map<ShipFuel::FuelType,
         units::density::kilograms_per_liter_t> ShipFuel::fuelDensities =
    {
        {ShipFuel::FuelType::Diesel,
         units::density::kilograms_per_liter_t(0.820)},
        {ShipFuel::FuelType::HFO,
         units::density::kilograms_per_liter_t(1.010)},
        {ShipFuel::FuelType::LNG,
         units::density::kilograms_per_liter_t(0.450)},
        {ShipFuel::FuelType::MDO,
         units::density::kilograms_per_liter_t(0.890)},
        {ShipFuel::FuelType::MGO,
         units::density::kilograms_per_liter_t(0.860)},
        {ShipFuel::FuelType::Biofuel,
         units::density::kilograms_per_liter_t(0.860)}
};

std::map<ShipFuel::FuelType,
         units::energy::megajoule_t> ShipFuel::calorificValues =
    {
        {ShipFuel::FuelType::Diesel,
         units::energy::megajoule_t(45.5)},
        {ShipFuel::FuelType::HFO,
         units::energy::megajoule_t(40.9)},
        {ShipFuel::FuelType::LNG,
         units::energy::megajoule_t(55.5)},
        {ShipFuel::FuelType::MDO,
         units::energy::megajoule_t(44.0)},
        {ShipFuel::FuelType::MGO,
         units::energy::megajoule_t(45.84)},
        {ShipFuel::FuelType::Biofuel,
         units::energy::megajoule_t(39.0)}
};


units::mass::kilogram_t ShipFuel::getWeight(units::volume::liter_t quantity,
                                            FuelType fuelType)
{
    auto it = fuelDensities.find(fuelType);
    if (it != fuelDensities.end()) {
        auto density = it->second;
        return quantity * density;
    }
    else
    {
        std::cerr << "Invalid fuel type" << std::endl;
        return units::mass::kilogram_t(0.0);
    }
}

units::volume::liter_t ShipFuel::convertKwhToLiters(
    units::energy::kilowatt_hour_t energy,
    FuelType fuelType)
{
    // Convert kWh to MJ
    units::energy::megajoule_t energyInMJ =
        energy.convert<units::energy::megajoule>();

    auto it = fuelDensities.find(fuelType);
    if (it != fuelDensities.end()) {
        auto density = it->second; // Get the density in kg/L

        auto calorificValueIt = calorificValues.find(fuelType);
        if(calorificValueIt != calorificValues.end()) {
            auto calorificValue = calorificValueIt->second;
            // Convert energy in MJ to mass in kg
            auto mass = (energyInMJ / calorificValue).value();
            // Convert mass in kg to volume in liter
            auto volume_l =
                units::volume::liter_t(mass * density.value());
            return volume_l;
        }
        else
        {
            std::cerr <<
                "Calorific value not found for fuel type" << std::endl;
            return units::volume::liter_t(0.0);
        }

    }
    else
    {
        std::cerr << "Invalid fuel type" << std::endl;
        return units::volume::liter_t(0.0);
    }
}

units::energy::kilowatt_hour_t ShipFuel::convertLitersToKwh(
    units::volume::liter_t volume,
    FuelType fuelType)
{
    // Find the density of the fuel type in fuelDensities
    auto it = fuelDensities.find(fuelType);
    if (it != fuelDensities.end()) {
        auto density = it->second; // Get the density in kg/L

        // Find the calorific value of the fuel type in calorificValues
        auto calorificValueIt = calorificValues.find(fuelType);
        if(calorificValueIt != calorificValues.end()) {
            auto calorificValue = calorificValueIt->second;
            // Convert volume in liters to mass in kg
            auto mass_kg = (volume / density).value();
            // Convert mass in kg to energy in MJ
            auto energy_mj =
                units::energy::megajoule_t(mass_kg * calorificValue.value());
            // Convert energy in MJ to energy in kWh
            auto energy_kwh =
                energy_mj.convert<units::energy::kilowatt_hour>();
            return energy_kwh;
        }
        else
        {
            std::cerr <<
                "Calorific value not found for fuel type" << std::endl;
            return units::energy::kilowatt_hour_t(0.0);
        }
    }
    else
    {
        std::cerr << "Invalid fuel type" << std::endl;
        return units::energy::kilowatt_hour_t(0.0);
    }
}
