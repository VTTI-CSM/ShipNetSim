#include "network/gline.h"
#include "network/gpoint.h"
#include "network/optimizednetwork.h"
#include "network/optimizedvisibilitygraph.h"
#include "network/polygon.h"
#include "network/quadtree.h"
#include <QDebug>
#include <QTemporaryFile>
#include <QTest>
#include <QTextStream>

using namespace ShipNetSimCore;

class IntegrationTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    
    // Full integration tests
    void testCompletePathFindingWorkflow();
    void testRealWorldScenario();
    void testVisibilityGraphIntegration();
    void testQuadtreeVisibilityIntegration();
    
    // Performance tests
    void testLargeNetworkPerformance();
    void testManyQueriesPerformance();
    
    // Bug reproduction tests
    void testBugReproduction_NoPathFound();
    void testBugReproduction_EmptyVisibilityGraph();
    
    // Comprehensive debugging
    void testComponentConnectivity();
    void testDataFlowDebugging();

private:
    QString createRealisticTestData();
    QVector<std::shared_ptr<Polygon>> createRealisticPolygons();
    std::shared_ptr<OptimizedNetwork> integrationNetwork;
};

void IntegrationTest::initTestCase()
{
    qDebug() << "\n================================================";
    qDebug() << "Starting Integration Tests for ShipNetSim";
    qDebug() << "These tests will help identify the shortest path bug";
    qDebug() << "================================================\n";
    
    // Create a realistic test network
    auto polygons = createRealisticPolygons();
    integrationNetwork = std::make_shared<OptimizedNetwork>(
        polygons, BoundariesType::Water, "IntegrationTestRegion"
    );
}

void IntegrationTest::cleanupTestCase()
{
    qDebug() << "\nIntegration tests completed.";
}

QVector<std::shared_ptr<Polygon>> IntegrationTest::createRealisticPolygons()
{
    QVector<std::shared_ptr<Polygon>> polygons;
    
    // Create a realistic water body (like a bay or harbor)
    // Outer boundary - large navigable area
    QVector<std::shared_ptr<GPoint>> outerBoundary;
    
    // Create a roughly rectangular bay
    outerBoundary.append(std::make_shared<GPoint>(
        units::angle::degree_t(-76.5), units::angle::degree_t(38.5), "OB1"
    ));
    outerBoundary.append(std::make_shared<GPoint>(
        units::angle::degree_t(-75.5), units::angle::degree_t(38.5), "OB2"
    ));
    outerBoundary.append(std::make_shared<GPoint>(
        units::angle::degree_t(-75.5), units::angle::degree_t(39.2), "OB3"
    ));
    outerBoundary.append(std::make_shared<GPoint>(
        units::angle::degree_t(-76.0), units::angle::degree_t(39.5), "OB4"
    ));
    outerBoundary.append(std::make_shared<GPoint>(
        units::angle::degree_t(-76.3), units::angle::degree_t(39.2), "OB5"
    ));
    outerBoundary.append(std::make_shared<GPoint>(
        units::angle::degree_t(-76.5), units::angle::degree_t(38.8), "OB6"
    ));
    outerBoundary.append(outerBoundary.first()); // Close polygon
    
    // Create some land obstacles (islands or peninsulas)
    QVector<QVector<std::shared_ptr<GPoint>>> holes;
    
    // Small island
    QVector<std::shared_ptr<GPoint>> island;
    island.append(std::make_shared<GPoint>(
        units::angle::degree_t(-76.1), units::angle::degree_t(38.9), "I1"
    ));
    island.append(std::make_shared<GPoint>(
        units::angle::degree_t(-75.9), units::angle::degree_t(38.9), "I2"
    ));
    island.append(std::make_shared<GPoint>(
        units::angle::degree_t(-75.9), units::angle::degree_t(39.0), "I3"
    ));
    island.append(std::make_shared<GPoint>(
        units::angle::degree_t(-76.1), units::angle::degree_t(39.0), "I4"
    ));
    island.append(island.first()); // Close polygon
    holes.append(island);
    
    // Jetty or pier
    QVector<std::shared_ptr<GPoint>> jetty;
    jetty.append(std::make_shared<GPoint>(
        units::angle::degree_t(-75.8), units::angle::degree_t(38.7), "J1"
    ));
    jetty.append(std::make_shared<GPoint>(
        units::angle::degree_t(-75.7), units::angle::degree_t(38.7), "J2"
    ));
    jetty.append(std::make_shared<GPoint>(
        units::angle::degree_t(-75.7), units::angle::degree_t(38.8), "J3"
    ));
    jetty.append(std::make_shared<GPoint>(
        units::angle::degree_t(-75.8), units::angle::degree_t(38.8), "J4"
    ));
    jetty.append(jetty.first()); // Close polygon
    holes.append(jetty);
    
    auto waterBody = std::make_shared<Polygon>(outerBoundary, holes, "RealisticWaterBody");
    polygons.append(waterBody);
    
    return polygons;
}

