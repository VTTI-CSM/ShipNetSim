/**
 * @file VisibilityGraph.h
 *
 * @brief Definition of the VisibilityGraph class.
 *
 * This file contains the definition of the VisibilityGraph class,
 * which is used to represent a visibility graph for path planning
 * in a polygonal environment.
 *
 * @author Ahmed Aredah
 * @date 10.12.2023
 */
#ifndef VISIBILITYGRAPH_H
#define VISIBILITYGRAPH_H

#include "Point.h"
#include "Polygon.h"
#include "Line.h"
#include <unordered_map>
#include "../../third_party/units/units.h"
#include "visibilitygraph.h"
#include "hierarchicalvisibilitygraph.h"

/**
 * @struct ShortestPathResult
 *
 * @brief Represents the result of a shortest path calculation.
 *
 * This struct contains two QVector objects, one storing the lines of
 * the path and the other storing the points of the path.
 */
// struct ShortestPathResult {
//     QVector<std::shared_ptr<Line>> lines;
//     QVector<std::shared_ptr<Point>> points;
// };

/**
 * @class VisibilityGraph
 *
 * @brief Represents a visibility graph.
 *
 * The VisibilityGraph class is used to represent a visibility graph
 * for path planning in a polygonal environment. The graph is composed
 * of nodes, which are points in the environment, and edges, which are
 * lines between points that are visible to each other.
 *
 * @see Point
 * @see Polygon
 * @see Line
 */
class VisibilityGraph
{
private:
//    std::shared_ptr<Point> startNode; ///< Start point of the path.
//    std::shared_ptr<Point> endNode; ///< End point of the path.
    QVector<std::shared_ptr<Point>> mustTraversePoints;
    std::shared_ptr<Polygon> polygon; ///< Polygonal environment.
    std::unordered_map<std::shared_ptr<Point>,
                       QVector<std::pair<std::shared_ptr<Line>,
                                         units::length::meter_t>>>
        graph; ///< Graph.


    /**
     * @brief Remove vertices and edges associated with a node.
     *
     * @param nodeToRemove Node to be removed.
     */
    void removeVerticesAndEdges(std::shared_ptr<Point> nodeToRemove);


    ShortestPathResult dijkstraShortestPath(
        std::shared_ptr<Point> startPoint,
        std::shared_ptr<Point> endPoint);

public:

    /**
     * @brief Constructor.
     *
     * Initializes a visibility graph with a start point, end point, and
     * polygonal environment.
     *
     * @param startNode Start point of the path.
     * @param endNode End point of the path.
     * @param polygon Polygonal environment.
     */
    VisibilityGraph(std::shared_ptr<Point> startNode,
                    std::shared_ptr<Point> endNode,
                    std::shared_ptr<Polygon> polygon);


    /**
     * @brief Constructor.
     *
     * Initializes a visibility graph with a must-visit points, and
     * polygonal environment. The path will be built in the point order.
     *
     * @param points The points where the path will be built
     * in the same order.
     * @param polygon Polygonal environment.
     */
    VisibilityGraph(QVector<std::shared_ptr<Point>> points,
                    std::shared_ptr<Polygon> polygon);

    /**
     * @brief Constructor.
     *
     * Initializes a visibility graph with a polygonal environment.
     *
     * @param polygon Polygonal environment.
     */
    VisibilityGraph(std::shared_ptr<Polygon> polygon);

    /**
     * @brief Destructor.
     */
    ~VisibilityGraph();

    /**
     * @brief Set the must-traverse points of the path.
     * It must include starting and ending points.
     *
     * @param points The must traverse points on the path.
     */
    void setTraversePoints(QVector<std::shared_ptr<Point>> points);

    /**
     * @brief Get the start point of the path.
     *
     * @return Start point of the path.
     */
    std::shared_ptr<Point> startPoint();

    /**
     * @brief Get the end point of the path.
     *
     * @return End point of the path.
     */
    std::shared_ptr<Point> endPoint();

    /**
     * @brief Build the visibility graph.
     */
    void buildGraph();

    /**
     * @brief Calculate the shortest path using Dijkstra's algorithm.
     *
     * @return Shortest path result.
     */
    ShortestPathResult dijkstraShortestPath();

    /**
     * @brief Get points from a vector of lines.
     *
     * @param pathLines Vector of lines.
     * @return Vector of points.
     */
    QVector<std::shared_ptr<Point>>
    getPointsFromLines(
        const QVector<std::shared_ptr<Line>>& pathLines);

    /**
     * @brief Get lines from a vector of points.
     *
     * @param pathPoints Vector of points.
     * @return Vector of lines.
     */
    QVector<std::shared_ptr<Line>>
    getLinesFromPoints(
        const QVector<std::shared_ptr<Point>>& pathPoints);
};

#endif // VISIBILITYGRAPH_H
