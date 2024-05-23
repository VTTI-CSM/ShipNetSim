/**
 * @class OptimizedVisibilityGraph
 *
 * @brief Provides functionalities for constructing a
 *        visibility graph and finding shortest paths within it.
 *
 * The OptimizedVisibilityGraph class is designed to create a
 * visibility graph from a set of polygons.
 * It allows for efficient pathfinding between points in the
 * graph using algorithms like A* and Dijkstra.
 * The graph is constructed by determining visibility between
 * points on the polygons' outer boundaries,
 * using a Quadtree for spatial indexing to optimize performance.
 *
 * Key Functionalities:
 * - Building a visibility graph from a given set of polygons.
 * - Finding the shortest path between points using A* or
 *      Dijkstra's algorithms.
 *
 * Usage:
 * 1. Create an instance of the class with a QVector of Polygons.
 * 2. Build the visibility graph using buildGraph().
 * 3. Use findShortestPathAStar() or findShortestPathDijkstra()
 *      to find shortest paths.
 */

#ifndef OPTIMIZEDVISIBILITYGRAPH_H
#define OPTIMIZEDVISIBILITYGRAPH_H

#include <QReadWriteLock>
#include <QHash>
#include <QtConcurrent>
#include <QVector>
#include <QMap>
#include "Polygon.h"
#include "gline.h"
#include "quadtree.h"
#include "seaport.h"

namespace ShipNetSimCore
{
enum class BoundariesType{
    Water,
    Land
};

enum class PathFindingAlgorithm
{
    AStar,
    Dijkstra
};

/**
 * @struct ShortestPathResult
 *
 * @brief Represents the result of a shortest path calculation.
 *
 * This struct contains two QVector objects, one storing the lines of
 * the path and the other storing the points of the path.
 */
struct ShortestPathResult
{
    QVector<std::shared_ptr<GLine>> lines;
    QVector<std::shared_ptr<GPoint>> points;

    /**
     * @brief Check if the output is valid.
     *
     * An output is valid if:
     *  1. Has 2 points minimum,
     *  2. Has 1 line minimum, and
     *  3. Number of lines is less than number of points by only 1 count.
     *
     *
     * @return true if the result is valid, false otherwise.
     */
    bool isValid();
};

/**
 * @brief The OptimizedVisibilityGraph class.
 *
 * This class defines the visibility graph based on the QuadTree.
 * It is recommended to use it when the dataset is very large and
 * the shortest path is computed between polygons. In this implementation,
 * the holes are not considered.
 */
class OptimizedVisibilityGraph
{
public:

    /**
     * @brief Constructor to initialize the visibility graph
     *          with a set of polygons.
     */
    OptimizedVisibilityGraph();

    /**
     * @brief Constructor to initialize the visibility graph
     *          with a set of polygons.
     * @param polygons A QVector of Polygon objects to be
     *          used for building the visibility graph.
     * @param wrapAround A flag to indicate the map is
     *          wrapped around for full earth map.
     */
    OptimizedVisibilityGraph(
        const QVector<std::shared_ptr<Polygon>> usedPolygons,
        BoundariesType boundaryType =
        BoundariesType::Water);

    /**
     * @brief Deconstructor to delete the visibility graph.
     */
    ~OptimizedVisibilityGraph();

    /**
     * @brief Get the left-lower corner point of the map
     * @details This function gets the lowerst point values in the map
     *              which is equivilant to the buttom-left corner of the map.
     *              The point has this structure:
     *              (min long in the map, min latitude in the map)
     * @return The min point of the map.
     */
    GPoint getMinMapPoint();

    /**
     * @brief Get the right-top corner point of the map
     * @details This function gets the highest point values in the map
     *              which is equivilant to the top-right corner of the map.
     *              The point has this structure:
     *              (max long in the map, max latitude in the map)
     * @return The max point of the map.
     */
    GPoint getMaxMapPoint();