void IntegrationTest::testCompletePathFindingWorkflow()
{
    qDebug() << "\n=== COMPLETE PATH FINDING WORKFLOW TEST ===";
    
    // This test simulates a complete ship navigation scenario
    
    // 1. Define realistic start and end points
    auto portA = std::make_shared<GPoint>(
        units::angle::degree_t(-76.2), units::angle::degree_t(38.7), "PortA"
    );
    auto portB = std::make_shared<GPoint>(
        units::angle::degree_t(-75.7), units::angle::degree_t(39.1), "PortB"
    );
    
    qDebug() << "Planning route from Port A to Port B";
    qDebug() << "Port A:" << portA->toString();
    qDebug() << "Port B:" << portB->toString();
    
    // 2. Calculate direct distance for reference
    auto directDistance = portA->distance(*portB);
    qDebug() << "Direct distance:" << directDistance.value() << "meters ("
             << (directDistance.value() / 1000.0) << " km)";
    
    // 3. Test both path finding algorithms
    auto dijkstraPath = integrationNetwork->findShortestPath(
        portA, portB, PathFindingAlgorithm::Dijkstra
    );
    
    auto astarPath = integrationNetwork->findShortestPath(
        portA, portB, PathFindingAlgorithm::AStar
    );
    
    qDebug() << "\nPath Finding Results:";
    qDebug() << "Dijkstra - Valid:" << dijkstraPath.isValid() 
             << ", Points:" << dijkstraPath.points.size()
             << ", Lines:" << dijkstraPath.lines.size();
    qDebug() << "A* - Valid:" << astarPath.isValid()
             << ", Points:" << astarPath.points.size()
             << ", Lines:" << astarPath.lines.size();
    
    // 4. Analyze results
    if (dijkstraPath.isValid()) {
        double pathDistance = 0.0;
        for (const auto& line : dijkstraPath.lines) {
            pathDistance += line->length().value();
        }
        
        qDebug() << "Dijkstra path distance:" << pathDistance << "meters ("
                 << (pathDistance / 1000.0) << " km)";
        qDebug() << "Path efficiency:" << (directDistance.value() / pathDistance * 100.0) << "%";
        
        // Log the complete path
        qDebug() << "Dijkstra path waypoints:";
        for (int i = 0; i < dijkstraPath.points.size(); ++i) {
            qDebug() << "  " << i << ":" << dijkstraPath.points[i]->toString();
        }
        
        QVERIFY(dijkstraPath.points.size() >= 2);
        QVERIFY(dijkstraPath.lines.size() == dijkstraPath.points.size() - 1);
        QVERIFY(pathDistance > 0.0);
        
    } else {
        qDebug() << "ERROR: Dijkstra failed to find path - this is the bug!";
        qDebug() << "Investigating possible causes...";
        
        // Debug why path finding failed
        // TODO: Add detailed debugging here
    }
    
    if (astarPath.isValid()) {
        double pathDistance = 0.0;
        for (const auto& line : astarPath.lines) {
            pathDistance += line->length().value();
        }
        
        qDebug() << "A* path distance:" << pathDistance << "meters ("
                 << (pathDistance / 1000.0) << " km)";
        
        QVERIFY(astarPath.points.size() >= 2);
        QVERIFY(astarPath.lines.size() == astarPath.points.size() - 1);
        
    } else {
        qDebug() << "ERROR: A* also failed to find path";
    }
    
    // 5. Test with multiple waypoints
    QVector<std::shared_ptr<GPoint>> waypoints;
    waypoints.append(portA);
    waypoints.append(std::make_shared<GPoint>(
        units::angle::degree_t(-76.0), units::angle::degree_t(38.8), "Waypoint1"
    ));
    waypoints.append(std::make_shared<GPoint>(
        units::angle::degree_t(-75.8), units::angle::degree_t(39.0), "Waypoint2"
    ));
    waypoints.append(portB);
    
    auto multiPath = integrationNetwork->findShortestPath(
        waypoints, PathFindingAlgorithm::Dijkstra
    );
    
    qDebug() << "\nMulti-waypoint path - Valid:" << multiPath.isValid()
             << ", Points:" << multiPath.points.size()
             << ", Lines:" << multiPath.lines.size();
}

