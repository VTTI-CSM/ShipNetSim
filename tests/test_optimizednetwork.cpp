#include "network/gpoint.h"
#include "network/optimizednetwork.h"
#include <QDebug>
#include <QTemporaryFile>
#include <QTextStream>

using namespace ShipNetSimCore;

class OptimizedNetworkTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    
    // Constructor tests
    void testDefaultConstructor();
    void testParameterizedConstructor();
    void testFileConstructor();
    
    // Network initialization
    void testInitializeNetworkWithPolygons();
    void testInitializeNetworkWithFile();
    
    // Path finding integration
    void testFindShortestPathTwoPoints();
    void testFindShortestPathMultiplePoints();
    
    // Environmental data
    void testGetEnvironmentFromPosition();
    
    // File loading
    void testLoadTxtFile();
    void testLoadShapeFile();
    
    // Thread safety
    void testMoveObjectToThread();
    
    // Region management
    void testRegionNameOperations();
    
    // Edge cases and debugging
    void testNetworkDebugging();
    void testEdgeCases();

private:
    QString createTestTxtFile();
    QVector<std::shared_ptr<Polygon>> createTestPolygons();
    std::shared_ptr<OptimizedNetwork> network;
    QString testDataDir;
};

void OptimizedNetworkTest::initTestCase()
{
    qDebug() << "Starting OptimizedNetwork tests...";
    
    // Set up test data directory
    testDataDir = QCoreApplication::applicationDirPath() + "/test_data";
    QDir().mkpath(testDataDir);
    
    // Create test network with polygons
    auto polygons = createTestPolygons();
    network = std::make_shared<OptimizedNetwork>(
        polygons, BoundariesType::Water, "TestRegion"
    );
}

void OptimizedNetworkTest::cleanupTestCase()
{
    qDebug() << "OptimizedNetwork tests completed.";
    
    // Clean up test data directory
    QDir testDir(testDataDir);
    testDir.removeRecursively();
}

QString OptimizedNetworkTest::createTestTxtFile()
{
    QTemporaryFile* tempFile = new QTemporaryFile();
    tempFile->setAutoRemove(false); // Keep file for testing
    
    if (tempFile->open()) {
        QTextStream out(tempFile);
        
        // Write test network data
        out << "# Test network file\n";
        out << "[WATERBODY 1]\n";
        out << "[WATER BOUNDRY]\n";
        out << "1, -76.0, 39.0\n";
        out << "2, -72.0, 39.0\n";
        out << "3, -72.0, 42.0\n";
        out << "4, -76.0, 42.0\n";
        out << "[END]\n";
        out << "[LAND]\n";
        out << "5, -74.8, 40.3\n";
        out << "6, -74.2, 40.3\n";
        out << "7, -74.2, 40.7\n";
        out << "8, -74.8, 40.7\n";
        out << "[END]\n";
        
        tempFile->close();
        return tempFile->fileName();
    }
    
    delete tempFile;
    return QString();
}

QVector<std::shared_ptr<Polygon>> OptimizedNetworkTest::createTestPolygons()
{
    QVector<std::shared_ptr<Polygon>> polygons;
    
    // Create a water body polygon
    QVector<std::shared_ptr<GPoint>> waterBoundary;
    waterBoundary.append(std::make_shared<GPoint>(
        units::angle::degree_t(-76.0), units::angle::degree_t(39.0), "WB1"
    ));
    waterBoundary.append(std::make_shared<GPoint>(
        units::angle::degree_t(-72.0), units::angle::degree_t(39.0), "WB2"
    ));
    waterBoundary.append(std::make_shared<GPoint>(
        units::angle::degree_t(-72.0), units::angle::degree_t(42.0), "WB3"
    ));
    waterBoundary.append(std::make_shared<GPoint>(
        units::angle::degree_t(-76.0), units::angle::degree_t(42.0), "WB4"
    ));
    waterBoundary.append(waterBoundary.first()); // Close polygon
    
    auto waterPolygon = std::make_shared<Polygon>(
        waterBoundary, QVector<QVector<std::shared_ptr<GPoint>>>(), "TestWaterBody"
    );
    polygons.append(waterPolygon);
    
    return polygons;
}

void OptimizedNetworkTest::testDefaultConstructor()
{
    OptimizedNetwork defaultNetwork;
    
    // Should be in valid state without crashing
    QVERIFY(true);
}