    /**
     * @brief Load seaports with polygon points.
     * @details Load the seaports with nearest water polygon points.
     * @param seaPorts the seaports.
     */
    void loadSeaPortsPolygonCoordinates(
        QVector<std::shared_ptr<SeaPort>>& seaPorts);

    /**
     * @brief Set Polygons that the visibility graph is build on.
     *
     * This function defines the new polygons this visibility graph
     * is built on. The current visibility graph that is built will
     * be discarded. The function @ref{buildGraoh} should be called
     * again.
     *
     * @param polygons A QVector of Polygon objects to be
     *          used for building the visibility graph.
     *
     */
    void setPolygons(
        const QVector<std::shared_ptr<Polygon>>& newPolygons);

    /**
     * @brief Finds the shortest path between two points
     *          using the A* algorithm.
     * @param start The starting point of the path.
     * @param goal The goal point of the path.
     * @return ShortestPathResult containing the points
     *          and lines forming the shortest path.
     */
    ShortestPathResult findShortestPathAStar(
        const std::shared_ptr<GPoint>& start,
        const std::shared_ptr<GPoint>& goal);

    /**
     * @brief Finds the shortest path between two points
     *          using the dijkistra algorithm.
     * @param start The starting point of the path.
     * @param goal The goal point of the path.
     * @return ShortestPathResult containing the points
     *          and lines forming the shortest path.
     */
    ShortestPathResult findShortestPathDijkstra(
        const std::shared_ptr<GPoint>& start,
        const std::shared_ptr<GPoint>& goal);

    /**
     * @brief Finds the shortest path that traverses through
     *        a sequence of specified points.
     *
     * @details This function calculates the shortest path that
     *          must traverse through a given sequence of points
     *          in the order they are provided. It uses a specified
     *          pathfinding strategy (like Dijkstra's algorithm or
     *          the A* algorithm) to find the shortest path between
     *          consecutive points.
     *
     * @param mustTraversePoints A QVector of shared pointers
     *        to Point objects. These are the points
     *        that the path must traverse, in the order they
     *        are provided.
     * @param pathfindingStrategy A function that takes two
     *        shared pointers to Point objects (representing
     *        the start and end points) and returns the shortest
     *        path between them as a ShortestPathResult.
     *        This function should implement a pathfinding
     *        algorithm (like Dijkstra's or A*).
     *
     * @return ShortestPathResult which includes the points and
     *         lines comprising the shortest path.
     *
     * Example Usage:
     *
     * 1. Using with Dijkstra's algorithm:
     *    auto pathDijkstra = optimizedVisibilityGraph.findShortestPath(
     *        mustTraversePoints,
     *        std::bind(&OptimizedVisibilityGraph::findShortestPathDijkstra,
     *                  &optimizedVisibilityGraph, std::placeholders::_1,
     *                  std::placeholders::_2));
     *
     * 2. Using with A* algorithm:
     *    auto pathAStar = optimizedVisibilityGraph.findShortestPath(
     *        mustTraversePoints,
     *        std::bind(&OptimizedVisibilityGraph::findShortestPathAStar,
     *                  &optimizedVisibilityGraph, std::placeholders::_1,
     *                  std::placeholders::_2));
     */
    ShortestPathResult findShortestPath(
        QVector<std::shared_ptr<GPoint>> mustTraversePoints,
        PathFindingAlgorithm algorithm);

    /**
     * @brief Clear the visibility graph cache
     */
    void clear();

private:

    mutable std::mutex mutex;

    bool enableWrapAround;

    BoundariesType mBoundaryType;

    /**
     * @brief Stores the polygons used to build the visibility graph.
     *
     * This QVector contains the Polygon objects that define
     * the environment or space within which the visibility
     * graph is constructed. Only the outer boundaries of
     * these polygons are considered for constructing the graph.
     */
    QVector<std::shared_ptr<Polygon>> polygons;

    /**
     * @brief A unique pointer to a Quadtree object used
     *          for spatial indexing.
     *
     * The Quadtree is used to optimize visibility checks
     * between points. It spatially indexes
     * the edges of the polygons, allowing for efficient
     * querying of which edges might intersect
     * with a line of sight between any two points in the graph.
     */
    std::unique_ptr<Quadtree> quadtree;