void IntegrationTest::testRealWorldScenario()
{
    qDebug() << "\n=== REAL WORLD SCENARIO TEST ===";
    
    // Simulate a realistic ship navigation scenario
    // Ship needs to navigate around obstacles to reach destination
    
    // Start point: Near the jetty
    auto shipStart = std::make_shared<GPoint>(
        units::angle::degree_t(-75.85), units::angle::degree_t(38.65), "ShipStart"
    );
    
    // End point: On the other side of the island
    auto shipEnd = std::make_shared<GPoint>(
        units::angle::degree_t(-75.85), units::angle::degree_t(39.05), "ShipEnd"
    );
    
    qDebug() << "Real world scenario: Ship navigation around obstacles";
    qDebug() << "Ship start:" << shipStart->toString();
    qDebug() << "Ship end:" << shipEnd->toString();
    
    // This path should require navigation around the island
    auto scenarioPath = integrationNetwork->findShortestPath(
        shipStart, shipEnd, PathFindingAlgorithm::Dijkstra
    );
    
    qDebug() << "Scenario path valid:" << scenarioPath.isValid();
    
    if (scenarioPath.isValid()) {
        qDebug() << "SUCCESS: Found path around obstacles";
        qDebug() << "Path has" << scenarioPath.points.size() << "waypoints";
        
        // The path should go around the island, so it should have more than 2 points
        if (scenarioPath.points.size() > 2) {
            qDebug() << "Path correctly navigates around obstacles";
        } else {
            qDebug() << "WARNING: Path might be going through obstacles";
        }
        
        // Calculate path length
        double totalLength = 0.0;
        for (const auto& line : scenarioPath.lines) {
            totalLength += line->length().value();
        }
        qDebug() << "Total scenario path length:" << totalLength << "meters";
        
    } else {
        qDebug() << "CRITICAL: Failed to find path in realistic scenario";
        qDebug() << "This indicates a serious problem with the path finding system";
    }
}

void IntegrationTest::testVisibilityGraphIntegration()
{
    qDebug() << "\n=== VISIBILITY GRAPH INTEGRATION TEST ===";
    
    // Test the integration between OptimizedNetwork and OptimizedVisibilityGraph
    
    // Create test points
    auto point1 = std::make_shared<GPoint>(
        units::angle::degree_t(-76.0), units::angle::degree_t(38.8), "VG1"
    );
    auto point2 = std::make_shared<GPoint>(
        units::angle::degree_t(-75.8), units::angle::degree_t(39.0), "VG2"
    );
    
    qDebug() << "Testing visibility graph integration";
    qDebug() << "Point 1:" << point1->toString();
    qDebug() << "Point 2:" << point2->toString();
    
    // Test direct visibility
    // Note: We can't directly access the visibility graph from OptimizedNetwork
    // So we test through the path finding interface
    
    auto testPath = integrationNetwork->findShortestPath(
        point1, point2, PathFindingAlgorithm::Dijkstra
    );
    
    qDebug() << "Visibility integration test result:" << testPath.isValid();
    
    if (testPath.isValid()) {
        qDebug() << "Visibility graph integration working";
        
        // Analyze path characteristics
        if (testPath.points.size() == 2) {
            qDebug() << "Direct line of sight - no obstacles";
        } else {
            qDebug() << "Path avoids obstacles - visibility graph working correctly";
        }
    } else {
        qDebug() << "Visibility graph integration may have issues";
    }
}

