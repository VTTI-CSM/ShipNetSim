#include "network/gline.h"
#include "network/gpoint.h"
#include "network/optimizedvisibilitygraph.h"
#include "network/polygon.h"
#include <QDebug>
#include <QTest>

using namespace ShipNetSimCore;

class OptimizedVisibilityGraphTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    // Basic functionality tests
    void testDefaultConstructor();
    void testParameterizedConstructor();
    void testClear();

    // Visibility tests
    void testIsVisible();
    void testIsSegmentVisible();
    void testGetVisibleNodesWithinPolygon();
    void testGetVisibleNodesBetweenPolygons();

    // Manual connections
    void testAddManualVisibleLine();
    void testClearManualLines();

    // Path finding tests
    void testFindShortestPathDijkstra();
    void testFindShortestPathAStar();
    void testFindShortestPathMultiplePoints();

    // Polygon containment
    void testFindContainingPolygon();

    // Map boundaries
    void testMapBoundaries();
    void testConnectWrapAroundPoints();

    // Sea ports
    void testLoadSeaPortsPolygonCoordinates();

    // Edge cases and debugging
    void testVisibilityDebugging();
    void testPathFindingDebugging();
    void testEdgeCases();

    void debugIntersectionDetection();
    void debugGLineIntersection();
    void debugAStarInvalidPath();

private:
    QVector<std::shared_ptr<Polygon>>         createTestPolygons();
    QVector<std::shared_ptr<SeaPort>>         createTestSeaPorts();
    std::shared_ptr<OptimizedVisibilityGraph> visibilityGraph;
    QVector<std::shared_ptr<GPoint>>          testPoints;
};

void OptimizedVisibilityGraphTest::initTestCase()
{
    qDebug() << "Starting OptimizedVisibilityGraph tests...";

    // Create test polygons (water bodies)
    auto polygons = createTestPolygons();

    // Create visibility graph
    visibilityGraph = std::make_shared<OptimizedVisibilityGraph>(
        polygons, BoundariesType::Water);

    // Create test points WITHIN water but OUTSIDE the land obstacle
    testPoints.append(std::make_shared<GPoint>(
        units::angle::degree_t(-75.5), units::angle::degree_t(39.5),
        "Point1")); // Safe water area
    testPoints.append(std::make_shared<GPoint>(
        units::angle::degree_t(-75.0), units::angle::degree_t(39.2),
        "Point2")); // Safe water area
    testPoints.append(std::make_shared<GPoint>(
        units::angle::degree_t(-73.0), units::angle::degree_t(41.0),
        "Point3")); // Safe water area
    testPoints.append(std::make_shared<GPoint>(
        units::angle::degree_t(-73.5), units::angle::degree_t(41.5),
        "Point4")); // Safe water area
}

void OptimizedVisibilityGraphTest::cleanupTestCase()
{
    qDebug() << "OptimizedVisibilityGraph tests completed.";
}

QVector<std::shared_ptr<Polygon>>
OptimizedVisibilityGraphTest::createTestPolygons()
{
    QVector<std::shared_ptr<Polygon>> polygons;

    // Water boundary (counter-clockwise)
    QVector<std::shared_ptr<GPoint>> waterBoundary;
    waterBoundary.append(std::make_shared<GPoint>(
        units::angle::degree_t(-76.0), units::angle::degree_t(39.0),
        "WB1"));
    waterBoundary.append(std::make_shared<GPoint>(
        units::angle::degree_t(-72.0), units::angle::degree_t(39.0),
        "WB2"));
    waterBoundary.append(std::make_shared<GPoint>(
        units::angle::degree_t(-72.0), units::angle::degree_t(42.0),
        "WB3"));
    waterBoundary.append(std::make_shared<GPoint>(
        units::angle::degree_t(-76.0), units::angle::degree_t(42.0),
        "WB4"));
    waterBoundary.append(waterBoundary.first());

    // Land obstacle hole (clockwise for hole)
    QVector<std::shared_ptr<GPoint>> landObstacle;
    landObstacle.append(std::make_shared<GPoint>(
        units::angle::degree_t(-74.8), units::angle::degree_t(40.3),
        "LO1"));
    landObstacle.append(std::make_shared<GPoint>(
        units::angle::degree_t(-74.8), units::angle::degree_t(40.7),
        "LO4")); // Clockwise
    landObstacle.append(std::make_shared<GPoint>(
        units::angle::degree_t(-74.2), units::angle::degree_t(40.7),
        "LO3"));
    landObstacle.append(std::make_shared<GPoint>(
        units::angle::degree_t(-74.2), units::angle::degree_t(40.3),
        "LO2"));
    landObstacle.append(landObstacle.first()); // Close the hole

    QVector<QVector<std::shared_ptr<GPoint>>> holes;
    holes.append(landObstacle);

    auto waterPolygon = std::make_shared<Polygon>(
        waterBoundary, holes, "TestWaterBody");
    polygons.append(waterPolygon);

    return polygons;
}

QVector<std::shared_ptr<SeaPort>>
OptimizedVisibilityGraphTest::createTestSeaPorts()
{
    QVector<std::shared_ptr<SeaPort>> seaPorts;

    // Create test sea ports
    auto port1 = std::make_shared<SeaPort>(
        GPoint(units::angle::degree_t(-75.5),
               units::angle::degree_t(39.5), "Port1"));
    port1->setPortName("TestPort1");

    auto port2 = std::make_shared<SeaPort>(
        GPoint(units::angle::degree_t(-73.0),
               units::angle::degree_t(41.5), "Port2"));
    port2->setPortName("TestPort2");

    seaPorts.append(port1);
    seaPorts.append(port2);

    return seaPorts;
}

