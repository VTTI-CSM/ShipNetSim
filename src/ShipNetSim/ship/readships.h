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
#include "ship.h"

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
 * name and a function to convert it to the correct type.
 */
struct ParamInfo
{
    QString name;       ///< Name of the parameter.
    /// Function to convert the parameter from a string to the correct type.
    std::function<std::any(const QString&)> converter;
};

/**
 * @brief Converts a string to a double, with error checking.
 * @param str The string to convert.
 * @param errorMsg Error message to display if the conversion fails.
 * @return The converted double.
 */
static double convertToDouble(const QString& str, const char* errorMsg)
{
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
int convertToInt(const QString &str, const char *errorMsg)
{
    bool ok;
    int result = str.toInt(&ok); // convert to int
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
static std::any toBoolT(const QString& str)
{
    return QVariant(str).toBool(); // return the bool equivelant
}

/**
 * @brief Converts a string to an int, with error checking.
 * @param str The string to convert.
 * @return The converted int.
 */
static std::any toIntT(const QString& str)
{
    return convertToInt(str, "%1 is not an int!");
}

/**
 * @brief Converts a string to a double, with error checking.
 * @param str The string to convert.
 * @return The converted double.
 */
static std::any toDoubleT(const QString& str)
{
    return convertToDouble(str, "%1 is not a double!\n");
}

/**
 * @brief Converts a string to a nanometer unit, with error checking.
 * @param str The string to convert.
 * @return The converted nanometer unit.
 */
static std::any toNanoMeterT(const QString& str)
{
    double result =
        convertToDouble(str, "%s is not a valid double for nanometers!\n");
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
static std::any toMeterT(const QString& str)
{
    double result =
        convertToDouble(str, "%s is not a valid double for meters!\n");
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
static std::any toCubicMeterT(const QString& str)
{
    double result =
        convertToDouble(str, "%s is not a valid double for cubic meters!\n");
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
static std::any toLiterT(const QString& str)
{
    double result =
        convertToDouble(str, "%s is not a valid double for liters!\n");
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
static std::any toSquareMeterT(const QString& str)
{
    double result =
        convertToDouble(str, "%s is not a valid double for square meters!\n");
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
static std::any toDegreesT(const QString& str)
{
    double result =
        convertToDouble(str, "%s is not a valid double for angle degrees!\n");
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
static std::any toTonsT(const QString& str)
{
    double result =
        convertToDouble(str, "%s is not a valid double for metric tons!\n");
    return units::mass::metric_ton_t(result);
}

/**
 * Returns the input string.
 * This is a no-op conversion, used for parameters that are
 * already in the correct format.
 * @param str The input string.
 * @return A std::any object holding the input string.
 */
static std::any toQStringT(const QString& str)
{
    return str;
}


/**
 * @brief Converts a string to a QMap of doubles.
 *
 * This function takes a string that represents a map of double
 * key-value pairs. The pairs are separated by a predefined delimiter,
 * and each key and value are also separated by a delimiter.
 * The function then converts each key and value to double and
 * adds them to a QMap, which is then returned as a std::any object.
 *
 * @param str The input string to be converted.
 * @return A std::any object that holds the QMap of double key-value pairs.
 */
static std::any toQMapDoublesT(const QString& str)
{
    QMap<double, double> resultMap;

    // Split the string into pairs
    QStringList pairs = str.split(delim[1]);
    for (const QString& pair : pairs) {
        // Split each pair into key and value
        QStringList keyValuePair = pair.split(delim[2]);

        // Convert key and value to double
        double key =
            convertToDouble(keyValuePair[0].trimmed(),
                            "Invalid double conversion for key: %s");
        double value =
            convertToDouble(keyValuePair[1].trimmed(),
                            "Invalid double conversion for value: %s");

        // If there are exactly two elements (key and value),
        // add them to the map
        if (keyValuePair.size() == 2)
        {
            resultMap[key] = value;
        }
        else
        {
            // If there are not exactly two elements, terminate with a
            // fatal error
            qFatal("Malformed key-value pair: %s", pair.toUtf8().constData());
        }
    }
    return resultMap;
}

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
static std::any toEngineRPMT(const QString& str)
{
    // Define a QMap to hold the converted key-value pairs.
    QMap<units::power::kilowatt_t,
         units::angular_velocity::revolutions_per_minute_t> resultMap;

    // Split the string into pairs.
    QStringList pairs = str.split(delim[1]);
    for (const QString& pair : pairs) {
        // Split each pair into key and value.
        QStringList keyValuePair = pair.split(delim[2]);

        // Convert key and value to their respective units.
        units::power::kilowatt_t key =
            units::power::kilowatt_t(
                convertToDouble(keyValuePair[0].trimmed(),
                                "Invalid double conversion for key: %s"));
        units::angular_velocity::revolutions_per_minute_t value =
            units::angular_velocity::revolutions_per_minute_t(
                convertToDouble(keyValuePair[1].trimmed(),
                                "Invalid double conversion for value: %s"));

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
static std::any toEngineEfficiency(const QString& str)
{
    // Define a QMap to hold the converted key-value pairs.
    QMap<units::power::kilowatt_t, double> resultMap;

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
                                "Invalid double conversion for key: %s"));

        // Convert value to a double.
        double value = convertToDouble(
            keyValuePair[1].trimmed(),
            "Invalid double conversion for value: %s");

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
static std::any toPathPointsT(const QString& str)
{
    // Define a QVector to hold the shared pointers to Point objects.
    QVector<std::shared_ptr<Point>> points;

    // Split the string into coordinate pairs.
    QStringList pairs = str.split(delim[1]);
    for (const QString& pair : pairs) {
        // Split each pair into x and y coordinates.
        QStringList keyValuePair = pair.split(delim[2]);

        // Convert x coordinate to meters.
        auto x1 =
            units::length::meter_t(
                convertToDouble(keyValuePair[0].trimmed(),
                                "Invalid double conversion for x1: %s"));

        // Convert y coordinate to meters.
        auto x2 =
            units::length::meter_t(
                convertToDouble(keyValuePair[1].trimmed(),
                                "Invalid double conversion for x2: %s"));

        // If there are exactly two elements (x and y), create a Point
        // object and add it to the QVector.
        if (keyValuePair.size() == 2) {
            auto p = std::make_shared<Point>(x1, x2);
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
static std::any toAppendagesWetSurfacesT(const QString& str)
{
    QMap<Ship::ShipAppendage, units::area::square_meter_t> appendages;

    // Split the string into pairs of appendage and area
    QStringList pairs = str.split(delim[1]);

    for (const QString& pair : pairs)
    {
        // Split each pair into appendage and area
        QStringList keyValuePair = pair.split(delim[2]);

        // Convert appendage to int and area to double
        int rawAppendage =
            convertToInt(keyValuePair[0].trimmed(),
                         "Invalid int conversion for appendage: %s");
        auto area =
            units::area::square_meter_t(
                convertToDouble(keyValuePair[1].trimmed(),
                                "Invalid double conversion for area: %s"));

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
static std::any toCSternT(const QString& str)
{
    // Convert string to int and cast to CStern enum
    int rawValue = convertToInt(str.trimmed(),
                                "Invalid conversion to int for CStern: %s");
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
static std::any toFuelTypeT(const QString& str)
{
    // Convert string to int and cast to FuelType enum
    int rawValue = convertToInt(str.trimmed(),
                                "Invalid conversion to int for fuel type: %s");
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
    {"ID", toQStringT},
    {"Path", toPathPointsT},
    {"WaterlineLength", toMeterT},
    {"Beam", toMeterT},
    {"DraftAtForward", toMeterT},
    {"DraftAtAft", toMeterT},
    {"VolumetricDisplacement", toCubicMeterT},
    {"WettedHullSurface", toSquareMeterT},
    {"BulbousBowTransverseAreaCenterHeight", toMeterT},
    {"BulbousBowTransverseArea", toMeterT},
    {"ImmersedTransomArea", toMeterT},
    {"HalfWaterlineEntranceAngle", toDegreesT},
    {"SurfaceRoughness", toNanoMeterT},
    {"RunLength", toMeterT},
    {"LongitudinalBuoyancyCenter", toDoubleT},
    {"SternShapeParam", toCSternT},
    {"MidshipSectionCoef", toDoubleT},
    {"WaterplaneAreaCoef", toDoubleT},
    {"PrismaticCoef", toDoubleT},
    {"BlockCoef", toDoubleT},

    // Fuel and tank parameters
    {"FuelType", toFuelTypeT},
    {"TankSize", toLiterT},
    {"TankInitialCapacityPercentage", toDoubleT},
    {"TankDepthOfDischage", toDoubleT},

    // Engine parameters
    {"EnginesCountPerPropeller", toIntT},
    {"EngineBrakePowerRPMMap", toEngineRPMT},
    {"EngineEfficiency", toEngineEfficiency},

    // Gearbox parameters
    {"GearboxRatio", toDoubleT},
    {"GearboxEfficiency", toDoubleT},

    // Propeller parameters
    {"ShaftEfficiency",toDoubleT},
    {"PropellerCount", toIntT},
    {"OpenWaterPropellerEfficiency", toQMapDoublesT},
    {"PropellerDiameter", toMeterT},
    {"PropellerExpandedAreaRatio",toDoubleT},

    // Operational parameters
    {"StopIfNoEnergy", toBoolT},
    {"MaxRudderAngle", toDegreesT},

    // Weight parameters
    {"VesselWeight", toTonsT},
    {"CargoWeight", toTonsT},

    // Appendages parameters
    {"AppendagesWettedSurfaces", toAppendagesWetSurfacesT}
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
static void readShipsFile(QString filename)
{
    // Counter to keep track of ship IDs.
    static qint64 idCounter = 0;

    // Map to store parameters and their values.
    QMap<QString, std::any> parameters;

    // Open the file for reading.
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) {
        // Log an error message if file cannot be opened.
        qCritical() << "Failed to open the file.";
        return;
    }

    // Create a QTextStream to read from the file.
    QTextStream in(&file);
    QString line;

    // Loop through each line in the file.
    while (!in.atEnd()) {
        // Read a line from the file and remove leading and trailing whitespace.
        line = in.readLine().trimmed();

        // Split the line into parts using the first delimiter in the delim list.
        QStringList parts = line.split(delim[0]);

        // Check if the number of parts matches the number of expected parameters.
        if (parts.size() == FileOrderedparameters.size())
        {
            // Loop through each part and process it.
            for (int i = 0; i < parts.size(); ++i)
            {
                // Use the converter function for the parameter to process the part
                // and store the result in the parameters map.
                parameters[FileOrderedparameters[i].name] =
                    FileOrderedparameters[i].converter(parts[i]);
            }
        }
    }
}


}
#endif // READSHIPS_H
