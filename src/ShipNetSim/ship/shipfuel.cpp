#include "shipfuel.h"

namespace ShipNetSimCore
{

std::map<ShipFuel::FuelType, ShipFuel::FuelProperties> ShipFuel::fuelDetails =
    {
    {ShipFuel::FuelType::Diesel,
     {units::density::kilograms_per_liter_t(0.820),
         units::energy::megajoule_t(45.5),
         0.875, 0.000015}},
    {ShipFuel::FuelType::HFO,
     {units::density::kilograms_per_liter_t(1.010),
      units::energy::megajoule_t(40.9),
      0.85, 0.02}},
    {ShipFuel::FuelType::LNG,
     {units::density::kilograms_per_liter_t(0.450),
      units::energy::megajoule_t(55.5),
      0.75, 0.001}},
    {ShipFuel::FuelType::MDO,
     {units::density::kilograms_per_liter_t(0.890),
     units::energy::megajoule_t(44.0),
      0.86, 0.0015}},
    {ShipFuel::FuelType::MGO,
     {units::density::kilograms_per_liter_t(0.860),
     units::energy::megajoule_t(45.84),
      0.875, 0.005}},
    {ShipFuel::FuelType::Biofuel,
     {units::density::kilograms_per_liter_t(0.860),
     units::energy::megajoule_t(39.0),
      0.67, 0.000015}},
    {ShipFuel::FuelType::Electric,
     {units::density::kilograms_per_liter_t(std::nan("noValue")),
     units::energy::megajoule_t(std::nan("noValue")),
      std::nan("noValue"),
      std::nan("noValue")}}
};



QVector<ShipFuel::FuelType> ShipFuel::mFuelTypes =
    {
    ShipFuel::FuelType::Diesel,
    ShipFuel::FuelType::HFO,
    ShipFuel::FuelType::LNG,
    ShipFuel::FuelType::MDO,
    ShipFuel::FuelType::MGO,
    ShipFuel::FuelType::Biofuel,
    ShipFuel::FuelType::Electric
};

QString ShipFuel::convertFuelTypeToString(ShipFuel::FuelType fuelType)
{
    switch(fuelType)
    {
    case FuelType::Biofuel:
        return "Bio-Diesel";
        break;
    case FuelType::Diesel:
        return "Diesel";
        break;
    case FuelType::Electric:
        return "Electric";
        break;
    case FuelType::HFO:
        return "Heavy Fuel Oil";
        break;
    case FuelType::LNG:
        return "Liquefied Natural Gas";
        break;
    case FuelType::MDO:
        return "Marine Diesel Oil";
        break;
    case FuelType::MGO:
        return "Marine Gas Oil";
        break;
    default:
        throw std::runtime_error("Fuel Not Implemented");
        return "";
    }
}


units::mass::kilogram_t ShipFuel::getWeight(units::volume::liter_t quantity,
                                            FuelType fuelType)
{
    auto it = fuelDetails.find(fuelType);
    if (it != fuelDetails.end()) {
        if (std::isnan(it->second.density.value()))
        { return units::mass::kilogram_t(0.0); };
        auto density = it->second.density;
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

    auto it = fuelDetails.find(fuelType);
    if (it != fuelDetails.end()) {
        if (std::isnan(it->second.density.value()))
        { return units::volume::liter_t(0.0); };

        auto density = it->second.density; // Get the density in kg/L

        auto calorificValueIt = fuelDetails.find(fuelType);
        if(calorificValueIt != fuelDetails.end()) {
            auto calorificValue = calorificValueIt->second.calorificValue;
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
    auto it = fuelDetails.find(fuelType);
    if (it != fuelDetails.end()) {
        if (std::isnan(it->second.density.value()))
        { return units::energy::kilowatt_hour_t(0.0); };

        auto density = it->second.density; // Get the density in kg/L

        // Find the calorific value of the fuel type in calorificValues
        auto calorificValueIt = fuelDetails.find(fuelType);
        if(calorificValueIt != fuelDetails.end()) {
            auto calorificValue = calorificValueIt->second.calorificValue;
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
};

units::mass::kilogram_t ShipFuel::convertLitersToCarbonDioxide(
    units::volume::liter_t volume,
    FuelType fuelType)
{
    auto kg = getWeight(volume, fuelType);
    auto it = fuelDetails.find(fuelType);
    if (it != fuelDetails.end()) {
        if (std::isnan(it->second.carbonContent))
        { return units::mass::kilogram_t(0.0); };


        auto CC = it->second.carbonContent; // Get the Carbon content as a fraction

        // 44 is the molar mass of CO2,
        // 12 is the molar mass of carbon.
        return
            units::mass::kilogram_t(CC * (44.0 / 12.0) * kg);
    }
};

units::mass::kilogram_t ShipFuel::convertLitersToSulfurDioxide(
    units::volume::liter_t volume,
    FuelType fuelType)
{
    auto kg = getWeight(volume, fuelType);
    auto it = fuelDetails.find(fuelType);
    if (it != fuelDetails.end()) {
        if (std::isnan(it->second.sulfureContent))
        { return units::mass::kilogram_t(0.0); };


        auto CC = it->second.sulfureContent; // Get the Sulfur content as a fraction

        // 44 is the molar mass of SO2,
        // 12 is the molar mass of sulfur.
        return
            units::mass::kilogram_t(CC * (64.0 / 32.0) * kg);
    }
}

QVector<ShipFuel::FuelType> ShipFuel::getFuelTypes() {
    return mFuelTypes;
};

QStringList ShipFuel::getFuelTypeList() {
    QStringList fuelTypes;
    for (auto& ft: mFuelTypes) {
        fuelTypes << convertFuelTypeToString(ft);
    }
    return fuelTypes;
}

};