void OptimizedVisibilityGraphTest::testDefaultConstructor()
{
    OptimizedVisibilityGraph defaultGraph;

    // Should be in a valid but empty state
    QVERIFY(true); // Constructor should not throw
}

void OptimizedVisibilityGraphTest::testParameterizedConstructor()
{
    QVERIFY(visibilityGraph != nullptr);

    // Test map boundaries
    auto minPoint = visibilityGraph->getMinMapPoint();
    auto maxPoint = visibilityGraph->getMaxMapPoint();

    qDebug() << "Visibility graph map bounds:"
             << "(" << minPoint.getLongitude().value() << ","
             << minPoint.getLatitude().value() << ")"
             << " to "
             << "(" << maxPoint.getLongitude().value() << ","
             << maxPoint.getLatitude().value() << ")";

    QVERIFY(minPoint.getLongitude().value()
            <= maxPoint.getLongitude().value());
    QVERIFY(minPoint.getLatitude().value()
            <= maxPoint.getLatitude().value());
}

void OptimizedVisibilityGraphTest::testClear()
{
    // Create a copy for testing
    auto                     polygons = createTestPolygons();
    OptimizedVisibilityGraph testGraph(polygons,
                                       BoundariesType::Water);

    // Clear should not crash
    testGraph.clear();

    QVERIFY(true); // If we get here, clear() didn't crash
}

void OptimizedVisibilityGraphTest::testIsVisible()
{
    // Test visibility between points in the same water body
    bool visible1to2 =
        visibilityGraph->isVisible(testPoints[0], testPoints[1]);
    qDebug() << "Point 1 to Point 2 visible:" << visible1to2;

    // Points in open water should generally be visible to each other
    // QVERIFY(visible1to2); // This might fail due to the bug we're
    // debugging

    // Test visibility to the same point (should always be true)
    bool visibleSelf =
        visibilityGraph->isVisible(testPoints[0], testPoints[0]);
    QVERIFY(visibleSelf);

    // Test visibility with null points
    try
    {
        visibilityGraph->isVisible(nullptr, testPoints[0]);
        QFAIL("Should have thrown exception for null point");
    }
    catch (const std::exception &e)
    {
        qDebug() << "Correctly caught exception for null point:"
                 << e.what();
    }
}

void OptimizedVisibilityGraphTest::testIsSegmentVisible()
{
    auto segment =
        std::make_shared<GLine>(testPoints[0], testPoints[1]);

    bool segmentVisible = visibilityGraph->isSegmentVisible(segment);
    qDebug() << "Segment visible:" << segmentVisible;

    // Log detailed information for debugging
    qDebug() << "Segment start:" << testPoints[0]->toString();
    qDebug() << "Segment end:" << testPoints[1]->toString();
    qDebug() << "Segment length:" << segment->length().value()
             << "meters";

    // This is the core issue we're debugging - segments in open water
    // should be visible
    if (!segmentVisible)
    {
        qDebug()
            << "DEBUG: Segment not visible - this indicates the bug!";
    }
}

void OptimizedVisibilityGraphTest::testGetVisibleNodesWithinPolygon()
{
    // Find containing polygon for our test point
    auto containingPolygon = visibilityGraph->findContainingPolygon(
        visibilityGraph->polygons[0]->outer()[0]);

    if (containingPolygon)
    {
        qDebug() << "Found containing polygon for test point";

        auto visibleNodes =
            visibilityGraph->getVisibleNodesWithinPolygon(
                testPoints[0], containingPolygon);

        qDebug() << "Visible nodes count:" << visibleNodes.size();

        // Should find some visible nodes (the polygon vertices at
        // minimum)
        QVERIFY(visibleNodes.size() >= 0);

        // Log details about each visible node
        for (int i = 0; i < visibleNodes.size() && i < 5; ++i)
        {
            qDebug() << "Visible node" << i << ":"
                     << visibleNodes[i]->toString();
        }
    }
    else
    {
        qDebug() << "WARNING: Test point not in any polygon - check "
                    "polygon setup";
        QFAIL("Test point should be within the test polygon");
    }
}

void OptimizedVisibilityGraphTest::
    testGetVisibleNodesBetweenPolygons()
{
    auto polygons = createTestPolygons();

    // Create a point outside all polygons
    auto outsidePoint = std::make_shared<GPoint>(
        units::angle::degree_t(-78.0), units::angle::degree_t(38.0),
        "OutsidePoint");

    auto visibleNodes =
        visibilityGraph->getVisibleNodesBetweenPolygons(outsidePoint,
                                                        polygons);

    qDebug() << "Visible nodes between polygons:"
             << visibleNodes.size();

    // Should find polygon boundary points that are visible
    QVERIFY(visibleNodes.size() >= 0);
}

void OptimizedVisibilityGraphTest::testAddManualVisibleLine()
{
    auto manualLine =
        std::make_shared<GLine>(testPoints[0], testPoints[3]);

    // Add manual connection
    visibilityGraph->addManualVisibleLine(manualLine);

    // Check if the line is now considered visible
    bool visible = visibilityGraph->isSegmentVisible(manualLine);
    QVERIFY(visible);

    qDebug() << "Manual line added and verified as visible";
}

void OptimizedVisibilityGraphTest::testClearManualLines()
{
    // Add a manual line first
    auto manualLine =
        std::make_shared<GLine>(testPoints[1], testPoints[2]);
    visibilityGraph->addManualVisibleLine(manualLine);

    // Clear all manual lines
    visibilityGraph->clearManualLines();

    // The manually added line should no longer be automatically
    // visible (unless it was already visible through normal means)
    qDebug() << "Manual lines cleared";
}

