/**
 * @file network.h
 *
 * @brief This file contains the declaration of the Network class, which
 * represents a network of navigable waterways for ships.
 *
 * The Network class holds information about the water boundaries, visibility
 * graph, and region name. It provides methods to set and get water boundaries
 * and region name, and to calculate the shortest path between two points
 * using Dijkstra's algorithm.
 *
 * @author Ahmed Aredah
 * @date 10.12.2023
 */
#ifndef NETWORK_H
#define NETWORK_H


#include "polygon.h"
#include "visibilitygraph.h"
#include "QObject"

class Ship;

/**
 * @class Network
 *
 * @brief Represents a network of navigable waterways for ships.
 *
 * The Network class contains data related to the water boundaries of a
 * region, the visibility graph that represents navigable paths, and the
 * name of the region. It provides methods to set and get the water boundaries
 * and region name, and to calculate the shortest path between two points
 * within the network using Dijkstra's algorithm.
 */
class Network : QObject
{
    Q_OBJECT

private:
    // The water boundaries of the region, represented as a polygon.
    std::shared_ptr<Polygon> mWaterBoundries;

    // The visibility graph representing navigable paths in the region.
    std::shared_ptr<VisibilityGraph> mVisibilityGraph;

    // The name of the region.
    QString mRegionName;

    // Private method to check if a point is within a given polygon.
    bool containsPoint(const QVector<std::shared_ptr<Point>>& polygon,
                       const Point& pt);
public:
    // Default constructor.
    Network();

    // Constructor with water boundaries and region name parameters.
    Network(std::shared_ptr<Polygon> waterBoundries,
            QString regionName = "");

    // Constructor to load network data from a file.
    Network(QString filename);

    // Destructor.
    ~Network();

    // Sets the water boundaries of the region.
    void setWaterBoundries(std::shared_ptr<Polygon> waterBoundries);

    // Gets the name of the region.
    QString getRegionName();

    // Sets the name of the region.
    void setRegionName(QString newName);

    // Calculates the shortest path between two points using
    // Dijkstra's algorithm.
    ShortestPathResult dijkstraShortestPath(std::shared_ptr<Point> startPoint,
                                            std::shared_ptr<Point> endpoint);
};

#endif // NETWORK_H