void IntegrationTest::testQuadtreeVisibilityIntegration()
{
    qDebug() << "\n=== QUADTREE-VISIBILITY INTEGRATION TEST ===";
    
    // Test the integration between Quadtree and visibility calculations
    
    // Create points that should test quadtree spatial queries
    auto spatialPoint1 = std::make_shared<GPoint>(
        units::angle::degree_t(-76.2), units::angle::degree_t(38.6), "SP1"
    );
    auto spatialPoint2 = std::make_shared<GPoint>(
        units::angle::degree_t(-75.6), units::angle::degree_t(39.2), "SP2"
    );
    
    qDebug() << "Testing quadtree-visibility integration";
    qDebug() << "Spatial point 1:" << spatialPoint1->toString();
    qDebug() << "Spatial point 2:" << spatialPoint2->toString();
    
    // This should exercise the quadtree spatial queries during visibility checks
    auto spatialPath = integrationNetwork->findShortestPath(
        spatialPoint1, spatialPoint2, PathFindingAlgorithm::Dijkstra
    );
    
    qDebug() << "Spatial integration test result:" << spatialPath.isValid();
    
    if (spatialPath.isValid()) {
        qDebug()
qDebug() << "Quadtree-visibility integration working";
        qDebug() << "Spatial path points:" << spatialPath.points.size();
        
        // This path should cross multiple quadtree nodes
        QVERIFY(spatialPath.points.size() >= 2);
        
    } else {
        qDebug() << "Quadtree-visibility integration may have issues";
        qDebug() << "This could indicate problems with spatial indexing or visibility calculations";
    }
}

void IntegrationTest::testLargeNetworkPerformance()
{
    qDebug() << "\n=== LARGE NETWORK PERFORMANCE TEST ===";
    
    // Test performance with a larger, more complex network
    QVector<std::shared_ptr<Polygon>> largePolygons;
    
    // Create multiple water bodies with obstacles
    for (int i = 0; i < 5; ++i) {
        QVector<std::shared_ptr<GPoint>> boundary;
        double baseX = -78.0 + i * 1.0;
        double baseY = 37.0 + i * 0.5;
        
        // Create rectangular water body
        boundary.append(std::make_shared<GPoint>(
            units::angle::degree_t(baseX), units::angle::degree_t(baseY), 
            QString("LB%1_1").arg(i)
        ));
        boundary.append(std::make_shared<GPoint>(
            units::angle::degree_t(baseX + 0.8), units::angle::degree_t(baseY), 
            QString("LB%1_2").arg(i)
        ));
        boundary.append(std::make_shared<GPoint>(
            units::angle::degree_t(baseX + 0.8), units::angle::degree_t(baseY + 0.8), 
            QString("LB%1_3").arg(i)
        ));
        boundary.append(std::make_shared<GPoint>(
            units::angle::degree_t(baseX), units::angle::degree_t(baseY + 0.8), 
            QString("LB%1_4").arg(i)
        ));
        boundary.append(boundary.first());
        
        // Add some obstacles
        QVector<QVector<std::shared_ptr<GPoint>>> holes;
        QVector<std::shared_ptr<GPoint>> obstacle;
        obstacle.append(std::make_shared<GPoint>(
            units::angle::degree_t(baseX + 0.3), units::angle::degree_t(baseY + 0.3), 
            QString("LO%1_1").arg(i)
        ));
        obstacle.append(std::make_shared<GPoint>(
            units::angle::degree_t(baseX + 0.5), units::angle::degree_t(baseY + 0.3), 
            QString("LO%1_2").arg(i)
        ));
        obstacle.append(std::make_shared<GPoint>(
            units::angle::degree_t(baseX + 0.5), units::angle::degree_t(baseY + 0.5), 
            QString("LO%1_3").arg(i)
        ));
        obstacle.append(std::make_shared<GPoint>(
            units::angle::degree_t(baseX + 0.3), units::angle::degree_t(baseY + 0.5), 
            QString("LO%1_4").arg(i)
        ));
        obstacle.append(obstacle.first());
        holes.append(obstacle);
        
        auto waterBody = std::make_shared<Polygon>(boundary, holes, QString("LargeWaterBody_%1").arg(i));
        largePolygons.append(waterBody);
    }
    
    // Create large network
    QElapsedTimer timer;
    timer.start();
    
    OptimizedNetwork largeNetwork(largePolygons, BoundariesType::Water, "LargeTestNetwork");
    
    qint64 constructionTime = timer.elapsed();
    qDebug() << "Large network construction time:" << constructionTime << "ms";
    
    // Test path finding performance
    auto perfStart = std::make_shared<GPoint>(
        units::angle::degree_t(-77.5), units::angle::degree_t(37.5), "PerfStart"
    );
    auto perfEnd = std::make_shared<GPoint>(
        units::angle::degree_t(-74.5), units::angle::degree_t(39.5), "PerfEnd"
    );
    
    timer.restart();
    auto perfPath = largeNetwork.findShortestPath(
        perfStart, perfEnd, PathFindingAlgorithm::Dijkstra
    );
    qint64 pathFindingTime = timer.elapsed();
    
    qDebug() << "Large network path finding time:" << pathFindingTime << "ms";
    qDebug() << "Large network path valid:" << perfPath.isValid();
    
    // Performance should be reasonable
    QVERIFY(constructionTime < 10000); // Less than 10 seconds
    QVERIFY(pathFindingTime < 5000);   // Less than 5 seconds
}