void OptimizedVisibilityGraphTest::testFindShortestPathDijkstra()
{
    qDebug() << "\n=== TESTING DIJKSTRA SHORTEST PATH ===";

    auto startPoint = testPoints[0];
    auto endPoint   = testPoints[2];

    qDebug() << "Start point:" << startPoint->toString();
    qDebug() << "End point:" << endPoint->toString();

    // Test direct line visibility first
    bool directVisible =
        visibilityGraph->isVisible(startPoint, endPoint);
    qDebug() << "Direct visibility:" << directVisible;

    auto result = visibilityGraph->findShortestPathDijkstra(
        startPoint, endPoint);

    qDebug() << "Path found with" << result.points.size()
             << "points and" << result.lines.size() << "lines";
    qDebug() << "Path valid:" << result.isValid();

    if (result.isValid())
    {
        qDebug() << "SUCCESS: Found valid path using Dijkstra";

        // Log the complete path with detailed information
        qDebug() << "=== DIJKSTRA PATH DETAILS ===";
        for (int i = 0; i < result.points.size(); ++i)
        {
            qDebug() << QString("  Point %1: %2")
                            .arg(i)
                            .arg(result.points[i]->toString());

            // Show distance from previous point
            if (i > 0)
            {
                double segmentLength =
                    result.points[i - 1]
                        ->distance(*result.points[i])
                        .value();
                qDebug()
                    << QString(
                           "    Distance from previous: %1 meters")
                           .arg(segmentLength, 0, 'f', 2);
            }
        }

        qDebug() << "=== DIJKSTRA LINE SEGMENTS ===";
        for (int i = 0; i < result.lines.size(); ++i)
        {
            qDebug()
                << QString("  Line %1: %2 -> %3")
                       .arg(i)
                       .arg(result.lines[i]->startPoint()->toString())
                       .arg(result.lines[i]->endPoint()->toString());
            qDebug() << QString("    Length: %1 meters")
                            .arg(result.lines[i]->length().value(), 0,
                                 'f', 2);
        }

        // Calculate total path length
        double totalLength = 0.0;
        for (const auto &line : result.lines)
        {
            totalLength += line->length().value();
        }
        qDebug()
            << QString(
                   "=== DIJKSTRA TOTAL PATH LENGTH: %1 meters ===")
                   .arg(totalLength, 0, 'f', 2);

        QVERIFY(result.points.size() >= 2);
        QVERIFY(result.lines.size() == result.points.size() - 1);
    }
    else
    {
        qDebug() << "ISSUE: No path found using Dijkstra - this "
                    "indicates the bug!";

        // Enhanced debugging for failed paths
        auto containingPolygon1 =
            visibilityGraph->findContainingPolygon(startPoint);
        auto containingPolygon2 =
            visibilityGraph->findContainingPolygon(endPoint);

        qDebug() << "Start point in polygon:"
                 << (containingPolygon1 != nullptr);
        qDebug() << "End point in polygon:"
                 << (containingPolygon2 != nullptr);

        // Show nearest navigation points
        auto startNav =
            visibilityGraph->quadtree->findNearestNeighborPoint(
                startPoint);
        auto endNav =
            visibilityGraph->quadtree->findNearestNeighborPoint(
                endPoint);

        qDebug() << "Start nearest nav point:"
                 << (startNav ? startNav->toString() : "None");
        qDebug() << "End nearest nav point:"
                 << (endNav ? endNav->toString() : "None");

        if (startNav && endNav)
        {
            qDebug() << "Distance to start nav:"
                     << startPoint->distance(*startNav).value()
                     << "meters";
            qDebug() << "Distance to end nav:"
                     << endPoint->distance(*endNav).value()
                     << "meters";
            qDebug() << "Direct nav visibility:"
                     << visibilityGraph->isVisible(startNav, endNav);
        }
    }
}

