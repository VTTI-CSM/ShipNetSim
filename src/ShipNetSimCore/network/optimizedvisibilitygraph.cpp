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
        if (polygon->ringsContain(node))
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

    // 2. Water polygon validation
    if (mBoundaryType == BoundariesType::Water)
    {
        // Check if segment is valid within any containing polygon
        auto startPolygon =
            findContainingPolygon(segment->startPoint());
        auto endPolygon = findContainingPolygon(segment->endPoint());

        // If both points are in the same water polygon, validate the
        // segment
        if (startPolygon && endPolygon && startPolygon == endPolygon)
        {
            if (!startPolygon->isValidWaterSegment(segment))
            {
                return false;
            }
        }
        // If points are in different polygons or one is outside,
        // check if segment crosses any polygon holes
        else
        {
            for (const auto &polygon : polygons)
            {
                if (polygon->segmentCrossesHoles(segment))
                {
                    return false;
                }
            }
        }
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
        // ONLY skip edges that share BOTH endpoints (i.e., the exact
        // same line)
        bool sharesBothEndpoints =
            ((*(edge->startPoint()) == *segmentStart)
             && (*(edge->endPoint()) == *segmentEnd))
            || ((*(edge->startPoint()) == *segmentEnd)
                && (*(edge->endPoint()) == *segmentStart));

        if (sharesBothEndpoints)
        {
            return false; // Skip identical edge
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
    const std::shared_ptr<GPoint> &point) const
{
    for (const auto &polygon : polygons)
    {
        if (polygon->ringsContain(point))
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
    if (!start || !end)
    {
        qWarning() << "Invalid start or end point provided to "
                      "Dijkstra's algorithm";
        return ShortestPathResult();
    }

    if (*start == *end)
    {
        ShortestPathResult result;
        result.points.append(start);
        return result;
    }

    // For ship navigation, always find closest navigable points on
    // polygon vertices
    std::shared_ptr<GPoint> startNavPoint;
    std::shared_ptr<GPoint> endNavPoint;

    // Ships navigate between polygon vertices (ports, waypoints
    // on coastlines)
    startNavPoint = quadtree->findNearestNeighborPoint(start);
    endNavPoint   = quadtree->findNearestNeighborPoint(end);

    if (!startNavPoint || !endNavPoint)
    {
        qWarning() << "Could not find navigable points for ship path";
        return ShortestPathResult();
    }

    qDebug() << "Ship navigation:";
    qDebug() << "  Start position:" << start->toString();
    qDebug() << "  Nearest nav point:" << startNavPoint->toString();
    qDebug() << "  End position:" << end->toString();
    qDebug() << "  Nearest nav point:" << endNavPoint->toString();

    // If start/end are already very close to nav points, use them
    // directly
    const double PROXIMITY_THRESHOLD = 100.0; // meters
    if (start->distance(*startNavPoint).value() < PROXIMITY_THRESHOLD)
    {
        startNavPoint = start;
    }
    if (end->distance(*endNavPoint).value() < PROXIMITY_THRESHOLD)
    {
        endNavPoint = end;
    }

    // Initialize data structures
    std::unordered_map<std::shared_ptr<GPoint>,
                       std::shared_ptr<GPoint>, GPoint::Hash,
                       GPoint::Equal>
        prev;
    std::unordered_map<std::shared_ptr<GPoint>, double, GPoint::Hash,
                       GPoint::Equal>
                                                         dist;
    std::set<std::pair<double, std::shared_ptr<GPoint>>> queue;

    // Lambda for safe distance initialization
    auto initializeDistance =
        [&](const std::shared_ptr<GPoint> &point) {
            if (dist.find(point) == dist.end())
            {
                dist[point] = std::numeric_limits<double>::infinity();
            }
        };

    // Lambda for removing point from queue
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

    // Initialize start point
    initializeDistance(startNavPoint);
    dist[startNavPoint] = 0.0;
    queue.insert({0.0, startNavPoint});

    // Add end point to ensure it's tracked
    initializeDistance(endNavPoint);

    // CRITICAL: Add manual connections for start/end if they're not
    // polygon vertices
    if (startNavPoint != start)
    {
        initializeDistance(start);
    }
    if (endNavPoint != end)
    {
        initializeDistance(end);
    }

    while (!queue.empty())
    {
        auto currentPair = *queue.begin();
        queue.erase(queue.begin());

        std::shared_ptr<GPoint> current     = currentPair.second;
        double                  currentDist = currentPair.first;

        // Skip if we've already found a better path
        if (currentDist > dist[current])
        {
            continue;
        }

        // Check if we reached the navigation end point
        if (*current == *endNavPoint)
        {
            qDebug() << "Dijkstra: Reached navigation end point";

            // If end nav point is different from actual end, add
            // final connection
            if (*endNavPoint != *end)
            {
                initializeDistance(end);
                double finalDist =
                    dist[endNavPoint]
                    + endNavPoint->distance(*end).value();
                if (finalDist < dist[end])
                {
                    dist[end] = finalDist;
                    prev[end] = endNavPoint;
                }
            }
            break;
        }

        // Get visible neighbors
        QVector<std::shared_ptr<GPoint>> visibleNodes;
        {
            QReadLocker locker(&quadtreeLock);

            if (mBoundaryType == BoundariesType::Water)
            {
                // For water navigation, find containing polygon
                auto currentPolygon = findContainingPolygon(current);
                if (!currentPolygon)
                {
                    // If current point is not in a polygon, it might
                    // be a vertex Find all polygons and get visible
                    // nodes between them
                    visibleNodes = getVisibleNodesBetweenPolygons(
                        current, polygons);
                }
                else
                {
                    visibleNodes = getVisibleNodesWithinPolygon(
                        current, currentPolygon);
                }

                // CRITICAL: Ensure end navigation point is considered
                // if visible
                if (*current != *endNavPoint
                    && isVisible(current, endNavPoint))
                {
                    if (!visibleNodes.contains(endNavPoint))
                    {
                        visibleNodes.append(endNavPoint);
                    }
                }

                // If this is the start nav point and different from
                // actual start, connect to actual start
                if (*current == *startNavPoint
                    && *startNavPoint != *start)
                {
                    if (isVisible(current, start))
                    {
                        visibleNodes.append(start);
                    }
                }
            }
            else
            {
                visibleNodes =
                    getVisibleNodesBetweenPolygons(current, polygons);
            }

            // Add wrap-around connections if enabled
            if (enableWrapAround)
            {
                auto wrapPoints = connectWrapAroundPoints(current);
                visibleNodes.append(wrapPoints);
            }
        }

        qDebug() << "Dijkstra: Point" << current->toString() << "has"
                 << visibleNodes.size() << "visible neighbors";

        // Process all neighbors
        for (const auto &neighbor : visibleNodes)
        {
            initializeDistance(neighbor);

            double edgeWeight = current->distance(*neighbor).value();
            double alt        = dist[current] + edgeWeight;

            if (alt < dist[neighbor])
            {
                removeFromQueue(neighbor);
                dist[neighbor] = alt;
                prev[neighbor] = current;
                queue.insert({dist[neighbor], neighbor});
            }
        }
    }

    // Determine which end point to use for reconstruction
    std::shared_ptr<GPoint> pathEndPoint = endNavPoint;
    if (*endNavPoint != *end && prev.find(end) != prev.end())
    {
        pathEndPoint = end; // Use actual end if we found a path to it
    }

    // Check if we found a path
    if (prev.find(pathEndPoint) == prev.end()
        && *startNavPoint != *pathEndPoint)
    {
        qDebug() << "Dijkstra: No path found from"
                 << startNavPoint->toString() << "to"
                 << pathEndPoint->toString();
        return ShortestPathResult();
    }

    qDebug() << "Dijkstra: Reconstructing path to"
             << pathEndPoint->toString();
    auto result = reconstructPath(prev, pathEndPoint);

    // If we used navigation points, prepend/append the actual
    // start/end
    if (*startNavPoint != *start && !result.points.isEmpty())
    {
        result.points.prepend(start);
        if (!result.lines.isEmpty())
        {
            result.lines.prepend(
                std::make_shared<GLine>(start, result.points[1]));
        }
    }

    return result;
}

ShortestPathResult OptimizedVisibilityGraph::findShortestPathAStar(
    const std::shared_ptr<GPoint> &start,
    const std::shared_ptr<GPoint> &end)
{
    if (!start || !end)
    {
        qWarning()
            << "Invalid start or end point provided to A* algorithm";
        return ShortestPathResult();
    }

    if (*start == *end)
    {
        ShortestPathResult result;
        result.points.append(start);
        return result;
    }

    // For ship navigation, always find closest navigable points on
    // polygon vertices
    std::shared_ptr<GPoint> startNavPoint;
    std::shared_ptr<GPoint> endNavPoint;

    // Ships navigate between polygon vertices (ports, waypoints
    // on coastlines)
    startNavPoint = quadtree->findNearestNeighborPoint(start);
    endNavPoint   = quadtree->findNearestNeighborPoint(end);

    if (!startNavPoint || !endNavPoint)
    {
        qWarning() << "Could not find navigable points for ship path";
        return ShortestPathResult();
    }

    qDebug() << "A* Ship navigation:";
    qDebug() << "  Start position:" << start->toString();
    qDebug() << "  Nearest nav point:" << startNavPoint->toString();
    qDebug() << "  End position:" << end->toString();
    qDebug() << "  Nearest nav point:" << endNavPoint->toString();

    // If start/end are already very close to nav points, use them
    // directly
    const double PROXIMITY_THRESHOLD = 100.0; // meters
    if (start->distance(*startNavPoint).value() < PROXIMITY_THRESHOLD)
    {
        startNavPoint = start;
    }
    if (end->distance(*endNavPoint).value() < PROXIMITY_THRESHOLD)
    {
        endNavPoint = end;
    }

    // Data structures for A* tracking
    std::unordered_map<std::shared_ptr<GPoint>,
                       std::shared_ptr<GPoint>, GPoint::Hash,
                       GPoint::Equal>
        cameFrom;
    std::unordered_map<std::shared_ptr<GPoint>, double, GPoint::Hash,
                       GPoint::Equal>
        gScore;
    std::unordered_map<std::shared_ptr<GPoint>, double, GPoint::Hash,
                       GPoint::Equal>
                                                         fScore;
    std::set<std::pair<double, std::shared_ptr<GPoint>>> openSet;

    // Lambda for safe score initialization
    auto initializeScores =
        [&](const std::shared_ptr<GPoint> &point) {
            if (gScore.find(point) == gScore.end())
            {
                gScore[point] =
                    std::numeric_limits<double>::infinity();
                fScore[point] =
                    std::numeric_limits<double>::infinity();
            }
        };

    // Lambda for removing point from openSet
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

    // Initialize start point
    initializeScores(startNavPoint);
    gScore[startNavPoint] = 0.0;
    fScore[startNavPoint] =
        startNavPoint->distance(*endNavPoint).value();
    openSet.insert({fScore[startNavPoint], startNavPoint});

    // Initialize end point
    initializeScores(endNavPoint);

    // CRITICAL: Add manual connections for start/end if they're not
    // polygon vertices
    if (startNavPoint != start)
    {
        initializeScores(start);
    }
    if (endNavPoint != end)
    {
        initializeScores(end);
    }

    while (!openSet.empty())
    {
        // Get the node in openSet having the lowest fScore value
        auto currentPair = *openSet.begin();
        openSet.erase(openSet.begin());

        std::shared_ptr<GPoint> current = currentPair.second;

        // Check if we reached the navigation end point
        if (*current == *endNavPoint)
        {
            qDebug() << "A*: Reached navigation end point";

            // If end nav point is different from actual end, add
            // final connection
            if (*endNavPoint != *end)
            {
                initializeScores(end);
                double finalGScore =
                    gScore[endNavPoint]
                    + endNavPoint->distance(*end).value();
                if (finalGScore < gScore[end])
                {
                    gScore[end]   = finalGScore;
                    cameFrom[end] = endNavPoint;
                }
            }
            break;
        }

        // Get neighbors
        QVector<std::shared_ptr<GPoint>> neighbors;
        {
            QReadLocker locker(&quadtreeLock);

            if (mBoundaryType == BoundariesType::Water)
            {
                // For water navigation, find containing polygon
                auto currentPolygon = findContainingPolygon(current);
                if (!currentPolygon)
                {
                    // If current point is not in a polygon, it might
                    // be a vertex Find all polygons and get visible
                    // nodes between them
                    neighbors = getVisibleNodesBetweenPolygons(
                        current, polygons);
                }
                else
                {
                    neighbors = getVisibleNodesWithinPolygon(
                        current, currentPolygon);
                }

                // CRITICAL: Ensure end navigation point is considered
                // if visible
                if (*current != *endNavPoint
                    && isVisible(current, endNavPoint))
                {
                    if (!neighbors.contains(endNavPoint))
                    {
                        neighbors.append(endNavPoint);
                    }
                }

                // If this is the start nav point and different from
                // actual start, connect to actual start
                if (*current == *startNavPoint
                    && *startNavPoint != *start)
                {
                    if (isVisible(current, start))
                    {
                        neighbors.append(start);
                    }
                }
            }
            else
            {
                neighbors =
                    getVisibleNodesBetweenPolygons(current, polygons);

                // Add manual points for visibility (same as in
                // Dijkstra)
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
        }

        qDebug() << "A*: Point" << current->toString() << "has"
                 << neighbors.size() << "neighbors";

        // Process all neighbors
        for (const auto &neighbor : neighbors)
        {
            // Initialize neighbor scores if needed
            initializeScores(neighbor);

            // Calculate tentative gScore
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
                    + neighbor->distance(*endNavPoint).value();

                // Update openSet (remove old entry, add new one)
                removeFromOpenSet(neighbor);
                openSet.insert({fScore[neighbor], neighbor});

                qDebug()
                    << "A*: Updated neighbor" << neighbor->toString()
                    << "gScore=" << gScore[neighbor]
                    << "fScore=" << fScore[neighbor];
            }
        }
    }

    // Determine which end point to use for reconstruction
    std::shared_ptr<GPoint> pathEndPoint = endNavPoint;
    if (*endNavPoint != *end && cameFrom.find(end) != cameFrom.end())
    {
        pathEndPoint = end; // Use actual end if we found a path to it
    }

    // Check if we found a path
    if (cameFrom.find(pathEndPoint) == cameFrom.end()
        && *startNavPoint != *pathEndPoint)
    {
        qDebug() << "A*: No path found from"
                 << startNavPoint->toString() << "to"
                 << pathEndPoint->toString();
        return ShortestPathResult();
    }

    qDebug() << "A*: Reconstructing path to"
             << pathEndPoint->toString();
    auto result = reconstructPath(cameFrom, pathEndPoint);

    // If we used navigation points, prepend/append the actual
    // start/end
    if (*startNavPoint != *start && !result.points.isEmpty())
    {
        result.points.prepend(start);
        if (!result.lines.isEmpty())
        {
            result.lines.prepend(
                std::make_shared<GLine>(start, result.points[1]));
        }
    }

    return result;
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

            if (isSegmentVisible(l)) // TODO: Fix the
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
