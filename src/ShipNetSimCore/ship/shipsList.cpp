#include "shipsList.h"

namespace ShipNetSimCore
{
namespace ShipsList
{

QVector<QMap<QString, std::any>>
readShipsFile(QString filename, OptimizedNetwork *network,
              bool isResistanceStudyOnly)
{
    if (isResistanceStudyOnly)
    {
        FileOrderedparameters[1].isOptional = true;
    }
    else
    {
        if (network == nullptr)
        {
            throw ShipLoadException("Network cannot be null");
        }
    }

    // Map to store parameters and their values.
    QMap<QString, std::any> parameters;
    // a vector holding ships
    QVector<QMap<QString, std::any>> ships;

    // Open the file for reading.
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly))
    {
        // Log an error message if file cannot be opened.
        throw ShipLoadException("Failed to open the ships file: "
                                + filename);
        return ships;
    }

    // Create a QTextStream to read from the file.
    QTextStream in(&file);
    QString     line;

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

        parameters =
            readShipFromString(line, network, isResistanceStudyOnly);

        ships.push_back(parameters);
        parameters.clear();
    }

    return ships;
}

QVector<QMap<QString, QString>>
readShipsFileToStrings(QString filename)
{

    // Map to store parameters and their values.
    QMap<QString, QString> parameters;
    // a vector holding ships
    QVector<QMap<QString, QString>> ships;

    // Open the file for reading.
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly))
    {
        // Log an error message if file cannot be opened.
        throw ShipLoadException("Failed to open the ships file: "
                                + filename);

        return ships;
    }

    // Create a QTextStream to read from the file.
    QTextStream in(&file);
    QString     line;

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

        parameters = readShipFromStringToStrings(line);

        ships.push_back(parameters);
        parameters.clear();
    }

    return ships;
}

QMap<QString, QString> readShipFromStringToStrings(QString line)
{
    // Map to store parameters and their values.
    QMap<QString, QString> parameters;

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
            // Load each parameter as a string only without further
            // processing
            parameters[ShipsList::FileOrderedparameters[i].name] =
                parts[i];
        }
    }
    else
    {
        throw ShipLoadException("Not all parameters are provided! "
                                "\n Check the ships file");
    }

    return parameters;
}

QMap<QString, std::any> readShipFromString(QString           line,
                                           OptimizedNetwork *network,
                                           bool isResistanceStudyOnly)
{

    // Map to store parameters and their values.
    QMap<QString, std::any> parameters;

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
            parameters[ShipsList::FileOrderedparameters[i].name] =
                FileOrderedparameters[i].converter(
                    parts[i], FileOrderedparameters[i].isOptional);
        }

        if (network != nullptr && !isResistanceStudyOnly)
        {
            auto pathPoints = Utils::getValueFromMap<
                QVector<std::shared_ptr<GPoint>>>(
                parameters, "Path",
                QVector<std::shared_ptr<GPoint>>());
            ShortestPathResult results = network->findShortestPath(
                pathPoints, PathFindingAlgorithm::Dijkstra);

            if (!results.isValid())
            {
                throw ShipLoadException(
                    "Could not find ship path!\n");
            }

            parameters["PathPoints"] = results.points;
            parameters["PathLines"]  = results.lines;
        }
        if (network == nullptr || isResistanceStudyOnly)
        {
            QVector<std::shared_ptr<GPoint>> fakePoints;
            QVector<std::shared_ptr<GLine>>  fakeLines;

            fakePoints.push_back(std::make_shared<GPoint>(
                units::angle::degree_t(0.0),
                units::angle::degree_t(0.0)));
            fakePoints.push_back(std::make_shared<GPoint>(
                units::angle::degree_t(100.0),
                units::angle::degree_t(100.0)));
            fakeLines.push_back(std::make_shared<GLine>(
                fakePoints[0], fakePoints[1]));
            parameters["PathPoints"] = fakePoints;
            parameters["PathLines"]  = fakeLines;
        }
    }
    else
    {
        throw ShipLoadException("Not all parameters are provided! "
                                "\n Check the ships file");
    }

    return parameters;
}

