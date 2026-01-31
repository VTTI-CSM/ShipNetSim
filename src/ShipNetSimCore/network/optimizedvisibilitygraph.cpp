#include "optimizedvisibilitygraph.h"
#include "../utils/utils.h"
#include <QQueue>
#include <QSet>
#include <functional>
#include <set>
#include <unordered_map>

namespace ShipNetSimCore
{

bool ShortestPathResult::isValid() const
{
    return (points.size() >= 2 && lines.size() >= 1
            && lines.size() == (points.size() - 1));
}
OptimizedVisibilityGraph::OptimizedVisibilityGraph()
    : enableWrapAround(false)
    , mBoundaryType(BoundariesType::Water)
    , quadtree(nullptr)
{}

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

        // Build antimeridian portal connections for water navigation
        if (enableWrapAround && mBoundaryType == BoundariesType::Water)
        {
            buildAntimeridianPortals();
        }
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
    if (!quadtree)
    {
        qWarning() << "getMinMapPoint: Quadtree not initialized";
        return GPoint();
    }
    return quadtree->getMapMinPoint();
}

GPoint OptimizedVisibilityGraph::getMaxMapPoint()
{
    if (!quadtree)
    {
        qWarning() << "getMaxMapPoint: Quadtree not initialized";
        return GPoint();
    }
    return quadtree->getMapMaxPoint();
}

