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

#ifndef READSHIPS_H
#define READSHIPS_H
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

/**
 * @namespace readShips
 * @brief This namespace contains functions to read and process
 * ship data from a file.
 */
namespace readShips
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
struct ParamInfo
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
};

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
int convertToInt(const QString &str, const char *errorMsg, bool isOptional)
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


// /**
//  * @brief Converts a string to a QMap of doubles.
//  *
//  * This function takes a string that represents a map of double
//  * key-value pairs. The pairs are separated by a predefined delimiter,
//  * and each key and value are also separated by a delimiter.
//  * The function then converts each key and value to double and
//  * adds them to a QMap, which is then returned as a std::any object.
//  *
//  * @param str The input string to be converted.
//  * @return A std::any object that holds the QMap of double key-value pairs.
//  */
// static std::any toQMapDoublesT(const QString& str)
// {
//     QMap<double, double> resultMap;

//     // Split the string into pairs
//     QStringList pairs = str.split(delim[1]);
//     for (const QString& pair : pairs) {
//         // Split each pair into key and value
//         QStringList keyValuePair = pair.split(delim[2]);

//         // Convert key and value to double
//         double key =
//             convertToDouble(keyValuePair[0].trimmed(),
//                             "Invalid double conversion for key: %s");
//         double value =
//             convertToDouble(keyValuePair[1].trimmed(),
//                             "Invalid double conversion for value: %s");

//         // If there are exactly two elements (key and value),
//         // add them to the map
//         if (keyValuePair.size() == 2)
//         {
//             resultMap[key] = value;
//         }
//         else
//         {
//             // If there are not exactly two elements, terminate with a
//             // fatal error
//             qFatal("Malformed key-value pair: %s", pair.toUtf8().constData());
//         }
//     }
//     return resultMap;
// }

/**
 * @brief Converts a string to a QMap of kilowatt to revolutions per minute.
 *
 * This function takes a string that represents a map of kilowatt to
 * revolutions per minute key-value pairs.
 * The pairs are separated by a predefined delimiter, and each
 * key and value are also separated by a delimiter.
 * The function then converts each key and value to their respective
 *  units and adds them to a QMap,
 * which is then returned as a std::any object.
 *
 * @param str The input string to be converted.
 * @return A std::any object that holds the QMap of kilowatt to
 * revolutions per minute key-value pairs.
 */
static std::any toEngineRPMT(const QString& str, bool isOptional)
{
    // Define a QMap to hold the converted key-value pairs.
    QMap<units::power::kilowatt_t,
         units::angular_velocity::revolutions_per_minute_t> resultMap;

    if (isOptional)
    {
        // if the string has no information, return nothing
        if (str.contains("na", Qt::CaseInsensitive))
        {
            return resultMap;
        }
    }

    // Split the string into pairs.
    QStringList pairs = str.split(delim[1]);
    for (const QString& pair : pairs)
    {
        // Split each pair into key and value.
        QStringList keyValuePair = pair.split(delim[2]);

        // Convert key and value to their respective units.
        units::power::kilowatt_t key =
            units::power::kilowatt_t(
                convertToDouble(keyValuePair[0].trimmed(),
                                "Invalid double conversion for key: %s", false));

        units::angular_velocity::revolutions_per_minute_t value =
            units::angular_velocity::revolutions_per_minute_t(
                 convertToDouble(keyValuePair[1].trimmed(),
                                "Invalid double conversion for value: %s", false));

        // If there are exactly two elements (key and value), add them to
        // the map.
        if (keyValuePair.size() == 2)
        {
            resultMap[key] = value;
        }
        else
        {
            // If there are not exactly two elements, terminate with a fatal
            // error.
            qFatal("Malformed key-value pair: %s", pair.toUtf8().constData());
        }
    }
    return resultMap;
}


/**
 * @brief Converts a string to a QMap of kilowatt to efficiency ratio.
 *
 * This function takes a string that represents a map of kilowatt to
 * efficiency ratio key-value pairs.
 * The pairs are separated by a predefined delimiter, and each
 * key and value are also separated by a delimiter.
 * The function then converts each key to kilowatts, each value
 * to a double representing the efficiency ratio,
 * and adds them to a QMap, which is then returned as a std::any object.
 *
 * @param str The input string to be converted.
 * @return A std::any object that holds the QMap of kilowatt to
 * efficiency ratio key-value pairs.
 */
