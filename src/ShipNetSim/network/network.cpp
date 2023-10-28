/**
 * @file Network.cpp
 *
 * @brief Implementation of the Network class.
 *
 * This file contains the implementation of the Network class. The
 * Network class represents the whole network of water bodies, including
 * water boundaries and inner holes.
 *
 * @author Ahmed Aredah
 * @date 10.12.2023
 */

// Include required header files
#include "network.h"     // Network class definition
#include "qdebug.h"      // Debugging utilities
#include <QCoreApplication>  // Core application utilities
#include <QFile>             // File input/output utilities
#include <QTextStream>       // Text stream utilities
#include <QDebug>            // Debugging utilities

// Default constructor
Network::Network() : QObject(nullptr)
{
    // Empty constructor body
}

// Constructor with water boundaries and region name
Network::Network(std::shared_ptr<Polygon> waterBoundries,
                 QString regionName) : QObject(nullptr)
{
    mWaterBoundries = waterBoundries;  // Set water boundaries
    mVisibilityGraph =
        std::make_shared<VisibilityGraph>(
        mWaterBoundries);  // Create visibility graph
    mRegionName = regionName;  // Set region name
}

// Constructor with filename
Network::Network(QString filename)
{
    static qint64 idCounter = 0;  // Counter for unique IDs
    QVector<std::shared_ptr<Point>> boundary;  // Store water body boundaries
    QVector<QVector<std::shared_ptr<Point>>> holes;  // Store inner holes

    // Open file with given filename
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) {
        qCritical() << "Failed to open the file.";  // Log error message
        return;
    }

    // Create text stream from file
    QTextStream in(&file);
    QString line;  // Store each line from file
    QString name;  // Store region name

    // Read file line by line
    while (!in.atEnd()) {
        line = in.readLine().trimmed();  // Read and trim line from file

        // Parse MAX_SPEED section
        if (line == "[MAX_SPEED]")
        {
            line = in.readLine();  // Read next line
            double maxSpeed = line.split(" ")[0].toDouble(); // Parse max speed
        }
        // Parse DEPTH section
        else if (line == "[DEPTH]")
        {
            line = in.readLine();  // Read next line
            double depth = line.split(" ")[0].toDouble();  // Parse depth
        }
        // Parse NAME section
        else if (line == "[NAME]")
        {
            line = in.readLine();  // Read next line
            name = line;  // Store region name
        }
        // Parse WATER and HOLE sections
        else if (line == "[WATER]" || line.startsWith("[HOLE"))
        {
            QVector<std::shared_ptr<Point>> polygon;  // Store polygon points

            // Read each point from WATER or HOLE section
            while (!in.atEnd() && (line = in.readLine().trimmed()) != "[END]")
            {
                QStringList parts = line.split(",");  // Split line by comma

                // Check if line has correct format
                if (parts.size() == 3)
                {
                    auto userid = parts[0];  // Store user ID
                    auto x =
                        units::length::meter_t(
                        parts[1].toDouble());  // Parse X coordinate
                    auto y =
                        units::length::meter_t(
                        parts[2].toDouble());  // Parse Y coordinate

                    Point pt(x, y, 0, 0);  // Create Point object

                    // Check if point is not already in polygon
                    if (!containsPoint(polygon, pt))
                    {
                        qint64 simulatorid =
                            ++idCounter;  // Generate unique ID

                        // Create shared Point object
                        std::shared_ptr<Point> p =
                            std::make_shared<Point>(x,
                                                    y,
                                                    userid,
                                                    simulatorid);
                        polygon.append(p);  // Add Point object to polygon
                    }
                }
                else
                {
                    qDebug() << "A point must have an ID, "
                                "x, and y coordinates!";  // Log error message
                }
            }

            // Store boundary or hole points
            if (line == "[WATER]")
            {
                boundary = polygon;  // Store water body boundary
            }
            else
            {
                holes.append(polygon);  // Store inner hole
            }
        }
    }

    file.close();  // Close file

    auto waterBody =
        std::make_shared<Polygon>(boundary,
                                  holes);  // Create shared Polygon object
    Network(waterBody,
            name);  // Init Network object with water body and region name

    return;  // Return from constructor
}

// Check if a point is in a polygon
bool Network::containsPoint(const QVector<std::shared_ptr<Point>>& polygon,
                            const Point& pt)
{
    // Iterate through each point in polygon
    for (const auto& point : polygon)
    {
        // Check if point is equal to pt
        if (*point == pt)
        {
            return true;  // Return true if point is in polygon
        }
    }
    return false;  // Return false if point is not in polygon
}

// Destructor
Network::~Network()
{
    // Empty destructor body
}

// Set water boundaries
void Network::setWaterBoundries(std::shared_ptr<Polygon> waterBoundries)
{
    mWaterBoundries = waterBoundries;  // Set water boundaries
    mVisibilityGraph =
        std::make_shared<VisibilityGraph>(
        mWaterBoundries);  // Create visibility graph
}

// Get region name
QString Network::getRegionName()
{
    return mRegionName;  // Return region name
}

// Set region name
void Network::setRegionName(QString newName)
{
    mRegionName = newName;  // Set region name
}

// Calculate shortest path using Dijkstra's algorithm
ShortestPathResult Network::dijkstraShortestPath(
    std::shared_ptr<Point> startPoint,
    std::shared_ptr<Point> endpoint)
{
    // Check if water boundaries are defined
    if (!mWaterBoundries)
    {
        throw std::exception("Water boundary "
                             "is not defined yet!");  // Throw exception
    }

    // Set start and end points in visibility graph
    mVisibilityGraph->setStartPoint(startPoint);
    mVisibilityGraph->setEndPoint(endpoint);

    // Build visibility graph
    mVisibilityGraph->buildGraph();

    // Calculate shortest path and return result
    return mVisibilityGraph->dijkstraShortestPath();
}