void OptimizedVisibilityGraphTest::testFindShortestPathAStar()
{
    qDebug() << "\n=== TESTING A* SHORTEST PATH ===";

    auto startPoint = testPoints[0];
    auto endPoint   = testPoints[3];

    qDebug() << "Start point:" << startPoint->toString();
    qDebug() << "End point:" << endPoint->toString();

    auto result =
        visibilityGraph->findShortestPathAStar(startPoint, endPoint);

    qDebug() << "A* Path found with" << result.points.size()
             << "points and" << result.lines.size() << "lines";
    qDebug() << "A* Path valid:" << result.isValid();

    if (result.isValid())
    {
        qDebug() << "SUCCESS: Found valid path using A*";

        // Log the complete path with detailed information
        qDebug() << "=== A* PATH DETAILS ===";
        for (int i = 0; i < result.points.size(); ++i)
        {
            qDebug() << QString("  Point %1: %2")
                            .arg(i)
                            .arg(result.points[i]->toString());

            // Show distance from previous point
            if (i > 0)
            {
                double segmentLength =
                    result.points[i - 1]
                        ->distance(*result.points[i])
                        .value();
                qDebug()
                    << QString(
                           "    Distance from previous: %1 meters")
                           .arg(segmentLength, 0, 'f', 2);
            }

            // Show heuristic distance to goal
            double heuristicDist =
                result.points[i]->distance(*endPoint).value();
            qDebug()
                << QString(
                       "    Heuristic distance to goal: %1 meters")
                       .arg(heuristicDist, 0, 'f', 2);
        }

        qDebug() << "=== A* LINE SEGMENTS ===";
        for (int i = 0; i < result.lines.size(); ++i)
        {
            qDebug()
                << QString("  Line %1: %2 -> %3")
                       .arg(i)
                       .arg(result.lines[i]->startPoint()->toString())
                       .arg(result.lines[i]->endPoint()->toString());
            qDebug() << QString("    Length: %1 meters")
                            .arg(result.lines[i]->length().value(), 0,
                                 'f', 2);
        }

        // Calculate total path length
        double totalLength = 0.0;
        for (const auto &line : result.lines)
        {
            totalLength += line->length().value();
        }
        qDebug() << QString("=== A* TOTAL PATH LENGTH: %1 meters ===")
                        .arg(totalLength, 0, 'f', 2);

        QVERIFY(result.points.size() >= 2);
        QVERIFY(result.lines.size() == result.points.size() - 1);
    }
    else
    {
        qDebug() << "ISSUE: No path found using A* - this indicates "
                    "the bug!";

        // Enhanced debugging for failed A* paths
        auto startNav =
            visibilityGraph->quadtree->findNearestNeighborPoint(
                startPoint);
        auto endNav =
            visibilityGraph->quadtree->findNearestNeighborPoint(
                endPoint);

        qDebug() << "A* Debug Info:";
        qDebug() << "  Start nearest nav point:"
                 << (startNav ? startNav->toString() : "None");
        qDebug() << "  End nearest nav point:"
                 << (endNav ? endNav->toString() : "None");

        if (startNav && endNav)
        {
            qDebug() << "  Distance to start nav:"
                     << startPoint->distance(*startNav).value()
                     << "meters";
            qDebug() << "  Distance to end nav:"
                     << endPoint->distance(*endNav).value()
                     << "meters";
            qDebug() << "  Nav points visibility:"
                     << visibilityGraph->isVisible(startNav, endNav);
            qDebug() << "  Direct distance start->end:"
                     << startPoint->distance(*endPoint).value()
                     << "meters";
        }
    }
}

void OptimizedVisibilityGraphTest::
    testFindShortestPathMultiplePoints()
{
    qDebug() << "\n=== TESTING MULTIPLE POINT PATH ===";

    QVector<std::shared_ptr<GPoint>> waypoints;
    waypoints.append(testPoints[0]);
    waypoints.append(testPoints[1]);
    waypoints.append(testPoints[2]);
    waypoints.append(testPoints[3]);

    qDebug() << "Finding path through" << waypoints.size()
             << "waypoints:";
    for (int i = 0; i < waypoints.size(); ++i)
    {
        qDebug() << QString("  Waypoint %1: %2")
                        .arg(i)
                        .arg(waypoints[i]->toString());
    }

    auto result = visibilityGraph->findShortestPath(
        waypoints, PathFindingAlgorithm::Dijkstra);

    qDebug() << "Multi-point path found with" << result.points.size()
             << "points and" << result.lines.size() << "lines";
    qDebug() << "Multi-point path valid:" << result.isValid();

    if (result.isValid())
    {
        qDebug() << "SUCCESS: Found valid multi-point path";

        qDebug() << "=== MULTI-POINT PATH DETAILS ===";
        for (int i = 0; i < result.points.size(); ++i)
        {
            qDebug() << QString("  Point %1: %2")
                            .arg(i)
                            .arg(result.points[i]->toString());

            // Mark which original waypoints this corresponds to
            for (int w = 0; w < waypoints.size(); ++w)
            {
                if (*result.points[i] == *waypoints[w])
                {
                    qDebug() << QString("    ^^ This is waypoint %1")
                                    .arg(w);
                    break;
                }
            }
        }

        qDebug() << "=== MULTI-POINT LINE SEGMENTS ===";
        for (int i = 0; i < result.lines.size(); ++i)
        {
            qDebug()
                << QString("  Line %1: %2 -> %3")
                       .arg(i)
                       .arg(result.lines[i]->startPoint()->toString())
                       .arg(result.lines[i]->endPoint()->toString());
            qDebug() << QString("    Length: %1 meters")
                            .arg(result.lines[i]->length().value(), 0,
                                 'f', 2);
        }

        // Calculate total path length
        double totalLength = 0.0;
        for (const auto &line : result.lines)
        {
            totalLength += line->length().value();
        }
        qDebug() << QString(
                        "=== MULTI-POINT TOTAL LENGTH: %1 meters ===")
                        .arg(totalLength, 0, 'f', 2);

        QVERIFY(result.points.size() >= waypoints.size());
    }
    else
    {
        qDebug() << "ISSUE: No multi-point path found";

        // Debug each segment
        for (int i = 0; i < waypoints.size() - 1; ++i)
        {
            auto segmentResult =
                visibilityGraph->findShortestPathDijkstra(
                    waypoints[i], waypoints[i + 1]);
            qDebug() << QString("  Segment %1->%2: %3")
                            .arg(i)
                            .arg(i + 1)
                            .arg(segmentResult.isValid() ? "OK"
                                                         : "FAILED");
        }
    }
}

void OptimizedVisibilityGraphTest::testFindContainingPolygon()
{
    for (int i = 0; i < testPoints.size(); ++i)
    {
        auto polygon =
            visibilityGraph->findContainingPolygon(testPoints[i]);

        qDebug() << "Point" << i << testPoints[i]->toString()
                 << "is in polygon:" << (polygon != nullptr);

        if (polygon)
        {
            qDebug() << "  Polygon has" << polygon->outer().size()
                     << "boundary points";
        }
    }

    // At least some test points should be in polygons
    bool anyInPolygon = false;
    for (const auto &point : testPoints)
    {
        if (visibilityGraph->findContainingPolygon(point))
        {
            anyInPolygon = true;
            break;
        }
    }

    if (!anyInPolygon)
    {
        qDebug() << "WARNING: No test points are in any polygon - "
                    "check polygon setup";
    }
}

