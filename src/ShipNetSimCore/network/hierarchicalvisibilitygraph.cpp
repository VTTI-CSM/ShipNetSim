#include "hierarchicalvisibilitygraph.h"
#include "../utils/utils.h"
#include <QElapsedTimer>
#include <QMutex>
#include <chrono>
#include <functional>
#include <numeric>
#include <queue>
#include <unordered_map>
#include <unordered_set>

namespace ShipNetSimCore
{

// =============================================================================
// ShortestPathResult
// =============================================================================

bool ShortestPathResult::isValid() const
{
    return (points.size() >= 2 && lines.size() >= 1
            && lines.size() == (points.size() - 1));
}

// =============================================================================
// Construction
// =============================================================================

HierarchicalVisibilityGraph::HierarchicalVisibilityGraph()
    : enableWrapAround(false)
{}

HierarchicalVisibilityGraph::HierarchicalVisibilityGraph(
    const QVector<std::shared_ptr<Polygon>>& usedPolygons)
    : polygons(usedPolygons)
{
    if (polygons.isEmpty())
    {
        qWarning() << "Warning: Empty polygon list provided to "
                      "HierarchicalVisibilityGraph";
    }

    enableWrapAround = true;

    // Set polygon ownership on vertices for O(1) lookup
    for (const auto& polygon : polygons)
    {
        if (!polygon) continue;
        for (const auto& vertex : polygon->outer())
        {
            if (vertex) vertex->addOwningPolygon(polygon);
        }
        for (const auto& hole : polygon->inners())
        {
            for (const auto& vertex : hole)
            {
                if (vertex) vertex->addOwningPolygon(polygon);
            }
        }
    }

    buildAllLevels();
}

HierarchicalVisibilityGraph::~HierarchicalVisibilityGraph() {}

// =============================================================================
// Level Building
// =============================================================================

void HierarchicalVisibilityGraph::buildAllLevels()
{
    for (int i = 0; i < NUM_LEVELS; ++i)
    {
        buildLevel(i);
    }

    // Build adjacency for coarser levels (1-3) in parallel
    // Level 0 adjacency is computed on-demand since it's the full resolution
    for (int i = 1; i < NUM_LEVELS; ++i)
    {
        buildAdjacencyForLevel(i);
    }
}

void HierarchicalVisibilityGraph::buildLevel(int idx)
{
    auto& level = mLevels[idx];
    level.levelIndex = idx;
    level.toleranceMeters = LEVEL_TOLERANCES[idx];

    if (idx == 0)
    {
        // Level 0: original polygons
        level.polygons = polygons;
    }
    else
    {
        // Levels 1-3: simplified polygons
        for (const auto& poly : polygons)
        {
            if (!poly) continue;
            auto simplified = poly->simplify(level.toleranceMeters);
            if (simplified && simplified->outerVertexCount() >= 3)
            {
                level.polygons.append(simplified);
            }
            else
            {
                // Fall back to original if simplification fails
                level.polygons.append(poly);
            }
        }
    }

    // Create Quadtree for this level
    try
    {
        level.quadtree = std::make_unique<Quadtree>(level.polygons);
    }
    catch (const std::exception& e)
    {
        qCritical() << "Exception creating Quadtree for level" << idx
                     << ":" << e.what();
        level.quadtree = std::make_unique<Quadtree>(
            QVector<std::shared_ptr<Polygon>>());
    }

    // Collect all vertices
    int vertexIdx = 0;
    for (int pi = 0; pi < level.polygons.size(); ++pi)
    {
        const auto& poly = level.polygons[pi];
        if (!poly) continue;

        for (const auto& vertex : poly->outer())
        {
            if (!vertex) continue;
            level.vertices.append(vertex);
            level.vertexIndex[vertex] = vertexIdx;
            level.vertexPolygonId.push_back(pi);
            vertexIdx++;
        }

        for (const auto& hole : poly->inners())
        {
            for (const auto& vertex : hole)
            {
                if (!vertex) continue;
                level.vertices.append(vertex);
                level.vertexIndex[vertex] = vertexIdx;
                level.vertexPolygonId.push_back(pi);
                vertexIdx++;
            }
        }
    }

    qDebug() << "Level" << idx << ": tolerance="
             << level.toleranceMeters << "m, vertices="
             << level.vertices.size();
}

void HierarchicalVisibilityGraph::buildAdjacencyForLevel(int idx)
{
    auto& level = mLevels[idx];
    int n = level.vertices.size();
    level.adjacency.resize(n);

    if (n == 0) return;

    qDebug() << "Building adjacency for level" << idx
             << "with" << n << "vertices";

    QElapsedTimer timer;
    timer.start();

    QVector<int> indices(n);
    std::iota(indices.begin(), indices.end(), 0);

    // Per-vertex neighbor lists — each thread writes only to its own slot.
    // No mutex needed during the parallel phase.
    std::vector<std::vector<int>> perVertexNeighbors(n);

    QtConcurrent::blockingMap(indices, [&](int i) {
        int polyI = level.vertexPolygonId[i];

        for (int j = i + 1; j < n; ++j)
        {
            int polyJ = level.vertexPolygonId[j];

            bool visible = false;

            if (polyI == polyJ)
            {
                // Same polygon: use fast planar check
                visible = isVisibleInSimplifiedPolygon(
                    level.vertices[i], level.vertices[j],
                    level.polygons[polyI]);
            }
            else
            {
                // Different polygons: use quadtree-based check
                visible = isVisible(level.vertices[i],
                                    level.vertices[j], idx);
            }

            if (visible)
            {
                // Thread processing vertex i is the only writer to slot i
                perVertexNeighbors[i].push_back(j);
            }
        }
    });

    // Single-threaded merge: build symmetric adjacency from per-vertex results
    for (int i = 0; i < n; ++i)
    {
        for (int neighbor : perVertexNeighbors[i])
        {
            level.adjacency[i].push_back(neighbor);
            level.adjacency[neighbor].push_back(i);
        }
    }

    qDebug() << "Adjacency for level" << idx << "built in"
             << timer.elapsed() << "ms";
}

// =============================================================================
// Point Snapping
// =============================================================================

std::shared_ptr<GPoint> HierarchicalVisibilityGraph::snapToWater(
    const std::shared_ptr<GPoint>& point, int level) const
{
    const auto& lvl = mLevels[level];

    // Check if point is already in a water polygon
    for (const auto& polygon : lvl.polygons)
    {
        if (polygon->isPointWithinPolygon(*point))
        {
            // Ensure owning polygons are set so isSegmentVisible
            // doesn't need to call findAllContainingPolygons
            if (point->getOwningPolygons().isEmpty())
            {
                point->addOwningPolygon(polygon);
            }
            return point; // Already in water
        }
    }

    // Point is on land - determine why and snap
    for (const auto& polygon : lvl.polygons)
    {
        if (!polygon->isPointWithinExteriorRing(*point))
            continue;

        // Check if inside a hole (island)
        int holeIdx = polygon->findContainingHoleIndex(*point);
        if (holeIdx >= 0)
        {
            // Snap to nearest hole vertex
            auto holeVertices = polygon->inners()[holeIdx];
            units::length::meter_t minDist(
                std::numeric_limits<double>::max());
            std::shared_ptr<GPoint> nearest;

            for (const auto& vertex : holeVertices)
            {
                auto dist = point->distance(*vertex);
                if (dist < minDist)
                {
                    minDist = dist;
                    nearest = vertex;
                }
            }
            if (nearest) return nearest;
        }
        else
        {
            // Inside exterior but not in hole - use nearest neighbor
            if (lvl.quadtree)
                return lvl.quadtree->findNearestNeighborPoint(point);
        }
    }

    // Outside all polygons - find nearest outer vertex
    units::length::meter_t minDist(std::numeric_limits<double>::max());
    std::shared_ptr<GPoint> nearest;

    for (const auto& polygon : lvl.polygons)
    {
        for (const auto& vertex : polygon->outer())
        {
            auto dist = point->distance(*vertex);
            if (dist < minDist)
            {
                minDist = dist;
                nearest = vertex;
            }
        }
    }

    return nearest;
}

// =============================================================================
// A* at Single Level
// =============================================================================

ShortestPathResult HierarchicalVisibilityGraph::aStarAtLevel(
    const std::shared_ptr<GPoint>& start,
    const std::shared_ptr<GPoint>& goal,
    int level,
    const Corridor* corridor,
    const std::shared_ptr<GPoint>& preSnappedStart,
    const std::shared_ptr<GPoint>& preSnappedGoal)
{
    if (!start || !goal)
    {
        qWarning() << "Invalid start or end point for A* at level" << level;
        return ShortestPathResult();
    }

    if (*start == *goal)
    {
        ShortestPathResult result;
        result.points.append(start);
        return result;
    }

    // Use pre-snapped points if provided, otherwise snap
    auto startNav = preSnappedStart ? preSnappedStart
                                    : snapToWater(start, level);
    auto endNav = preSnappedGoal ? preSnappedGoal
                                : snapToWater(goal, level);

    if (!startNav || !endNav)
    {
        qWarning() << "Could not snap points to water at level" << level;
        return ShortestPathResult();
    }

    qDebug() << "A* level" << level << ": from"
             << startNav->toString() << "to" << endNav->toString();

    // A* data structures
    std::unordered_map<std::shared_ptr<GPoint>, std::shared_ptr<GPoint>,
                       GPoint::Hash, GPoint::Equal> cameFrom;
    std::unordered_map<std::shared_ptr<GPoint>, double,
                       GPoint::Hash, GPoint::Equal> gScore;

    // Min-heap with lazy deletion
    std::priority_queue<
        std::pair<double, std::shared_ptr<GPoint>>,
        std::vector<std::pair<double, std::shared_ptr<GPoint>>>,
        std::greater<std::pair<double, std::shared_ptr<GPoint>>>>
        openSet;

    std::unordered_set<std::shared_ptr<GPoint>,
                       GPoint::Hash, GPoint::Equal> closedSet;

    auto initScore = [&](const std::shared_ptr<GPoint>& pt) {
        if (gScore.find(pt) == gScore.end())
            gScore[pt] = std::numeric_limits<double>::infinity();
    };

    auto heuristic = [&](const std::shared_ptr<GPoint>& pt) {
        return pt->distance(*endNav).value();
    };

    initScore(startNav);
    gScore[startNav] = 0.0;
    openSet.push({heuristic(startNav), startNav});
    initScore(endNav);

    QElapsedTimer progressTimer;
    progressTimer.start();
    qint64 lastProgressEmit = 0;

    while (!openSet.empty())
    {
        if (QThread::currentThread()->isInterruptionRequested())
        {
            qWarning() << "A* cancelled by thread interruption";
            return ShortestPathResult();
        }

        auto [currentF, current] = openSet.top();
        openSet.pop();

        if (closedSet.count(current))
            continue;
        closedSet.insert(current);

        // Emit progress every 1 second
        qint64 nowMs = progressTimer.elapsed();
        if (nowMs - lastProgressEmit >= 1000)
        {
            lastProgressEmit = nowMs;
            emit pathFindingProgress(-1, 0, nowMs / 1000.0);
        }

        if (*current == *endNav)
        {
            break;
        }

        // Get neighbors
        auto neighbors = getVisibleNodesForPoint(current, level, corridor);

        // Check direct visibility to goal only when reasonably close.
        // heuristic(current) is the geodesic distance to goal.
        // Only attempt expensive visibility check when within threshold.
        double distToGoal = heuristic(current);
        if (*current != *endNav && !closedSet.count(endNav)
            && distToGoal < 50000.0  // 50km threshold
            && isVisible(current, endNav, level))
        {
            neighbors.append(endNav);
        }

        // Wrap-around connections
        if (enableWrapAround && level == 0)
        {
            neighbors.append(connectWrapAroundPoints(current, endNav));
        }

        for (const auto& neighbor : neighbors)
        {
            if (closedSet.count(neighbor))
                continue;

            initScore(neighbor);

            double tentG = gScore[current]
                           + current->distance(*neighbor).value();

            if (tentG < gScore[neighbor])
            {
                cameFrom[neighbor] = current;
                gScore[neighbor] = tentG;
                openSet.push({tentG + heuristic(neighbor), neighbor});
            }
        }
    }

    if (cameFrom.find(endNav) == cameFrom.end()
        && *startNav != *endNav)
    {
        qDebug() << "A* level" << level << ": no path found";
        return ShortestPathResult();
    }

    return reconstructPath(cameFrom, endNav, level);
}

// =============================================================================
// Hierarchical Search
// =============================================================================

ShortestPathResult HierarchicalVisibilityGraph::hierarchicalSearch(
    const std::shared_ptr<GPoint>& start,
    const std::shared_ptr<GPoint>& goal)
{
    qDebug() << "=== Hierarchical Search ===";

    // Track the best coarse result available for corridor building.
    // Coarse results are NEVER returned — only used to guide Level 0.
    ShortestPathResult bestCoarseResult;

    // Pre-snap points once per level to avoid redundant polygon scans
    auto snapped3Start = snapToWater(start, 3);
    auto snapped3Goal  = snapToWater(goal, 3);

    // --- Level 3 (coarsest) ---
    auto result3 = aStarAtLevel(start, goal, 3, nullptr,
                                snapped3Start, snapped3Goal);

    if (!result3.isValid())
    {
        qDebug() << "Level 3 failed, falling back to unconstrained level 0";
        auto snapped0Start = snapToWater(start, 0);
        auto snapped0Goal  = snapToWater(goal, 0);
        return aStarAtLevel(start, goal, 0, nullptr,
                            snapped0Start, snapped0Goal);
    }
    bestCoarseResult = result3;

    // --- Level 2 ---
    auto snapped2Start = snapToWater(start, 2);
    auto snapped2Goal  = snapToWater(goal, 2);

    double expansion2 = LEVEL_TOLERANCES[3] * 3.0;
    auto corridor2 = buildCorridor(result3, 2, expansion2);
    auto result2 = aStarAtLevel(start, goal, 2, &corridor2,
                                snapped2Start, snapped2Goal);

    if (!result2.isValid())
    {
        auto widerCorridor2 = buildCorridor(result3, 2, expansion2 * 3.0);
        result2 = aStarAtLevel(start, goal, 2, &widerCorridor2,
                               snapped2Start, snapped2Goal);
    }

    if (result2.isValid())
    {
        bestCoarseResult = result2;
    }

    // --- Level 1 ---
    auto snapped1Start = snapToWater(start, 1);
    auto snapped1Goal  = snapToWater(goal, 1);

    double expansion1 = LEVEL_TOLERANCES[2] * 3.0;
    auto corridor1 = buildCorridor(bestCoarseResult, 1, expansion1);
    auto result1 = aStarAtLevel(start, goal, 1, &corridor1,
                                snapped1Start, snapped1Goal);

    if (!result1.isValid())
    {
        auto widerCorridor1 = buildCorridor(bestCoarseResult, 1,
                                            expansion1 * 3.0);
        result1 = aStarAtLevel(start, goal, 1, &widerCorridor1,
                               snapped1Start, snapped1Goal);
    }

    if (result1.isValid())
    {
        bestCoarseResult = result1;
    }

    // --- Level 0 (original polygons — only valid final result) ---
    // Pre-snap once for Level 0 — used across all corridor attempts
    auto snapped0Start = snapToWater(start, 0);
    auto snapped0Goal  = snapToWater(goal, 0);

    // Pre-compute adjacency within corridor for instant neighbor lookups.
    // Each wider corridor incrementally expands the previous one.
    double expansion0 = LEVEL_TOLERANCES[1] * 3.0;
    auto corridor0 = buildCorridor(bestCoarseResult, 0, expansion0);
    precomputeCorridorAdjacency(corridor0, start, goal);
    auto result0 = aStarAtLevel(start, goal, 0, &corridor0,
                                snapped0Start, snapped0Goal);

    if (result0.isValid())
        return result0;

    // Wider corridor — carry forward corridor0's adjacency
    auto widerCorridor0 = buildCorridor(bestCoarseResult, 0,
                                        expansion0 * 3.0);
    precomputeCorridorAdjacency(widerCorridor0, start, goal, &corridor0);
    result0 = aStarAtLevel(start, goal, 0, &widerCorridor0,
                           snapped0Start, snapped0Goal);

    if (result0.isValid())
        return result0;

    // Very wide corridor — carry forward widerCorridor0's adjacency
    auto veryWideCorridor0 = buildCorridor(bestCoarseResult, 0,
                                           expansion0 * 10.0);
    precomputeCorridorAdjacency(veryWideCorridor0, start, goal,
                                &widerCorridor0);
    result0 = aStarAtLevel(start, goal, 0, &veryWideCorridor0,
                           snapped0Start, snapped0Goal);

    if (result0.isValid())
        return result0;

    // Final fallback: unconstrained Level 0 A*
    qDebug() << "All corridor attempts failed, "
                "falling back to unconstrained level 0";
    return aStarAtLevel(start, goal, 0, nullptr,
                        snapped0Start, snapped0Goal);
}

// =============================================================================
// Corridor Construction
// =============================================================================

Corridor HierarchicalVisibilityGraph::buildCorridor(
    const ShortestPathResult& coarsePath,
    int targetLevel,
    double expansion)
{
    Corridor corridor;
    if (coarsePath.points.isEmpty())
        return corridor;

    corridor.minLon = std::numeric_limits<double>::max();
    corridor.maxLon = std::numeric_limits<double>::lowest();
    corridor.minLat = std::numeric_limits<double>::max();
    corridor.maxLat = std::numeric_limits<double>::lowest();

    for (const auto& waypoint : coarsePath.points)
    {
        double lon = waypoint->getLongitude().value();
        double lat = waypoint->getLatitude().value();

        double latRad = lat * M_PI / 180.0;
        double cosLat = std::cos(latRad);
        double lonExpand = (cosLat > 1e-6)
                               ? expansion / (111000.0 * cosLat)
                               : 180.0;
        double latExpand = expansion / 111000.0;

        corridor.minLon = std::min(corridor.minLon, lon - lonExpand);
        corridor.maxLon = std::max(corridor.maxLon, lon + lonExpand);
        corridor.minLat = std::min(corridor.minLat, lat - latExpand);
        corridor.maxLat = std::max(corridor.maxLat, lat + latExpand);
    }

    // Populate allowed vertex indices from target level
    const auto& lvl = mLevels[targetLevel];
    for (int i = 0; i < lvl.vertices.size(); ++i)
    {
        double lon = lvl.vertices[i]->getLongitude().value();
        double lat = lvl.vertices[i]->getLatitude().value();
        if (corridor.containsPoint(lon, lat))
        {
            corridor.allowedVertexIndices.insert(i);
        }
    }

    qDebug() << "Corridor for level" << targetLevel << ":"
             << corridor.allowedVertexIndices.size()
             << "allowed vertices out of" << lvl.vertices.size();

    return corridor;
}

void HierarchicalVisibilityGraph::precomputeCorridorAdjacency(
    Corridor& corridor,
    const std::shared_ptr<GPoint>& start,
    const std::shared_ptr<GPoint>& goal,
    const Corridor* previousCorridor)
{
    const auto& lvl = mLevels[0];

    // Track which vertices are inherited from the previous corridor
    int inheritedCount = 0;

    // If we have a previous corridor, carry forward its vertices and adjacency
    if (previousCorridor && previousCorridor->hasAdjacency)
    {
        // Copy all previous corridor vertices first
        for (int i = 0; i < previousCorridor->vertices.size(); ++i)
        {
            corridor.vertices.append(previousCorridor->vertices[i]);
            corridor.vertexIndex[previousCorridor->vertices[i]] = i;
        }
        inheritedCount = corridor.vertices.size();

        // Copy previous adjacency
        corridor.adjacency.resize(inheritedCount);
        for (int i = 0; i < inheritedCount; ++i)
        {
            corridor.adjacency[i] = previousCorridor->adjacency[i];
        }
    }

    // Add new corridor vertices (not already in the corridor)
    for (int idx : corridor.allowedVertexIndices)
    {
        if (idx < lvl.vertices.size())
        {
            const auto& v = lvl.vertices[idx];
            if (corridor.vertexIndex.find(v) == corridor.vertexIndex.end())
            {
                corridor.vertexIndex[v] = corridor.vertices.size();
                corridor.vertices.append(v);
            }
        }
    }

    // Add start and goal if not already present
    auto addIfMissing = [&](const std::shared_ptr<GPoint>& pt) {
        if (!pt) return;
        auto snapped = snapToWater(pt, 0);
        if (!snapped) return;
        if (corridor.vertexIndex.find(snapped) == corridor.vertexIndex.end())
        {
            corridor.vertexIndex[snapped] = corridor.vertices.size();
            corridor.vertices.append(snapped);
        }
    };
    addIfMissing(start);
    addIfMissing(goal);

    int n = corridor.vertices.size();
    corridor.adjacency.resize(n);

    if (n == 0)
    {
        corridor.hasAdjacency = true;
        return;
    }

    int newCount = n - inheritedCount;
    long long totalPairs = static_cast<long long>(n) * (n - 1) / 2;
    long long inheritedPairs = previousCorridor
        ? static_cast<long long>(inheritedCount) * (inheritedCount - 1) / 2
        : 0;

    qDebug() << "Pre-computing corridor adjacency for" << n
             << "vertices (" << newCount << " new,"
             << inheritedCount << " inherited,"
             << (totalPairs - inheritedPairs) << "new pairs)";

    QElapsedTimer timer;
    timer.start();

    // Only compute visibility for pairs involving at least one new vertex.
    // Old-old pairs are already in the inherited adjacency.
    QVector<int> indices(n);
    std::iota(indices.begin(), indices.end(), 0);

    QMutex adjacencyMutex;

    QtConcurrent::blockingMap(indices, [&](int i) {
        std::vector<int> myNeighbors;

        // For inherited vertices (i < inheritedCount), only check
        // against new vertices (j >= inheritedCount).
        // For new vertices, check against all j > i.
        int jStart = (i < inheritedCount) ? inheritedCount : (i + 1);

        for (int j = jStart; j < n; ++j)
        {
            if (isVisible(corridor.vertices[i], corridor.vertices[j], 0))
            {
                myNeighbors.push_back(j);
            }
        }

        QMutexLocker lock(&adjacencyMutex);
        for (int neighbor : myNeighbors)
        {
            corridor.adjacency[i].push_back(neighbor);
            corridor.adjacency[neighbor].push_back(i);
        }
    });

    corridor.hasAdjacency = true;

    qDebug() << "Corridor adjacency built in" << timer.elapsed() << "ms"
             << "(skipped" << inheritedPairs << "inherited pairs)";
}

// =============================================================================
// Visibility Checking
// =============================================================================

bool HierarchicalVisibilityGraph::isVisible(
    const std::shared_ptr<GPoint>& node1,
    const std::shared_ptr<GPoint>& node2,
    int level) const
{
    if (!node1 || !node2)
        throw std::runtime_error("Point is not valid!\n");

    if (*node1 == *node2)
        return true;

    // Fast haversine distance check to skip short segments without
    // constructing a GLine (avoids 2 geodesic Inverse calls + heap alloc).
    double lat1 = node1->getLatitude().value() * M_PI / 180.0;
    double lat2 = node2->getLatitude().value() * M_PI / 180.0;
    double dLat = lat2 - lat1;
    double dLon = (node2->getLongitude().value() -
                   node1->getLongitude().value()) * M_PI / 180.0;
    double a = std::sin(dLat / 2.0) * std::sin(dLat / 2.0) +
               std::cos(lat1) * std::cos(lat2) *
               std::sin(dLon / 2.0) * std::sin(dLon / 2.0);
    double approxDistMeters = 6371000.0 * 2.0 * std::asin(std::sqrt(a));

    if (approxDistMeters < 1.0)
        return true;

    // Construct GLine only when needed — this is the expensive part
    auto segment = std::make_shared<GLine>(node1, node2);
    return isSegmentVisible(segment, level);
}

bool HierarchicalVisibilityGraph::isSegmentVisible(
    const std::shared_ptr<GLine>& segment,
    int level) const
{
    if (!segment)
        return false;

    // Check manual lines
    if (!manualLinesSet.empty())
    {
        if (manualLinesSet.find(segment) != manualLinesSet.end())
            return true;
    }

    const auto& lvl = mLevels[level];
    if (!lvl.quadtree)
        return false;

    // Handle antimeridian crossing
    if (Quadtree::isSegmentCrossingAntimeridian(segment))
    {
        auto splitSegments =
            Quadtree::splitSegmentAtAntimeridian(segment);
        for (const auto& seg : splitSegments)
        {
            if (!isSegmentVisible(seg, level))
                return false;
        }
        return true;
    }

    // Note: Read lock removed from hot path. Level data is immutable after
    // buildAllLevels(). Write locks in clear()/setPolygons() protect against
    // concurrent structural modifications which should not overlap with queries.

    if (segment->length() < units::length::meter_t(1.0))
        return true;

    // Water polygon validation
    QVector<std::shared_ptr<Polygon>> startPolygons =
        segment->startPoint()->getOwningPolygons();
    QVector<std::shared_ptr<Polygon>> endPolygons =
        segment->endPoint()->getOwningPolygons();

    if (startPolygons.isEmpty())
        startPolygons = findAllContainingPolygons(segment->startPoint());
    if (endPolygons.isEmpty())
        endPolygons = findAllContainingPolygons(segment->endPoint());

    // Find common polygon
    std::shared_ptr<Polygon> commonPolygon = nullptr;
    for (const auto& sp : startPolygons)
    {
        for (const auto& ep : endPolygons)
        {
            if (sp == ep)
            {
                commonPolygon = sp;
                break;
            }
        }
        if (commonPolygon) break;
    }

    if (commonPolygon)
    {
        if (!commonPolygon->isValidWaterSegment(segment))
            return false;
    }
    else
    {
        for (const auto& polygon : lvl.polygons)
        {
            if (!polygon->segmentBoundsIntersect(segment))
                continue;
            if (polygon->segmentCrossesHoles(segment))
                return false;
        }
    }

    // Quadtree edge intersection check
    auto intersectingNodes =
        lvl.quadtree->findNodesIntersectingLineSegmentParallel(segment);

    auto segStart = segment->startPoint();
    auto segEnd = segment->endPoint();
    double startLon = segStart->getLongitude().value();
    double endLon = segEnd->getLongitude().value();
    double startLat = segStart->getLatitude().value();
    double endLat = segEnd->getLatitude().value();
    double segMinLat = std::min(startLat, endLat);
    double segMaxLat = std::max(startLat, endLat);

    // Handle antimeridian split segments
    double segMinLon, segMaxLon;
    double lonDiff = std::abs(endLon - startLon);
    const double ANTI_TOL = 1e-6;
    bool startAtAnti = std::abs(std::abs(startLon) - 180.0) < ANTI_TOL;
    bool endAtAnti = std::abs(std::abs(endLon) - 180.0) < ANTI_TOL;
    bool isAntiSplit = (startAtAnti || endAtAnti) && lonDiff > 90.0;

    if (isAntiSplit)
    {
        double otherLon = startAtAnti ? endLon : startLon;
        if (otherLon < 0)
        {
            segMinLon = -180.0;
            segMaxLon = otherLon;
        }
        else
        {
            segMinLon = otherLon;
            segMaxLon = 180.0;
        }
    }
    else
    {
        segMinLon = std::min(startLon, endLon);
        segMaxLon = std::max(startLon, endLon);
    }

    auto checkEdge = [&](const std::shared_ptr<GLine>& edge) {
        auto edgeStartPt = edge->startPoint();
        auto edgeEndPt = edge->endPoint();
        double eLon1 = edgeStartPt->getLongitude().value();
        double eLat1 = edgeStartPt->getLatitude().value();
        double eLon2 = edgeEndPt->getLongitude().value();
        double eLat2 = edgeEndPt->getLatitude().value();

        if (std::abs(eLon1 - eLon2) > 180.0)
            return false;

        double edgeMinLon = std::min(eLon1, eLon2);
        double edgeMaxLon = std::max(eLon1, eLon2);
        double edgeMinLat = std::min(eLat1, eLat2);
        double edgeMaxLat = std::max(eLat1, eLat2);

        if (edgeMaxLon < segMinLon || edgeMinLon > segMaxLon
            || edgeMaxLat < segMinLat || edgeMinLat > segMaxLat)
            return false;

        const double COORD_TOL = 0.00001;
        auto coordsNear = [COORD_TOL](double lon1, double lat1,
                                       double lon2, double lat2) {
            return std::abs(lon1 - lon2) < COORD_TOL
                && std::abs(lat1 - lat2) < COORD_TOL;
        };

        bool sharesEndpoint =
            coordsNear(eLon1, eLat1, startLon, startLat) ||
            coordsNear(eLon1, eLat1, endLon, endLat) ||
            coordsNear(eLon2, eLat2, startLon, startLat) ||
            coordsNear(eLon2, eLat2, endLon, endLat);

        if (sharesEndpoint)
            return false;

        auto pointOnEdgeFast = [&](double pLon, double pLat) {
            if (pLon < edgeMinLon - COORD_TOL || pLon > edgeMaxLon + COORD_TOL ||
                pLat < edgeMinLat - COORD_TOL || pLat > edgeMaxLat + COORD_TOL)
                return false;

            double dx = eLon2 - eLon1;
            double dy = eLat2 - eLat1;
            double dpx = pLon - eLon1;
            double dpy = pLat - eLat1;
            double cross = dx * dpy - dy * dpx;
            double lenSq = dx * dx + dy * dy;
            return (cross * cross) < (COORD_TOL * COORD_TOL * lenSq * 100.0);
        };

        if (pointOnEdgeFast(startLon, startLat) ||
            pointOnEdgeFast(endLon, endLat))
            return false;

        return segment->intersects(*edge, true);
    };

    if (intersectingNodes.size() > 1000)
    {
        QAtomicInt hasIntersection(0);

        auto processNode = [&](Quadtree::Node* node) {
            if (hasIntersection.loadAcquire())
                return;
            for (const auto& edge :
                 lvl.quadtree->getAllSegmentsInNode(node))
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

    for (auto* node : intersectingNodes)
    {
        for (const auto& edge : lvl.quadtree->getAllSegmentsInNode(node))
        {
            if (checkEdge(edge))
                return false;
        }
    }

    return true;
}

bool HierarchicalVisibilityGraph::isVisibleInSimplifiedPolygon(
    const std::shared_ptr<GPoint>& v1,
    const std::shared_ptr<GPoint>& v2,
    const std::shared_ptr<Polygon>& simplifiedPolygon) const
{
    if (!v1 || !v2 || !simplifiedPolygon)
        return false;

    if (*v1 == *v2)
        return true;

    const auto& outer = simplifiedPolygon->outer();
    int n = outer.size();
    if (n < 3)
        return true;

    double ax = v1->getLongitude().value();
    double ay = v1->getLatitude().value();
    double bx = v2->getLongitude().value();
    double by = v2->getLatitude().value();

    auto sign = [](double v) -> int {
        if (v > 1e-12) return 1;
        if (v < -1e-12) return -1;
        return 0;
    };

    auto cross = [](double ox, double oy, double ax, double ay,
                    double bx, double by) -> double {
        return (ax - ox) * (by - oy) - (ay - oy) * (bx - ox);
    };

    for (int i = 0; i < n; ++i)
    {
        const auto& edgeStart = outer[i];
        const auto& edgeEnd = outer[(i + 1) % n];

        double cx = edgeStart->getLongitude().value();
        double cy = edgeStart->getLatitude().value();
        double dx = edgeEnd->getLongitude().value();
        double dy = edgeEnd->getLatitude().value();

        bool aIsEndpoint = (std::abs(ax - cx) < 1e-9 && std::abs(ay - cy) < 1e-9) ||
                           (std::abs(ax - dx) < 1e-9 && std::abs(ay - dy) < 1e-9);
        bool bIsEndpoint = (std::abs(bx - cx) < 1e-9 && std::abs(by - cy) < 1e-9) ||
                           (std::abs(bx - dx) < 1e-9 && std::abs(by - dy) < 1e-9);

        if (aIsEndpoint || bIsEndpoint)
            continue;

        int d1 = sign(cross(cx, cy, dx, dy, ax, ay));
        int d2 = sign(cross(cx, cy, dx, dy, bx, by));
        int d3 = sign(cross(ax, ay, bx, by, cx, cy));
        int d4 = sign(cross(ax, ay, bx, by, dx, dy));

        if (d1 != d2 && d3 != d4 && d1 != 0 && d2 != 0 && d3 != 0 && d4 != 0)
            return false;
    }

    // Midpoint-in-polygon check
    double midLon = (ax + bx) / 2.0;
    double midLat = (ay + by) / 2.0;
    GPoint midPoint{units::angle::degree_t{midLon},
                    units::angle::degree_t{midLat}};
    if (!simplifiedPolygon->isPointWithinPolygon(midPoint))
        return false;

    return true;
}

// =============================================================================
// Containment
// =============================================================================

std::shared_ptr<Polygon>
HierarchicalVisibilityGraph::findContainingPolygon(
    const std::shared_ptr<GPoint>& point) const
{
    if (!point)
        return nullptr;

    // Use level 0 polygons (original)
    for (const auto& polygon : mLevels[0].polygons)
    {
        if (polygon->isPointWithinPolygon(*point))
            return polygon;
    }
    return nullptr;
}

QVector<std::shared_ptr<Polygon>>
HierarchicalVisibilityGraph::findAllContainingPolygons(
    const std::shared_ptr<GPoint>& point) const
{
    QVector<std::shared_ptr<Polygon>> result;
    if (!point)
        return result;

    double ptLon = point->getLongitude().value();
    double ptLat = point->getLatitude().value();

    for (const auto& polygon : mLevels[0].polygons)
    {
        double minLon, maxLon, minLat, maxLat;
        polygon->getEnvelope(minLon, maxLon, minLat, maxLat);

        if (ptLon < minLon || ptLon > maxLon ||
            ptLat < minLat || ptLat > maxLat)
            continue;

        if (polygon->isPointWithinPolygon(*point)
            || polygon->ringsContain(point))
        {
            result.append(polygon);
        }
    }

    return result;
}

// =============================================================================
// Neighbor Discovery
// =============================================================================

QVector<std::shared_ptr<GPoint>>
HierarchicalVisibilityGraph::getVisibleNodesForPoint(
    const std::shared_ptr<GPoint>& node,
    int level,
    const Corridor* corridor)
{
    QVector<std::shared_ptr<GPoint>> result;
    const auto& lvl = mLevels[level];

    // Fast path: use corridor's pre-computed adjacency (Level 0)
    if (corridor && corridor->hasAdjacency)
    {
        auto cit = corridor->vertexIndex.find(node);
        if (cit != corridor->vertexIndex.end())
        {
            int idx = cit->second;
            if (idx < static_cast<int>(corridor->adjacency.size()))
            {
                for (int neighborIdx : corridor->adjacency[idx])
                {
                    result.append(corridor->vertices[neighborIdx]);
                }
            }
            // Add manual connections and return
            QReadLocker locker(&mManualLock);
            auto mit = manualConnections.find(node);
            if (mit != manualConnections.end())
            {
                result += mit->second;
            }
            return result;
        }
        // Node not in corridor adjacency — fall through to on-demand
    }

    // Check if node is a known vertex with pre-computed adjacency (levels 1-3)
    auto it = lvl.vertexIndex.find(node);
    if (it != lvl.vertexIndex.end() && level > 0)
    {
        int idx = it->second;
        if (idx < static_cast<int>(lvl.adjacency.size()))
        {
            for (int neighborIdx : lvl.adjacency[idx])
            {
                if (corridor &&
                    corridor->allowedVertexIndices.find(neighborIdx) ==
                        corridor->allowedVertexIndices.end())
                {
                    continue;
                }
                result.append(lvl.vertices[neighborIdx]);
            }
        }
    }
    else
    {
        // On-demand visibility (start/goal or unconstrained Level 0)
        auto containingPolygons = findAllContainingPolygons(node);

        if (containingPolygons.isEmpty())
        {
            result = getVisibleNodesBetweenPolygons(node, lvl.polygons);
        }
        else
        {
            for (const auto& poly : containingPolygons)
            {
                auto nodesInPoly = getVisibleNodesWithinPolygon(node, poly);
                for (const auto& n : nodesInPoly)
                {
                    if (corridor)
                    {
                        double lon = n->getLongitude().value();
                        double lat = n->getLatitude().value();
                        if (!corridor->containsPoint(lon, lat))
                            continue;
                    }
                    result.append(n);
                }
            }
        }
    }

    // Add manual connections
    QReadLocker locker(&mManualLock);
    auto mit = manualConnections.find(node);
    if (mit != manualConnections.end())
    {
        result += mit->second;
    }

    return result;
}

QVector<std::shared_ptr<GPoint>>
HierarchicalVisibilityGraph::getVisibleNodesWithinPolygon(
    const std::shared_ptr<GPoint>& node,
    const std::shared_ptr<Polygon>& polygon)
{
    // Check GPoint visibility cache first
    if (node->hasVisibleNeighborsCache(polygon.get()))
    {
        return node->getVisibleNeighborsInPolygon(polygon.get());
    }

    QVector<std::shared_ptr<GPoint>> candidates;

    const auto& outerPoints = polygon->outer();
    std::copy_if(outerPoints.begin(), outerPoints.end(),
                 std::back_inserter(candidates),
                 [&node](const auto& p) { return *p != *node; });

    for (const auto& hole : polygon->inners())
    {
        std::copy_if(hole.begin(), hole.end(),
                     std::back_inserter(candidates),
                     [&node](const auto& p) { return *p != *node; });
    }

    QFuture<std::shared_ptr<GPoint>> future = QtConcurrent::filtered(
        ThreadConfig::getSharedThreadPool(),
        candidates,
        [this, node](const std::shared_ptr<GPoint>& point) {
            return isVisible(node, point, 0);
        });

    QVector<std::shared_ptr<GPoint>> visibleNodes =
        future.results().toVector();

    // Add manual connections
    {
        QReadLocker locker(&mManualLock);
        auto it = manualConnections.find(node);
        if (it != manualConnections.end())
        {
            visibleNodes += it->second;
        }
    }

    node->setVisibleNeighborsInPolygon(polygon.get(), visibleNodes);
    return visibleNodes;
}

QVector<std::shared_ptr<GPoint>>
HierarchicalVisibilityGraph::getVisibleNodesBetweenPolygons(
    const std::shared_ptr<GPoint>& node,
    const QVector<std::shared_ptr<Polygon>>& allPolygons)
{
    QVector<std::shared_ptr<GPoint>> tasks;

    double nodeLon = node->getLongitude().value();
    double nodeLat = node->getLatitude().value();

    for (const auto& polygon : allPolygons)
    {
        double minLon, maxLon, minLat, maxLat;
        polygon->getEnvelope(minLon, maxLon, minLat, maxLat);

        if (nodeLon < minLon || nodeLon > maxLon ||
            nodeLat < minLat || nodeLat > maxLat)
            continue;

        bool isPartOfPolygon = polygon->ringsContain(node)
                               || polygon->isPointWithinPolygon(*node);
        if (!isPartOfPolygon)
            continue;

        const auto outerPoints = polygon->outer();
        std::copy_if(outerPoints.begin(), outerPoints.end(),
                     std::back_inserter(tasks),
                     [&node](const auto& p) { return *p != *node; });

        for (const auto& hole : polygon->inners())
        {
            std::copy_if(hole.begin(), hole.end(),
                         std::back_inserter(tasks),
                         [&node](const auto& p) { return *p != *node; });
        }
    }

    QFuture<std::shared_ptr<GPoint>> future = QtConcurrent::filtered(
        ThreadConfig::getSharedThreadPool(),
        tasks, [this, node](const std::shared_ptr<GPoint>& point) {
            return isVisible(node, point, 0);
        });

    QVector<std::shared_ptr<GPoint>> visibleNodes =
        future.results().toVector();

    QReadLocker locker(&mManualLock);
    auto it = manualConnections.find(node);
    if (it != manualConnections.end())
    {
        visibleNodes += it->second;
    }

    return visibleNodes;
}

// =============================================================================
// Path Reconstruction
// =============================================================================

ShortestPathResult HierarchicalVisibilityGraph::reconstructPath(
    const std::unordered_map<
        std::shared_ptr<GPoint>, std::shared_ptr<GPoint>,
        GPoint::Hash, GPoint::Equal>& cameFrom,
    std::shared_ptr<GPoint> current,
    int level)
{
    ShortestPathResult result;
    const auto& lvl = mLevels[level];

    // Build path in reverse (append is O(1)), then reverse at the end
    // instead of prepend which is O(n) per call.
    while (cameFrom.find(current) != cameFrom.end())
    {
        result.points.append(current);
        std::shared_ptr<GPoint> next = cameFrom.at(current);

        std::shared_ptr<GLine> lineSegment;
        if (lvl.quadtree)
            lineSegment = lvl.quadtree->findLineSegment(next, current);

        if (lineSegment)
        {
            result.lines.append(lineSegment);
        }
        else
        {
            result.lines.append(
                std::make_shared<GLine>(next, current));
        }
        current = next;
    }
    result.points.append(current);

    // Reverse both to get correct order — O(n) total
    std::reverse(result.points.begin(), result.points.end());
    std::reverse(result.lines.begin(), result.lines.end());

    return result;
}

// =============================================================================
// Multi-Segment Helper
// =============================================================================

ShortestPathResult HierarchicalVisibilityGraph::findShortestPathHelper(
    QVector<std::shared_ptr<GPoint>> mustTraversePoints)
{
    ShortestPathResult result;

    if (mustTraversePoints.size() < 2)
    {
        if (!mustTraversePoints.isEmpty() && mustTraversePoints.first())
            result.points.append(mustTraversePoints.first());
        return result;
    }

    for (const auto& point : mustTraversePoints)
    {
        if (!point)
        {
            qWarning() << "Null point in path points list";
            return result;
        }
    }

    QElapsedTimer progressTimer;
    progressTimer.start();
    int totalSegments = mustTraversePoints.size() - 1;

    emit pathFindingProgress(0, totalSegments,
                             progressTimer.elapsed() / 1000.0);

    result.points.append(mustTraversePoints.first());

    for (qsizetype i = 0; i < mustTraversePoints.size() - 1; ++i)
    {
        auto startPoint = mustTraversePoints[i];
        auto endPoint = mustTraversePoints[i + 1];

        try
        {
            auto l = std::make_shared<GLine>(startPoint, endPoint);

            if (isSegmentVisible(l, 0))
            {
                // Deduplicate at segment boundaries: only check last point
                // since waypoints are processed sequentially
                if (result.points.isEmpty() ||
                    *(result.points.last()) != *startPoint)
                {
                    result.points.push_back(startPoint);
                }

                if (result.points.isEmpty() ||
                    *(result.points.last()) != *endPoint)
                {
                    result.points.push_back(endPoint);
                }

                result.lines.push_back(l);
            }
            else
            {
                ShortestPathResult twoPointResult =
                    hierarchicalSearch(startPoint, endPoint);

                for (qsizetype j = 1;
                     j < twoPointResult.points.size(); ++j)
                {
                    result.points.append(twoPointResult.points[j]);
                }

                for (auto& line : twoPointResult.lines)
                {
                    result.lines.append(line);
                }
            }
        }
        catch (const std::exception& e)
        {
            qWarning() << "Exception creating line segment:" << e.what();
            continue;
        }

        emit pathFindingProgress(static_cast<int>(i + 1), totalSegments,
                                 progressTimer.elapsed() / 1000.0);
    }

    return result;
}

// =============================================================================
// Wrap-Around / Antimeridian
// =============================================================================

bool HierarchicalVisibilityGraph::shouldCrossAntimeridian(
    double startLon, double goalLon)
{
    double directDiff = std::abs(goalLon - startLon);
    return directDiff > 180.0;
}

QVector<std::shared_ptr<GPoint>>
HierarchicalVisibilityGraph::connectWrapAroundPoints(
    const std::shared_ptr<GPoint>& point,
    const std::shared_ptr<GPoint>& goalPoint)
{
    QVector<std::shared_ptr<GPoint>> wrapAroundPoints;

    const auto& lvl = mLevels[0];
    GPoint mapMin, mapMax;
    double mapWidth;
    bool nearBoundary = false;

    {
        QReadLocker locker(&lvl.lock);
        if (!lvl.quadtree)
            return wrapAroundPoints;

        nearBoundary = lvl.quadtree->isNearBoundary(point);
        mapMin = lvl.quadtree->getMapMinPoint();
        mapMax = lvl.quadtree->getMapMaxPoint();
        mapWidth = (mapMax.getLongitude() - mapMin.getLongitude()).value();
    }

    double pointLon = point->getLongitude().value();

    // Goal-aware antimeridian crossing
    if (goalPoint)
    {
        double goalLon = goalPoint->getLongitude().value();

        if (shouldCrossAntimeridian(pointLon, goalLon))
        {
            double targetLon = (pointLon > 0) ? 180.0 : -180.0;
            double zoneLon = (targetLon > 0)
                                 ? 180.0 - PORTAL_ZONE_DEGREES
                                 : -180.0;
            double pointLat = point->getLatitude().value();
            double latRange = PORTAL_LAT_TOLERANCE * 2;
            QRectF portalZone(zoneLon, pointLat - latRange,
                              PORTAL_ZONE_DEGREES, latRange * 2);

            auto portalVertices = lvl.quadtree->findVerticesInRange(portalZone);

            for (const auto& portalVertex : portalVertices)
            {
                if (isVisible(point, portalVertex, 0))
                {
                    if (!wrapAroundPoints.contains(portalVertex))
                        wrapAroundPoints.append(portalVertex);
                }
            }
        }
    }

    if (!nearBoundary)
        return wrapAroundPoints;

    auto createMirrorPoint = [&](double offset) {
        double newLon = pointLon + offset;
        return std::make_shared<GPoint>(
            units::angle::degree_t(newLon), point->getLatitude());
    };

    QVector<std::shared_ptr<GPoint>> mirrorPoints;

    if ((mapMax.getLongitude() - point->getLongitude()).value() < 1.0)
    {
        mirrorPoints.append(createMirrorPoint(-mapWidth));
    }
    else if ((point->getLongitude() - mapMin.getLongitude()).value() < 1.0)
    {
        mirrorPoints.append(createMirrorPoint(mapWidth));
    }

    QVector<std::shared_ptr<GPoint>> allVisible;
    for (const auto& wrappedPoint : mirrorPoints)
    {
        QVector<std::shared_ptr<GPoint>> wrappedVisible;
        if (auto poly = findContainingPolygon(point))
        {
            wrappedVisible = getVisibleNodesWithinPolygon(wrappedPoint, poly);
        }
        else
        {
            wrappedVisible = getVisibleNodesBetweenPolygons(
                wrappedPoint, lvl.polygons);
        }

        for (auto& p : wrappedVisible)
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

    for (const auto& candidate : allVisible)
    {
        auto wrapSegment = std::make_shared<GLine>(point, candidate);
        if (isSegmentVisible(wrapSegment, 0))
        {
            if (!wrapAroundPoints.contains(candidate))
                wrapAroundPoints.append(candidate);
        }
    }

    return wrapAroundPoints;
}

// =============================================================================
// Manual Lines, SeaPorts, Clear, SetPolygons
// =============================================================================

void HierarchicalVisibilityGraph::addManualVisibleLine(
    const std::shared_ptr<GLine>& line)
{
    QWriteLocker locker(&mManualLock);

    manualLinesSet.insert(line);
    manualConnections[line->startPoint()].append(line->endPoint());
    manualConnections[line->endPoint()].append(line->startPoint());

    if (manualPointsSet.insert(line->startPoint()).second)
        manualPoints.append(line->startPoint());
    if (manualPointsSet.insert(line->endPoint()).second)
        manualPoints.append(line->endPoint());
}

void HierarchicalVisibilityGraph::clearManualLines()
{
    QWriteLocker locker(&mManualLock);
    manualLinesSet.clear();
    manualConnections.clear();
    manualPoints.clear();
    manualPointsSet.clear();
}

void HierarchicalVisibilityGraph::loadSeaPortsPolygonCoordinates(
    QVector<std::shared_ptr<SeaPort>>& seaPorts)
{
    const auto& lvl = mLevels[0];
    if (!lvl.quadtree)
    {
        qWarning() << "loadSeaPortsPolygonCoordinates: Quadtree not initialized";
        return;
    }
    for (auto& seaPort : seaPorts)
    {
        std::shared_ptr<GPoint> portCoord =
            std::make_shared<GPoint>(seaPort->getPortCoordinate());
        seaPort->setClosestPointOnWaterPolygon(
            lvl.quadtree->findNearestNeighborPoint(portCoord));
    }
}

GPoint HierarchicalVisibilityGraph::getMinMapPoint()
{
    const auto& lvl = mLevels[0];
    if (!lvl.quadtree)
    {
        qWarning() << "getMinMapPoint: Quadtree not initialized";
        return GPoint();
    }
    return lvl.quadtree->getMapMinPoint();
}

GPoint HierarchicalVisibilityGraph::getMaxMapPoint()
{
    const auto& lvl = mLevels[0];
    if (!lvl.quadtree)
    {
        qWarning() << "getMaxMapPoint: Quadtree not initialized";
        return GPoint();
    }
    return lvl.quadtree->getMapMaxPoint();
}

void HierarchicalVisibilityGraph::clear()
{
    // Clear vertex ownership and visibility cache
    for (const auto& polygon : polygons)
    {
        if (!polygon) continue;
        for (const auto& vertex : polygon->outer())
        {
            if (vertex)
            {
                vertex->clearOwningPolygons();
                vertex->clearVisibleNeighborsCache();
            }
        }
        for (const auto& hole : polygon->inners())
        {
            for (const auto& vertex : hole)
            {
                if (vertex)
                {
                    vertex->clearOwningPolygons();
                    vertex->clearVisibleNeighborsCache();
                }
            }
        }
    }
    polygons.clear();

    // Clear manual lines
    {
        QWriteLocker locker(&mManualLock);
        manualLinesSet.clear();
        manualConnections.clear();
        manualPoints.clear();
        manualPointsSet.clear();
    }

    // Clear all levels
    for (int i = 0; i < NUM_LEVELS; ++i)
    {
        auto& lvl = mLevels[i];
        QWriteLocker locker(&lvl.lock);
        if (lvl.quadtree)
            lvl.quadtree->clearTree();
        lvl.polygons.clear();
        lvl.vertices.clear();
        lvl.adjacency.clear();
        lvl.vertexIndex.clear();
        lvl.vertexPolygonId.clear();
    }
}

void HierarchicalVisibilityGraph::setPolygons(
    const QVector<std::shared_ptr<Polygon>>& newPolygons)
{
    // Clear ownership from old vertices
    for (const auto& polygon : polygons)
    {
        if (!polygon) continue;
        for (const auto& vertex : polygon->outer())
        {
            if (vertex)
            {
                vertex->clearOwningPolygons();
                vertex->clearVisibleNeighborsCache();
            }
        }
        for (const auto& hole : polygon->inners())
        {
            for (const auto& vertex : hole)
            {
                if (vertex)
                {
                    vertex->clearOwningPolygons();
                    vertex->clearVisibleNeighborsCache();
                }
            }
        }
    }

    polygons = newPolygons;

    // Set ownership on new vertices
    for (const auto& polygon : polygons)
    {
        if (!polygon) continue;
        for (const auto& vertex : polygon->outer())
        {
            if (vertex)
                vertex->addOwningPolygon(polygon);
        }
        for (const auto& hole : polygon->inners())
        {
            for (const auto& vertex : hole)
            {
                if (vertex)
                    vertex->addOwningPolygon(polygon);
            }
        }
    }

    // Rebuild all levels
    for (int i = 0; i < NUM_LEVELS; ++i)
    {
        auto& lvl = mLevels[i];
        QWriteLocker locker(&lvl.lock);
        if (lvl.quadtree)
            lvl.quadtree->clearTree();
        lvl.polygons.clear();
        lvl.vertices.clear();
        lvl.adjacency.clear();
        lvl.vertexIndex.clear();
        lvl.vertexPolygonId.clear();
    }

    buildAllLevels();
}

// =============================================================================
// Public API
// =============================================================================

ShortestPathResult HierarchicalVisibilityGraph::findShortestPath(
    const std::shared_ptr<GPoint>& start,
    const std::shared_ptr<GPoint>& goal)
{
    return hierarchicalSearch(start, goal);
}

ShortestPathResult HierarchicalVisibilityGraph::findShortestPath(
    QVector<std::shared_ptr<GPoint>> mustTraversePoints)
{
    return findShortestPathHelper(mustTraversePoints);
}

Quadtree* HierarchicalVisibilityGraph::getLevel0Quadtree() const
{
    return mLevels[0].quadtree.get();
}

}; // namespace ShipNetSimCore