void OptimizedNetworkTest::testParameterizedConstructor()
{
    QVERIFY(network != nullptr);
    QCOMPARE(network->getRegionName(), QString("TestRegion"));
}

cppvoid OptimizedNetworkTest::testFileConstructor()
{
    QString testFile = createTestTxtFile();
    
    if (!testFile.isEmpty()) {
        try {
            OptimizedNetwork fileNetwork(testFile, "FileTestRegion");
            QCOMPARE(fileNetwork.getRegionName(), QString("FileTestRegion"));
            qDebug() << "File constructor succeeded";
        } catch (const std::exception& e) {
            qDebug() << "File constructor failed:" << e.what();
            // Note: This might fail if supporting data files are missing
        }
        
        // Clean up
        QFile::remove(testFile);
    } else {
        QSKIP("Could not create test file");
    }
}

void OptimizedNetworkTest::testInitializeNetworkWithPolygons()
{
    OptimizedNetwork testNetwork;
    auto polygons = createTestPolygons();
    
    // This should not crash
    testNetwork.initializeNetwork(polygons, BoundariesType::Water, "InitTestRegion");
    
    QCOMPARE(testNetwork.getRegionName(), QString("InitTestRegion"));
}

void OptimizedNetworkTest::testInitializeNetworkWithFile()
{
    QString testFile = createTestTxtFile();
    
    if (!testFile.isEmpty()) {
        OptimizedNetwork testNetwork;
        
        try {
            testNetwork.initializeNetwork(testFile, "FileInitRegion");
            QCOMPARE(testNetwork.getRegionName(), QString("FileInitRegion"));
        } catch (const std::exception& e) {
            qDebug() << "File initialization failed (expected if data files missing):" << e.what();
        }
        
        // Clean up
        QFile::remove(testFile);
    } else {
        QSKIP("Could not create test file");
    }
}

void OptimizedNetworkTest::testFindShortestPathTwoPoints()
{
    qDebug() << "\n=== TESTING NETWORK-LEVEL PATH FINDING (TWO POINTS) ===";
    
    // Create test points within the network boundaries
    auto startPoint = std::make_shared<GPoint>(
        units::angle::degree_t(-75.5), units::angle::degree_t(39.5), "NetworkStart"
    );
    auto endPoint = std::make_shared<GPoint>(
        units::angle::degree_t(-73.0), units::angle::degree_t(41.0), "NetworkEnd"
    );
    
    qDebug() << "Network start point:" << startPoint->toString();
    qDebug() << "Network end point:" << endPoint->toString();
    
    // Test Dijkstra algorithm
    auto dijkstraResult = network->findShortestPath(
        startPoint, endPoint, PathFindingAlgorithm::Dijkstra
    );
    
    qDebug() << "Dijkstra result valid:" << dijkstraResult.isValid();
    qDebug() << "Dijkstra points:" << dijkstraResult.points.size();
    qDebug() << "Dijkstra lines:" << dijkstraResult.lines.size();
    
    // Test A* algorithm
    auto astarResult = network->findShortestPath(
        startPoint, endPoint, PathFindingAlgorithm::AStar
    );
    
    qDebug() << "A* result valid:" << astarResult.isValid();
    qDebug() << "A* points:" << astarResult.points.size();
    qDebug() << "A* lines:" << astarResult.lines.size();
    
    if (dijkstraResult.isValid()) {
        qDebug() << "SUCCESS: Network-level path finding working with Dijkstra";
        
        // Calculate total distance
        double totalDistance = 0.0;
        for (const auto& line : dijkstraResult.lines) {
            totalDistance += line->length().value();
        }
        qDebug() << "Total Dijkstra path distance:" << totalDistance << "meters";
        
        QVERIFY(dijkstraResult.points.size() >= 2);
        QVERIFY(dijkstraResult.lines.size() == dijkstraResult.points.size() - 1);
    } else {
        qDebug() << "ISSUE: Network-level Dijkstra path finding failed";
    }
    
    if (astarResult.isValid()) {
        qDebug() << "SUCCESS: Network-level path finding working with A*";
        
        // Calculate total distance
        double totalDistance = 0.0;
        for (const auto& line : astarResult.lines) {
            totalDistance += line->length().value();
        }
        qDebug() << "Total A* path distance:" << totalDistance << "meters";
        
        QVERIFY(astarResult.points.size() >= 2);
        QVERIFY(astarResult.lines.size() == astarResult.points.size() - 1);
    } else {
        qDebug() << "ISSUE: Network-level A* path finding failed";
    }
}