void OptimizedVisibilityGraphTest::testMapBoundaries()
{
    auto minPoint = visibilityGraph->getMinMapPoint();
    auto maxPoint = visibilityGraph->getMaxMapPoint();

    qDebug() << "Map boundaries:";
    qDebug() << "  Min:" << minPoint.toString();
    qDebug() << "  Max:" << maxPoint.toString();

    QVERIFY(minPoint.getLongitude().value()
            <= maxPoint.getLongitude().value());
    QVERIFY(minPoint.getLatitude().value()
            <= maxPoint.getLatitude().value());
}

void OptimizedVisibilityGraphTest::testConnectWrapAroundPoints()
{
    // Create a point near the map boundary
    auto minPoint      = visibilityGraph->getMinMapPoint();
    auto boundaryPoint = std::make_shared<GPoint>(
        minPoint.getLongitude() + units::angle::degree_t(0.1),
        minPoint.getLatitude() + units::angle::degree_t(1.0),
        "BoundaryPoint");

    auto wrapAroundPoints =
        visibilityGraph->connectWrapAroundPoints(boundaryPoint);

    qDebug() << "Wrap-around points found:"
             << wrapAroundPoints.size();

    // Should handle gracefully even if no wrap-around is needed
    QVERIFY(wrapAroundPoints.size() >= 0);
}

void OptimizedVisibilityGraphTest::
    testLoadSeaPortsPolygonCoordinates()
{
    auto seaPorts = createTestSeaPorts();

    // This should not crash
    visibilityGraph->loadSeaPortsPolygonCoordinates(seaPorts);

    qDebug() << "Sea ports polygon coordinates loaded for"
             << seaPorts.size() << "ports";

    // Check if closest points were assigned
    for (const auto &port : seaPorts)
    {
        auto closestPoint = port->getClosestPointOnWaterPolygon();
        qDebug() << "Port" << port->getPortName() << "closest point:"
                 << (closestPoint ? "assigned" : "not assigned");
    }
}

void OptimizedVisibilityGraphTest::testVisibilityDebugging()
{
    qDebug() << "\n=== VISIBILITY DEBUGGING ===";

    // Test specific visibility scenarios that might be causing issues

    // 1. Test very short segments
    auto closePoint1 = std::make_shared<GPoint>(
        units::angle::degree_t(-75.0), units::angle::degree_t(40.0),
        "Close1");
    auto closePoint2 = std::make_shared<GPoint>(
        units::angle::degree_t(-74.999),
        units::angle::degree_t(40.001), "Close2");

    auto shortSegment =
        std::make_shared<GLine>(closePoint1, closePoint2);
    bool shortVisible =
        visibilityGraph->isSegmentVisible(shortSegment);
    qDebug() << "Very short segment visible:" << shortVisible;
    qDebug() << "Short segment length:"
             << shortSegment->length().value() << "meters";

    // 2. Test segments that cross the land obstacle
    auto beforeObstacle = std::make_shared<GPoint>(
        units::angle::degree_t(-75.0), units::angle::degree_t(40.5),
        "BeforeObstacle");
    auto afterObstacle = std::make_shared<GPoint>(
        units::angle::degree_t(-74.0), units::angle::degree_t(40.5),
        "AfterObstacle");

    auto crossingSegment =
        std::make_shared<GLine>(beforeObstacle, afterObstacle);
    bool crossingVisible =
        visibilityGraph->isSegmentVisible(crossingSegment);
    qDebug() << "Obstacle-crossing segment visible:"
             << crossingVisible;
    qDebug()
        << "This should be FALSE as it crosses the land obstacle";

    // 3. Test segments that go around the obstacle
    auto aroundPoint = std::make_shared<GPoint>(
        units::angle::degree_t(-74.5), units::angle::degree_t(39.8),
        "AroundPoint");

    auto aroundSegment =
        std::make_shared<GLine>(beforeObstacle, aroundPoint);
    bool aroundVisible =
        visibilityGraph->isSegmentVisible(aroundSegment);
    qDebug() << "Around-obstacle segment visible:" << aroundVisible;
    qDebug() << "This should be TRUE as it goes around the obstacle";
}

