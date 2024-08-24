/**
 * @file ReadShips.h
 * @brief This file provides functions to read and process ship data
 * from a file.
 *
 * The file format is expected to have specific parameters separated
 * by delimiters.
 * Each parameter is then converted to the appropriate type using
 * conversion functions defined in this file.
 * The resulting data is then used to create a Ship object.
 *
 * The conversion functions use the units library to handle various
 * units of measurement.
 *
 * This file is part of a project that handles ship data and performs
 * calculations based on that data.
 *
 * @author Ahmed Aredah
 * @date 10.20.2023
 */

#ifndef SHIPSLIST_H
#define SHIPSLIST_H
#include <QString>
#include <QVector>
#include <QCoreApplication>
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <any>
#include "../../third_party/units/units.h"
#include "qvariant.h"
#include "../network/point.h"
#include "../network/optimizednetwork.h"
#include "ship.h"
#include "../utils/utils.h"
#include "ishipengine.h"
#include "../export.h"

namespace ShipNetSimCore
{
/**
 * @namespace ShipsList
 * @brief This namespace contains functions to read and process
 * ship data from a file.
 */
namespace ShipsList
{

/**
 * @var QList<QString> delim
 * @brief List of delimiters used to separate values in a file.
 */
static QList<QString> delim = {"\t", ";", ","};

/**
 * @struct ParamInfo
 * @brief Contains information about a parameter including its
 * name and a function to convert it to the correct type
 * along with an isOptional parameter to that function.
 */
struct SHIPNETSIM_EXPORT ParamInfo
{
    QString name;       ///< Name of the parameter.
    /// Function to convert the parameter from a string to the correct type.
    std::function<std::any(const QString&, bool&)> converter;
    bool isOptional;  ///< handle nan or na if passed to the function

    // Adding a constructor for easier initialization
    ParamInfo(const QString& name,
              std::function<std::any(const QString&, bool&)> converter,
              bool isOptional)
        : name(name), converter(converter), isOptional(isOptional) {}

    // Default constructor for creating an empty instance
    ParamInfo() : name(""), converter(nullptr), isOptional(false) {}