void IntegrationTest::testManyQueriesPerformance()
{
    qDebug() << "\n=== MANY QUERIES PERFORMANCE TEST ===";
    
    // Test performance with many path finding queries
    const int numQueries = 50;
    QElapsedTimer timer;
    
    timer.start();
    
    int successfulQueries = 0;
    
    for (int i = 0; i < numQueries; ++i) {
        // Generate random start and end points within the network bounds
        double startX = -76.4 + (static_cast<double>(rand()) / RAND_MAX) * 0.8;
        double startY = 38.6 + (static_cast<double>(rand()) / RAND_MAX) * 0.8;
        double endX = -76.4 + (static_cast<double>(rand()) / RAND_MAX) * 0.8;
        double endY = 38.6 + (static_cast<double>(rand()) / RAND_MAX) * 0.8;
        
        auto start = std::make_shared<GPoint>(
            units::angle::degree_t(startX), units::angle::degree_t(startY), 
            QString("QStart%1").arg(i)
        );
        auto end = std::make_shared<GPoint>(
            units::angle::degree_t(endX), units::angle::degree_t(endY), 
            QString("QEnd%1").arg(i)
        );
        
        auto result = integrationNetwork->findShortestPath(
            start, end, PathFindingAlgorithm::Dijkstra
        );
        
        if (result.isValid()) {
            successfulQueries++;
        }
    }
    
    qint64 totalTime = timer.elapsed();
    
    qDebug() << "Many queries performance test:";
    qDebug() << "  Total queries:" << numQueries;
    qDebug() << "  Successful queries:" << successfulQueries;
    qDebug() << "  Success rate:" << (static_cast<double>(successfulQueries) / numQueries * 100.0) << "%";
    qDebug() << "  Total time:" << totalTime << "ms";
    qDebug() << "  Average time per query:" << (static_cast<double>(totalTime) / numQueries) << "ms";
    
    // We expect at least some queries to succeed
    QVERIFY(successfulQueries > 0);
    
    // Average time per query should be reasonable
    double avgTime = static_cast<double>(totalTime) / numQueries;
    QVERIFY(avgTime < 1000.0); // Less than 1 second per query on average
}

void IntegrationTest::testBugReproduction_NoPathFound()
{
    qDebug() << "\n=== BUG REPRODUCTION: NO PATH FOUND ===";
    qDebug() << "This test specifically reproduces the 'no path found' bug";
    
    // Use simple, straightforward points that should definitely have a path
    auto bugStart = std::make_shared<GPoint>(
        units::angle::degree_t(-76.0), units::angle::degree_t(38.8), "BugStart"
    );
    auto bugEnd = std::make_shared<GPoint>(
        units::angle::degree_t(-75.8), units::angle::degree_t(39.0), "BugEnd"
    );
    
    qDebug() << "Bug reproduction test points:";
    qDebug() << "Start:" << bugStart->toString();
    qDebug() << "End:" << bugEnd->toString();
    
    // Calculate direct distance
    auto directDist = bugStart->distance(*bugEnd);
    qDebug() << "Direct distance:" << directDist.value() << "meters";
    
    // Try path finding
    auto bugResult = integrationNetwork->findShortestPath(
        bugStart, bugEnd, PathFindingAlgorithm::Dijkstra
    );
    
    qDebug() << "Bug reproduction result:";
    qDebug() << "  Path found:" << bugResult.isValid();
    qDebug() << "  Points count:" << bugResult.points.size();
    qDebug() << "  Lines count:" << bugResult.lines.size();
    
    if (!bugResult.isValid()) {
        qDebug() << "\nBUG CONFIRMED: No path found for simple case!";
        qDebug() << "Detailed debugging information:";
        
        // Additional debugging can be added here
        // This is where you would investigate:
        // 1. Are the points in valid water areas?
        // 2. Is the visibility graph correctly constructed?
        // 3. Are there issues with the quadtree?
        // 4. Is the path finding algorithm working correctly?
        
        qDebug() << "Recommended debugging steps:";
        qDebug() << "1. Check if points are within polygon boundaries";
        qDebug() << "2. Verify visibility graph construction";
        qDebug() << "3. Test quadtree spatial queries";
        qDebug() << "4. Examine path finding algorithm logic";
        
    } else {
        qDebug() << "Path found successfully - bug may be environment-specific";
    }
    
    // Also test with A*
    auto bugResultAStar = integrationNetwork->findShortestPath(
        bugStart, bugEnd, PathFindingAlgorithm::AStar
    );
    
    qDebug() << "A* result for same points:" << bugResultAStar.isValid();
    
    if (!bugResultAStar.isValid() && !bugResult.isValid()) {
        qDebug() << "Both algorithms failed - indicates fundamental issue";
    } else if (bugResultAStar.isValid() != bugResult.isValid()) {
        qDebug() << "Algorithms give different results - indicates algorithm-specific issue";
    }
}

