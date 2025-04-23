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
#include "../../third_party/units/units.h"
#include "../export.h"
#include "../network/optimizednetwork.h"
#include "../network/point.h"
#include "../utils/utils.h"
#include "ishipengine.h"
#include "qvariant.h"
#include "ship.h"
#include <QCoreApplication>
#include <QDebug>
#include <QFile>
#include <QString>
#include <QTextStream>
#include <QVector>
#include <any>

namespace ShipNetSimCore
{
/**
 * @namespace ShipsList
 * @brief This namespace contains functions to read and process
 * ship data from a file.
 */
namespace ShipsList
{

class ShipLoadException : public std::runtime_error
{
public:
    explicit ShipLoadException(const QString &message)
        : std::runtime_error(message.toStdString())
    {
    }
};

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
    QString name; ///< Name of the parameter.
    /// Function to convert the parameter from a string to the correct
    /// type.
    std::function<std::any(const QString &, bool &)> converter;
    bool isOptional; ///< handle nan or na if passed to the function

    // Adding a constructor for easier initialization
    ParamInfo(
        const QString                                   &name,
        std::function<std::any(const QString &, bool &)> converter,
        bool                                             isOptional)
        : name(name)
        , converter(converter)
        , isOptional(isOptional)
    {
    }

    // Default constructor for creating an empty instance
    ParamInfo()
        : name("")
        , converter(nullptr)
        , isOptional(false)
    {
    }