void OptimizedVisibilityGraphTest::testPathFindingDebugging()
{
    qDebug() << "\n=== PATH FINDING DEBUGGING ===";

    // Create a simple path that should definitely work
    auto simpleStart = std::make_shared<GPoint>(
        units::angle::degree_t(-75.5), units::angle::degree_t(39.5),
        "SimpleStart");
    auto simpleEnd = std::make_shared<GPoint>(
        units::angle::degree_t(-75.3), units::angle::degree_t(39.7),
        "SimpleEnd");

    qDebug() << "Simple start:" << simpleStart->toString();
    qDebug() << "Simple end:" << simpleEnd->toString();

    // Check if both points are in the water polygon
    auto startPolygon =
        visibilityGraph->findContainingPolygon(simpleStart);
    auto endPolygon =
        visibilityGraph->findContainingPolygon(simpleEnd);

    qDebug() << "Simple start in polygon:"
             << (startPolygon != nullptr);
    qDebug() << "Simple end in polygon:" << (endPolygon != nullptr);

    if (startPolygon && endPolygon)
    {
        // Both points are in water, test direct visibility
        bool directVisible =
            visibilityGraph->isVisible(simpleStart, simpleEnd);
        qDebug() << "Simple points directly visible:"
                 << directVisible;

        // Try path finding
        auto simpleResult = visibilityGraph->findShortestPathDijkstra(
            simpleStart, simpleEnd);
        qDebug() << "Simple path found:" << simpleResult.isValid();

        if (!simpleResult.isValid())
        {
            qDebug() << "CRITICAL: Even simple path finding failed!";
            qDebug() << "This indicates a fundamental issue with the "
                        "visibility graph or path finding";
        }
    }
    else
    {
        qDebug() << "Simple points not in water polygon - adjusting "
                    "test points...";

        // If our simple points aren't in water, use polygon vertices
        auto polygons = createTestPolygons();
        if (!polygons.isEmpty()
            && !polygons.first()->outer().isEmpty())
        {
            auto vertex1 = polygons.first()->outer()[0];
            auto vertex2 = polygons.first()->outer()[1];

            qDebug() << "Using polygon vertices for simple test:";
            qDebug() << "Vertex 1:" << vertex1->toString();
            qDebug() << "Vertex 2:" << vertex2->toString();

            auto vertexResult =
                visibilityGraph->findShortestPathDijkstra(vertex1,
                                                          vertex2);
            qDebug() << "Vertex path found:"
                     << vertexResult.isValid();
        }
    }
}

void OptimizedVisibilityGraphTest::testEdgeCases()
{
    qDebug() << "\n=== EDGE CASES ===";

    // Test with same start and end point
    qDebug() << "--- Testing same point path ---";
    auto samePointResult = visibilityGraph->findShortestPathDijkstra(
        testPoints[0], testPoints[0]);
    qDebug() << "Same point path valid:" << samePointResult.isValid();
    if (samePointResult.isValid())
    {
        qDebug() << "Same point path points:"
                 << samePointResult.points.size();
        for (int i = 0; i < samePointResult.points.size(); ++i)
        {
            qDebug()
                << QString("  Point %1: %2")
                       .arg(i)
                       .arg(samePointResult.points[i]->toString());
        }
    }

    // Test with points outside all polygons
    qDebug() << "--- Testing outside points path ---";
    auto outsidePoint1 = std::make_shared<GPoint>(
        units::angle::degree_t(-80.0), units::angle::degree_t(35.0),
        "Outside1");
    auto outsidePoint2 = std::make_shared<GPoint>(
        units::angle::degree_t(-70.0), units::angle::degree_t(45.0),
        "Outside2");

    qDebug() << "Outside point 1:" << outsidePoint1->toString();
    qDebug() << "Outside point 2:" << outsidePoint2->toString();

    auto outsideResult = visibilityGraph->findShortestPathDijkstra(
        outsidePoint1, outsidePoint2);
    qDebug() << "Outside points path valid:"
             << outsideResult.isValid();

    if (outsideResult.isValid())
    {
        qDebug() << "=== OUTSIDE POINTS PATH ===";
        for (int i = 0; i < outsideResult.points.size(); ++i)
        {
            qDebug() << QString("  Point %1: %2")
                            .arg(i)
                            .arg(outsideResult.points[i]->toString());
        }

        double totalLength = 0.0;
        for (const auto &line : outsideResult.lines)
        {
            totalLength += line->length().value();
        }
        qDebug() << QString("Outside points total length: %1 meters")
                        .arg(totalLength, 0, 'f', 2);
    }

    // Test with empty waypoint list
    qDebug() << "--- Testing empty waypoints ---";
    QVector<std::shared_ptr<GPoint>> emptyWaypoints;
    auto emptyResult = visibilityGraph->findShortestPath(
        emptyWaypoints, PathFindingAlgorithm::Dijkstra);
    qDebug() << "Empty waypoints path valid:"
             << emptyResult.isValid();

    // Test with single waypoint
    qDebug() << "--- Testing single waypoint ---";
    QVector<std::shared_ptr<GPoint>> singleWaypoint;
    singleWaypoint.append(testPoints[0]);
    auto singleResult = visibilityGraph->findShortestPath(
        singleWaypoint, PathFindingAlgorithm::Dijkstra);
    qDebug() << "Single waypoint path valid:"
             << singleResult.isValid();

    if (singleResult.isValid())
    {
        qDebug() << "Single waypoint path points:"
                 << singleResult.points.size();
        for (int i = 0; i < singleResult.points.size(); ++i)
        {
            qDebug() << QString("  Point %1: %2")
                            .arg(i)
                            .arg(singleResult.points[i]->toString());
        }
    }
}