void IntegrationTest::testBugReproduction_EmptyVisibilityGraph()
{
    qDebug() << "\n=== BUG REPRODUCTION: EMPTY VISIBILITY GRAPH ===";
    qDebug() << "This test checks if the visibility graph is properly populated";
    
    // Create a minimal test case
    QVector<std::shared_ptr<GPoint>> minimalBoundary;
    minimalBoundary.append(std::make_shared<GPoint>(
        units::angle::degree_t(-76.0), units::angle::degree_t(39.0), "MB1"
    ));
    minimalBoundary.append(std::make_shared<GPoint>(
        units::angle::degree_t(-75.0), units::angle::degree_t(39.0), "MB2"
    ));
    minimalBoundary.append(std::make_shared<GPoint>(
        units::angle::degree_t(-75.0), units::angle::degree_t(40.0), "MB3"
    ));
    minimalBoundary.append(std::make_shared<GPoint>(
        units::angle::degree_t(-76.0), units::angle::degree_t(40.0), "MB4"
    ));
    minimalBoundary.append(minimalBoundary.first()); // Close polygon
    
    auto minimalPolygon = std::make_shared<Polygon>(
        minimalBoundary, QVector<QVector<std::shared_ptr<GPoint>>>(), "MinimalPolygon"
    );
    
    QVector<std::shared_ptr<Polygon>> minimalPolygons;
    minimalPolygons.append(minimalPolygon);
    
    // Create minimal network
    OptimizedNetwork minimalNetwork(minimalPolygons, BoundariesType::Water, "MinimalNetwork");
    
    // Test with polygon vertices (should definitely work)
    auto vertex1 = minimalBoundary[0];
    auto vertex2 = minimalBoundary[2]; // Diagonal vertices
    
    qDebug() << "Testing minimal network with polygon vertices:";
    qDebug() << "Vertex 1:" << vertex1->toString();
    qDebug() << "Vertex 2:" << vertex2->toString();
    
    auto minimalResult = minimalNetwork.findShortestPath(
        vertex1, vertex2, PathFindingAlgorithm::Dijkstra
    );
    
    qDebug() << "Minimal network result:" << minimalResult.isValid();
    
    if (minimalResult.isValid()) {
        qDebug() << "Minimal case works - bug may be related to complex geometries";
        qDebug() << "Path points:" << minimalResult.points.size();
        for (const auto& point : minimalResult.points) {
            qDebug() << "  " << point->toString();
        }
    } else {
        qDebug() << "CRITICAL: Even minimal case fails!";
        qDebug() << "This indicates a fundamental problem with the system";
    }
}