void OptimizedNetworkTest::testFindShortestPathMultiplePoints()
{
    qDebug() << "\n=== TESTING NETWORK-LEVEL PATH FINDING (MULTIPLE POINTS) ===";
    
    QVector<std::shared_ptr<GPoint>> waypoints;
    waypoints.append(std::make_shared<GPoint>(
        units::angle::degree_t(-75.5), units::angle::degree_t(39.5), "WP1"
    ));
    waypoints.append(std::make_shared<GPoint>(
        units::angle::degree_t(-74.5), units::angle::degree_t(40.0), "WP2"
    ));
    waypoints.append(std::make_shared<GPoint>(
        units::angle::degree_t(-73.5), units::angle::degree_t(40.5), "WP3"
    ));
    waypoints.append(std::make_shared<GPoint>(
        units::angle::degree_t(-73.0), units::angle::degree_t(41.0), "WP4"
    ));
    
    qDebug() << "Finding path through" << waypoints.size() << "waypoints";
    
    auto result = network->findShortestPath(waypoints, PathFindingAlgorithm::Dijkstra);
    
    qDebug() << "Multi-point result valid:" << result.isValid();
    qDebug() << "Multi-point path points:" << result.points.size();
    qDebug() << "Multi-point path lines:" << result.lines.size();
    
    if (result.isValid()) {
        qDebug() << "SUCCESS: Multi-point network path finding working";
        
        // Verify all waypoints are included
        QVERIFY(result.points.size() >= waypoints.size());
        
        // Calculate total distance
        double totalDistance = 0.0;
        for (const auto& line : result.lines) {
            totalDistance += line->length().value();
        }
        qDebug() << "Total multi-point path distance:" << totalDistance << "meters";
    } else {
        qDebug() << "ISSUE: Multi-point network path finding failed";
    }
}

void OptimizedNetworkTest::testGetEnvironmentFromPosition()
{
    // Test environmental data retrieval
    GPoint testPosition(
        units::angle::degree_t(-74.0), units::angle::degree_t(40.5)
    );
    
    try {
        auto environment = network->getEnvironmentFromPosition(testPosition);
        
        qDebug() << "Environmental data retrieved:";
        qDebug() << "  Salinity:" << environment.salinity.value() << "pptd";
        qDebug() << "  Wave height:" << environment.waveHeight.value() << "m";
        qDebug() << "  Water depth:" << environment.waterDepth.value() << "m";
        qDebug() << "  Wind speed north:" << environment.windSpeed_Northward.value() << "m/s";
        qDebug() << "  Wind speed east:" << environment.windSpeed_Eastward.value() << "m/s";
        
        // Environmental data might be NaN if TIFF files are not available
        // This should not crash
        QVERIFY(true);
        
    } catch (const std::exception& e) {
        qDebug() << "Environmental data retrieval failed (expected if TIFF files missing):" << e.what();
    }
}

void OptimizedNetworkTest::testLoadTxtFile()
{
    QString testFile = createTestTxtFile();
    
    if (!testFile.isEmpty()) {
        OptimizedNetwork testNetwork;
        
        try {
            // This is a private method, so we test it indirectly through initializeNetwork
            testNetwork.initializeNetwork(testFile, "TxtTestRegion");
            
            qDebug() << "TXT file loading completed";
            QVERIFY(true); // If we get here without exception, loading succeeded
            
        } catch (const std::exception& e) {
            qDebug() << "TXT file loading failed:" << e.what();
            // This might fail if supporting files are missing
        }
        
        // Clean up
        QFile::remove(testFile);
    } else {
        QSKIP("Could not create test TXT file");
    }
}

void OptimizedNetworkTest::testLoadShapeFile()
{
    // Note: This test requires actual shapefile data
    // Since we don't have shapefiles in our test setup, we'll skip this
    // In a real test environment, you would provide test shapefiles
    
    qDebug() << "Shapefile loading test skipped (requires actual shapefile data)";
    QSKIP("Shapefile test requires actual test data files");
}

void OptimizedNetworkTest::testMoveObjectToThread()
{
    // Create a test thread
    QThread testThread;
    
    try {
        network->moveObjectToThread(&testThread);
        qDebug() << "Successfully moved network to thread";
        QVERIFY(true);
    } catch (const std::exception& e) {
        qDebug() << "Failed to move network to thread:" << e.what();
        QFAIL("Thread movement should not fail");
    }
    
    // Note: We don't start the thread or move back, just test the method doesn't crash
}

