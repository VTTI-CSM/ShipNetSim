#include <QSet>
#include <QQueue>
#include <functional>
#include <set>
#include <thread>
#include <unordered_map>
#include "optimizedvisibilitygraph.h"

namespace ShipNetSimCore
{

bool ShortestPathResult::isValid()
{
    return (points.size() >= 2 &&
            lines.size() >= 1 &&
            lines.size() == (points.size() - 1));
}
OptimizedVisibilityGraph::OptimizedVisibilityGraph() {}

OptimizedVisibilityGraph::OptimizedVisibilityGraph(
    const QVector<std::shared_ptr<Polygon>> usedPolygons,
    BoundariesType boundaryType) :
    mBoundaryType(boundaryType),
    polygons(usedPolygons)
{
    // Initialize Quadtree with polygons
    quadtree = std::make_unique<Quadtree>(polygons);
}

// Destructor
OptimizedVisibilityGraph::~OptimizedVisibilityGraph() {}

GPoint OptimizedVisibilityGraph::getMinMapPoint()
{
    return quadtree->getMapMinPoint();
}

GPoint OptimizedVisibilityGraph::getMaxMapPoint()
{
    return quadtree->getMapMaxPoint();
}

void OptimizedVisibilityGraph::loadSeaPortsPolygonCoordinates(
    QVector<std::shared_ptr<SeaPort> > &seaPorts)
{
    for (auto& seaPort : seaPorts)
    {
        std::shared_ptr<GPoint> portCoord =
            std::make_shared<GPoint>(seaPort->getPortCoordinate());
        seaPort->setClosestPointOnWaterPolygon(
            quadtree->findNearestNeighborPoint(portCoord));
    }
}

void OptimizedVisibilityGraph::setPolygons(
    const QVector<std::shared_ptr<Polygon>>& newPolygons)
{
    visibilityCache.clear();
    quadtree->clearTree();
    polygons = newPolygons;
}

QVector<std::shared_ptr<GPoint>> OptimizedVisibilityGraph::
    getVisibleNodesBetweenPolygons(
    const std::shared_ptr<GPoint>& node,
    const QVector<std::shared_ptr<Polygon>>& allPolygons)
{

    QVector<std::shared_ptr<GPoint>> visibleNodes;

    // First, check if the visibility for the node is already cached
    {
        QReadLocker locker(&cacheLock);
        if (visibilityCache.contains(node)) {
            // Return the cached result
            return visibilityCache[node];
        }
    }

    // Prepare a vector of tasks
    QVector<std::shared_ptr<GPoint>> tasks;
    for (const auto& polygon : allPolygons) {
        if (polygon->contains(node)) continue;

        for (const auto& point : polygon->outer()) {
            if (point != node) tasks.push_back(point);
        }

        for (const auto& hole : polygon->inners()) {
            for (const auto& holePoint : hole) {
                if (holePoint != node) tasks.push_back(holePoint);
            }
        }
    }

    // Visibility check function
    auto checkVisibility = [this, node, &visibleNodes]
        (std::shared_ptr<GPoint>& point)
    {
        if (isVisible(node, point))
        {
            QWriteLocker locker(&cacheLock);
            visibleNodes.push_back(point);
        }
    };

    // Concurrently process each task
    QtConcurrent::map(tasks.begin(), tasks.end(), checkVisibility);

    // Wait for all tasks to finish
    QFuture<void> future = QtConcurrent::map(tasks, checkVisibility);
    future.waitForFinished();

    // Cache the calculated visible nodes
    {
        QWriteLocker locker(&cacheLock);
        visibilityCache[node].append(visibleNodes);
    }

    return visibilityCache[node];
}

// with threads
QVector<std::shared_ptr<GPoint>> OptimizedVisibilityGraph::
    getVisibleNodesWithinPolygon(
    const std::shared_ptr<GPoint>& node,
    const std::shared_ptr<Polygon>& polygon)
{
    // First, check if the visibility for the node is already cached
    {
        QReadLocker locker(&cacheLock);
        if (visibilityCache.contains(node))
        {
            // Return the cached result
            return visibilityCache[node];
        }
    } // locker goes out of scope here, releasing the read lock >>> important

    // Get a copy of the polygon's boundary points
    QVector<std::shared_ptr<GPoint>> points = polygon->outer();

    // Define a lambda function for visibility check
    auto checkVisibility = [this, &node](std::shared_ptr<GPoint>& point)
    {
        if (*(node) == *(point))
        {
            return;
        }

        // Implement the logic to check if 'point' is visible from 'node'
        // Checking line of sight intersections
        if (this->isVisible(node, point))
        {
            QWriteLocker locker(&cacheLock);
            visibilityCache[node].push_back(point);
        } // locker goes out of scope here, releasing the read lock
    };

    // Use QtConcurrent::map for parallel processing
    QFuture<void> future =
        QtConcurrent::map(points.begin(), points.end(), checkVisibility);

    // Wait for all concurrent tasks to finish
    // This step is necessary to ensure that all points have been processed
    // before we proceed to return the result
    future.waitForFinished();

    // Now, return the cached result for the node
    QReadLocker locker(&cacheLock);
    return visibilityCache.value(node);
}


bool OptimizedVisibilityGraph::isVisible(
    const std::shared_ptr<GPoint>& node1,
    const std::shared_ptr<GPoint>& node2) const
{
    std::lock_guard<std::mutex> guard(mutex);

    if (!node1 || !node2) {
        throw std::runtime_error("Point is not valid!\n");
    }

    // if the two points are the same, return true
    if (*(node1) == *(node2))
    {
        return true;
    }

    // Create a line segment between node1 and node2
    std::shared_ptr<GLine> lineSegment =
        std::make_shared<GLine>(node1, node2);

    // Normal case
    return isSegmentVisible(lineSegment);
}

bool OptimizedVisibilityGraph::isSegmentVisible(
    const std::shared_ptr<GLine>& segment) const
{
    // 1. Quick check for length - if points are very close, return true
    if (segment->startPoint()->distance(*segment->endPoint()) <
        units::length::meter_t(1.0)) {
        return true;
    }

    // 2. Get intersecting nodes using parallel processing
    auto intersectingNodes =
        quadtree->findNodesIntersectingLineSegmentParallel(segment);

    // 3. Use quick bounding box rejection before detailed intersection
    auto segBounds = std::make_pair(
        std::min(segment->startPoint()->getLongitude().value(),
                 segment->endPoint()->getLongitude().value()),
        std::max(segment->startPoint()->getLongitude().value(),
                 segment->endPoint()->getLongitude().value()));

    // 4. Process nodes in parallel for large datasets
    if (intersectingNodes.size() > 1000) {
        QAtomicInt hasIntersection = 0;

        // Create a vector of line segments from nodes to process
        QVector<QVector<std::shared_ptr<GLine>>> nodeSegmentsToProcess;
        for (auto* node : intersectingNodes) {
            nodeSegmentsToProcess.push_back(quadtree->
                                            getAllSegmentsInNode(node));
        }

        // Process segments in parallel for faster checks
        auto processSegments = [&hasIntersection, &segment, &segBounds]
            (const QVector<std::shared_ptr<GLine>>& nodeSegments) {
                if (hasIntersection.loadAcquire()) {
                    return;
                }

                // Sort segments by distance to potentially
                // find intersections faster
                auto sortedSegments = nodeSegments;
                std::sort(sortedSegments.begin(), sortedSegments.end(),
                          [&segment](const auto& a, const auto& b) {
                              return a->startPoint()->
                                     distance(*segment->startPoint()) <
                                     b->startPoint()->
                                     distance(*segment->startPoint());
                          });

                // Check each segment in the node
                for (const auto& edge : sortedSegments) {
                    if (hasIntersection.loadAcquire()) break;

                    auto edgeBounds = std::make_pair(
                        std::min(edge->startPoint()->getLongitude().value(),
                                 edge->endPoint()->getLongitude().value()),
                        std::max(edge->startPoint()->getLongitude().value(),
                                 edge->endPoint()->getLongitude().value()));

                    // Quick rejection test
                    if (edgeBounds.second < segBounds.first ||
                        edgeBounds.first > segBounds.second) {
                        continue;
                    }

                    if (segment->intersects(*edge, true)) {
                        hasIntersection.storeRelease(1);
                        break;
                    }
                }
            };

        QFuture<void> future = QtConcurrent::map(
            nodeSegmentsToProcess, processSegments);
        future.waitForFinished();

        return !hasIntersection.loadAcquire();

    } else {
        // Sequential processing for smaller datasets
        for (auto* node : intersectingNodes) {
            auto nodeSegments = quadtree->getAllSegmentsInNode(node);
            for (const auto& edge : nodeSegments) {
                auto edgeBounds = std::make_pair(
                    std::min(edge->startPoint()->getLongitude().value(),
                             edge->endPoint()->getLongitude().value()),
                    std::max(edge->startPoint()->getLongitude().value(),
                             edge->endPoint()->getLongitude().value()));

                // Quick rejection test
                if (edgeBounds.second < segBounds.first ||
                    edgeBounds.first > segBounds.second) {
                    continue;
                }

                if (segment->intersects(*edge, true)) {
                    return false;
                }
            }
        }
        return true;
    }
}


std::shared_ptr<Polygon> OptimizedVisibilityGraph::findContainingPolygon(
    const std::shared_ptr<GPoint>& point)
{
    for (const auto& polygon : polygons)
    {
        if (polygon->contains(point))
        {
            return polygon;
        }
    }
    return nullptr; // the case when the point is not in any polygon
}


ShortestPathResult OptimizedVisibilityGraph::findShortestPathDijkstra(
    const std::shared_ptr<GPoint>& start,
    const std::shared_ptr<GPoint>& end)
{

    // Initialize data structures for tracking the shortest path
    std::unordered_map<std::shared_ptr<GPoint>,
                       std::shared_ptr<GPoint>,
                       GPoint::Hash, GPoint::Equal> prev;
    std::unordered_map<std::shared_ptr<GPoint>, double,
                       GPoint::Hash, GPoint::Equal> dist;
    std::set<std::pair<double, std::shared_ptr<GPoint>>> queue;
    std::shared_ptr<Polygon> currentPolygon = nullptr;
    std::shared_ptr<GPoint> newStart;

    // check if the point is whinin the polygon structure.
    // If not, get the nearst point within the polygon structure.
    currentPolygon = findContainingPolygon(start);
    if (currentPolygon == nullptr)
    {
        newStart =
            quadtree->findNearestNeighborPoint(start);
        currentPolygon = findContainingPolygon(newStart);
    }


    // Initialize distances to infinity for all points
    dist[start] = 0.0;
    queue.insert({0.0, start});

    while (!queue.empty())
    {
        // Extract the node with the smallest distance
        std::shared_ptr<GPoint> current = queue.begin()->second;
        queue.erase(queue.begin());

        // If the node is the end node, break from the loop
        if (*(current) == *(end))
        {
            break;
        }

        QVector<std::shared_ptr<GPoint>> visibleNodes;
        if (mBoundaryType == BoundariesType::Water)
        {
            // get the visible points within this polygon
            visibleNodes =
                getVisibleNodesWithinPolygon(current, currentPolygon);
        }
        else
        {
            // get he visible points between the nodes
            // (not within the polygon)
            visibleNodes =
                getVisibleNodesBetweenPolygons(current, polygons);
        }

        for (const auto& neighbor : visibleNodes)
        {
            // Initialize distance for unvisited neighbors
            if (dist.find(neighbor) == dist.end())
            {
                dist[neighbor] =
                    std::numeric_limits<double>::infinity();
            }

            double alt = dist[current] +
                         current->distance(*neighbor).value();

            // Check if a new shorter path is found
            if (alt < dist[neighbor]) {
                queue.erase({dist[neighbor], neighbor});
                dist[neighbor] = alt;
                prev[neighbor] = current;
                queue.insert({dist[neighbor], neighbor});
            }
        }
    }

    // Reconstruct the path from the end node
    return reconstructPath(prev, end);
}

ShortestPathResult OptimizedVisibilityGraph::findShortestPathAStar(
    const std::shared_ptr<GPoint>& start,
    const std::shared_ptr<GPoint>& end) {

    // Data structures for tracking the shortest path
    std::unordered_map<std::shared_ptr<GPoint>,
                       std::shared_ptr<GPoint>,
                       GPoint::Hash, GPoint::Equal> cameFrom;
    std::unordered_map<std::shared_ptr<GPoint>, double> gScore;
    std::unordered_map<std::shared_ptr<GPoint>, double> fScore;
    std::set<std::pair<double, std::shared_ptr<GPoint>>> openSet;
    std::shared_ptr<Polygon> currentPolygon = nullptr;


    // Lambda function for set containment check
    auto setContains =
        [&openSet](const std::shared_ptr<GPoint>& point) -> bool
    {
        return std::any_of(openSet.begin(), openSet.end(),
                           [&point](const std::pair<
                                    double,
                                    std::shared_ptr<GPoint>>& element)
                           {
                               return *(element.second) == *point;
                           });
    };

    // Initialize the start point
    gScore[start] = 0.0;
    fScore[start] = start->distance(*end).value();
    openSet.insert({fScore[start], start});

    while (!openSet.empty()) {
        // Get the node in open set having the lowest fScore[] value
        std::shared_ptr<GPoint> current = openSet.begin()->second;
        openSet.erase(openSet.begin());

        if (*current == *end) {
            return reconstructPath(cameFrom, end);
        }

        QVector<std::shared_ptr<GPoint>> neighbors;
        if (mBoundaryType == BoundariesType::Water) {
            if (!currentPolygon || !currentPolygon->contains(current))
            {
                currentPolygon = findContainingPolygon(current);
            }
            neighbors =
                getVisibleNodesWithinPolygon(current, currentPolygon);

            // Apply wrap-around logic if enabled
            // if (enableWrapAround) {
            //     std::shared_ptr<Point> wrapAroundPoints =
            //         connectWrapAroundPoints(current);
            //     neighbors.append(wrapAroundPoints);
            // }
        }
        else
        {
            neighbors =
                getVisibleNodesBetweenPolygons(current, polygons);
            // Apply wrap-around logic if enabled
            // if (enableWrapAround) {
            //     std::shared_ptr<Point> wrapAroundPoints =
            //         connectWrapAroundPoints(current);
            //     neighbors.append(wrapAroundPoints);
            // }
        }

        for (const auto& neighbor : neighbors) {
            // The distance from start to a neighbor
            double tentative_gScore =
                gScore[current] +
                current->distance(*neighbor).value();

            if (tentative_gScore < gScore[neighbor]) {
                // This path is the best until now. Record it!
                cameFrom[neighbor] = current;
                gScore[neighbor] = tentative_gScore;
                fScore[neighbor] =
                    gScore[neighbor] +
                    neighbor->distance(*end).value();

                if (!setContains(neighbor)) {
                    openSet.insert({fScore[neighbor], neighbor});
                }
            }
        }
    }

    return ShortestPathResult(); // Return empty path if no path is found
}

ShortestPathResult OptimizedVisibilityGraph::reconstructPath(
    const std::unordered_map<std::shared_ptr<GPoint>,
                             std::shared_ptr<GPoint>,
                             GPoint::Hash, GPoint::Equal> &cameFrom,
    std::shared_ptr<GPoint> current)
{

    ShortestPathResult result;  // contains the final values

    // loop over the map to find the shortest path
    // the cameFrom is produced from the shortest path
    // finding algorithm
    while (cameFrom.find(current) != cameFrom.end())
    {
        result.points.prepend(current);
        std::shared_ptr<GPoint> next = cameFrom.at(current);
        std::shared_ptr<GLine> lineSegment =
            quadtree->findLineSegment(next, current);
        if (lineSegment)
        {
            // Line segment found
            result.lines.prepend(lineSegment);
        }
        else
        {
            // Line is not found , create it
            result.lines.prepend(std::make_shared<GLine>(next, current));
        }
        current = next;
    }
    result.points.prepend(current); // Add the start point
    return result;
}

ShortestPathResult OptimizedVisibilityGraph::findShortestPath(
    QVector<std::shared_ptr<GPoint>> mustTraversePoints,
    PathFindingAlgorithm algorithm)
{

    // find the shortest path based on the prefered algorithm

    if (algorithm == PathFindingAlgorithm::AStar)
    {
        return findShortestPathHelper(
            mustTraversePoints,
            std::bind(&OptimizedVisibilityGraph::findShortestPathAStar,
                      this, std::placeholders::_1,
                      std::placeholders::_2));
    }
    else
    {
        return findShortestPathHelper(
            mustTraversePoints,
            std::bind(&OptimizedVisibilityGraph::findShortestPathDijkstra,
                      this, std::placeholders::_1,
                      std::placeholders::_2));
    }

}

ShortestPathResult OptimizedVisibilityGraph::findShortestPathHelper(
    QVector<std::shared_ptr<GPoint>> mustTraversePoints,
    std::function<ShortestPathResult(
        const std::shared_ptr<GPoint>&,
        const std::shared_ptr<GPoint>&)> pathfindingStrategy)
{
    ShortestPathResult result;

    // Check if we have enough points to form a path
    if (mustTraversePoints.size() < 2)
    {
        // If there's only one point or none, return empty
        // result or result with only one point
        if (!mustTraversePoints.isEmpty())
        {
            result.points.append(mustTraversePoints.first());
        }
        return result;
    }

    // We start with the first point
    result.points.append(mustTraversePoints.first());

    for (qsizetype i = 0; i < mustTraversePoints.size() - 1; ++i)
    {
        auto startPoint = mustTraversePoints[i];
        auto endPoint = mustTraversePoints[i + 1];

        // get the max speed of the polygon
        // units::velocity::meters_per_second_t maxAllowedSpeed =
        //     units::velocity::meters_per_second_t(100.0);

        // // get the polygon containing the point
        // for (auto& poly: polygons)
        // {
        //     if (poly->pointIsInPolygon(*startPoint) ||
        //         poly->PointIsPolygonStructure(*startPoint))
        //     {
        //         maxAllowedSpeed = poly->getMaxAllowedSpeed();
        //         break;
        //     }
        // }

        // check if the user provided a line of sight
        std::shared_ptr<GLine> l =
            std::make_shared<GLine>(startPoint, endPoint);

        if (isSegmentVisible(l))
        {
            // Check if startPoint is not already in result.points
            if (std::find(result.points.begin(),
                          result.points.end(), startPoint) ==
                result.points.end())
            {
                result.points.push_back(startPoint);
            }

            // Check if endPoint is not already in result.points
            if (std::find(result.points.begin(),
                          result.points.end(), endPoint) ==
                result.points.end())
            {
                result.points.push_back(endPoint);
            }

            // Check if l is not already in result.lines
            if (std::find(result.lines.begin(),
                          result.lines.end(), l) ==
                result.lines.end())
            {
                result.lines.push_back(l);
            }
        }
        else
        {
            // Compute the shortest path between startPoint and endPoint
            ShortestPathResult twoPointResult =
                pathfindingStrategy(startPoint, endPoint);

            // We've already added the startPoint to the path,
            // so we start with the second point
            for (qsizetype j = 1; j < twoPointResult.points.size(); ++j)
            {
                result.points.append(twoPointResult.points[j]);
            }

            // Append the lines connecting these points to the result's lines
            // Since lines are connecting the points, the number of lines
            // should always be one less than the number of points
            for (auto& line : twoPointResult.lines)
            {
                result.lines.append(line);
            }
        }
    }

    return result;
}

// QVector<std::shared_ptr<GPoint>> OptimizedVisibilityGraph::
//     connectWrapAroundPoints(
//         const std::shared_ptr<GPoint>& point,
//         const std::shared_ptr<Polygon>& polygon)
// {
//     QVector<std::shared_ptr<GPoint>> wrapAroundPoints;

//     // Determine the corresponding points on the opposite boundary
//     // Example: If the point is near the right edge,
//     // find points near the left edge and vice versa.

//     if (!quadtree->isNearBoundary(point))
//     {
//         return wrapAroundPoints;
//     }
//     // Here, wrap-around happens only on the x-axis
//     double newX = (point->x() < quadtree->getMapMinPoint().x()) ?
//                       quadtree->getMapMaxPoint().x().value() :
//                       quadtree->getMapMinPoint().x().value();

//     std::shared_ptr<GPoint> correspondingPoint =
//         std::make_shared<GPoint>(
//             units::length::meter_t(newX), point->y());

//     QVector<std::shared_ptr<GPoint>> allPointsOnOppositeBoundary;
//     if (mBoundaryType == BoundariesType::Water)
//     {
//         allPointsOnOppositeBoundary =
//             getVisibleNodesWithinPolygon(correspondingPoint, polygon);
//     }
//     else
//     {
//         allPointsOnOppositeBoundary =
//             getVisibleNodesBetweenPolygons(correspondingPoint, polygons);
//     }


//     // For each potential corresponding point, check if it's visible
//     // and add it to the wrapAroundPoints vector.
//     for (const auto& potentialPoint : allPointsOnOppositeBoundary)
//     {
//         if (isVisible(correspondingPoint, potentialPoint)) {
//             wrapAroundPoints.push_back(potentialPoint);
//         }
//     }

//     return wrapAroundPoints;
// }


void OptimizedVisibilityGraph::clear()
{
    quadtree->clearTree();
    polygons.clear();
    visibilityCache.clear();
    // edges.clear();
    // nodes.clear();
}


};