    QReadWriteLock cacheLock;
    /**
     * @brief A map that holds the visible nodes of current
     * node once calculated.
     *
     * The cache is used to memories nodes that are visible
     * to the current node. It is used to reduce computatis
     * and reduce cost. It only gets cleaned if new polygons
     * are added or the user cleaned the class explicitly.
     */
    QHash<std::shared_ptr<GPoint>,
          QVector<std::shared_ptr<GPoint>>> visibilityCache;


    /// Determines if there is a direct line of sight between two nodes.
    bool isVisible(const std::shared_ptr<GPoint>& node1,
                   const std::shared_ptr<GPoint>& node2) const;
    ///  Determines if there is a direct line of sight between two nodes.
    bool isSegmentVisible(
        const std::shared_ptr<GLine>& segment) const;

    ///  Reconstructs the shortest path from a series of came-from nodes.
    ShortestPathResult reconstructPath(
        const std::unordered_map<std::shared_ptr<GPoint>,
                                 std::shared_ptr<GPoint>,
                                 GPoint::Hash, GPoint::Equal>& cameFrom,
        std::shared_ptr<GPoint> current);

    QVector<std::shared_ptr<GPoint>> getVisibleNodesBetweenPolygons(
            const std::shared_ptr<GPoint>& node,
            const QVector<std::shared_ptr<Polygon>>& allPolygons);

    QVector<std::shared_ptr<GPoint>> getVisibleNodesWithinPolygon(
        const std::shared_ptr<GPoint>& node,
        const std::shared_ptr<Polygon>& polygon);

    bool isPointVisibleWithinPolygon(
        const std::shared_ptr<GPoint>& node,
        const std::shared_ptr<GPoint>& target,
        const std::shared_ptr<Polygon>& polygon);

    std::shared_ptr<Polygon> findContainingPolygon(
        const std::shared_ptr<GPoint>& point);

    /// Connect left-right points of the map for a wrap-around technique
    // QVector<std::shared_ptr<GPoint>> connectWrapAroundPoints(
    //     const std::shared_ptr<GPoint>& point,
    //     const std::shared_ptr<Polygon>& polygon);

    ShortestPathResult findShortestPathHelper(
        QVector<std::shared_ptr<GPoint>> mustTraversePoints,
        std::function<ShortestPathResult(
            const std::shared_ptr<GPoint>&,
            const std::shared_ptr<GPoint>&)> pathfindingStrategy);
    // /**
    //  * @brief Maps each unique point in the visibility graph
    //  *          to a unique identifier (node ID).
    //  *
    //  * This QMap stores a mapping between points
    //  * (vertices of the polygons) and their corresponding
    //  * node IDs in the graph. Each point is represented
    //  * by a shared pointer to a Point object,
    //  * and the node ID is a unique integer assigned to that point.
    //  */
    // QMap<std::shared_ptr<Point>, qsizetype> nodes;

    // /**
    //  * @brief Represents the edges in the visibility graph.
    //  *
    //  * This QVector of QVectors stores the adjacency list
    //  * of the graph. Each index in the outer
    //  * QVector corresponds to a node ID, and the inner
    //  * QVector contains the node IDs of all points that
    //  * are visible (directly connected) from this node.
    //  * It effectively represents the edges of the graph.
    //  */
    // QVector<QVector<qsizetype>> edges;




    // /// Retrieves all nodes visible from a given node.
    // QVector<std::shared_ptr<Point>> getVisibleNodes(
    //     const std::shared_ptr<Point>& node);

    // /// Adds a point as a node in the graph.
    // qsizetype addNode(const std::shared_ptr<Point>& point);

    // /// Creates an edge between two nodes.
    // void addEdge(qsizetype node1, qsizetype node2);
};
};
#endif // OPTIMIZEDVISIBILITYGRAPH_H