bool writeShipsFile(QString                                filename,
                    const QVector<QMap<QString, QString>> &ships,
                    const QVector<QString> headerLines)
{
    // Open the file for writing.
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        throw ShipLoadException("Failed to open the ships file "
                                + filename + " for writing.");
        return false;
    }

    // Create a QTextStream to write to the file.
    QTextStream out(&file);

    if (!headerLines.isEmpty())
    {
        for (const auto &headerLine : headerLines)
        {
            out << "# " << headerLine << "\n";
        }
    }

    // Loop through each ship in the vector.
    for (const auto &ship : ships)
    {
        QStringList parts;

        // Loop through each parameter in the FileOrderedparameters
        // list.
        for (const auto &param : FileOrderedparameters)
        {
            // Check if the parameter exists in the ship map.
            if (ship.contains(param.name))
            {
                // Get the value from the map and convert it to a
                // string.
                const auto &value = ship[param.name];
                if (value.isEmpty() && param.isOptional)
                {
                    parts.append("NAN");
                }
                else
                {
                    parts.append(toString(value));
                }
            }
            else
            {
                // If the parameter is optional, add an empty string.
                if (param.isOptional)
                {
                    parts.append("NAN");
                }
                else
                {
                    throw ShipLoadException(
                        "Missing non-optional parameter: "
                        + param.name);
                    return false;
                }
            }
        }

        // Join the parts with the delimiter and write the line to the
        // file.
        out << parts.join(delim[0]) << "\n";
    }

    // Close the file.
    file.close();

    return true;
}

template <typename T>
std::shared_ptr<Ship>
loadShipFromParameters(QMap<QString, T>  shipDetails,
                       OptimizedNetwork *network,
                       bool              isResistanceStudyOnly)
{

    QMap<QString, std::any> convertedParameters;

    if constexpr (std::is_same_v<T, QString>)
    {
        for (auto it = shipDetails.begin(); it != shipDetails.end();
             ++it)
        {
            QString key   = it.key();
            QString value = it.value();
            auto    param =
                findParamInfoByKey(key, FileOrderedparameters);
            if (!param)
            {
                throw ShipLoadException(
                    "Could not find ship parameter");
            }
            convertedParameters[key] =
                param->converter(value, param->isOptional);
        }
    }
    else
    {
        convertedParameters = shipDetails;
    }

    if (network != nullptr && !isResistanceStudyOnly)
    {
        auto pathPoints =
            Utils::getValueFromMap<QVector<std::shared_ptr<GPoint>>>(
                convertedParameters, "Path",
                QVector<std::shared_ptr<GPoint>>());
        ShortestPathResult results = network->findShortestPath(
            pathPoints, PathFindingAlgorithm::Dijkstra);

        if (!results.isValid())
        {
            throw ShipLoadException("Could not find ship path!\n");
        }

        convertedParameters["PathPoints"] = results.points;
        convertedParameters["PathLines"]  = results.lines;
    }
    if (network == nullptr || isResistanceStudyOnly)
    {
        QVector<std::shared_ptr<GPoint>> fakePoints;
        QVector<std::shared_ptr<GLine>>  fakeLines;

        fakePoints.push_back(
            std::make_shared<GPoint>(units::angle::degree_t(0.0),
                                     units::angle::degree_t(0.0)));
        fakePoints.push_back(
            std::make_shared<GPoint>(units::angle::degree_t(100.0),
                                     units::angle::degree_t(100.0)));
        fakeLines.push_back(
            std::make_shared<GLine>(fakePoints[0], fakePoints[1]));
        convertedParameters["PathPoints"] = fakePoints;
        convertedParameters["PathLines"]  = fakeLines;
    }

    return std::make_shared<Ship>(convertedParameters);
}

template <typename T>
QVector<std::shared_ptr<Ship>>
loadShipsFromParameters(QVector<QMap<QString, T>> shipsDetails,
                        OptimizedNetwork         *network,
                        bool isResistanceStudyOnly)
{
    QVector<std::shared_ptr<Ship>> ships;

    for (auto &parameters : shipsDetails)
    {
        auto ship = loadShipFromParameters(parameters, network,
                                           isResistanceStudyOnly);

        ships.push_back(std::move(ship));
    }

    return ships;
}

// Explicit instantiation of the template for QString type
template SHIPNETSIM_EXPORT std::shared_ptr<Ship>
loadShipFromParameters<QString>(QMap<QString, QString> shipsDetails,
                                OptimizedNetwork      *network,
                                bool isResistanceStudyOnly);

// Explicit instantiation of the template for std::any type
template SHIPNETSIM_EXPORT std::shared_ptr<Ship>
loadShipFromParameters<std::any>(QMap<QString, std::any> shipDetails,
                                 OptimizedNetwork       *network,
                                 bool isResistanceStudyOnly);