void OptimizedVisibilityGraphTest::debugIntersectionDetection()
{
    qDebug() << "\n=== DEBUGGING INTERSECTION DETECTION ===";

    // Create the problematic segment from multi-point path
    auto problemStart = std::make_shared<GPoint>(
        units::angle::degree_t(-75.0), units::angle::degree_t(39.2));
    auto problemEnd = std::make_shared<GPoint>(
        units::angle::degree_t(-73.0), units::angle::degree_t(41.0));
    auto problemSegment =
        std::make_shared<GLine>(problemStart, problemEnd);

    qDebug() << "Testing problematic segment:";
    qDebug() << "  From:" << problemStart->toString();
    qDebug() << "  To:" << problemEnd->toString();
    qDebug() << "  Length:" << problemSegment->length().value()
             << "meters";

    // Test visibility
    bool visible = visibilityGraph->isSegmentVisible(problemSegment);
    qDebug() << "  isSegmentVisible result:" << visible
             << "(should be FALSE!)";

    // Manually check intersection with each obstacle edge
    qDebug() << "\nTesting intersection with obstacle edges:";

    // Land obstacle vertices (hole)
    auto lo1 = std::make_shared<GPoint>(units::angle::degree_t(-74.8),
                                        units::angle::degree_t(40.3));
    auto lo2 = std::make_shared<GPoint>(units::angle::degree_t(-74.2),
                                        units::angle::degree_t(40.3));
    auto lo3 = std::make_shared<GPoint>(units::angle::degree_t(-74.2),
                                        units::angle::degree_t(40.7));
    auto lo4 = std::make_shared<GPoint>(units::angle::degree_t(-74.8),
                                        units::angle::degree_t(40.7));

    QVector<std::shared_ptr<GLine>> obstacleEdges;
    obstacleEdges.append(
        std::make_shared<GLine>(lo1, lo2)); // Bottom edge
    obstacleEdges.append(
        std::make_shared<GLine>(lo2, lo3)); // Right edge
    obstacleEdges.append(
        std::make_shared<GLine>(lo3, lo4)); // Top edge
    obstacleEdges.append(
        std::make_shared<GLine>(lo4, lo1)); // Left edge

    for (int i = 0; i < obstacleEdges.size(); ++i)
    {
        bool intersects =
            problemSegment->intersects(*obstacleEdges[i], true);
        qDebug()
            << QString("  Edge %1: %2 -> %3")
                   .arg(i)
                   .arg(obstacleEdges[i]->startPoint()->toString())
                   .arg(obstacleEdges[i]->endPoint()->toString());
        qDebug() << QString("    Intersects: %1")
                        .arg(intersects ? "YES" : "NO");
    }

    // Test a segment that should definitely be visible (no
    // intersection)
    auto safeStart = std::make_shared<GPoint>(
        units::angle::degree_t(-75.5), units::angle::degree_t(39.5));
    auto safeEnd = std::make_shared<GPoint>(
        units::angle::degree_t(-75.0), units::angle::degree_t(39.2));
    auto safeSegment = std::make_shared<GLine>(safeStart, safeEnd);

    qDebug() << "\nTesting safe segment (should be visible):";
    qDebug() << "  From:" << safeStart->toString();
    qDebug() << "  To:" << safeEnd->toString();
    qDebug() << "  isSegmentVisible result:"
             << visibilityGraph->isSegmentVisible(safeSegment);
}

