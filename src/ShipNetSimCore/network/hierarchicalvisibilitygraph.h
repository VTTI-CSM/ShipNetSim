#ifndef HIERARCHICALVISIBILITYGRAPH_H
#define HIERARCHICALVISIBILITYGRAPH_H

#include "gline.h"
#include "polygon.h"
#include "quadtree.h"
#include "seaport.h"
#include <QHash>
#include <QReadWriteLock>
#include <QVector>
#include <QtConcurrent>
#include <array>
#include <unordered_map>
#include <unordered_set>

namespace ShipNetSimCore
{

struct ShortestPathResult
{
    QVector<std::shared_ptr<GLine>>  lines;
    QVector<std::shared_ptr<GPoint>> points;

    bool isValid() const;
};

struct GraphLevel
{
    int    levelIndex;
    double toleranceMeters;
    QVector<std::shared_ptr<Polygon>> polygons;
    std::unique_ptr<Quadtree> quadtree;
    QVector<std::shared_ptr<GPoint>> vertices;
    std::vector<std::vector<int>> adjacency;
    std::unordered_map<std::shared_ptr<GPoint>, int,
        GPoint::Hash, GPoint::Equal> vertexIndex;
    std::vector<int> vertexPolygonId;
    mutable QReadWriteLock lock;
};

struct Corridor
{
    double minLon, maxLon, minLat, maxLat;
    std::unordered_set<int> allowedVertexIndices;

    // Pre-computed adjacency within this corridor (for Level 0)
    QVector<std::shared_ptr<GPoint>> vertices;
    std::vector<std::vector<int>> adjacency;
    std::unordered_map<std::shared_ptr<GPoint>, int,
        GPoint::Hash, GPoint::Equal> vertexIndex;
    bool hasAdjacency = false;

    bool containsPoint(double lon, double lat) const
    {
        return lon >= minLon && lon <= maxLon &&
               lat >= minLat && lat <= maxLat;
    }
};

class HierarchicalVisibilityGraph : public QObject
{
    Q_OBJECT

public:
    static constexpr int NUM_LEVELS = 4;
    static constexpr double LEVEL_TOLERANCES[NUM_LEVELS] = {
        0.0, 2000.0, 10000.0, 50000.0
    };

    HierarchicalVisibilityGraph();

    HierarchicalVisibilityGraph(
        const QVector<std::shared_ptr<Polygon>>& polygons);

    ~HierarchicalVisibilityGraph();

    ShortestPathResult
    findShortestPath(const std::shared_ptr<GPoint>& start,
                     const std::shared_ptr<GPoint>& goal);

    ShortestPathResult
    findShortestPath(QVector<std::shared_ptr<GPoint>> mustTraversePoints);

    void loadSeaPortsPolygonCoordinates(
        QVector<std::shared_ptr<SeaPort>>& seaPorts);

    void addManualVisibleLine(const std::shared_ptr<GLine>& line);
    void clearManualLines();

    GPoint getMinMapPoint();
    GPoint getMaxMapPoint();

    bool isVisible(const std::shared_ptr<GPoint>& p1,
                   const std::shared_ptr<GPoint>& p2,
                   int level = 0) const;

    bool isSegmentVisible(const std::shared_ptr<GLine>& segment,
                          int level = 0) const;

    void clear();

    void setPolygons(const QVector<std::shared_ptr<Polygon>>& newPolygons);

    Quadtree* getLevel0Quadtree() const;

    bool enableWrapAround;

    std::unordered_set<std::shared_ptr<GLine>, GLine::Hash, GLine::Equal>
        manualLinesSet;
    std::unordered_map<std::shared_ptr<GPoint>,
        QVector<std::shared_ptr<GPoint>>,
        GPoint::Hash, GPoint::Equal> manualConnections;
    QVector<std::shared_ptr<GPoint>> manualPoints;
    std::unordered_set<std::shared_ptr<GPoint>,
        GPoint::Hash, GPoint::Equal> manualPointsSet;

    QVector<std::shared_ptr<Polygon>> polygons;

    std::shared_ptr<Polygon>
    findContainingPolygon(const std::shared_ptr<GPoint>& point) const;

    QVector<std::shared_ptr<Polygon>>
    findAllContainingPolygons(const std::shared_ptr<GPoint>& point) const;

    QVector<std::shared_ptr<GPoint>>
    connectWrapAroundPoints(
        const std::shared_ptr<GPoint>& point,
        const std::shared_ptr<GPoint>& goalPoint = nullptr);

    static bool shouldCrossAntimeridian(double startLon, double goalLon);

signals:
    void pathFindingProgress(int segmentIndex, int totalSegments,
                             double elapsedSeconds);

private:
    std::array<GraphLevel, NUM_LEVELS> mLevels;

    mutable QReadWriteLock mManualLock;

    static constexpr double PORTAL_ZONE_DEGREES = 30.0;
    static constexpr double PORTAL_LAT_TOLERANCE = 10.0;

    void buildAllLevels();
    void buildLevel(int idx);
    void buildAdjacencyForLevel(int idx);

    std::shared_ptr<GPoint> snapToWater(
        const std::shared_ptr<GPoint>& point, int level) const;

    ShortestPathResult aStarAtLevel(
        const std::shared_ptr<GPoint>& start,
        const std::shared_ptr<GPoint>& goal,
        int level,
        const Corridor* corridor = nullptr,
        const std::shared_ptr<GPoint>& preSnappedStart = nullptr,
        const std::shared_ptr<GPoint>& preSnappedGoal = nullptr);

    Corridor buildCorridor(
        const ShortestPathResult& coarsePath,
        int targetLevel,
        double expansion);

    void precomputeCorridorAdjacency(
        Corridor& corridor,
        const std::shared_ptr<GPoint>& start,
        const std::shared_ptr<GPoint>& goal,
        const Corridor* previousCorridor = nullptr);

    ShortestPathResult hierarchicalSearch(
        const std::shared_ptr<GPoint>& start,
        const std::shared_ptr<GPoint>& goal);

    ShortestPathResult findShortestPathHelper(
        QVector<std::shared_ptr<GPoint>> mustTraversePoints);

    bool isVisibleInSimplifiedPolygon(
        const std::shared_ptr<GPoint>& v1,
        const std::shared_ptr<GPoint>& v2,
        const std::shared_ptr<Polygon>& poly) const;

    ShortestPathResult reconstructPath(
        const std::unordered_map<
            std::shared_ptr<GPoint>, std::shared_ptr<GPoint>,
            GPoint::Hash, GPoint::Equal>& cameFrom,
        std::shared_ptr<GPoint> current,
        int level);

    QVector<std::shared_ptr<GPoint>> getVisibleNodesForPoint(
        const std::shared_ptr<GPoint>& node,
        int level,
        const Corridor* corridor = nullptr);

    QVector<std::shared_ptr<GPoint>>
    getVisibleNodesWithinPolygon(
        const std::shared_ptr<GPoint>& node,
        const std::shared_ptr<Polygon>& polygon);

    QVector<std::shared_ptr<GPoint>>
    getVisibleNodesBetweenPolygons(
        const std::shared_ptr<GPoint>& node,
        const QVector<std::shared_ptr<Polygon>>& allPolygons);
};

}; // namespace ShipNetSimCore
#endif // HIERARCHICALVISIBILITYGRAPH_H