static std::any toEngineEfficiency(const QString& str, bool isOptional)
{
    // Define a QMap to hold the converted key-value pairs.
    QMap<units::power::kilowatt_t, double> resultMap;

    // if the string has no information, return nothing
    if (isOptional)
    {
        if (str.contains("na", Qt::CaseInsensitive))
        {
            return resultMap;
        }
    }

    // Split the string into pairs.
    QStringList pairs = str.split(delim[1]);
    for (const QString& pair : pairs)
    {
        // Split each pair into key and value.
        QStringList keyValuePair = pair.split(delim[2]);

        // Convert key to kilowatts.
        units::power::kilowatt_t key =
            units::power::kilowatt_t(
                convertToDouble(keyValuePair[0].trimmed(),
                                "Invalid double conversion for key: %s", false));

        // Convert value to a double.
        double value = convertToDouble(
            keyValuePair[1].trimmed(),
            "Invalid double conversion for value: %s", false);

        // If there are exactly two elements (key and value), add them
        // to the map.
        if (keyValuePair.size() == 2)
        {
            resultMap[key] = value;
        }
        else
        {
            // If there are not exactly two elements, terminate with
            // a fatal error.
            qFatal("Malformed key-value pair: %s", pair.toUtf8().constData());
        }
    }
    return resultMap;
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


/**
 * @brief A QVector of ParamInfo structures that defines the order
 * and processing functions for file parameters.
 *
 * Each ParamInfo structure contains a string name and a function
 * pointer converter. The name is the name of the parameter as
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
    {"VolumetricDisplacement", toCubicMeterT, false},
    {"WettedHullSurface", toSquareMeterT, false},
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
    {"EngineBrakePowerToRPMMap", toEngineRPMT, false},
    {"EngineBrakePowerToEfficiency", toEngineEfficiency, false},

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
static QVector<std::shared_ptr<Ship>> readShipsFile(
    QString filename,
    std::shared_ptr<OptimizedNetwork> network = nullptr,
    bool isResistanceStudyOnly = false)
{
    if (isResistanceStudyOnly)
    {
        FileOrderedparameters[1].isOptional = true;
    }
    else
    {
        if (network == nullptr)
        {
            qFatal("network cannot be null");
        }
    }
    // Counter to keep track of ship IDs.
//    static qsizetype idCounter = 0;

    // Map to store parameters and their values.
    QMap<QString, std::any> parameters;
    // a vector holding ships
    QVector<std::shared_ptr<Ship>> ships;

    // Open the file for reading.
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) {
        // Log an error message if file cannot be opened.
        qCritical() << "Failed to open the file.";
        return ships;
    }

    // Create a QTextStream to read from the file.
    QTextStream in(&file);
    QString line;

    // Loop through each line in the file.
    while (!in.atEnd())
    {
        // Read a line from the file and remove leading and
        // trailing whitespace.
        line = in.readLine().trimmed();

        // Remove comments (everything after '#')
        int commentIndex = line.indexOf('#');
        if (commentIndex != -1)
        {
            line = line.left(commentIndex).trimmed();
        }

        if (line.isEmpty())
        {
            continue; // Skip empty or comment-only lines
        }

        // Split the line into parts using the first delimiter
        // in the delim list.
        QStringList parts = line.split(delim[0]);

        // Check if the number of parts matches the number
        // of expected parameters.
        if (parts.size() == FileOrderedparameters.size())
        {
            // Loop through each part and process it.
            for (int i = 0; i < parts.size(); ++i)
            {
                // Use the converter function for the parameter
                // to process the part and store the result
                // in the parameters map.
                parameters[FileOrderedparameters[i].name] =
                    FileOrderedparameters[i].converter(
                        parts[i],
                        FileOrderedparameters[i].isOptional);
            }

            if (network != nullptr && !isResistanceStudyOnly)
            {
                auto pathPoints =
                    Utils::getValueFromMap<QVector<std::shared_ptr<GPoint>>>(
                        parameters, "Path", QVector<std::shared_ptr<GPoint>>());
                ShortestPathResult results =
                    network->findShortestPath(pathPoints,
                                              PathFindingAlgorithm::Dijkstra);

                if (!results.isValid())
                {
                    qFatal("Could not find ship path!\n");
                }

                parameters["PathPoints"] = results.points;
                parameters["PathLines"]  = results.lines;
            }
            if (network == nullptr || isResistanceStudyOnly)
            {
                QVector<std::shared_ptr<GPoint>> fakePoints;
                QVector<std::shared_ptr<GLine>> fakeLines;

                fakePoints.push_back(std::make_shared<GPoint>(
                    units::angle::degree_t(0.0),
                    units::angle::degree_t(0.0)));
                fakePoints.push_back(std::make_shared<GPoint>(
                    units::angle::degree_t(100.0),
                    units::angle::degree_t(100.0)));
                fakeLines.push_back(std::make_shared<GLine>(fakePoints[0],
                                                           fakePoints[1]));
                parameters["PathPoints"] = fakePoints;
                parameters["PathLines"]  = fakeLines;
            }

            auto ship = std::make_shared<Ship>(parameters);
            ships.push_back(ship);
        }
        else
        {
            qFatal("Not all parameters are provided! "
                   "\n Check the ships file");
        }
    }

    return ships;
}


}
#endif // READSHIPS_H