    // Method to check if the ParamInfo instance is empty
    bool isEmpty() const {
        return name.isEmpty() && converter == nullptr && !isOptional;
    }
};

static ShipNetSimCore::ShipsList::ParamInfo*
findParamInfoByKey(const QString& key, QVector<ParamInfo>& parameters)
{
    for (auto& param : parameters) {
        if (param.name.trimmed().toLower() == key.trimmed().toLower()) {
            return &param;
        }
    }

    // Return an nullptr
    return nullptr;
}

/**
 * @brief Converts a string to a double, with error checking.
 * @param str The string to convert.
 * @param errorMsg Error message to display if the conversion fails.
 * @return The converted double.
 */
static double convertToDouble(const QString& str, const char* errorMsg, bool isOptional)
{
    if (isOptional)
    {
        // if the string has nan, return the default nan
        if (str.contains("na", Qt::CaseInsensitive))
        {
            return std::nan("uninitialized");
        }
    }
    // otherwize, do the conversion
    bool ok;
    double result = str.toDouble(&ok); // convert to double
    if (!ok) // check if the conversion worked!
    {
        qFatal(errorMsg, str.toUtf8().constData()); // quit the process if not
    }
    return result;
}

/**
 * @brief Converts a string to an int, with error checking.
 * @param str The string to convert.
 * @param errorMsg Error message to display if the conversion fails.
 * @return The converted int.
 */
static int convertToInt(const QString &str,
                        const char *errorMsg,
                        bool isOptional)
{
    if (isOptional)
    {
        // if the string has nan, return the default nan
        if (str.contains("na", Qt::CaseInsensitive))
        {
            return -100;
        }
    }
    // otherwize, do the conversion
    bool ok;
    int result = (int)(str.toDouble(&ok)); // convert to int
    if (!ok) // check if the conversion worked!
    {
        qFatal(errorMsg, str.toUtf8().constData()); // quit the process if not
    }
    return result;
}

/**
 * @brief Converts a string to a bool.
 * @param str The string to convert.
 * @return The converted bool.
 */
static std::any toBoolT(const QString& str, bool isOptional)
{
    // if the string has no information, return nothing
    if (isOptional)
    {
        // if the string has nan, return the default nan
        if (str.contains("na", Qt::CaseInsensitive))
        {
            return std::nan("uninitialized");
        }
    }
    // otherwize, do the conversion
    return QVariant(str).toBool(); // return the bool equivelant
}

/**
 * @brief Converts a string to an int, with error checking.
 * @param str The string to convert.
 * @return The converted int.
 */
static std::any toIntT(const QString& str, bool isOptional)
{
    return convertToInt(str, "%1 is not an int!", isOptional);
}

/**
 * @brief Converts a string to a double, with error checking.
 * @param str The string to convert.
 * @return The converted double.
 */
static std::any toDoubleT(const QString& str, bool isOptional)
{
    return convertToDouble(str, "%1 is not a double!\n", isOptional);
}

/**
 * @brief Converts a string to a nanometer unit, with error checking.
 * @param str The string to convert.
 * @return The converted nanometer unit.
 */
static std::any toNanoMeterT(const QString& str, bool isOptional)
{
    double result =
        convertToDouble(str, "%s is not a valid double for nanometers!\n",
                        isOptional);
    return units::length::nanometer_t(result);
}

/**
 * Converts a string to meter_t unit.
 * The string is first converted to a double value.
 * If the conversion is invalid, an error message is shown.
 * The double value is then used to create a meter_t object.
 * @param str The input string to be converted.
 * @return A std::any object holding the meter_t unit.
 */
static std::any toMeterT(const QString& str, bool isOptional)
{
    double result =
        convertToDouble(str, "%s is not a valid double for meters!\n",
                        isOptional);
    return units::length::meter_t(result);
}

/**
 * Converts a string to cubic_meter_t unit.
 * The string is first converted to a double value.
 * If the conversion is invalid, an error message is shown.
 * The double value is then used to create a cubic_meter_t object.
 * @param str The input string to be converted.
 * @return A std::any object holding the cubic_meter_t unit.
 */
static std::any toCubicMeterT(const QString& str, bool isOptional)
{
    double result =
        convertToDouble(str, "%s is not a valid double for cubic meters!\n",
                        isOptional);
    return units::volume::cubic_meter_t(result);
}

/**
 * Converts a string to liter_t unit.
 * The string is first converted to a double value.
 * If the conversion is invalid, an error message is shown.
 * The double value is then used to create a liter_t object.
 * @param str The input string to be converted.
 * @return A std::any object holding the liter_t unit.
 */
static std::any toLiterT(const QString& str, bool isOptional)
{
    double result =
        convertToDouble(str, "%s is not a valid double for liters!\n",
                        isOptional);
    return units::volume::liter_t(result);
}

/**
 * Converts a string to square_meter_t unit.
 * The string is first converted to a double value.
 * If the conversion is invalid, an error message is shown.
 * The double value is then used to create a square_meter_t object.
 * @param str The input string to be converted.
 * @return A std::any object holding the square_meter_t unit.
 */
static std::any toSquareMeterT(const QString& str, bool isOptional)
{
    double result =
        convertToDouble(str, "%s is not a valid double for square meters!\n",
                        isOptional);
    return units::area::square_meter_t(result);
}

/**
 * Converts a string to degree_t unit.
 * The string is first converted to a double value.
 * If the conversion is invalid, an error message is shown.
 * The double value is then used to create a degree_t object.
 * @param str The input string to be converted.
 * @return A std::any object holding the degree_t unit.
 */
static std::any toDegreesT(const QString& str, bool isOptional)
{
    double result =
        convertToDouble(str, "%s is not a valid double for angle degrees!\n",
                        isOptional);
    return units::angle::degree_t(result);
}

/**
 * Converts a string to metric_ton_t unit.
 * The string is first converted to a double value.
 * If the conversion is invalid, an error message is shown.
 * The double value is then used to create a metric_ton_t object.
 * @param str The input string to be converted.
 * @return A std::any object holding the metric_ton_t unit.
 */
static std::any toTonsT(const QString& str, bool isOptional)
{
    double result =
        convertToDouble(str, "%s is not a valid double for metric tons!\n",
                        isOptional);
    return units::mass::metric_ton_t(result);
}

static std::any toMeterPerSecond(const QString& str, bool isOptional)
{
    double result = convertToDouble(str, "%s is not a valid double for "
                                         "speed in knot!\n",
                                    isOptional);
    return units::velocity::knot_t(result).convert<
        units::velocity::meters_per_second>();
}

/**
 * Returns the input string.
 * This is a no-op conversion, used for parameters that are
 * already in the correct format.
 * @param str The input string.
 * @return A std::any object holding the input string.
 */
static std::any toQStringT(const QString& str, bool isOptional)
{
    // if the string has no information, return nothing
    if (isOptional)
    {
        if (str.contains("na", Qt::CaseInsensitive))
        {
            return "";
        }
    }

    return str;
}

static std::any toEnginePowerVectorT(const QString& str, bool isOptional)
{
    // Define a vector to hold data
    QVector<units::power::kilowatt_t> result;

    if (isOptional)
    {
        // if the string has no information, return nothing
        if (str.contains("na", Qt::CaseInsensitive))
        {
            return result;
        }
    }

    QStringList pointsData = str.split(delim[1]);
    for (const QString& pointData : pointsData)
    {
        units::power::kilowatt_t power =
            units::power::kilowatt_t(
                convertToDouble(pointData.trimmed(),
                                "Invalid double conversion "
                                "for key: %s", false));
        result.push_back(power);
    }

    if (result.size() != 4) {
        // If there are not exactly 4 elements representing l1, l2, l3, l4,
        // terminate with a fatal error.
        qFatal("Malformed Engine Properties."
               "\nEngine Operational Power Settings must have "
               "4 data points representing L1, L2, L3, L4 on the "
               "engine layout!");
    }

    return result;

}
/**
 * @brief Converts a string to a QVector of engine properties.
 *
 * This function takes a string that represents a vector of engine properties
 * in the following order: Brake Powers, revolutions per minute, Efficiencies.
 * The values are separated by a predefined delimiter, and each
 * key and value are also separated by a delimiter.
 * The function then converts each value to their respective
 *  units and adds them to a QVector,
 * which is then returned as a std::any object.
 * The engine properties should be specified at L1, L2, L3, L4 of the
 * engine layout.
 *
 * @param str The input string to be converted.
 * @return A std::any object that holds the QVector<EngineProperites>.
 */
static std::any toEnginePowerRPMEfficiencyT(const QString& str, bool isOptional)
{
    // Define a vector to hold data
    QVector<IShipEngine::EngineProperties> result;

    if (isOptional)
    {
        // if the string has no information, return nothing
        if (str.contains("na", Qt::CaseInsensitive))
        {
            return result;
        }
    }

    // Split the string into pairs.
    QStringList pointsData = str.split(delim[1]);
    for (const QString& pointData : pointsData)
    {
        // Split each pair into key and value.
        QStringList values = pointData.split(delim[2]);

        // If the properties are not 3 values, return error.
        if (values.size() != 3)
        {
            // If there are not exactly 3 elements, terminate with a fatal
            // error.
            qFatal("Malformed Engine Property: %s"
                   "\nEngine Power-RPM-Efficiency Mapping must have "
                   "3 values representing Break Power, RPM, Efficiency!",
                   pointData.toUtf8().constData());
        }

        // Convert key and value to their respective units.
        units::power::kilowatt_t power =
            units::power::kilowatt_t(
                convertToDouble(values[0].trimmed(),
                                "Invalid double conversion "
                                "for key: %s", false));

        units::angular_velocity::revolutions_per_minute_t rpm =
            units::angular_velocity::revolutions_per_minute_t(
                convertToDouble(values[1].trimmed(),
                                "Invalid double conversion "
                                "for value: %s", false));

        double eff =
            convertToDouble(values[2].trimmed(),
                            "Invalud double conversion "
                            "for value: %s", false);

        result.push_back(IShipEngine::EngineProperties(power, rpm, eff));
    }

    return result;
}

/**
 * @brief Converts a string to a QVector of shared pointers to Point objects.
 *
 * This function takes a string that represents a list of coordinate
 * pairs separated by a predefined delimiter.
 * Each coordinate pair is then split into x and y coordinates,
 * which are then converted to meters and used to
 * construct a Point object. The Point objects are stored as
 * shared pointers in a QVector, which is then returned
 * as a std::any object.
 *
 * @param str The input string to be converted.
 * @return A std::any object that holds the QVector of shared
 * pointers to Point objects.
 */
static std::any toPathPointsT(const QString& str, bool isOptional)
{
    // Define a QVector to hold the shared pointers to Point objects.
    QVector<std::shared_ptr<GPoint>> points;

    // if the string has no information, return nothing
    if (isOptional)
    {
        if (str.contains("na", Qt::CaseInsensitive))
        {
            return points;
        }
    }


    // Split the string into coordinate pairs.
    QStringList pairs = str.split(delim[1]);
    for (const QString& pair : pairs)
    {
        // Split each pair into x and y coordinates.
        QStringList keyValuePair = pair.split(delim[2]);

        // Convert x coordinate to meters.
        auto x1 =
            units::angle::degree_t(
                convertToDouble(keyValuePair[0].trimmed(),
                                "Invalid double conversion for x1: %s", false));

        // Convert y coordinate to meters.
        auto x2 =
            units::angle::degree_t(
                convertToDouble(keyValuePair[1].trimmed(),
                                "Invalid double conversion for x2: %s", false));

        // check if the coordinates are WGS84
        if (std::abs(x1.value()) > 180.0 || std::abs(x2.value()) > 90.0)
        {
            qFatal("Not WGS Coordinate Points: %s", pair.toUtf8().constData());
        }

        // If there are exactly two elements (x and y), create a Point
        // object and add it to the QVector.
        if (keyValuePair.size() == 2)
        {
            auto p = std::make_shared<GPoint>(x1, x2,
                                             "Ship User Path Point");
            points.append(p);
        }
        else
        {
            // If there are not exactly two elements, terminate with a
            // fatal error.
            qFatal("Malformed key-value pair: %s", pair.toUtf8().constData());
        }
    }
    return points;
}


/**
 * Converts a string of appendages wetted surfaces to a QMap.
 * Each pair of appendage and its wetted surface area is
 * separated by `delim[1]`, and the key and value in each pair are
 * separated by `delim[2]`.
 * The string values are then converted to appropriate data types.
 * If any malformed key-value pair is encountered, the program
 * exits with an error message.
 * @param str The input string representing appendages and their
 * wetted surface areas.
 * @return A std::any object holding the QMap with ShipAppendage
 * as key and square meter as value.
 */
static std::any toAppendagesWetSurfacesT(const QString& str, bool isOptional)
{
    QMap<Ship::ShipAppendage, units::area::square_meter_t> appendages;

    // if the string has no information, return nothing
    if (isOptional)
    {
        if (str.contains("na", Qt::CaseInsensitive)) // it is nan or na
        {
            return appendages;
        }
    }

    // Split the string into pairs of appendage and area
    QStringList pairs = str.split(delim[1]);

    for (const QString& pair : pairs)
    {
        // Split each pair into appendage and area
        QStringList keyValuePair = pair.split(delim[2]);

        // Convert appendage to int and area to double
        int rawAppendage =
            convertToInt(keyValuePair[0].trimmed(),
                         "Invalid int conversion for appendage: %s", false);
        auto area =
            units::area::square_meter_t(
                convertToDouble(keyValuePair[1].trimmed(),
                                "Invalid double conversion for area: %s", false));

        if (keyValuePair.size() == 2)
        {
            Ship::ShipAppendage appendage =
                static_cast<Ship::ShipAppendage>(rawAppendage);
            appendages[appendage] = area;
        }
        else
        {
            qFatal("Malformed key-value pair: %s", pair.toUtf8().constData());
        }
    }
    return appendages;
}

/**
 * Converts a string to CStern enum value.
 * The string is first trimmed and converted to an integer.
 * If the conversion is invalid, an error message is shown and the
 * program exits.
 * The integer is then casted to CStern enum value.
 * @param str The input string to be converted.
 * @return A std::any object holding the CStern enum value.
 */
static std::any toCSternT(const QString& str, bool isOptional)
{
    // Convert string to int and cast to CStern enum
    int rawValue = convertToInt(str.trimmed(),
                                "Invalid conversion to int for CStern: %s",
                                isOptional);
    return static_cast<Ship::CStern>(rawValue);
}

/**
 * Converts a string to FuelType enum value.
 * The string is first trimmed and converted to an integer.
 * If the conversion is invalid, an error message is shown and the
 * program exits.
 * The integer is then casted to FuelType enum value.
 * @param str The input string to be converted.
 * @return A std::any object holding the FuelType enum value.
 */
static std::any toFuelTypeT(const QString& str, bool isOptional)
{
    // Convert string to int and cast to FuelType enum
    int rawValue = convertToInt(str.trimmed(),
                                "Invalid conversion to int for fuel type: %s",
                                isOptional);
    return static_cast<ShipFuel::FuelType>(rawValue);
}


static QString toString(const std::any& value) {
    if (value.type() == typeid(QString))
    {
        return std::any_cast<QString>(value);
    } else if (value.type() == typeid(double))
    {
        return QString::number(std::any_cast<double>(value));
    } else if (value.type() == typeid(int))
    {
        return QString::number(std::any_cast<int>(value));
    } else if (value.type() == typeid(bool))
    {
        return std::any_cast<bool>(value) ? "true" : "false";
    } else if (value.type() == typeid(units::length::meter_t))
    {
        return QString::number(
            std::any_cast<units::length::meter_t>(value).value());
    } else if (value.type() == typeid(units::volume::cubic_meter_t)) {
        return QString::number(
            std::any_cast<units::volume::cubic_meter_t>(value).value());
    } else if (value.type() == typeid(units::volume::liter_t))
    {
        return QString::number(
            std::any_cast<units::volume::liter_t>(value).value());
    } else if (value.type() == typeid(units::area::square_meter_t)) {
        return QString::number(
            std::any_cast<units::area::square_meter_t>(value).value());
    } else if (value.type() == typeid(units::angle::degree_t))
    {
        return QString::number(std::any_cast<units::angle::degree_t>(value).value());
    } else if (value.type() == typeid(units::mass::metric_ton_t))
    {
        return QString::number(
            std::any_cast<units::mass::metric_ton_t>(value).value());
    } else if (value.type() == typeid(QVector<std::shared_ptr<GPoint>>))
    {
        const auto& points =
            std::any_cast<QVector<std::shared_ptr<GPoint>>>(value);
        QStringList pointStrs;
        for (const auto& point : points) {
            pointStrs << QString("%1,%2").arg(point->getLongitude().value())
                             .arg(point->getLatitude().value());
        }
        return pointStrs.join(";");
    } else if (value.type() == typeid(QVector<units::power::kilowatt_t>))
    {
        const auto& powers =
            std::any_cast<QVector<units::power::kilowatt_t>>(value);
        QStringList powerStrs;
        for (const auto& power : powers) {
            powerStrs << QString::number(power.value());
        }
        return powerStrs.join(";");
    } else if (value.type() == typeid(QVector<IShipEngine::EngineProperties>))
    {
        const auto& properties =
            std::any_cast<QVector<IShipEngine::EngineProperties>>(value);
        QStringList propertyStrs;
        for (const auto& prop : properties) {
            propertyStrs <<
                QString("%1,%2,%3").arg(prop.breakPower.value()).
                            arg(prop.RPM.value()).arg(prop.efficiency);
        }
        return propertyStrs.join(";");
    } else if (value.type() ==
               typeid(QMap<Ship::ShipAppendage, units::area::square_meter_t>))
    {
        const auto& appendages =
            std::any_cast<QMap<Ship::ShipAppendage,
                               units::area::square_meter_t>>(value);
        QStringList appendageStrs;
        for (auto it = appendages.begin(); it != appendages.end(); ++it)
        {
            appendageStrs << QString("%1,%2").
                             arg(std::any_cast<int>(it.key())).
                             arg(it.value().value());
        }
        return appendageStrs.join(";");
    }
    return QString(); // Default case
}


/**
 * @brief A QVector of ParamInfo structures that defines the order
 * and processing functions for file parameters.
 *
 * Each ParamInfo structure contains a string name, a function
 * pointer converter, and a boolean indicating if this parameter is optional
 * or not. The name is the name of the parameter as
 * it appears in the file. The converter function is used to convert
 * the parameter value from a string to its correct data type.
 *
 * The order of the ParamInfo structures in the vector defines the
 * order in which the parameters should appear in the file.
 */
static QVector<ParamInfo> FileOrderedparameters =
    {
    // Basic ship information parameters
    {"ID", toQStringT, false},
    {"Path", toPathPointsT, false},
    {"MaxSpeed", toMeterPerSecond, false},
    {"WaterlineLength", toMeterT, false},
    {"LengthBetweenPerpendiculars", toMeterT, false},
    {"Beam", toMeterT, false},
    {"DraftAtForward", toMeterT, false},
    {"DraftAtAft", toMeterT, false},
    {"VolumetricDisplacement", toCubicMeterT, true},
    {"WettedHullSurface", toSquareMeterT, true},
    {"ShipAndCargoAreaAboveWaterline", toSquareMeterT, false},
    {"BulbousBowTransverseAreaCenterHeight", toMeterT, false},
    {"BulbousBowTransverseArea", toSquareMeterT, false},
    {"ImmersedTransomArea", toSquareMeterT, false},
    {"HalfWaterlineEntranceAngle", toDegreesT, true},
    {"SurfaceRoughness", toNanoMeterT, false},
    // {"RunLength", toMeterT, true},
    {"LongitudinalBuoyancyCenter", toDoubleT, false},
    {"SternShapeParam", toCSternT, false},
    {"MidshipSectionCoef", toDoubleT, true},
    {"WaterplaneAreaCoef", toDoubleT, true},
    {"PrismaticCoef", toDoubleT, true},
    {"BlockCoef", toDoubleT, true},

    // Fuel and tank parameters
    {"FuelType", toFuelTypeT, false},
    {"TankSize", toLiterT, false},
    {"TankInitialCapacityPercentage", toDoubleT, false},
    {"TankDepthOfDischage", toDoubleT, false},

    // Engine parameters
    {"EnginesCountPerPropeller", toIntT, false},
    {"EngineOperationalPowerSettings", toEnginePowerVectorT, false},
    {"EngineTierIIPropertiesPoints", toEnginePowerRPMEfficiencyT, false},
    {"EngineTierIIIPropertiesPoints", toEnginePowerRPMEfficiencyT, true},

    // Gearbox parameters
    {"GearboxRatio", toDoubleT, false},
    {"GearboxEfficiency", toDoubleT, false},

    // Propeller parameters
    {"ShaftEfficiency",toDoubleT, false},
    {"PropellerCount", toIntT, false},
    {"PropellerDiameter", toMeterT, false},
    {"PropellerPitch", toMeterT, false},
    {"PropellerBladesCount", toIntT, false},
    {"PropellerExpandedAreaRatio",toDoubleT, false},

    // Operational parameters
    {"StopIfNoEnergy", toBoolT, true},
    {"MaxRudderAngle", toDegreesT, true},

    // Weight parameters
    {"VesselWeight", toTonsT, false},
    {"CargoWeight", toTonsT, false},

    // Appendages parameters
    {"AppendagesWettedSurfaces", toAppendagesWetSurfacesT, true}
};

/**
 * @brief Reads ship data from a file and processes it.
 *
 * This function opens the specified file and reads its contents line by line.
 * Each line is then split into parts using the first delimiter in
 * the delim list. The parts are then processed using the corresponding
 * converter functions defined in FileOrderedparameters,
 * and the results are stored in a QMap with the parameter name as the key.
 * If the file cannot be opened, an error message is logged.
 *
 * @param filename The name of the file to read from.
 */
SHIPNETSIM_EXPORT QVector<QMap<QString, std::any>> readShipsFile(
    QString filename,
    OptimizedNetwork* network = nullptr,
    bool isResistanceStudyOnly = false);


/**
 * @brief Reads a ship parameters from a string.
 * @param str
 * @param network
 * @return
 */
SHIPNETSIM_EXPORT QMap<QString, std::any>
readShipFromString(QString str,
                   OptimizedNetwork* network = nullptr,
                   bool isResistanceStudyOnly = false);

SHIPNETSIM_EXPORT bool writeShipsFile(
    QString filename,
    const QVector<QMap<QString, std::any>>& ships);

template<typename T>
SHIPNETSIM_EXPORT std::shared_ptr<Ship>
loadShipFromParameters(QMap<QString, T> shipsDetails);

template<typename T>
SHIPNETSIM_EXPORT QVector<std::shared_ptr<Ship>>
loadShipsFromParameters(QVector<QMap<QString, T>> shipsDetails);

}
};
#endif // SHIPSLIST_H