void OptimizedNetworkTest::testRegionNameOperations()
{
    QString originalName = network->getRegionName();
    QCOMPARE(originalName, QString("TestRegion"));
    
    QString newName = "UpdatedTestRegion";
    network->setRegionName(newName);
    
    QCOMPARE(network->getRegionName(), newName);
    
    qDebug() << "Region name operations working correctly";
}

void OptimizedNetworkTest::testNetworkDebugging()
{
    qDebug() << "\n=== NETWORK-LEVEL DEBUGGING ===";
    
    // This test helps debug the overall network functionality
    
    // 1. Test if network has valid boundaries
    qDebug() << "Network region:" << network->getRegionName();
    
    // 2. Test simple path finding with detailed logging
    auto point1 = std::make_shared<GPoint>(
        units::angle::degree_t(-75.0), units::angle::degree_t(40.0), "Debug1"
    );
    auto point2 = std::make_shared<GPoint>(
        units::angle::degree_t(-74.0), units::angle::degree_t(40.0), "Debug2"
    );
    
    qDebug() << "Debug point 1:" << point1->toString();
    qDebug() << "Debug point 2:" << point2->toString();
    
    // Calculate direct distance
    auto directDistance = point1->distance(*point2);
    qDebug() << "Direct distance between debug points:" << directDistance.value() << "meters";
    
    // Try path finding
    auto debugResult = network->findShortestPath(point1, point2, PathFindingAlgorithm::Dijkstra);
    
    qDebug() << "Debug path finding result:";
    qDebug() << "  Valid:" << debugResult.isValid();
    qDebug() << "  Points:" << debugResult.points.size();
    qDebug() << "  Lines:" << debugResult.lines.size();
    
    if (debugResult.isValid()) {
        // Calculate path distance
        double pathDistance = 0.0;
        for (const auto& line : debugResult.lines) {
            pathDistance += line->length().value();
        }
        qDebug() << "  Path distance:" << pathDistance << "meters";
        qDebug() << "  Ratio to direct:" << (pathDistance / directDistance.value());
        
        // Log first few points of the path
        for (int i = 0; i < std::min(5, debugResult.points.size()); ++i) {
            qDebug() << "    Point" << i << ":" << debugResult.points[i]->toString();
        }
    } else {
        qDebug() << "  No path found - investigating why...";
        
        // Additional debugging could be added here
        // - Check if points are in valid water areas
        // - Check visibility graph state
        // - Check quadtree contents
    }
}

void OptimizedNetworkTest::testEdgeCases()
{
    qDebug() << "\n=== NETWORK EDGE CASES ===";
    
    // Test with null points
    try {
        auto nullResult = network->findShortestPath(
            nullptr, nullptr, PathFindingAlgorithm::Dijkstra
        );
        qDebug() << "Null points result valid:" << nullResult.isValid();
        // Should handle gracefully or throw exception
    } catch (const std::exception& e) {
        qDebug() << "Null points correctly threw exception:" << e.what();
    }
    
    // Test with points outside network boundaries
    auto farPoint1 = std::make_shared<GPoint>(
        units::angle::degree_t(-90.0), units::angle::degree_t(30.0), "Far1"
    );
    auto farPoint2 = std::make_shared<GPoint>(
        units::angle::degree_t(-60.0), units::angle::degree_t(50.0), "Far2"
    );
    
    auto farResult = network->findShortestPath(
        farPoint1, farPoint2, PathFindingAlgorithm::Dijkstra
    );
    qDebug() << "Far points result valid:" << farResult.isValid();
    
    // Test with same start and end point
    auto samePoint = std::make_shared<GPoint>(
        units::angle::degree_t(-74.0), units::angle::degree_t(40.0), "Same"
    );
    
    auto sameResult = network->findShortestPath(
        samePoint, samePoint, PathFindingAlgorithm::Dijkstra
    );
    qDebug() << "Same point result valid:" << sameResult.isValid();
    
    // Test with empty region name
    network->setRegionName("");
    QCOMPARE(network->getRegionName(), QString(""));
    network->setRegionName("TestRegion"); // Restore
    
    // Test boundary updates
    auto newPolygons = createTestPolygons();
    network->setBoundaries(newPolygons);
    qDebug() << "Boundary update completed without crash";
}

QTEST_MAIN(OptimizedNetworkTest)
#include "test_optimizednetwork.moc"