void OptimizedVisibilityGraph::loadSeaPortsPolygonCoordinates(
    QVector<std::shared_ptr<SeaPort>> &seaPorts)
{
    if (!quadtree)
    {
        qWarning() << "loadSeaPortsPolygonCoordinates: "
                      "Quadtree not initialized";
        return;
    }
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
    {
        QWriteLocker containmentLocker(&containmentCacheLock);
        polygonContainmentCache.clear();
    }
    if (quadtree)
    {
        quadtree->clearTree();
    }
    polygons = newPolygons;

    // Rebuild antimeridian portals for water navigation
    if (enableWrapAround && mBoundaryType == BoundariesType::Water)
    {
        // Need to release lock before calling buildAntimeridianPortals
        // which calls isSegmentVisible that needs the lock
        locker.unlock();
        buildAntimeridianPortals();
    }
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

    // For water navigation, only get vertices from polygons where this node
    // is either on the boundary (ringsContain) or inside (isPointWithinPolygon)
    // This prevents trying to connect to unrelated polygons
    for (const auto &polygon : allPolygons)
    {
        // Check if node is part of this polygon (boundary or inside)
        bool isPartOfPolygon = polygon->ringsContain(node)
                               || polygon->isPointWithinPolygon(*node);

        if (!isPartOfPolygon)
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

    // Parallel filter using QtConcurrent with limited thread pool
    QFuture<std::shared_ptr<GPoint>> future = QtConcurrent::filtered(
        ThreadConfig::getSharedThreadPool(),
        tasks, [this, node](const std::shared_ptr<GPoint> &point) {
            return isVisible(node, point);
        });

    // Get results and convert to QVector
    QVector<std::shared_ptr<GPoint>> visibleNodes =
        future.results().toVector();

    // Add manual connections (search by coordinate values, not pointer)
    for (auto it = manualConnections.constBegin();
         it != manualConnections.constEnd(); ++it)
    {
        if (*it.key() == *node)
        {
            visibleNodes += it.value();
            break;
        }
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
    // NOTE: We don't use caching here because with overlapping polygons,
    // the same node can have different visible neighbors depending on
    // which polygon is being checked. The caller handles merging results.

    // Prepare points list (exclude node itself)
    // Include both outer boundary points AND hole vertices
    // (for water polygons, holes are islands - ships navigate around them)
    QVector<std::shared_ptr<GPoint>> candidates;

    // Add outer boundary points
    const auto &outerPoints = polygon->outer();
    std::copy_if(outerPoints.begin(), outerPoints.end(),
                 std::back_inserter(candidates),
                 [&node](const auto &p) { return *p != *node; });

    // Add hole vertices (island corners for navigation waypoints)
    for (const auto &hole : polygon->inners())
    {
        std::copy_if(hole.begin(), hole.end(),
                     std::back_inserter(candidates),
                     [&node](const auto &p) { return *p != *node; });
    }

    // Parallel visibility check with limited thread pool
    QFuture<std::shared_ptr<GPoint>> future = QtConcurrent::filtered(
        ThreadConfig::getSharedThreadPool(),
        candidates,
        [this, node](const std::shared_ptr<GPoint> &point) {
            return isVisible(node, point);
        });

    QVector<std::shared_ptr<GPoint>> visibleNodes = future.results().toVector();

    // Add manual connections (e.g., antimeridian portal connections)
    // Note: We search by coordinate values, not pointer address, because
    // the pathfinding may use different shared_ptr instances for the same point
    {
        QReadLocker locker(&cacheLock);
        for (auto it = manualConnections.constBegin();
             it != manualConnections.constEnd(); ++it)
        {
            // Compare by coordinate values, not pointer address
            if (*it.key() == *node)
            {
                visibleNodes += it.value();
                break;  // Found matching coordinates
            }
        }
    }

    return visibleNodes;
}

void OptimizedVisibilityGraph::addManualVisibleLine(
    const std::shared_ptr<GLine> &line)
{
    QWriteLocker locker(&cacheLock);

    // Add line to manualLinesSet (O(1) insertion with hash set)
    manualLinesSet.insert(line);

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
    {
        QWriteLocker containmentLocker(&containmentCacheLock);
        polygonContainmentCache.clear();
    }
}

void OptimizedVisibilityGraph::clearManualLines()
{
    QWriteLocker locker(&cacheLock);
    manualLinesSet.clear();
    manualConnections.clear();
    manualPoints.clear();
    visibilityCache.clear();
    {
        QWriteLocker containmentLocker(&containmentCacheLock);
        polygonContainmentCache.clear();
    }
}

bool OptimizedVisibilityGraph::shouldCrossAntimeridian(
    double startLon, double goalLon)
{
    // Direct longitude difference
    double directDiff = std::abs(goalLon - startLon);
    // If direct path > 180 degrees, wrap-around is shorter
    return directDiff > 180.0;
}

void OptimizedVisibilityGraph::buildAntimeridianPortals()
{
    mEastPortalVertices.clear();
    mWestPortalVertices.clear();

    // Collect vertices near antimeridian from all polygons
    for (const auto &polygon : polygons)
    {
        if (!polygon)
            continue;

        // Process outer boundary
        for (const auto &vertex : polygon->outer())
        {
            if (!vertex)
                continue;
            double lon = vertex->getLongitude().value();
            if (lon >= (180.0 - PORTAL_ZONE_DEGREES))
            {
                mEastPortalVertices.append(vertex);
            }
            else if (lon <= (-180.0 + PORTAL_ZONE_DEGREES))
            {
                mWestPortalVertices.append(vertex);
            }
        }

        // Process holes (islands)
        for (const auto &hole : polygon->inners())
        {
            for (const auto &vertex : hole)
            {
                if (!vertex)
                    continue;
                double lon = vertex->getLongitude().value();
                if (lon >= (180.0 - PORTAL_ZONE_DEGREES))
                {
                    mEastPortalVertices.append(vertex);
                }
                else if (lon <= (-180.0 + PORTAL_ZONE_DEGREES))
                {
                    mWestPortalVertices.append(vertex);
                }
            }
        }
    }

    qDebug() << "Antimeridian portals: Found" << mEastPortalVertices.size()
             << "east vertices and" << mWestPortalVertices.size()
             << "west vertices from polygons";

    // =========================================================================
    // Generate synthetic portal vertices at regular latitude intervals
    // This ensures ships can cross the antimeridian at any latitude where
    // there is water, not just where polygon vertices happen to exist.
    // =========================================================================
    const double SYNTHETIC_PORTAL_INTERVAL = 5.0;  // degrees latitude
    const double SYNTHETIC_PORTAL_MIN_LAT = -80.0; // avoid polar regions
    const double SYNTHETIC_PORTAL_MAX_LAT = 80.0;

    int syntheticPortalCount = 0;

    for (double lat = SYNTHETIC_PORTAL_MIN_LAT;
         lat <= SYNTHETIC_PORTAL_MAX_LAT;
         lat += SYNTHETIC_PORTAL_INTERVAL)
    {
        // Create portal vertices at exactly ±180° longitude
        auto eastPortal = std::make_shared<GPoint>(
            units::angle::degree_t(180.0), units::angle::degree_t(lat));
        auto westPortal = std::make_shared<GPoint>(
            units::angle::degree_t(-180.0), units::angle::degree_t(lat));

        // Check if both sides are in water (for Water boundaries)
        // For Land boundaries, we'd check differently
        bool eastInWater = false;
        bool westInWater = false;

        if (mBoundaryType == BoundariesType::Water)
        {
            // Check if the portal point is inside any water polygon
            for (const auto &polygon : polygons)
            {
                if (polygon && polygon->isPointWithinPolygon(*eastPortal))
                {
                    eastInWater = true;
                    break;
                }
            }
            for (const auto &polygon : polygons)
            {
                if (polygon && polygon->isPointWithinPolygon(*westPortal))
                {
                    westInWater = true;
                    break;
                }
            }
        }
        else
        {
            // For land boundaries, portals are valid if NOT inside land
            eastInWater = true;
            westInWater = true;
            for (const auto &polygon : polygons)
            {
                if (polygon && polygon->isPointWithinPolygon(*eastPortal))
                {
                    eastInWater = false;
                    break;
                }
            }
            for (const auto &polygon : polygons)
            {
                if (polygon && polygon->isPointWithinPolygon(*westPortal))
                {
                    westInWater = false;
                    break;
                }
            }
        }

        // If both sides are navigable, create the portal connection
        if (eastInWater && westInWater)
        {
            // Add to portal vertex lists (for getPortalVerticesNear)
            mEastPortalVertices.append(eastPortal);
            mWestPortalVertices.append(westPortal);

            // Connect the pair - they're the same physical point
            // so this is effectively a zero-cost teleport
            addManualVisibleLine(
                std::make_shared<GLine>(eastPortal, westPortal));

            syntheticPortalCount++;
        }
    }

    qDebug() << "Antimeridian portals: Created" << syntheticPortalCount
             << "synthetic portal pairs at" << SYNTHETIC_PORTAL_INTERVAL
             << "degree intervals";

    // Create portal connections between east and west sides
    // (for polygon vertices that aren't at exact ±180°)
    int portalCount = 0;
    for (const auto &eastVertex : mEastPortalVertices)
    {
        for (const auto &westVertex : mWestPortalVertices)
        {
            // Skip if both are synthetic (already connected above)
            bool eastIsSynthetic =
                std::abs(eastVertex->getLongitude().value() - 180.0) < 0.001;
            bool westIsSynthetic =
                std::abs(westVertex->getLongitude().value() + 180.0) < 0.001;
            if (eastIsSynthetic && westIsSynthetic)
            {
                continue;
            }

            double latDiff = std::abs(
                eastVertex->getLatitude().value() -
                westVertex->getLatitude().value());

            if (latDiff <= PORTAL_LAT_TOLERANCE)
            {
                auto portalLine =
                    std::make_shared<GLine>(eastVertex, westVertex);
                if (isSegmentVisible(portalLine))
                {
                    addManualVisibleLine(portalLine);
                    portalCount++;
                }
            }
        }
    }

    // Connect vertices exactly at +/-180 (same physical location)
    // that weren't already handled by synthetic portal generation
    const double BOUNDARY_TOL = 0.01;
    for (const auto &east : mEastPortalVertices)
    {
        if (std::abs(east->getLongitude().value() - 180.0) < BOUNDARY_TOL)
        {
            for (const auto &west : mWestPortalVertices)
            {
                if (std::abs(west->getLongitude().value() + 180.0)
                    < BOUNDARY_TOL)
                {
                    double latDiff = std::abs(
                        east->getLatitude().value() -
                        west->getLatitude().value());
                    if (latDiff < BOUNDARY_TOL)
                    {
                        // Same physical point - connect directly
                        // (may duplicate synthetic connections, but that's OK)
                        addManualVisibleLine(
                            std::make_shared<GLine>(east, west));
                        portalCount++;
                    }
                }
            }
        }
    }

    qDebug() << "Antimeridian portals: Created" << portalCount
             << "additional portal connections between polygon vertices";

    qDebug() << "Antimeridian portals: Total portal vertices -"
             << mEastPortalVertices.size() << "east,"
             << mWestPortalVertices.size() << "west";
}

QVector<std::shared_ptr<GPoint>>
OptimizedVisibilityGraph::getPortalVerticesNear(
    double targetLon, double currentLat, double latRange) const
{
    QVector<std::shared_ptr<GPoint>> result;

    const auto &vertices = (targetLon > 0) ?
        mEastPortalVertices : mWestPortalVertices;

    for (const auto &vertex : vertices)
    {
        double latDiff =
            std::abs(vertex->getLatitude().value() - currentLat);
        if (latDiff <= latRange)
        {
            result.append(vertex);
        }
    }
    return result;
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
    if (!segment)
    {
        return false;
    }

    // Check if the segment is in manualLinesSet (O(1) lookup)
    // The hash set uses direction-independent hash/equality,
    // so a→b will match b→a automatically
    if (!manualLinesSet.empty())
    {
        if (manualLinesSet.find(segment) != manualLinesSet.end())
        {
            return true;
        }
    }

    // Check if graph is initialized
    if (!quadtree)
    {
        return false;
    }

    // Handle wrap-around segments
    if (Quadtree::isSegmentCrossingAntimeridian(segment))
    {
        auto splitSegments =
            Quadtree::splitSegmentAtAntimeridian(segment);
        for (const auto &seg : splitSegments)
        {
            if (!isSegmentVisible(seg))
            {
                return false;
            }
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
        // Find ALL polygons containing each endpoint
        // (handles overlapping polygons like Atlantic/Mediterranean)
        auto startPolygons =
            findAllContainingPolygons(segment->startPoint());
        auto endPolygons = findAllContainingPolygons(segment->endPoint());

        // Find any common polygon that contains both endpoints
        std::shared_ptr<Polygon> commonPolygon = nullptr;
        for (const auto &sp : startPolygons)
        {
            for (const auto &ep : endPolygons)
            {
                if (sp == ep)
                {
                    commonPolygon = sp;
                    break;
                }
            }
            if (commonPolygon)
                break;
        }

        // If both points share a common polygon, validate within it
        if (commonPolygon)
        {
            if (!commonPolygon->isValidWaterSegment(segment))
            {
                return false;
            }
        }
        // If points don't share a common polygon,
        // check if segment crosses any polygon holes
        // Only check polygons whose bounding box intersects the segment
        else
        {
            for (const auto &polygon : polygons)
            {
                // Skip polygons that don't intersect the segment's
                // bounding box
                if (!polygon->segmentBoundsIntersect(segment))
                {
                    continue;
                }
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
    double startLon = segmentStart->getLongitude().value();
    double endLon = segmentEnd->getLongitude().value();
    double segMinLat = std::min(segmentStart->getLatitude().value(),
                                segmentEnd->getLatitude().value());
    double segMaxLat = std::max(segmentStart->getLatitude().value(),
                                segmentEnd->getLatitude().value());

    // Special handling for segments that have been split at the antimeridian
    // If one endpoint is at ±180° and the other is far away (> 90° apart),
    // the segment was likely split and should be bounded to one hemisphere
    double segMinLon, segMaxLon;
    double lonDiff = std::abs(endLon - startLon);
    const double ANTIMERIDIAN_TOLERANCE = 1e-6;
    bool startAtAntimeridian = std::abs(std::abs(startLon) - 180.0) < ANTIMERIDIAN_TOLERANCE;
    bool endAtAntimeridian = std::abs(std::abs(endLon) - 180.0) < ANTIMERIDIAN_TOLERANCE;
    bool isAntimeridianSplit = (startAtAntimeridian || endAtAntimeridian) && lonDiff > 90.0;

    if (isAntimeridianSplit)
    {
        // Determine which hemisphere the segment is in based on the
        // non-antimeridian endpoint
        double otherLon = startAtAntimeridian ? endLon : startLon;

        if (otherLon < 0)
        {
            // Segment is in the Western hemisphere (negative longitudes to -180)
            // Path: otherLon → ... → -180 (or equivalently 180)
            segMinLon = -180.0;
            segMaxLon = otherLon;
        }
        else
        {
            // Segment is in the Eastern hemisphere (positive longitudes to +180)
            // Path: otherLon → ... → 180
            segMinLon = otherLon;
            segMaxLon = 180.0;
        }
    }
    else
    {
        segMinLon = std::min(startLon, endLon);
        segMaxLon = std::max(startLon, endLon);
    }

    // Pre-extract segment coordinates for optimized comparisons
    double startLat = segmentStart->getLatitude().value();
    double endLat = segmentEnd->getLatitude().value();

    // 4. Check edges in intersecting nodes
    // Optimized: bounding box rejection FIRST, then coordinate-based checks
    auto checkEdge = [&](const std::shared_ptr<GLine> &edge) {
        // STEP 1: Extract edge coordinates ONCE
        auto edgeStartPt = edge->startPoint();
        auto edgeEndPt = edge->endPoint();
        double eLon1 = edgeStartPt->getLongitude().value();
        double eLat1 = edgeStartPt->getLatitude().value();
        double eLon2 = edgeEndPt->getLongitude().value();
        double eLat2 = edgeEndPt->getLatitude().value();

        // STEP 2: Antimeridian check (cheap)
        double edgeLonDiff = std::abs(eLon1 - eLon2);
        if (edgeLonDiff > 180.0)
        {
            return false;  // Skip antimeridian-crossing edges
        }

        // STEP 3: Bounding box rejection FIRST (cheapest filter)
        double edgeMinLon = std::min(eLon1, eLon2);
        double edgeMaxLon = std::max(eLon1, eLon2);
        double edgeMinLat = std::min(eLat1, eLat2);
        double edgeMaxLat = std::max(eLat1, eLat2);

        if (edgeMaxLon < segMinLon || edgeMinLon > segMaxLon
            || edgeMaxLat < segMinLat || edgeMinLat > segMaxLat)
        {
            return false;  // Bounding boxes don't overlap
        }

        // STEP 4: Coordinate-based endpoint check (no geodesic calculation)
        // 0.00001 degrees ≈ 1.1 meters at equator
        const double COORD_TOL = 0.00001;
        auto coordsNear = [COORD_TOL](double lon1, double lat1, double lon2, double lat2) {
            return std::abs(lon1 - lon2) < COORD_TOL
                && std::abs(lat1 - lat2) < COORD_TOL;
        };

        bool sharesEndpoint =
            coordsNear(eLon1, eLat1, startLon, startLat) ||
            coordsNear(eLon1, eLat1, endLon, endLat) ||
            coordsNear(eLon2, eLat2, startLon, startLat) ||
            coordsNear(eLon2, eLat2, endLon, endLat);

        if (sharesEndpoint)
        {
            return false;  // Skip edge connected at a vertex
        }

        // STEP 5: Point-on-edge check using coordinate math (T-intersection)
        auto pointOnEdgeFast = [&](double pLon, double pLat) {
            // Bounding box check
            if (pLon < edgeMinLon - COORD_TOL || pLon > edgeMaxLon + COORD_TOL ||
                pLat < edgeMinLat - COORD_TOL || pLat > edgeMaxLat + COORD_TOL)
            {
                return false;
            }

            // Collinearity check using cross product
            // Point is on line segment if cross product ≈ 0
            double dx = eLon2 - eLon1;
            double dy = eLat2 - eLat1;
            double dpx = pLon - eLon1;
            double dpy = pLat - eLat1;

            double cross = dx * dpy - dy * dpx;
            double lenSq = dx * dx + dy * dy;

            // Avoid division - compare cross^2 to tolerance^2 * lenSq
            // Scale tolerance for better accuracy on longer edges
            return (cross * cross) < (COORD_TOL * COORD_TOL * lenSq * 100.0);
        };

        if (pointOnEdgeFast(startLon, startLat) || pointOnEdgeFast(endLon, endLat))
        {
            return false;  // Skip edge - segment endpoint lies on it
        }

        // STEP 6: Full intersection check (only reaches here after all filters)
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

        QtConcurrent::blockingMap(ThreadConfig::getSharedThreadPool(),
                                   intersectingNodes, processNode);
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
    if (!point)
    {
        return nullptr;
    }

    // Check cache first (read lock for concurrent access)
    {
        QReadLocker readLocker(&containmentCacheLock);
        auto it = polygonContainmentCache.find(point);
        if (it != polygonContainmentCache.end())
        {
            return it->second;  // Return cached result (may be nullptr)
        }
    }

    // Cache miss - compute the containing polygon
    // Acquire read lock to safely iterate over polygons vector
    std::shared_ptr<Polygon> result = nullptr;
    {
        QReadLocker locker(&quadtreeLock);
        for (const auto &polygon : polygons)
        {
            // Use isPointWithinPolygon which checks if point is:
            // 1. Inside the exterior ring (including boundary), AND
            // 2. NOT inside any interior holes
            // This correctly identifies points that are within navigable water
            if (polygon->isPointWithinPolygon(*point))
            {
                result = polygon;
                break;
            }
        }
    }

    // Store result in cache (write lock)
    {
        QWriteLocker writeLocker(&containmentCacheLock);
        polygonContainmentCache[point] = result;
    }

    return result;
}

QVector<std::shared_ptr<Polygon>>
OptimizedVisibilityGraph::findAllContainingPolygons(
    const std::shared_ptr<GPoint> &point) const
{
    QVector<std::shared_ptr<Polygon>> result;
    if (!point)
    {
        return result;
    }

    // Acquire read lock to safely iterate over polygons vector
    QReadLocker locker(&quadtreeLock);

    // Check all polygons (no early return like findContainingPolygon)
    for (const auto &polygon : polygons)
    {
        // Include polygons where the point is:
        // 1. Inside the polygon (but not in holes), OR
        // 2. On any ring boundary (outer or hole)
        // This handles navigation waypoints on island boundaries
        if (polygon->isPointWithinPolygon(*point)
            || polygon->ringsContain(point))
        {
            result.append(polygon);
        }
    }

    return result;
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

    // Check if the graph is properly initialized
    if (!quadtree || polygons.isEmpty())
    {
        qWarning() << "Dijkstra: Graph not initialized or empty";
        return ShortestPathResult();
    }

    if (*start == *end)
    {
        ShortestPathResult result;
        result.points.append(start);
        return result;
    }

    // Navigation points - use actual start/end when inside water polygons
    std::shared_ptr<GPoint> startNavPoint;
    std::shared_ptr<GPoint> endNavPoint;
    std::shared_ptr<Polygon> startPolygon;
    std::shared_ptr<Polygon> endPolygon;

    // For water navigation, verify points are in water and use them directly
    // If points are on land, snap to nearest water polygon vertex
    if (mBoundaryType == BoundariesType::Water)
    {
        startPolygon = findContainingPolygon(start);
        endPolygon = findContainingPolygon(end);

        if (!startPolygon)
        {
            // Start point is on land - snap to nearest water vertex
            qDebug() << "Dijkstra: Start point on land, snapping to "
                        "nearest water vertex:"
                     << start->toString();
            startNavPoint = quadtree->findNearestNeighborPoint(start);
            if (!startNavPoint)
            {
                qWarning() << "Dijkstra: Could not find nearest water "
                              "point for start:"
                           << start->toString();
                return ShortestPathResult();
            }

            // Point is on land - determine why and find nearest water vertex
            // Check each polygon to see if point is inside a hole (island)
            bool foundValidNav = false;
            for (const auto &polygon : polygons)
            {
                // First check if point is inside the exterior ring
                if (!polygon->isPointWithinExteriorRing(*start))
                {
                    continue; // Point not even in this polygon's bbox
                }

                // Check if point is inside a hole (island)
                int holeIdx = polygon->findContainingHoleIndex(*start);
                if (holeIdx >= 0)
                {
                    // Point is inside an island - find nearest vertex
                    // on THAT island's coastline
                    qDebug() << "Dijkstra: Start point inside hole"
                             << holeIdx << ", finding nearest coastline vertex";

                    auto holeVertices = polygon->inners()[holeIdx];
                    units::length::meter_t minDist(
                        std::numeric_limits<double>::max());

                    for (const auto &vertex : holeVertices)
                    {
                        auto dist = start->distance(*vertex);
                        if (dist < minDist)
                        {
                            minDist       = dist;
                            startNavPoint = vertex;
                        }
                    }
                    startPolygon  = polygon;
                    foundValidNav = true;
                    qDebug() << "Dijkstra: Snapped to hole vertex:"
                             << startNavPoint->toString();
                    break;
                }
                else
                {
                    // Point is in exterior but not in any hole - it's in water
                    // This shouldn't happen if findContainingPolygon failed,
                    // but handle it anyway by using the original snapped point
                    startPolygon  = polygon;
                    foundValidNav = true;
                    qDebug() << "Dijkstra: Point in water area, using snapped:"
                             << startNavPoint->toString();
                    break;
                }
            }

            if (!foundValidNav)
            {
                // Point is outside all polygons - find nearest outer vertex
                qDebug() << "Dijkstra: Start outside all polygons, "
                            "finding nearest outer boundary vertex";

                units::length::meter_t minDist(
                    std::numeric_limits<double>::max());

                for (const auto &polygon : polygons)
                {
                    for (const auto &vertex : polygon->outer())
                    {
                        auto dist = start->distance(*vertex);
                        if (dist < minDist)
                        {
                            minDist       = dist;
                            startNavPoint = vertex;
                            startPolygon  = polygon;
                        }
                    }
                }

                if (!startNavPoint)
                {
                    qWarning() << "Dijkstra: Could not find valid water "
                                  "vertex for start:"
                               << start->toString();
                    return ShortestPathResult();
                }
                qDebug() << "Dijkstra: Snapped to outer vertex:"
                         << startNavPoint->toString();
            }
        }
        else
        {
            // Start point is in water - use it directly
            startNavPoint = start;
        }

        if (!endPolygon)
        {
            // End point is on land - snap to nearest water vertex
            qDebug() << "Dijkstra: End point on land, snapping to "
                        "nearest water vertex:"
                     << end->toString();
            endNavPoint = quadtree->findNearestNeighborPoint(end);
            if (!endNavPoint)
            {
                qWarning() << "Dijkstra: Could not find nearest water "
                              "point for end:"
                           << end->toString();
                return ShortestPathResult();
            }

            // Point is on land - determine why and find nearest water vertex
            // Check each polygon to see if point is inside a hole (island)
            bool foundValidEndNav = false;
            for (const auto &polygon : polygons)
            {
                // First check if point is inside the exterior ring
                if (!polygon->isPointWithinExteriorRing(*end))
                {
                    continue; // Point not even in this polygon's bbox
                }

                // Check if point is inside a hole (island)
                int holeIdx = polygon->findContainingHoleIndex(*end);
                if (holeIdx >= 0)
                {
                    // Point is inside an island - find nearest vertex
                    // on THAT island's coastline
                    qDebug() << "Dijkstra: End point inside hole"
                             << holeIdx << ", finding nearest coastline vertex";

                    auto holeVertices = polygon->inners()[holeIdx];
                    units::length::meter_t minDist(
                        std::numeric_limits<double>::max());

                    for (const auto &vertex : holeVertices)
                    {
                        auto dist = end->distance(*vertex);
                        if (dist < minDist)
                        {
                            minDist     = dist;
                            endNavPoint = vertex;
                        }
                    }
                    endPolygon        = polygon;
                    foundValidEndNav  = true;
                    qDebug() << "Dijkstra: Snapped to hole vertex:"
                             << endNavPoint->toString();
                    break;
                }
                else
                {
                    // Point is in exterior but not in any hole - it's in water
                    endPolygon       = polygon;
                    foundValidEndNav = true;
                    qDebug() << "Dijkstra: Point in water area, using snapped:"
                             << endNavPoint->toString();
                    break;
                }
            }

            if (!foundValidEndNav)
            {
                // Point is outside all polygons - find nearest outer vertex
                qDebug() << "Dijkstra: End outside all polygons, "
                            "finding nearest outer boundary vertex";

                units::length::meter_t minDist(
                    std::numeric_limits<double>::max());

                for (const auto &polygon : polygons)
                {
                    for (const auto &vertex : polygon->outer())
                    {
                        auto dist = end->distance(*vertex);
                        if (dist < minDist)
                        {
                            minDist     = dist;
                            endNavPoint = vertex;
                            endPolygon  = polygon;
                        }
                    }
                }

                if (!endNavPoint)
                {
                    qWarning() << "Dijkstra: Could not find valid water "
                                  "vertex for end:"
                               << end->toString();
                    return ShortestPathResult();
                }
                qDebug() << "Dijkstra: Snapped to outer vertex:"
                         << endNavPoint->toString();
            }
        }
        else
        {
            // End point is in water - use it directly
            endNavPoint = end;
        }
    }
    else
    {
        // Land/other navigation uses global nearest neighbor
        startNavPoint = quadtree->findNearestNeighborPoint(start);
        endNavPoint = quadtree->findNearestNeighborPoint(end);

        if (!startNavPoint || !endNavPoint)
        {
            qWarning() << "Could not find navigable points for path";
            return ShortestPathResult();
        }
    }

    qDebug() << "Dijkstra Ship navigation:";
    qDebug() << "  Start position:" << start->toString();
    qDebug() << "  Nav point:" << startNavPoint->toString();
    qDebug() << "  End position:" << end->toString();
    qDebug() << "  Nav point:" << endNavPoint->toString();

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

    qDebug() << "Dijkstra: Starting search from" << startNavPoint->toString()
             << "to" << endNavPoint->toString();

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
            break;
        }

        // Get visible neighbors
        // NOTE: Do NOT hold quadtreeLock while calling visibility functions
        // that spawn QtConcurrent tasks - those functions have their own
        // internal locking. Holding the lock here would cause thread pool
        // starvation as worker threads try to acquire the same lock.
        QVector<std::shared_ptr<GPoint>> visibleNodes;

        // Copy polygons while holding the lock for use in visibility checks
        QVector<std::shared_ptr<Polygon>> localPolygons;
        {
            QReadLocker locker(&quadtreeLock);
            localPolygons = polygons;
        }

        if (mBoundaryType == BoundariesType::Water)
        {
            // For water navigation, find ALL containing polygons
            // (handles overlapping regions like Atlantic/Mediterranean)
            // findAllContainingPolygons has its own internal locking
            auto containingPolygons =
                findAllContainingPolygons(current);

            if (containingPolygons.isEmpty())
            {
                // If current point is not in a polygon, it might
                // be a vertex Find all polygons and get visible
                // nodes between them
                visibleNodes = getVisibleNodesBetweenPolygons(
                    current, localPolygons);
            }
            else
            {
                // Get visible nodes from ALL containing polygons
                for (const auto &poly : containingPolygons)
                {
                    auto nodesInPoly =
                        getVisibleNodesWithinPolygon(current, poly);
                    for (const auto &node : nodesInPoly)
                    {
                        if (!visibleNodes.contains(node))
                        {
                            visibleNodes.append(node);
                        }
                    }
                }
            }

            // CRITICAL: Ensure end navigation point is considered
            // if visible
            // isVisible has its own internal locking
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
                getVisibleNodesBetweenPolygons(current, localPolygons);
        }

        // Add wrap-around connections if enabled
        // connectWrapAroundPoints has its own internal locking
        if (enableWrapAround)
        {
            auto wrapPoints = connectWrapAroundPoints(current, endNavPoint);
            visibleNodes.append(wrapPoints);
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

    // Check if the graph is properly initialized
    if (!quadtree || polygons.isEmpty())
    {
        qWarning() << "A*: Graph not initialized or empty";
        return ShortestPathResult();
    }

    if (*start == *end)
    {
        ShortestPathResult result;
        result.points.append(start);
        return result;
    }

    // Navigation points - use actual start/end when inside water polygons
    std::shared_ptr<GPoint> startNavPoint;
    std::shared_ptr<GPoint> endNavPoint;
    std::shared_ptr<Polygon> startPolygon;
    std::shared_ptr<Polygon> endPolygon;

    // For water navigation, verify points are in water and use them directly
    // If points are on land, snap to nearest water polygon vertex
    if (mBoundaryType == BoundariesType::Water)
    {
        startPolygon = findContainingPolygon(start);
        endPolygon = findContainingPolygon(end);

        if (!startPolygon)
        {
            // Start point is on land - determine why and find nearest water vertex
            qDebug() << "A*: Start point on land:" << start->toString();

            bool foundValidNav = false;
            for (const auto &polygon : polygons)
            {
                if (!polygon->isPointWithinExteriorRing(*start))
                {
                    continue;
                }

                int holeIdx = polygon->findContainingHoleIndex(*start);
                if (holeIdx >= 0)
                {
                    qDebug() << "A*: Start inside hole" << holeIdx;
                    auto holeVertices = polygon->inners()[holeIdx];
                    units::length::meter_t minDist(
                        std::numeric_limits<double>::max());

                    for (const auto &vertex : holeVertices)
                    {
                        auto dist = start->distance(*vertex);
                        if (dist < minDist)
                        {
                            minDist       = dist;
                            startNavPoint = vertex;
                        }
                    }
                    startPolygon  = polygon;
                    foundValidNav = true;
                    qDebug() << "A*: Snapped to hole vertex:"
                             << startNavPoint->toString();
                    break;
                }
                else
                {
                    startPolygon  = polygon;
                    startNavPoint = quadtree->findNearestNeighborPoint(start);
                    foundValidNav = true;
                    break;
                }
            }

            if (!foundValidNav)
            {
                qDebug() << "A*: Start outside polygons, finding outer vertex";
                units::length::meter_t minDist(
                    std::numeric_limits<double>::max());

                for (const auto &polygon : polygons)
                {
                    for (const auto &vertex : polygon->outer())
                    {
                        auto dist = start->distance(*vertex);
                        if (dist < minDist)
                        {
                            minDist       = dist;
                            startNavPoint = vertex;
                            startPolygon  = polygon;
                        }
                    }
                }

                if (!startNavPoint)
                {
                    qWarning() << "A*: Could not find valid water vertex";
                    return ShortestPathResult();
                }
                qDebug() << "A*: Snapped to outer:" << startNavPoint->toString();
            }
        }
        else
        {
            startNavPoint = start;
        }

        if (!endPolygon)
        {
            qDebug() << "A*: End point on land:" << end->toString();

            bool foundValidEndNav = false;
            for (const auto &polygon : polygons)
            {
                if (!polygon->isPointWithinExteriorRing(*end))
                {
                    continue;
                }

                int holeIdx = polygon->findContainingHoleIndex(*end);
                if (holeIdx >= 0)
                {
                    qDebug() << "A*: End inside hole" << holeIdx;
                    auto holeVertices = polygon->inners()[holeIdx];
                    units::length::meter_t minDist(
                        std::numeric_limits<double>::max());

                    for (const auto &vertex : holeVertices)
                    {
                        auto dist = end->distance(*vertex);
                        if (dist < minDist)
                        {
                            minDist     = dist;
                            endNavPoint = vertex;
                        }
                    }
                    endPolygon        = polygon;
                    foundValidEndNav  = true;
                    qDebug() << "A*: Snapped to hole vertex:"
                             << endNavPoint->toString();
                    break;
                }
                else
                {
                    endPolygon       = polygon;
                    endNavPoint      = quadtree->findNearestNeighborPoint(end);
                    foundValidEndNav = true;
                    break;
                }
            }

            if (!foundValidEndNav)
            {
                qDebug() << "A*: End outside polygons, finding outer vertex";
                units::length::meter_t minDist(
                    std::numeric_limits<double>::max());

                for (const auto &polygon : polygons)
                {
                    for (const auto &vertex : polygon->outer())
                    {
                        auto dist = end->distance(*vertex);
                        if (dist < minDist)
                        {
                            minDist     = dist;
                            endNavPoint = vertex;
                            endPolygon  = polygon;
                        }
                    }
                }

                if (!endNavPoint)
                {
                    qWarning() << "A*: Could not find valid water vertex";
                    return ShortestPathResult();
                }
                qDebug() << "A*: Snapped to outer:" << endNavPoint->toString();
            }
        }
        else
        {
            endNavPoint = end;
        }

        if (startPolygon && endPolygon && startPolygon != endPolygon)
        {
            qDebug() << "A*: Start and end in different polygons";
        }
    }
    else
    {
        // Land/other navigation uses global nearest neighbor
        startNavPoint = quadtree->findNearestNeighborPoint(start);
        endNavPoint = quadtree->findNearestNeighborPoint(end);

        if (!startNavPoint || !endNavPoint)
        {
            qWarning() << "Could not find navigable points for path";
            return ShortestPathResult();
        }
    }

    qDebug() << "A* Ship navigation:";
    qDebug() << "  Start position:" << start->toString();
    qDebug() << "  Nav point:" << startNavPoint->toString();
    qDebug() << "  End position:" << end->toString();
    qDebug() << "  Nav point:" << endNavPoint->toString();

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
            break;
        }

        // Get neighbors
        // NOTE: Do NOT hold quadtreeLock while calling visibility functions
        // that spawn QtConcurrent tasks - those functions have their own
        // internal locking. Holding the lock here would cause thread pool
        // starvation as worker threads try to acquire the same lock.
        QVector<std::shared_ptr<GPoint>> neighbors;

        // Copy polygons and manualPoints while holding the lock
        QVector<std::shared_ptr<Polygon>> localPolygons;
        QVector<std::shared_ptr<GPoint>> localManualPoints;
        {
            QReadLocker locker(&quadtreeLock);
            localPolygons = polygons;
            localManualPoints = manualPoints;
        }

        if (mBoundaryType == BoundariesType::Water)
        {
            // For water navigation, find ALL containing polygons
            // (handles overlapping regions like Atlantic/Mediterranean)
            // findAllContainingPolygons has its own internal locking
            auto containingPolygons =
                findAllContainingPolygons(current);
            if (containingPolygons.isEmpty())
            {
                // If current point is not in a polygon, it might
                // be a vertex Find all polygons and get visible
                // nodes between them
                neighbors = getVisibleNodesBetweenPolygons(
                    current, localPolygons);
            }
            else
            {
                // Get visible nodes from ALL containing polygons
                for (const auto &poly : containingPolygons)
                {
                    auto nodesInPoly =
                        getVisibleNodesWithinPolygon(current, poly);
                    for (const auto &node : nodesInPoly)
                    {
                        if (!neighbors.contains(node))
                        {
                            neighbors.append(node);
                        }
                    }
                }
            }

            // CRITICAL: Ensure end navigation point is considered
            // if visible
            // isVisible has its own internal locking
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
                getVisibleNodesBetweenPolygons(current, localPolygons);

            // Add manual points for visibility (same as in Dijkstra)
            // isVisible has its own internal locking
            for (const auto &manualPoint : localManualPoints)
            {
                if (isVisible(current, manualPoint))
                {
                    neighbors.append(manualPoint);
                }
            }
        }

        // Apply wrap-around logic if enabled
        // connectWrapAroundPoints has its own internal locking
        if (enableWrapAround)
        {
            neighbors.append(connectWrapAroundPoints(current, endNavPoint));
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
    const std::shared_ptr<GPoint> &point,
    const std::shared_ptr<GPoint> &goalPoint)
{
    QVector<std::shared_ptr<GPoint>> wrapAroundPoints;

    // Get map boundary data while holding the lock
    GPoint mapMin, mapMax;
    double mapWidth;
    QVector<std::shared_ptr<Polygon>> localPolygons;
    bool nearBoundary = false;
    {
        QReadLocker locker(&quadtreeLock);
        if (!quadtree)
            return wrapAroundPoints;

        nearBoundary = quadtree->isNearBoundary(point);

        // Get map boundaries
        mapMin = quadtree->getMapMinPoint();
        mapMax = quadtree->getMapMaxPoint();
        mapWidth =
            (mapMax.getLongitude() - mapMin.getLongitude()).value();

        // Copy polygons for later use outside the lock
        localPolygons = polygons;
    }

    double pointLon = point->getLongitude().value();
    double pointLat = point->getLatitude().value();

    // Goal-aware antimeridian crossing logic
    // If we have a goal and crossing the antimeridian is beneficial,
    // add portal vertices as potential neighbors
    if (goalPoint && mBoundaryType == BoundariesType::Water)
    {
        double goalLon = goalPoint->getLongitude().value();

        if (shouldCrossAntimeridian(pointLon, goalLon))
        {
            // Determine which direction leads toward antimeridian
            double targetLon = (pointLon > 0) ? 180.0 : -180.0;

            // Get portal vertices near the antimeridian in the right direction
            auto portalVertices = getPortalVerticesNear(
                targetLon, pointLat, PORTAL_LAT_TOLERANCE * 2);

            // Add visible portal vertices as potential neighbors
            for (const auto &portalVertex : portalVertices)
            {
                if (isVisible(point, portalVertex))
                {
                    if (!wrapAroundPoints.contains(portalVertex))
                    {
                        wrapAroundPoints.append(portalVertex);
                    }
                }
            }
        }
    }

    // Original boundary-based wrap-around logic (for points near the edge)
    if (!nearBoundary)
    {
        return wrapAroundPoints;
    }

    // Create mirrored point
    auto createMirrorPoint = [&](double offset) {
        double newLon = pointLon + offset;
        return std::make_shared<GPoint>(
            units::angle::degree_t(newLon), point->getLatitude());
    };

    QVector<std::shared_ptr<GPoint>> mirrorPoints;

    // Check left boundary (near +180)
    if ((mapMax.getLongitude() - point->getLongitude()).value() < 1.0)
    {
        mirrorPoints.append(createMirrorPoint(-mapWidth));
    }
    // Check right boundary (near -180)
    else if ((point->getLongitude() - mapMin.getLongitude()).value()
             < 1.0)
    {
        mirrorPoints.append(createMirrorPoint(mapWidth));
    }

    // Find visible nodes for mirrored points
    // NOTE: Visibility functions have their own internal locking
    QVector<std::shared_ptr<GPoint>> allVisible;
    for (const auto &wrappedPoint : mirrorPoints)
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
                wrappedPoint, localPolygons);
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
    for (const auto &candidate : allVisible)
    {
        auto wrapSegment = std::make_shared<GLine>(point, candidate);
        if (isSegmentVisible(wrapSegment))
        {
            if (!wrapAroundPoints.contains(candidate))
            {
                wrapAroundPoints.append(candidate);
            }
        }
    }

    return wrapAroundPoints;
}

void OptimizedVisibilityGraph::clear()
{
    QWriteLocker locker(&quadtreeLock);

    if (quadtree)
    {
        quadtree->clearTree();
    }
    polygons.clear();

    // Clear antimeridian portal vertices
    mEastPortalVertices.clear();
    mWestPortalVertices.clear();

    // Clear manual lines (includes portal connections)
    {
        QWriteLocker cacheLocker(&cacheLock);
        manualLinesSet.clear();
        manualConnections.clear();
        manualPoints.clear();
        visibilityCache.clear();
    }

    {
        QWriteLocker containmentLocker(&containmentCacheLock);
        polygonContainmentCache.clear();
    }
}

}; // namespace ShipNetSimCore