// Explicit instantiation of the template for QString type
template SHIPNETSIM_EXPORT QVector<std::shared_ptr<Ship>>
                           loadShipsFromParameters<QString>(
    QVector<QMap<QString, QString>> shipsDetails,
    OptimizedNetwork *network, bool isResistanceStudyOnly);

// Explicit instantiation of the template for std::any type
template SHIPNETSIM_EXPORT QVector<std::shared_ptr<Ship>>
                           loadShipsFromParameters<std::any>(
    QVector<QMap<QString, std::any>> shipsDetails,
    OptimizedNetwork *network, bool isResistanceStudyOnly);

std::shared_ptr<Ship>
loadShipFromParameters(QJsonObject       shipJson,
                       OptimizedNetwork *network,
                       bool              isResistanceStudyOnly)
{

    // Map to store converted parameters
    QMap<QString, std::any> convertedParameters;

    // Iterate over all key-value pairs in the JSON object
    for (auto it = shipJson.begin(); it != shipJson.end(); ++it)
    {
        QString key = it.key();
        QString value;

        if (it.value().isString())
        {
            value = it.value().toString();
        }
        else if (it.value().isDouble())
        {
            value = QString::number(it.value().toDouble());
        }
        else if (it.value().isBool())
        {
            value = it.value().toBool() ? "true" : "false";
        }
        else
        {
            throw ShipLoadException("Unsupported value type for key: "
                                    + key);
        }

        // Find the parameter converter based on the key
        auto param = findParamInfoByKey(key, FileOrderedparameters);
        if (!param)
        {
            throw ShipLoadException(
                "Could not find ship parameter for key: " + key);
        }

        // Convert the string value using the appropriate converter
        convertedParameters[key] =
            param->converter(value, param->isOptional);
    }

    // Handle network-based path points if network is provided and
    // not in resistance study mode
    if (network != nullptr && !isResistanceStudyOnly)
    {
        auto pathPoints =
            Utils::getValueFromMap<QVector<std::shared_ptr<GPoint>>>(
                convertedParameters, "Path",
                QVector<std::shared_ptr<GPoint>>());

        // Use the network to find the shortest path
        ShortestPathResult results = network->findShortestPath(
            pathPoints, PathFindingAlgorithm::Dijkstra);

        if (!results.isValid())
        {
            throw ShipLoadException("Could not find ship path!");
        }

        // Add path points and lines to the parameters map
        convertedParameters["PathPoints"] = results.points;
        convertedParameters["PathLines"]  = results.lines;
    }

    // Fallback for resistance study or missing network
    if (network == nullptr || isResistanceStudyOnly)
    {
        QVector<std::shared_ptr<GPoint>> fakePoints;
        QVector<std::shared_ptr<GLine>>  fakeLines;

        fakePoints.push_back(
            std::make_shared<GPoint>(units::angle::degree_t(0.0),
                                     units::angle::degree_t(0.0)));
        fakePoints.push_back(
            std::make_shared<GPoint>(units::angle::degree_t(100.0),
                                     units::angle::degree_t(100.0)));
        fakeLines.push_back(
            std::make_shared<GLine>(fakePoints[0], fakePoints[1]));
        convertedParameters["PathPoints"] = fakePoints;
        convertedParameters["PathLines"]  = fakeLines;
    }

    // Create and return the Ship object using the converted
    // parameters
    return std::make_shared<Ship>(convertedParameters);
}

QVector<std::shared_ptr<Ship>>
loadShipsFromJson(const QJsonObject &shipsJson,
                  OptimizedNetwork  *network,
                  bool               isResistanceStudyOnly)
{
    QVector<std::shared_ptr<Ship>> ships;

    // The JSON object should contain an array of ships
    if (!shipsJson.contains("ships") || !shipsJson["ships"].isArray())
    {
        throw ShipLoadException("The JSON does not contain a "
                                "valid 'ships' array.");
        return ships;
    }

    // Extract the array of ships
    QJsonArray shipsArray = shipsJson["ships"].toArray();

    // Iterate over each ship object in the array
    for (const QJsonValue &shipValue : shipsArray)
    {
        if (!shipValue.isObject())
        {
            throw ShipLoadException("Invalid ship definition "
                                    "in the JSON array.");
            continue;
        }

        QJsonObject shipJson = shipValue.toObject();

        // Load each ship from the JSON object
        auto ship = loadShipFromParameters(shipJson, network,
                                           isResistanceStudyOnly);
        ships.push_back(ship);
    }

    return ships;
}

} // namespace ShipsList
} // namespace ShipNetSimCore