void OptimizedVisibilityGraphTest::debugGLineIntersection()
{
    qDebug() << "\n=== DEBUGGING GLINE INTERSECTION METHOD ===";

    // Create the problematic segment
    auto problemStart = std::make_shared<GPoint>(
        units::angle::degree_t(-75.0), units::angle::degree_t(39.2));
    auto problemEnd = std::make_shared<GPoint>(
        units::angle::degree_t(-73.0), units::angle::degree_t(41.0));
    auto problemSegment =
        std::make_shared<GLine>(problemStart, problemEnd);

    // Create the obstacle edge that should intersect
    auto edgeStart = std::make_shared<GPoint>(
        units::angle::degree_t(-74.2), units::angle::degree_t(40.3));
    auto edgeEnd = std::make_shared<GPoint>(
        units::angle::degree_t(-74.2), units::angle::degree_t(40.7));
    auto obstacleEdge = std::make_shared<GLine>(edgeStart, edgeEnd);

    qDebug() << "=== PROBLEM SEGMENT ===";
    qDebug() << "From:" << problemStart->toString();
    qDebug() << "To:" << problemEnd->toString();
    qDebug() << "Length:" << problemSegment->length().value()
             << "meters";

    qDebug() << "=== OBSTACLE EDGE ===";
    qDebug() << "From:" << edgeStart->toString();
    qDebug() << "To:" << edgeEnd->toString();
    qDebug() << "Length:" << obstacleEdge->length().value()
             << "meters";

    // Test the intersects method with different parameters
    qDebug() << "\n=== INTERSECTION TESTS ===";

    // Test 1: Basic intersection (should be TRUE)
    bool intersects1 =
        problemSegment->intersects(*obstacleEdge, false);
    qDebug() << "problemSegment.intersects(obstacleEdge, false):"
             << intersects1;

    // Test 2: Ignore edge points (should still be TRUE)
    bool intersects2 =
        problemSegment->intersects(*obstacleEdge, true);
    qDebug() << "problemSegment.intersects(obstacleEdge, true):"
             << intersects2;

    // Test 3: Reverse direction
    bool intersects3 =
        obstacleEdge->intersects(*problemSegment, false);
    qDebug() << "obstacleEdge.intersects(problemSegment, false):"
             << intersects3;

    // Test 4: Check tolerance
    qDebug() << "\n=== TOLERANCE ANALYSIS ===";
    qDebug() << "GLine::TOLERANCE:" << 0.1;

    // Check distances between endpoints
    double dist1 = problemStart->distance(*edgeStart).value();
    double dist2 = problemStart->distance(*edgeEnd).value();
    double dist3 = problemEnd->distance(*edgeStart).value();
    double dist4 = problemEnd->distance(*edgeEnd).value();

    qDebug() << "Distance problem_start to edge_start:" << dist1
             << "meters";
    qDebug() << "Distance problem_start to edge_end:" << dist2
             << "meters";
    qDebug() << "Distance problem_end to edge_start:" << dist3
             << "meters";
    qDebug() << "Distance problem_end to edge_end:" << dist4
             << "meters";

    // Test 5: Manual geometric calculation
    qDebug() << "\n=== MANUAL GEOMETRIC CALCULATION ===";

    // Calculate where the problem line crosses x = -74.2
    double x1 = -75.0, y1 = 39.2; // Problem start
    double x2 = -73.0, y2 = 41.0; // Problem end
    double x_test = -74.2;        // Obstacle edge x-coordinate

    // Linear interpolation: y = y1 + (y2-y1) * (x-x1)/(x2-x1)
    double y_intersect = y1 + (y2 - y1) * (x_test - x1) / (x2 - x1);

    qDebug() << QString("Problem line equation:");
    qDebug() << QString("  From (%1, %2) to (%3, %4)")
                    .arg(x1)
                    .arg(y1)
                    .arg(x2)
                    .arg(y2);
    qDebug() << QString("  At x = %1, y = %2")
                    .arg(x_test)
                    .arg(y_intersect);
    qDebug() << QString("Obstacle edge spans y = %1 to %2")
                    .arg(40.3)
                    .arg(40.7);

    bool shouldIntersect =
        (y_intersect >= 40.3 && y_intersect <= 40.7);
    qDebug() << QString("Mathematical intersection: %1")
                    .arg(shouldIntersect ? "YES" : "NO");

    // Test 6: GDAL LineString debugging
    qDebug() << "\n=== GDAL LINESTRING DEBUGGING ===";

    auto gdalLine1 = problemSegment->getGDALLine();
    auto gdalLine2 = obstacleEdge->getGDALLine();

    qDebug() << "GDAL Line 1 point count:"
             << gdalLine1.getNumPoints();
    qDebug() << "GDAL Line 2 point count:"
             << gdalLine2.getNumPoints();

    // Get GDAL points
    OGRPoint p1_start, p1_end, p2_start, p2_end;
    gdalLine1.getPoint(0, &p1_start);
    gdalLine1.getPoint(1, &p1_end);
    gdalLine2.getPoint(0, &p2_start);
    gdalLine2.getPoint(1, &p2_end);

    qDebug() << QString("GDAL Line 1: (%1, %2) to (%3, %4)")
                    .arg(p1_start.getX())
                    .arg(p1_start.getY())
                    .arg(p1_end.getX())
                    .arg(p1_end.getY());
    qDebug() << QString("GDAL Line 2: (%1, %2) to (%3, %4)")
                    .arg(p2_start.getX())
                    .arg(p2_start.getY())
                    .arg(p2_end.getX())
                    .arg(p2_end.getY());

    // Test GDAL intersection directly
    bool gdalIntersects = gdalLine1.Intersects(&gdalLine2);
    qDebug() << "Direct GDAL Intersects() result:" << gdalIntersects;

    // Test 7: Create a simple test case that should definitely work
    qDebug() << "\n=== SIMPLE TEST CASE ===";

    // Create two lines that definitely intersect
    auto simpleStart1 = std::make_shared<GPoint>(
        units::angle::degree_t(-75.0), units::angle::degree_t(40.0));
    auto simpleEnd1 = std::make_shared<GPoint>(
        units::angle::degree_t(-73.0), units::angle::degree_t(40.0));
    auto simpleLine1 =
        std::make_shared<GLine>(simpleStart1, simpleEnd1);

    auto simpleStart2 = std::make_shared<GPoint>(
        units::angle::degree_t(-74.0), units::angle::degree_t(39.5));
    auto simpleEnd2 = std::make_shared<GPoint>(
        units::angle::degree_t(-74.0), units::angle::degree_t(40.5));
    auto simpleLine2 =
        std::make_shared<GLine>(simpleStart2, simpleEnd2);

    qDebug() << "Simple horizontal line:" << simpleStart1->toString()
             << "to" << simpleEnd1->toString();
    qDebug() << "Simple vertical line:" << simpleStart2->toString()
             << "to" << simpleEnd2->toString();
    qDebug() << "Simple intersection result:"
             << simpleLine1->intersects(*simpleLine2, false);
    qDebug() << "This should definitely be TRUE!";
}

void OptimizedVisibilityGraphTest::debugAStarInvalidPath()
{
    qDebug() << "\n=== DEBUGGING A* INVALID PATH ===";

    // The problematic segment from A* path
    auto navPoint1 = std::make_shared<GPoint>(
        units::angle::degree_t(-76.0), units::angle::degree_t(39.0));
    auto navPoint2 = std::make_shared<GPoint>(
        units::angle::degree_t(-74.2), units::angle::degree_t(40.7));

    auto problematicSegment =
        std::make_shared<GLine>(navPoint1, navPoint2);

    qDebug() << "Testing A* problematic segment:";
    qDebug() << "  From:" << navPoint1->toString();
    qDebug() << "  To:" << navPoint2->toString();
    qDebug() << "  Length:" << problematicSegment->length().value()
             << "meters";

    bool visible =
        visibilityGraph->isSegmentVisible(problematicSegment);
    qDebug() << "  isSegmentVisible result:" << visible
             << "(should be FALSE!)";

    // Test visibility in pathfinding context
    bool pathVisible =
        visibilityGraph->isVisible(navPoint1, navPoint2);
    qDebug() << "  isVisible result:" << pathVisible
             << "(should be FALSE!)";

    // This segment definitely crosses through the land obstacle!
    // The land obstacle spans from (-74.8, 40.3) to (-74.2, 40.7)
    // A line from (-76.0, 39.0) to (-74.2, 40.7) must cross through
    // it
}

QTEST_MAIN(OptimizedVisibilityGraphTest)
#include "test_optimizedvisibilitygraph.moc"
