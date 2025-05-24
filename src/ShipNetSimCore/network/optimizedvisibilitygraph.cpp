#include "optimizedvisibilitygraph.h"
#include <QQueue>
#include <QSet>
#include <functional>
#include <set>
#include <unordered_map>

namespace ShipNetSimCore
{

bool ShortestPathResult::isValid()
{
    return (points.size() >= 2 && lines.size() >= 1
            && lines.size() == (points.size() - 1));
}
OptimizedVisibilityGraph::OptimizedVisibilityGraph() {}

OptimizedVisibilityGraph::OptimizedVisibilityGraph(
    const QVector<std::shared_ptr<Polygon>> usedPolygons,
    BoundariesType                          boundaryType)
    : mBoundaryType(boundaryType)
    , polygons(usedPolygons)
{
    // Validate polygon data
    if (polygons.isEmpty())
    {
        qWarning() << "Warning: Empty polygon list provided to "
                      "OptimizedVisibilityGraph";
    }

    // Validate each polygon
    for (const auto &polygon : polygons)
    {
        if (!polygon)
        {
            qWarning()
                << "Warning: Null polygon found in construction";
            continue;
        }
    }

    // Initialize Quadtree with polygons
    try
    {
        quadtree         = std::make_unique<Quadtree>(polygons);
        enableWrapAround = true;
    }
    catch (const std::exception &e)
    {
        qCritical() << "Exception in Quadtree construction:"
                    << e.what();
        // Create empty quadtree to maintain consistent state
        quadtree = std::make_unique<Quadtree>(
            QVector<std::shared_ptr<Polygon>>());
    }
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
    QVector<std::shared_ptr<SeaPort>> &seaPorts)
{
    for (auto &seaPort : seaPorts)
    {
        std::shared_ptr<GPoint> portCoord =
            std::make_shared<GPoint>(seaPort->getPortCoordinate());
        seaPort->setClosestPointOnWaterPolygon(
            quadtree->findNearestNeighborPoint(portCoord));
    }
}

void OptimizedVisibilityGraph::setPolygons(
    const QVector<std::shared_ptr<Polygon>> &newPolygons)
{
    QWriteLocker locker(&quadtreeLock); // Acquire write lock
    visibilityCache.clear();
    quadtree->clearTree();
    polygons = newPolygons;
}

QVector<std::shared_ptr<GPoint>>
OptimizedVisibilityGraph::getVisibleNodesBetweenPolygons(
    const std::shared_ptr<GPoint>           &node,
    const QVector<std::shared_ptr<Polygon>> &allPolygons)
{

    QReadLocker readLocker(&cacheLock);
    if (visibilityCache.contains(node))
        return visibilityCache[node];
    readLocker.unlock();

    // Prepare tasks
    QVector<std::shared_ptr<GPoint>> tasks;

    // Prepare candidate points using efficient iteration
    for (const auto &polygon : allPolygons)
    {
        if (polygon->contains(node))
            continue;

        // Add outer boundary points
        const auto outerPoints = polygon->outer();
        std::copy_if(outerPoints.begin(), outerPoints.end(),
                     std::back_inserter(tasks),
                     [&node](const auto &p) { return *p != *node; });

        // Add hole points
        for (const auto &hole : polygon->inners())
        {
            std::copy_if(
                hole.begin(), hole.end(), std::back_inserter(tasks),
                [&node](const auto &p) { return *p != *node; });
        }
    }

    // Parallel filter using QtConcurrent
    QFuture<std::shared_ptr<GPoint>> future = QtConcurrent::filtered(
        tasks, [this, node](const std::shared_ptr<GPoint> &point) {
            return isVisible(node, point);
        });

    // Get results and convert to QVector
    QVector<std::shared_ptr<GPoint>> visibleNodes =
        future.results().toVector();

    // Add manual connections
    if (manualConnections.contains(node))
    {
        visibleNodes += manualConnections.value(node);
    }

    QWriteLocker writeLocker(&cacheLock);
    if (!visibilityCache.contains(node))
    { // Check again after acquiring write lock
        visibilityCache.insert(node, visibleNodes);
    }

    return visibleNodes;
}

// with threads
QVector<std::shared_ptr<GPoint>>
OptimizedVisibilityGraph::getVisibleNodesWithinPolygon(
    const std::shared_ptr<GPoint>  &node,
    const std::shared_ptr<Polygon> &polygon)
{
    // 1. check if the visibility for the node is already cached
    {
        QReadLocker locker(&cacheLock);
        if (visibilityCache.contains(node))
        {
            // Return the cached result
            return visibilityCache[node];
        }
    } // locker goes out of scope here, releasing the read lock >>>
      // important

    // 2. Prepare points list (exclude node itself)
    QVector<std::shared_ptr<GPoint>> candidates;
    const auto                      &outerPoints = polygon->outer();
    std::copy_if(outerPoints.begin(), outerPoints.end(),
                 std::back_inserter(candidates),
                 [&node](const auto &p) { return *p != *node; });

    // 3. Parallel visibility check using filtered
    QFuture<std::shared_ptr<GPoint>> future = QtConcurrent::filtered(
        candidates,
        [this, node](const std::shared_ptr<GPoint> &point) {
            return isVisible(node, point);
        });

    // 4. Wait for completion and get results
    QVector<std::shared_ptr<GPoint>> visibleNodes =
        future.results().toVector();

    // 5. Double-checked locking pattern for cache update
    {
        QWriteLocker writeLocker(&cacheLock);
        if (!visibilityCache.contains(node))
        {
            visibilityCache.insert(node, visibleNodes);
        }
    }

    return visibilityCache.value(node);
}

void OptimizedVisibilityGraph::addManualVisibleLine(
    const std::shared_ptr<GLine> &line)
{
    QWriteLocker locker(&cacheLock);

    // Add line to manualLines
    manualLines.append(line);

    // Track connections for both directions
    manualConnections[line->startPoint()].append(line->endPoint());
    manualConnections[line->endPoint()].append(line->startPoint());

    // Add points to manualPoints if new
    if (!manualPoints.contains(line->startPoint()))
    {
        manualPoints.append(line->startPoint());
    }
    if (!manualPoints.contains(line->endPoint()))
    {
        manualPoints.append(line->endPoint());
    }

    visibilityCache.clear(); // Invalidate cache
}

void OptimizedVisibilityGraph::clearManualLines()
{
    QWriteLocker locker(&cacheLock);
    manualLines.clear();
    manualConnections.clear();
    manualPoints.clear();
    visibilityCache.clear();
}

bool OptimizedVisibilityGraph::isVisible(
    const std::shared_ptr<GPoint> &node1,
    const std::shared_ptr<GPoint> &node2) const
{
    if (!node1 || !node2)
    {
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
    const std::shared_ptr<GLine> &segment) const
{
    // Check if the segment is in manualLines (in any direction)
    for (const auto &manualLine : manualLines)
    {
        if (*manualLine == *segment
            || *manualLine == segment->reverse())
        {
            return true;
        }
    }

    // Handle wrap-around segments
    if (Quadtree::isSegmentCrossingAntimeridian(segment))
    {
        auto splitSegments =
            Quadtree::splitSegmentAtAntimeridian(segment);
        for (const auto &seg : splitSegments)
        {
            if (!isSegmentVisible(seg))
                return false;
        }
        return true;
    }

    QReadLocker locker(&quadtreeLock);

    // 1. Quick distance check
    if (segment->startPoint()->distance(*segment->endPoint())
        < units::length::meter_t(1.0))
    {
        return true;
    }

    // 2. Get intersecting nodes
    auto intersectingNodes =
        quadtree->findNodesIntersectingLineSegmentParallel(segment);

    // 3. Get segment bounds manually
    auto   segmentStart = segment->startPoint();
    auto   segmentEnd   = segment->endPoint();
    double segMinLon = std::min(segmentStart->getLongitude().value(),
                                segmentEnd->getLongitude().value());
    double segMaxLon = std::max(segmentStart->getLongitude().value(),
                                segmentEnd->getLongitude().value());
    double segMinLat = std::min(segmentStart->getLatitude().value(),
                                segmentEnd->getLatitude().value());
    double segMaxLat = std::max(segmentStart->getLatitude().value(),
                                segmentEnd->getLatitude().value());

    // 4. Check edges in intersecting nodes
    auto checkEdge = [&](const std::shared_ptr<GLine> &edge) {
        // Skip adjacent edges
        if ((*(edge->startPoint()) == *segmentStart)
            || (*(edge->startPoint()) == *segmentEnd)
            || (*(edge->endPoint()) == *segmentStart)
            || (*(edge->endPoint()) == *segmentEnd))
        {
            return false;
        }

        // Manual bounds check
        auto   edgeStart = edge->startPoint();
        auto   edgeEnd   = edge->endPoint();
        double edgeMinLon =
            std::min(edgeStart->getLongitude().value(),
                     edgeEnd->getLongitude().value());
        double edgeMaxLon =
            std::max(edgeStart->getLongitude().value(),
                     edgeEnd->getLongitude().value());
        double edgeMinLat = std::min(edgeStart->getLatitude().value(),
                                     edgeEnd->getLatitude().value());
        double edgeMaxLat = std::max(edgeStart->getLatitude().value(),
                                     edgeEnd->getLatitude().value());

        // Bounding box rejection
        if (edgeMaxLon < segMinLon || edgeMinLon > segMaxLon
            || edgeMaxLat < segMinLat || edgeMinLat > segMaxLat)
        {
            return false;
        }

        // Use existing intersects method with edge point checking
        return segment->intersects(*edge, true);
    };

    // 5. Parallel processing for large datasets
    if (intersectingNodes.size() > 1000)
    {
        QAtomicInt hasIntersection(0);

        auto processNode = [&](Quadtree::Node *node) {
            if (hasIntersection.loadAcquire())
                return;

            for (const auto &edge :
                 quadtree->getAllSegmentsInNode(node))
            {
                if (checkEdge(edge))
                {
                    hasIntersection.storeRelease(1);
                    break;
                }
            }
        };

        QtConcurrent::blockingMap(intersectingNodes, processNode);
        return !hasIntersection.loadAcquire();
    }

    // 6. Sequential processing for small datasets
    for (auto *node : intersectingNodes)
    {
        for (const auto &edge : quadtree->getAllSegmentsInNode(node))
        {
            if (checkEdge(edge))
                return false;
        }
    }

    return true;
}

std::shared_ptr<Polygon>
OptimizedVisibilityGraph::findContainingPolygon(
    const std::shared_ptr<GPoint> &point)
{
    for (const auto &polygon : polygons)
    {
        if (polygon->contains(point))
        {
            return polygon;
        }
    }
    return nullptr; // the case when the point is not in any polygon
}

ShortestPathResult OptimizedVisibilityGraph::findShortestPathDijkstra(
    const std::shared_ptr<GPoint> &start,
    const std::shared_ptr<GPoint> &end)
{
    // Validate input points
    if (!start || !end)
    {
        qWarning() << "Invalid start or end point provided to "
                      "Dijkstra's algorithm";
        return ShortestPathResult();
    }

    // Initialize data structures for tracking the shortest path
    std::unordered_map<std::shared_ptr<GPoint>,
                       std::shared_ptr<GPoint>, GPoint::Hash,
                       GPoint::Equal>
        prev;
    std::unordered_map<std::shared_ptr<GPoint>, double, GPoint::Hash,
                       GPoint::Equal>
                                                         dist;
    std::set<std::pair<double, std::shared_ptr<GPoint>>> queue;

    // Find valid start point
    std::shared_ptr<GPoint>  startPoint = start;
    std::shared_ptr<Polygon> currentPolygon =
        findContainingPolygon(startPoint);
    if (currentPolygon == nullptr)
    {
        startPoint     = quadtree->findNearestNeighborPoint(start);
        currentPolygon = findContainingPolygon(startPoint);
        if (!currentPolygon)
        {
            qWarning() << "Start point is not within any polygon and "
                          "has no valid nearest point.";
            return ShortestPathResult();
        }
    }

    // Lambda for finding and removing point from queue
    auto removeFromQueue = [&queue](
                               const std::shared_ptr<GPoint> &point) {
        auto it = std::find_if(
            queue.begin(), queue.end(),
            [&point](const std::pair<double, std::shared_ptr<GPoint>>
                         &element) {
                return *(element.second) == *point;
            });
        if (it != queue.end())
        {
            queue.erase(it);
        }
    };

    // Initialize distances with manual points and infinity
    for (const auto &point : manualPoints)
    {
        dist[point] = std::numeric_limits<double>::infinity();
    }

    // Initialize the start point
    dist[startPoint] = 0.0;
    queue.insert({0.0, startPoint});

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
        {
            QReadLocker locker(
                &quadtreeLock); // Lock for quadtree operations

            if (mBoundaryType == BoundariesType::Water)
            {
                // Get the visible points within this polygon
                if (!currentPolygon
                    || !currentPolygon->contains(current))
                {
                    currentPolygon = findContainingPolygon(current);
                    if (!currentPolygon)
                    {
                        continue; // Skip this point if not in any
                                  // polygon
                    }
                }
                visibleNodes = getVisibleNodesWithinPolygon(
                    current, currentPolygon);
            }
            else
            {
                // Get the visible points between the nodes (not
                // within the polygon)
                visibleNodes =
                    getVisibleNodesBetweenPolygons(current, polygons);
            }

            if (enableWrapAround)
            {
                auto wrapPoints = connectWrapAroundPoints(current);
                visibleNodes.append(wrapPoints);
            }
        } // Release lock after getting visible nodes

        for (const auto &neighbor : visibleNodes)
        {
            // Initialize distance for unvisited neighbors
            if (dist.find(neighbor) == dist.end())
            {
                dist[neighbor] =
                    std::numeric_limits<double>::infinity();
            }

            double alt =
                dist[current] + current->distance(*neighbor).value();

            // Check if a new shorter path is found
            if (alt < dist[neighbor])
            {
                // Safely update queue (remove then insert)
                removeFromQueue(neighbor);
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
    const std::shared_ptr<GPoint> &start,
    const std::shared_ptr<GPoint> &end)
{
    // Validate input points
    if (!start || !end)
    {
        qWarning()
            << "Invalid start or end point provided to A* algorithm";
        return ShortestPathResult();
    }

    // Data structures for tracking the shortest path
    std::unordered_map<std::shared_ptr<GPoint>,
                       std::shared_ptr<GPoint>, GPoint::Hash,
                       GPoint::Equal>
                                                         cameFrom;
    std::unordered_map<std::shared_ptr<GPoint>, double>  gScore;
    std::unordered_map<std::shared_ptr<GPoint>, double>  fScore;
    std::set<std::pair<double, std::shared_ptr<GPoint>>> openSet;

    // Find valid start point
    std::shared_ptr<GPoint>  startPoint = start;
    std::shared_ptr<Polygon> startPolygon =
        findContainingPolygon(startPoint);
    if (startPolygon == nullptr)
    {
        startPoint   = quadtree->findNearestNeighborPoint(start);
        startPolygon = findContainingPolygon(startPoint);
        if (!startPolygon)
        {
            qWarning() << "Start point is not within any polygon and "
                          "has no valid nearest point.";
            return ShortestPathResult();
        }
    }

    // Find valid end point
    std::shared_ptr<GPoint>  endPoint = end;
    std::shared_ptr<Polygon> endPolygon =
        findContainingPolygon(endPoint);
    if (endPolygon == nullptr)
    {
        endPoint   = quadtree->findNearestNeighborPoint(end);
        endPolygon = findContainingPolygon(endPoint);
        if (!endPolygon)
        {
            qWarning() << "End point is not within any polygon and "
                          "has no valid nearest point.";
            return ShortestPathResult();
        }
    }

    // Lambda for finding and removing point from openSet
    auto removeFromOpenSet =
        [&openSet](const std::shared_ptr<GPoint> &point) {
            auto it = std::find_if(
                openSet.begin(), openSet.end(),
                [&point](
                    const std::pair<double, std::shared_ptr<GPoint>>
                        &element) {
                    return *(element.second) == *point;
                });
            if (it != openSet.end())
            {
                openSet.erase(it);
            }
        };

    // Initialize all points with infinity
    for (const auto &point : manualPoints)
    {
        gScore[point] = std::numeric_limits<double>::infinity();
        fScore[point] = std::numeric_limits<double>::infinity();
    }

    // Initialize the start point
    gScore[startPoint] = 0.0;
    fScore[startPoint] = startPoint->distance(*endPoint).value();
    openSet.insert({fScore[startPoint], startPoint});

    std::shared_ptr<Polygon> currentPolygon = startPolygon;

    while (!openSet.empty())
    {
        // Get the node in open set having the lowest fScore[] value
        std::shared_ptr<GPoint> current = openSet.begin()->second;
        openSet.erase(openSet.begin());

        if (*current == *endPoint)
        {
            return reconstructPath(cameFrom, endPoint);
        }

        QVector<std::shared_ptr<GPoint>> neighbors;
        {
            QReadLocker locker(
                &quadtreeLock); // Lock for quadtree operations

            if (mBoundaryType == BoundariesType::Water)
            {
                if (!currentPolygon
                    || !currentPolygon->contains(current))
                {
                    currentPolygon = findContainingPolygon(current);
                    if (!currentPolygon)
                    {
                        continue; // Skip this iteration if no polygon
                                  // contains current
                    }
                }
                neighbors = getVisibleNodesWithinPolygon(
                    current, currentPolygon);
            }
            else
            {
                neighbors =
                    getVisibleNodesBetweenPolygons(current, polygons);

                // NEW: Filter manual points for visibility
                for (const auto &manualPoint : manualPoints)
                {
                    if (isVisible(current, manualPoint))
                    {
                        neighbors.append(manualPoint);
                    }
                }
            }

            // Apply wrap-around logic if enabled
            if (enableWrapAround)
            {
                neighbors.append(connectWrapAroundPoints(current));
            }
        } // Release lock after getting neighbors

        for (const auto &neighbor : neighbors)
        {
            // Initialize if not present
            if (gScore.find(neighbor) == gScore.end())
            {
                gScore[neighbor] =
                    std::numeric_limits<double>::infinity();
                fScore[neighbor] =
                    std::numeric_limits<double>::infinity();
            }

            // The distance from start to a neighbor
            double tentative_gScore =
                gScore[current]
                + current->distance(*neighbor).value();

            if (tentative_gScore < gScore[neighbor])
            {
                // This path is the best until now. Record it!
                cameFrom[neighbor] = current;
                gScore[neighbor]   = tentative_gScore;
                fScore[neighbor] =
                    gScore[neighbor]
                    + neighbor->distance(*endPoint).value();

                // Safely update openSet (remove then insert)
                removeFromOpenSet(neighbor);
                openSet.insert({fScore[neighbor], neighbor});
            }
        }
    }

    return ShortestPathResult(); // Return empty path if no path is
                                 // found
}

ShortestPathResult OptimizedVisibilityGraph::reconstructPath(
    const std::unordered_map<std::shared_ptr<GPoint>,
                             std::shared_ptr<GPoint>, GPoint::Hash,
                             GPoint::Equal> &cameFrom,
    std::shared_ptr<GPoint>                  current)
{

    ShortestPathResult result; // contains the final values

    // loop over the map to find the shortest path
    // the cameFrom is produced from the shortest path
    // finding algorithm
    while (cameFrom.find(current) != cameFrom.end())
    {
        result.points.prepend(current);
        std::shared_ptr<GPoint> next = cameFrom.at(current);
        std::shared_ptr<GLine>  lineSegment =
            quadtree->findLineSegment(next, current);
        if (lineSegment)
        {
            // Line segment found
            result.lines.prepend(lineSegment);
        }
        else
        {
            // Line is not found , create it
            result.lines.prepend(
                std::make_shared<GLine>(next, current));
        }
        current = next;
    }
    result.points.prepend(current); // Add the start point
    return result;
}

ShortestPathResult OptimizedVisibilityGraph::findShortestPath(
    QVector<std::shared_ptr<GPoint>> mustTraversePoints,
    PathFindingAlgorithm             algorithm)
{

    // find the shortest path based on the prefered algorithm

    if (algorithm == PathFindingAlgorithm::AStar)
    {
        return findShortestPathHelper(
            mustTraversePoints,
            std::bind(
                &OptimizedVisibilityGraph::findShortestPathAStar,
                this, std::placeholders::_1, std::placeholders::_2));
    }
    else
    {
        return findShortestPathHelper(
            mustTraversePoints,
            std::bind(
                &OptimizedVisibilityGraph::findShortestPathDijkstra,
                this, std::placeholders::_1, std::placeholders::_2));
    }
}

ShortestPathResult OptimizedVisibilityGraph::findShortestPathHelper(
    QVector<std::shared_ptr<GPoint>> mustTraversePoints,
    std::function<ShortestPathResult(const std::shared_ptr<GPoint> &,
                                     const std::shared_ptr<GPoint> &)>
        pathfindingStrategy)
{
    ShortestPathResult result;

    // Check for sufficient points and validate all points
    if (mustTraversePoints.size() < 2)
    {
        if (!mustTraversePoints.isEmpty()
            && mustTraversePoints.first())
        {
            result.points.append(mustTraversePoints.first());
        }
        return result;
    }

    // Validate all input points
    for (const auto &point : mustTraversePoints)
    {
        if (!point)
        {
            qWarning() << "Null point in path points list";
            return result; // Return empty result
        }
    }

    // Initial point
    result.points.append(mustTraversePoints.first());

    for (qsizetype i = 0; i < mustTraversePoints.size() - 1; ++i)
    {
        auto startPoint = mustTraversePoints[i];
        auto endPoint   = mustTraversePoints[i + 1];

        try
        {
            // Create line segment
            std::shared_ptr<GLine> l =
                std::make_shared<GLine>(startPoint, endPoint);

            if (true) // isSegmentVisible(l))   // TODO: Fix the
                      // isVisible function
            {
                // Check if startPoint is not already in result.points
                if (std::find(result.points.begin(),
                              result.points.end(), startPoint)
                    == result.points.end())
                {
                    result.points.push_back(startPoint);
                }

                // Check if endPoint is not already in result.points
                if (std::find(result.points.begin(),
                              result.points.end(), endPoint)
                    == result.points.end())
                {
                    result.points.push_back(endPoint);
                }

                // Check if l is not already in result.lines
                if (std::find(result.lines.begin(),
                              result.lines.end(), l)
                    == result.lines.end())
                {
                    result.lines.push_back(l);
                }
            }
            else
            {
                // Compute the shortest path between startPoint and
                // endPoint
                ShortestPathResult twoPointResult =
                    pathfindingStrategy(startPoint, endPoint);

                // We've already added the startPoint to the path,
                // so we start with the second point
                for (qsizetype j = 1;
                     j < twoPointResult.points.size(); ++j)
                {
                    result.points.append(twoPointResult.points[j]);
                }

                // Append the lines connecting these points to the
                // result's lines Since lines are connecting the
                // points, the number of lines should always be one
                // less than the number of points
                for (auto &line : twoPointResult.lines)
                {
                    result.lines.append(line);
                }
            }
        }
        catch (const std::exception &e)
        {
            qWarning() << "Exception creating line segment:"
                       << e.what();
            continue; // Skip this segment
        }
    }

    return result;
}

QVector<std::shared_ptr<GPoint>>
OptimizedVisibilityGraph::connectWrapAroundPoints(
    const std::shared_ptr<GPoint> &point)
{
    QVector<std::shared_ptr<GPoint>> wrapAroundPoints;
    if (!quadtree->isNearBoundary(point))
        return wrapAroundPoints;

    // Get map boundaries
    const auto   mapMin = quadtree->getMapMinPoint();
    const auto   mapMax = quadtree->getMapMaxPoint();
    const double mapWidth =
        (mapMax.getLongitude() - mapMin.getLongitude()).value();

    // Create mirrored point
    auto createMirrorPoint = [&](double offset) {
        double newLon = point->getLongitude().value() + offset;
        return std::make_shared<GPoint>(
            units::angle::degree_t(newLon), point->getLatitude());
    };

    // Check left boundary
    if ((mapMax.getLongitude() - point->getLongitude()).value() < 1.0)
    {
        wrapAroundPoints.append(createMirrorPoint(-mapWidth));
    }
    // Check right boundary
    else if ((point->getLongitude() - mapMin.getLongitude()).value()
             < 1.0)
    {
        wrapAroundPoints.append(createMirrorPoint(mapWidth));
    }

    // Find visible nodes for mirrored points
    QVector<std::shared_ptr<GPoint>> allVisible;
    for (const auto &wrappedPoint : wrapAroundPoints)
    {
        // Get visible nodes for wrapped point
        QVector<std::shared_ptr<GPoint>> wrappedVisible;
        if (mBoundaryType == BoundariesType::Water)
        {
            if (auto poly = findContainingPolygon(point))
            {
                wrappedVisible =
                    getVisibleNodesWithinPolygon(wrappedPoint, poly);
            }
        }
        else
        {
            wrappedVisible = getVisibleNodesBetweenPolygons(
                wrappedPoint, polygons);
        }

        // Convert back to original coordinate system
        for (auto &p : wrappedVisible)
        {
            double adjustedLon = p->getLongitude().value();
            if (adjustedLon > 180)
                adjustedLon -= 360;
            else if (adjustedLon < -180)
                adjustedLon += 360;

            allVisible.append(std::make_shared<GPoint>(
                units::angle::degree_t(adjustedLon),
                p->getLatitude()));
        }
    }

    // Filter valid visible points considering wrap-around
    QVector<std::shared_ptr<GPoint>> validVisible;
    for (const auto &candidate : allVisible)
    {
        auto wrapSegment = std::make_shared<GLine>(point, candidate);
        if (isSegmentVisible(wrapSegment))
        {
            validVisible.append(candidate);
        }
    }

    return validVisible;
}

void OptimizedVisibilityGraph::clear()
{
    quadtree->clearTree();
    polygons.clear();
    visibilityCache.clear();
    // edges.clear();
    // nodes.clear();
}

}; // namespace ShipNetSimCore