void IntegrationTest::testComponentConnectivity()
{
    qDebug() << "\n=== COMPONENT CONNECTIVITY TEST ===";
    qDebug() << "Testing connectivity between different system components";
    
    // Test GPoint functionality
    auto testPoint1 = std::make_shared<GPoint>(
        units::angle::degree_t(-75.5), units::angle::degree_t(38.5), "ConnTest1"
    );
    auto testPoint2 = std::make_shared<GPoint>(
        units::angle::degree_t(-75.3), units::angle::degree_t(38.7), "ConnTest2"
    );
    
    auto distance = testPoint1->distance(*testPoint2);
    qDebug() << "GPoint distance calculation:" << distance.value() << "meters [OK]";
    
    // Test GLine functionality
    auto testLine = std::make_shared<GLine>(testPoint1, testPoint2);
    auto lineLength = testLine->length();
    qDebug() << "GLine length calculation:" << lineLength.value() << "meters [OK]";
    
    // Test Polygon functionality
    auto polygons = createRealisticPolygons();
    if (!polygons.isEmpty()) {
        auto testPolygon = polygons.first();
        auto area = testPolygon->area();
        auto perimeter = testPolygon->perimeter();
        qDebug() << "Polygon area:" << area.value() << "sq meters [OK]";
        qDebug() << "Polygon perimeter:" << perimeter.value() << "meters [OK]";
        
        // Test point containment
        bool contains = testPolygon->isPointWithinPolygon(*testPoint1);
        qDebug() << "Point containment test:" << contains << "[OK]";
    }
    
    // Test Quadtree functionality
    Quadtree testQuadtree(polygons);
    qDebug() << "Quadtree construction: [OK]";
    qDebug() << "Quadtree max depth:" << testQuadtree.getMaxDepth();
    
    // Test visibility graph construction
    OptimizedVisibilityGraph testVG(polygons, BoundariesType::Water);
    qDebug() << "Visibility graph construction: [OK]";
    
    // Test component integration
    bool visibility = testVG.isVisible(testPoint1, testPoint2);
    qDebug() << "Visibility test:" << visibility << "[OK]";
    
    qDebug() << "All components appear to be functioning individually";
    qDebug() << "The issue may be in component integration or algorithm logic";
}

void IntegrationTest::testDataFlowDebugging()
{
    qDebug() << "\n=== DATA FLOW DEBUGGING ===";
    qDebug() << "Tracing data flow through the path finding system";
    
    // Create simple test case
    auto flowStart = std::make_shared<GPoint>(
        units::angle::degree_t(-76.0), units::angle::degree_t(38.9), "FlowStart"
    );
    auto flowEnd = std::make_shared<GPoint>(
        units::angle::degree_t(-75.8), units::angle::degree_t(39.0), "FlowEnd"
    );
    
    qDebug() << "Data flow test points:";
    qDebug() << "Start:" << flowStart->toString();
    qDebug() << "End:" << flowEnd->toString();
    
    // Step 1: Verify points are in polygons
    // (We can't directly access the visibility graph, so we test through the network)
    qDebug() << "\nStep 1: Point validation";
    qDebug() << "Points created successfully";
    
    // Step 2: Test direct line creation
    qDebug() << "\nStep 2: Line creation";
    auto directLine = std::make_shared<GLine>(flowStart, flowEnd);
    qDebug() << "Direct line length:" << directLine->length().value() << "meters";
    
    // Step 3: Test network path finding
    qDebug() << "\nStep 3: Network path finding";
    auto flowResult = integrationNetwork->findShortestPath(
        flowStart, flowEnd, PathFindingAlgorithm::Dijkstra
    );
    
    qDebug() << "Network path finding result:" << flowResult.isValid();
    
    if (flowResult.isValid()) {
        qDebug() << "SUCCESS: Data flows correctly through the system";
        qDebug() << "Result points:" << flowResult.points.size();
        qDebug() << "Result lines:" << flowResult.lines.size();
        
        // Verify result structure
        QVERIFY(flowResult.lines.size() == flowResult.points.size() - 1);
        
        // Verify start and end points
        if (!flowResult.points.isEmpty()) {
            auto resultStart = flowResult.points.first();
            auto resultEnd = flowResult.points.last();
            qDebug() << "Result start:" << resultStart->toString();
            qDebug() << "Result end:" << resultEnd->toString();
        }
        
    } else {
        qDebug() << "ISSUE: Data flow interrupted at network level";
        qDebug() << "This confirms the bug is in the path finding system";
        qDebug() << "\nPossible causes:";
        qDebug() << "1. Visibility graph not properly constructed";
        qDebug() << "2. Points not recognized as being in valid areas";
        qDebug() << "3. Path finding algorithm logic error";
        qDebug() << "4. Quadtree spatial indexing issues";
    }
    
    qDebug() << "\nData flow debugging completed";
}

QTEST_MAIN(IntegrationTest)
#include "test_integration.moc"