    // Method to check if the ParamInfo instance is empty
    bool isEmpty() const
    {
        return name.isEmpty() && converter == nullptr && !isOptional;
    }
};

static ShipNetSimCore::ShipsList::ParamInfo *
findParamInfoByKey(const QString &key, QVector<ParamInfo> &parameters)
{
    for (auto &param : parameters)
    {
        if (param.name.trimmed().toLower() == key.trimmed().toLower())
        {
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
static double convertToDouble(const QString &str,
                              const char *errorMsg, bool isOptional)
{
    if (isOptional)
    {
        // if the string has nan, return the default nan
        if (str.isEmpty() || str.contains("na", Qt::CaseInsensitive))
        {
            return std::nan("uninitialized");
        }
    }

    // Replace Unicode minus sign with standard minus
    // this may arise from some systems or the GUI
    QString normalizedStr = str;
    normalizedStr.replace(QChar(0x2212),
                          '-'); // Replace Unicode minus
                                // (−) with ASCII minus (-)

    // otherwize, do the conversion
    bool   ok;
    double result = normalizedStr.toDouble(&ok);
    if (!ok)
    {
        throw ShipLoadException(QString(errorMsg).arg(str));
    }
    return result;
}

/**
 * @brief Converts a string to an int, with error checking.
 * @param str The string to convert.
 * @param errorMsg Error message to display if the conversion fails.
 * @return The converted int.
 */
static int convertToInt(const QString &str, const char *errorMsg,
                        bool isOptional)
{
    if (isOptional)
    {
        // if the string has nan, return the default nan
        if (str.isEmpty() || str.contains("na", Qt::CaseInsensitive))
        {
            return -100;
        }
    }

    // Replace Unicode minus sign with standard minus
    // this may arise from some systems or the GUI
    QString normalizedStr = str;
    normalizedStr.replace(QChar(0x2212),
                          '-'); // Replace Unicode minus
                                // (−) with ASCII minus (-)

    // otherwize, do the conversion
    bool ok;
    int result = (int)(normalizedStr.toDouble(&ok)); // convert to int
    if (!ok) // check if the conversion worked!
    {
        throw ShipLoadException(QString(errorMsg).arg(str));
    }
    return result;
}

/**
 * @brief Converts a string to a bool.
 * @param str The string to convert.
 * @return The converted bool.
 */
static std::any toBoolT(const QString &str, bool isOptional)
{
    // if the string has no information, return nothing
    if (isOptional)
    {
        // if the string has nan, return the default nan
        if (str.isEmpty() || str.contains("na", Qt::CaseInsensitive))
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
static std::any toIntT(const QString &str, bool isOptional)
{
    return convertToInt(str, "%1 is not an int!", isOptional);
}

/**
 * @brief Converts a string to a double, with error checking.
 * @param str The string to convert.
 * @return The converted double.
 */
static std::any toDoubleT(const QString &str, bool isOptional)
{
    return convertToDouble(str, "%1 is not a double!\n", isOptional);
}

/**
 * @brief Converts a string to a nanometer unit, with error checking.
 * @param str The string to convert.
 * @return The converted nanometer unit.
 */
static std::any toNanoMeterT(const QString &str, bool isOptional)
{
    double result = convertToDouble(
        str, "%s is not a valid double for nanometers!\n",
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
static std::any toMeterT(const QString &str, bool isOptional)
{
    double result = convertToDouble(
        str, "%s is not a valid double for meters!\n", isOptional);
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
static std::any toCubicMeterT(const QString &str, bool isOptional)
{
    double result = convertToDouble(
        str, "%s is not a valid double for cubic meters!\n",
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
static std::any toLiterT(const QString &str, bool isOptional)
{
    double result = convertToDouble(
        str, "%s is not a valid double for liters!\n", isOptional);
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
static std::any toSquareMeterT(const QString &str, bool isOptional)
{
    double result = convertToDouble(
        str, "%s is not a valid double for square meters!\n",
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
static std::any toDegreesT(const QString &str, bool isOptional)
{
    double result = convertToDouble(
        str, "%s is not a valid double for angle degrees!\n",
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
static std::any toTonsT(const QString &str, bool isOptional)
{
    double result = convertToDouble(
        str, "%s is not a valid double for metric tons!\n",
        isOptional);
    return units::mass::metric_ton_t(result);
}

static std::any toMeterPerSecond(const QString &str, bool isOptional)
{
    double result = convertToDouble(str,
                                    "%s is not a valid double for "
                                    "speed in knot!\n",
                                    isOptional);
    return units::velocity::knot_t(result)
        .convert<units::velocity::meters_per_second>();
}

/**
 * Returns the input string.
 * This is a no-op conversion, used for parameters that are
 * already in the correct format.
 * @param str The input string.
 * @return A std::any object holding the input string.
 */
static std::any toQStringT(const QString &str, bool isOptional)
{
    // if the string has no information, return nothing
    if (isOptional)
    {
        if (str.isEmpty() || str.contains("na", Qt::CaseInsensitive))
        {
            return "";
        }
    }

    return str;
}

static std::any toEnginePowerVectorT(const QString &str,
                                     bool           isOptional)
{
    // Define a vector to hold data
    QVector<units::power::kilowatt_t> result;

    if (isOptional)
    {
        // if the string has no information, return nothing
        if (str.isEmpty() || str.contains("na", Qt::CaseInsensitive))
        {
            return result;
        }
    }

    QStringList pointsData = str.split(delim[1]);
    for (const QString &pointData : pointsData)
    {
        units::power::kilowatt_t power = units::power::kilowatt_t(
            convertToDouble(pointData.trimmed(),
                            "Invalid double conversion "
                            "for key: %s",
                            false));
        result.push_back(power);
    }

    if (result.size() != 4)
    {
        // If there are not exactly 4 elements representing l1, l2,
        // l3, l4, terminate with a fatal error.
        throw ShipLoadException(
            "Malformed Engine Properties."
            "\nEngine Operational Power Settings must have "
            "4 data points representing L1, L2, L3, L4 on the "
            "engine layout!");
    }

    return result;
}
/**
 * @brief Converts a string to a QVector of engine properties.
 *
 * This function takes a string that represents a vector of engine
 * properties in the following order: Brake Powers, revolutions per
 * minute, Efficiencies. The values are separated by a predefined
 * delimiter, and each key and value are also separated by a
 * delimiter. The function then converts each value to their
 * respective units and adds them to a QVector, which is then returned
 * as a std::any object. The engine properties should be specified at
 * L1, L2, L3, L4 of the engine layout.
 *
 * @param str The input string to be converted.
 * @return A std::any object that holds the QVector<EngineProperites>.
 */
static std::any toEnginePowerRPMEfficiencyT(const QString &str,
                                            bool           isOptional)
{
    // Define a vector to hold data
    QVector<IShipEngine::EngineProperties> result;

    if (isOptional)
    {
        // if the string has no information, return nothing
        if (str.isEmpty() || str.contains("na", Qt::CaseInsensitive))
        {
            return result;
        }
    }

    // Split the string into pairs.
    QStringList pointsData = str.split(delim[1]);

    for (const QString &pointData : pointsData)
    {
        // Split each pair into key and value.
        QStringList values = pointData.split(delim[2]);

        // If the properties are not 3 values, return error.
        if (values.size() != 3)
        {
            // If there are not exactly 3 elements, terminate with a
            // fatal error.
            throw ShipLoadException(
                "Malformed Engine Property: " + pointData
                + "\nEngine Power-RPM-Efficiency Mapping must have "
                  "3 values representing Break Power, RPM, "
                  "Efficiency!");
        }

        // Convert key and value to their respective units.
        units::power::kilowatt_t power = units::power::kilowatt_t(
            convertToDouble(values[0].trimmed(),
                            "Invalid double conversion "
                            "for key: %s",
                            false));

        units::angular_velocity::revolutions_per_minute_t rpm =
            units::angular_velocity::revolutions_per_minute_t(
                convertToDouble(values[1].trimmed(),
                                "Invalid double conversion "
                                "for value: %s",
                                false));

        double eff = convertToDouble(values[2].trimmed(),
                                     "Invalud double conversion "
                                     "for value: %s",
                                     false);

        result.push_back(
            IShipEngine::EngineProperties(power, rpm, eff));
    }

    return result;
}

/**
 * @brief Converts a string to a QVector of shared pointers to Point
 * objects.
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
static std::any toPathPointsT(const QString &str, bool isOptional)
{
    // Define a QVector to hold the shared pointers to Point objects.
    QVector<std::shared_ptr<GPoint>> points;

    // if the string has no information, return nothing
    if (isOptional)
    {
        if (str.isEmpty() || str.contains("na", Qt::CaseInsensitive))
        {
            return points;
        }
    }

    // Split the string into coordinate pairs.
    QStringList pairs = str.split(delim[1]);
    for (const QString &pair : pairs)
    {
        // Split each pair into x and y coordinates.
        QStringList keyValuePair = pair.split(delim[2]);

        // Convert x coordinate to meters.
        auto x1 = units::angle::degree_t(convertToDouble(
            keyValuePair[0].trimmed(),
            "Invalid double conversion for x1: %s", false));

        // Convert y coordinate to meters.
        auto x2 = units::angle::degree_t(convertToDouble(
            keyValuePair[1].trimmed(),
            "Invalid double conversion for x2: %s", false));

        // check if the coordinates are WGS84
        if (std::abs(x1.value()) > 180.0
            || std::abs(x2.value()) > 90.0)
        {
            throw ShipLoadException("Not WGS Coordinate Points: "
                                    + pair);
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
            throw ShipLoadException("Malformed key-value pair: "
                                    + pair);
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
static std::any toAppendagesWetSurfacesT(const QString &str,
                                         bool           isOptional)
{
    QMap<Ship::ShipAppendage, units::area::square_meter_t> appendages;

    // if the string has no information, return nothing
    if (isOptional)
    {
        if (str.isEmpty()
            || str.contains("na",
                            Qt::CaseInsensitive)) // it is nan or na
        {
            return appendages;
        }
    }

    // Split the string into pairs of appendage and area
    QStringList pairs = str.split(delim[1]);

    for (const QString &pair : pairs)
    {
        // Split each pair into appendage and area
        QStringList keyValuePair = pair.split(delim[2]);

        // Convert appendage to int and area to double
        int rawAppendage = convertToInt(
            keyValuePair[0].trimmed(),
            "Invalid int conversion for appendage: %s", false);
        auto area = units::area::square_meter_t(convertToDouble(
            keyValuePair[1].trimmed(),
            "Invalid double conversion for area: %s", false));

        if (keyValuePair.size() == 2)
        {
            Ship::ShipAppendage appendage =
                static_cast<Ship::ShipAppendage>(rawAppendage);
            appendages[appendage] = area;
        }
        else
        {
            throw ShipLoadException("Malformed key-value pair: "
                                    + pair);
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
static std::any toCSternT(const QString &str, bool isOptional)
{
    // Convert string to int and cast to CStern enum
    int rawValue = convertToInt(
        str.trimmed(), "Invalid conversion to int for CStern: %s",
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
static std::any toFuelTypeT(const QString &str, bool isOptional)
{
    // Convert string to int and cast to FuelType enum
    int rawValue = convertToInt(
        str.trimmed(), "Invalid conversion to int for fuel type: %s",
        isOptional);
    return static_cast<ShipFuel::FuelType>(rawValue);
}

static QString toString(const std::any &value)
{
    if (value.type() == typeid(QString))
    {
        return std::any_cast<QString>(value);
    }
    else if (value.type() == typeid(double))
    {
        return QString::number(std::any_cast<double>(value));
    }
    else if (value.type() == typeid(int))
    {
        return QString::number(std::any_cast<int>(value));
    }
    else if (value.type() == typeid(bool))
    {
        return std::any_cast<bool>(value) ? "true" : "false";
    }
    else if (value.type() == typeid(units::length::meter_t))
    {
        return QString::number(
            std::any_cast<units::length::meter_t>(value).value());
    }
    else if (value.type() == typeid(units::volume::cubic_meter_t))
    {
        return QString::number(
            std::any_cast<units::volume::cubic_meter_t>(value)
                .value());
    }
    else if (value.type() == typeid(units::volume::liter_t))
    {
        return QString::number(
            std::any_cast<units::volume::liter_t>(value).value());
    }
    else if (value.type() == typeid(units::area::square_meter_t))
    {
        return QString::number(
            std::any_cast<units::area::square_meter_t>(value)
                .value());
    }
    else if (value.type() == typeid(units::angle::degree_t))
    {
        return QString::number(
            std::any_cast<units::angle::degree_t>(value).value());
    }
    else if (value.type() == typeid(units::mass::metric_ton_t))
    {
        return QString::number(
            std::any_cast<units::mass::metric_ton_t>(value).value());
    }
    else if (value.type() == typeid(QVector<std::shared_ptr<GPoint>>))
    {
        const auto &points =
            std::any_cast<QVector<std::shared_ptr<GPoint>>>(value);
        QStringList pointStrs;
        for (const auto &point : points)
        {
            pointStrs << QString("%1,%2")
                             .arg(point->getLongitude().value())
                             .arg(point->getLatitude().value());
        }
        return pointStrs.join(";");
    }
    else if (value.type()
             == typeid(QVector<units::power::kilowatt_t>))
    {
        const auto &powers =
            std::any_cast<QVector<units::power::kilowatt_t>>(value);
        QStringList powerStrs;
        for (const auto &power : powers)
        {
            powerStrs << QString::number(power.value());
        }
        return powerStrs.join(";");
    }
    else if (value.type()
             == typeid(QVector<IShipEngine::EngineProperties>))
    {
        const auto &properties =
            std::any_cast<QVector<IShipEngine::EngineProperties>>(
                value);
        QStringList propertyStrs;
        for (const auto &prop : properties)
        {
            propertyStrs << QString("%1,%2,%3")
                                .arg(prop.breakPower.value())
                                .arg(prop.RPM.value())
                                .arg(prop.efficiency);
        }
        return propertyStrs.join(";");
    }
    else if (value.type()
             == typeid(QMap<Ship::ShipAppendage,
                            units::area::square_meter_t>))
    {
        const auto &appendages = std::any_cast<
            QMap<Ship::ShipAppendage, units::area::square_meter_t>>(
            value);
        QStringList appendageStrs;
        for (auto it = appendages.begin(); it != appendages.end();
             ++it)
        {
            appendageStrs << QString("%1,%2")
                                 .arg(std::any_cast<int>(it.key()))
                                 .arg(it.value().value());
        }
        return appendageStrs.join(";");
    }
    return QString(); // Default case
}

static std::any toTanksDetails(const QString &str, bool isOptional)
{
    QVector<QMap<QString, std::any>> tankDetails;

    // if the string has no information, return nothing
    if (isOptional)
    {
        if (str.isEmpty()
            || str.contains("na",
                            Qt::CaseInsensitive)) // it is nan or na
        {
            return tankDetails;
        }
    }
    // Split the string into pairs of appendage and area
    QStringList entries = str.split(delim[1]);

    for (const QString &entry : entries)
    {
        // Split each pair into appendage and area
        QStringList values = entry.split(delim[2]);

        if (values.size() == 4)
        {
            std::any fuelType      = toFuelTypeT(values[0], false);
            std::any totalCapacity = toLiterT(values[1], false);
            std::any initialCapPercent = toDoubleT(values[2], false);
            std::any depthOfDischarge  = toDoubleT(values[3], false);

            tankDetails.push_back(
                {{"FuelType", fuelType},
                 {"MaxCapacity", totalCapacity},
                 {"TankInitialCapacityPercentage", initialCapPercent},
                 {"TankDepthOfDischage", depthOfDischarge}});
        }
        else
        {
            throw ShipLoadException("Malformed tank details: "
                                    + entry);
        }
    }

    return tankDetails;
}

/**
 * @brief A QVector of ParamInfo structures that defines the order
 * and processing functions for file parameters.
 *
 * Each ParamInfo structure contains a string name, a function
 * pointer converter, and a boolean indicating if this parameter is
 * optional or not. The name is the name of the parameter as it
 * appears in the file. The converter function is used to convert the
 * parameter value from a string to its correct data type.
 *
 * The order of the ParamInfo structures in the vector defines the
 * order in which the parameters should appear in the file.
 */
static QVector<ParamInfo> FileOrderedparameters = {
    // Basic ship information parameters
    // parameter key, type, isOptional
    {"ID", toQStringT, false},                                 // 00
    {"Path", toPathPointsT, false},                            // 01
    {"MaxSpeed", toMeterPerSecond, false},                     // 02
    {"WaterlineLength", toMeterT, false},                      // 03
    {"LengthBetweenPerpendiculars", toMeterT, false},          // 04
    {"Beam", toMeterT, false},                                 // 05
    {"DraftAtForward", toMeterT, false},                       // 06
    {"DraftAtAft", toMeterT, false},                           // 07
    {"VolumetricDisplacement", toCubicMeterT, true},           // 08
    {"WettedHullSurface", toSquareMeterT, true},               // 09
    {"ShipAndCargoAreaAboveWaterline", toSquareMeterT, false}, // 10
    {"BulbousBowTransverseAreaCenterHeight", toMeterT, false}, // 11
    {"BulbousBowTransverseArea", toSquareMeterT, false},       // 12
    {"ImmersedTransomArea", toSquareMeterT, false},            // 13
    {"HalfWaterlineEntranceAngle", toDegreesT, true},          // 14
    {"SurfaceRoughness", toNanoMeterT, false},                 // 15
    // {"RunLength", toMeterT, true},
    {"LongitudinalBuoyancyCenter", toDoubleT, false}, // 16
    {"SternShapeParam", toCSternT, false},            // 17
    {"MidshipSectionCoef", toDoubleT, true},          // 18
    {"WaterplaneAreaCoef", toDoubleT, true},          // 19
    {"PrismaticCoef", toDoubleT, true},               // 20
    {"BlockCoef", toDoubleT, true},                   // 21

    // Fuel and tank parameters
    {"TanksDetails", toTanksDetails, false}, // 22

    // Engine parameters
    {"EnginesCountPerPropeller", toIntT, false}, // 23
    {"EngineTierIIPropertiesPoints", toEnginePowerRPMEfficiencyT,
     false}, // 24
    {"EngineTierIIIPropertiesPoints", toEnginePowerRPMEfficiencyT,
     true},                                                    // 25
    {"EngineTierIICurve", toEnginePowerRPMEfficiencyT, true},  // 26
    {"EngineTierIIICurve", toEnginePowerRPMEfficiencyT, true}, // 27

    // Gearbox parameters
    {"GearboxRatio", toDoubleT, false},      // 28
    {"GearboxEfficiency", toDoubleT, false}, // 29

    // Propeller parameters
    {"ShaftEfficiency", toDoubleT, false},            // 30
    {"PropellerCount", toIntT, false},                // 31
    {"PropellerDiameter", toMeterT, false},           // 32
    {"PropellerPitch", toMeterT, false},              // 33
    {"PropellerBladesCount", toIntT, false},          // 34
    {"PropellerExpandedAreaRatio", toDoubleT, false}, // 35

    // Operational parameters
    {"StopIfNoEnergy", toBoolT, true},    // 36
    {"MaxRudderAngle", toDegreesT, true}, // 37

    // Weight parameters
    {"VesselWeight", toTonsT, false}, // 38
    {"CargoWeight", toTonsT, false},  // 39

    // Appendages parameters
    {"AppendagesWettedSurfaces", toAppendagesWetSurfacesT, true} // 40
};

/**
 * @brief Reads ship data from a file and processes it.
 *
 * This function opens the specified file and reads its contents line
 * by line. Each line is then split into parts using the first
 * delimiter in the delim list. The parts are then processed using the
 * corresponding converter functions defined in FileOrderedparameters,
 * and the results are stored in a QMap with the parameter name as the
 * key. If the file cannot be opened, an error message is logged.
 *
 * @param filename The name of the file to read from.
 */
SHIPNETSIM_EXPORT QVector<QMap<QString, std::any>>
readShipsFile(QString filename, OptimizedNetwork *network = nullptr,
              bool isResistanceStudyOnly = false);

/**
 * @brief Reads ship data from a file and provide a map of each
 * parameter and its value as a string.
 *
 * This function opens the specified file and reads its contents line
 * by line. Each line is then split into parts using the first
 * delimiter in the delim list. The parts are then loaded by the order
 * of FileOrderedparameters, and the results are stored in a QMap with
 * the parameter name as the key. If the file cannot be opened, an
 * error message is logged.
 *
 * @param filename The name of the file to read from.
 */
SHIPNETSIM_EXPORT QVector<QMap<QString, QString>>
                  readShipsFileToStrings(QString filename);

/**
 * @brief Reads a ship parameters from a string.
 * @param str
 * @param network
 * @return
 */
SHIPNETSIM_EXPORT QMap<QString, std::any>
readShipFromString(QString str, OptimizedNetwork *network = nullptr,
                   bool isResistanceStudyOnly = false);

/**
 * @brief Reads a ship parameters from a string.
 * @param line
 * @return map of each parameter key and its value as a string
 */
SHIPNETSIM_EXPORT QMap<QString, QString>
                  readShipFromStringToStrings(QString line);

SHIPNETSIM_EXPORT bool
writeShipsFile(QString                                filename,
               const QVector<QMap<QString, QString>> &ships,
               const QVector<QString> headerLines = {});

template <typename T>
SHIPNETSIM_EXPORT std::shared_ptr<Ship>
loadShipFromParameters(QMap<QString, T>  shipsDetails,
                       OptimizedNetwork *network  = nullptr,
                       bool isResistanceStudyOnly = false);

template <typename T>
SHIPNETSIM_EXPORT QVector<std::shared_ptr<Ship>>
loadShipsFromParameters(QVector<QMap<QString, T>> shipsDetails,
                        OptimizedNetwork         *network = nullptr,
                        bool isResistanceStudyOnly        = false);

/**
 * @brief Loads a ship from a QJsonObject.
 *
 * This function extracts ship parameters from a JSON object,
 * converts them to the appropriate types, and constructs a Ship
 * object.
 *
 * @param shipJson The QJsonObject containing ship parameters.
 * @param network A pointer to the OptimizedNetwork for finding ship
 * paths.
 * @param isResistanceStudyOnly Whether the loading is for resistance
 *                              study only.
 * @return A shared pointer to the Ship object.
 */
SHIPNETSIM_EXPORT std::shared_ptr<Ship>
                  loadShipFromParameters(QJsonObject       shipJson,
                                         OptimizedNetwork *network = nullptr,
                                         bool isResistanceStudyOnly = false);

/**
 * @brief Loads ships from a QJsonObject.
 *
 * This function extracts ship parameters from a JSON object, converts
 * them to the appropriate types, and constructs a list of Ship
 * objects.
 *
 * The JSON object is expected to contain an array of ships, each
 * defined by a set of parameters.
 *
 * @param shipsJson The QJsonObject containing an array of ships.
 * @param network A pointer to the OptimizedNetwork for finding ship
 * paths.
 * @param isResistanceStudyOnly Whether the loading is for resistance
 * study only.
 * @return A vector of shared pointers to the Ship objects.
 */
SHIPNETSIM_EXPORT QVector<std::shared_ptr<Ship>>
                  loadShipsFromJson(const QJsonObject &shipsJson,
                                    OptimizedNetwork  *network = nullptr,
                                    bool               isResistanceStudyOnly = false);

} // namespace ShipsList
}; // namespace ShipNetSimCore
#endif // SHIPSLIST_H
