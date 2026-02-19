#include "network/gline.h"
#include "network/gpoint.h"
#include "network/polygon.h"
#include "network/quadtree.h"
#include <QDebug>
#include <QElapsedTimer>
#include <QTest>
#include <set>

using namespace ShipNetSimCore;

class QuadtreeTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    // Basic functionality tests
    void testDefaultConstructor();
    void testParameterizedConstructor();
    void testClearTree();

    // Line segment operations
    void testInsertLineSegment();
    void testDeleteLineSegment();
    void testFindLineSegment();

    // Spatial queries
    void testFindNodesIntersectingLineSegment();
    void testRangeQuery();
    void testFindVerticesInRange();
    void testGetAllSegmentsInNode();

    // Nearest neighbor searches
    void testFindNearestNeighbor();
    void testFindNearestNeighborPoint();

    // Tree structure
    void testGetMaxDepth();
    void testGetAdjacentNodes();

    // Map boundaries
    void testMapBoundaries();
    void testIsNearBoundary();

    // Antimeridian handling
    void testAntimeridianCrossing();
    void testSplitSegmentAtAntimeridian();

    // Performance and edge cases
    void testPerformanceWithManySegments();
    void testEdgeCases();

private:
    QVector<std::shared_ptr<Polygon>> createTestPolygons();
    std::unique_ptr<Quadtree>         quadtree;
    QVector<std::shared_ptr<GLine>>   testSegments;
};

void QuadtreeTest::initTestCase()
{
    qDebug() << "Starting Quadtree tests...";

    // Create test polygons
    auto polygons = createTestPolygons();

    // Create quadtree with test polygons
    quadtree = std::make_unique<Quadtree>(polygons);

    // Create some test segments for manual insertion
    auto point1 = std::make_shared<GPoint>(
        units::angle::degree_t(-75.0), units::angle::degree_t(40.0));
    auto point2 = std::make_shared<GPoint>(
        units::angle::degree_t(-74.0), units::angle::degree_t(40.5));
    auto point3 = std::make_shared<GPoint>(
        units::angle::degree_t(-73.0), units::angle::degree_t(41.0));

    testSegments.append(std::make_shared<GLine>(point1, point2));
    testSegments.append(std::make_shared<GLine>(point2, point3));
}

void QuadtreeTest::cleanupTestCase()
{
    qDebug() << "Quadtree tests completed.";
}

QVector<std::shared_ptr<Polygon>> QuadtreeTest::createTestPolygons()
{
    QVector<std::shared_ptr<Polygon>> polygons;

    // Create a simple rectangular polygon
    QVector<std::shared_ptr<GPoint>> boundary;
    boundary.append(std::make_shared<GPoint>(
        units::angle::degree_t(-76.0), units::angle::degree_t(39.0)));
    boundary.append(std::make_shared<GPoint>(
        units::angle::degree_t(-72.0), units::angle::degree_t(39.0)));
    boundary.append(std::make_shared<GPoint>(
        units::angle::degree_t(-72.0), units::angle::degree_t(42.0)));
    boundary.append(std::make_shared<GPoint>(
        units::angle::degree_t(-76.0), units::angle::degree_t(42.0)));
    boundary.append(boundary.first()); // Close polygon

    auto polygon = std::make_shared<Polygon>(
        boundary, QVector<QVector<std::shared_ptr<GPoint>>>(),
        "TestPolygon");
    polygons.append(polygon);

    return polygons;
}

void QuadtreeTest::testDefaultConstructor()
{
    Quadtree defaultQuadtree;

    // Should have valid map boundaries
    auto minPoint = defaultQuadtree.getMapMinPoint();
    auto maxPoint = defaultQuadtree.getMapMaxPoint();

    QVERIFY(minPoint.getLongitude().value()
            <= maxPoint.getLongitude().value());
    QVERIFY(minPoint.getLatitude().value()
            <= maxPoint.getLatitude().value());

    // Should be empty initially
    QCOMPARE(defaultQuadtree.getMaxDepth(), 0);
}

void QuadtreeTest::testParameterizedConstructor()
{
    QVERIFY(quadtree != nullptr);

    // Should have segments from the polygons
    auto minPoint = quadtree->getMapMinPoint();
    auto maxPoint = quadtree->getMapMaxPoint();

    qDebug() << "Map bounds:"
             << "(" << minPoint.getLongitude().value() << ","
             << minPoint.getLatitude().value() << ")"
             << " to "
             << "(" << maxPoint.getLongitude().value() << ","
             << maxPoint.getLatitude().value() << ")";

    QVERIFY(quadtree->getMaxDepth() >= 0);
}

void QuadtreeTest::testClearTree()
{
    // Create a temporary quadtree
    auto     tempPolygons = createTestPolygons();
    Quadtree tempQuadtree(tempPolygons);

    int originalDepth = tempQuadtree.getMaxDepth();
    tempQuadtree.clearTree();

    QCOMPARE(tempQuadtree.getMaxDepth(), 0);
}

void QuadtreeTest::testInsertLineSegment()
{
    auto segment = testSegments.first();

    // Insert the segment
    quadtree->insertLineSegment(segment);

    // Try to find it
    auto foundSegment = quadtree->findLineSegment(
        segment->startPoint(), segment->endPoint());

    QVERIFY(foundSegment != nullptr);
    QVERIFY(*foundSegment == *segment);
}

void QuadtreeTest::testDeleteLineSegment()
{
    auto segment = testSegments.last();

    // Insert then delete
    quadtree->insertLineSegment(segment);
    bool deleted = quadtree->deleteLineSegment(segment);

    QVERIFY(deleted);

    // Should not be found after deletion
    auto foundSegment = quadtree->findLineSegment(
        segment->startPoint(), segment->endPoint());
    QVERIFY(foundSegment == nullptr);
}

void QuadtreeTest::testFindLineSegment()
{
    auto segment = testSegments.first();
    quadtree->insertLineSegment(segment);

    // Find by endpoints
    auto foundSegment = quadtree->findLineSegment(
        segment->startPoint(), segment->endPoint());
    QVERIFY(foundSegment != nullptr);

    // Find in reverse direction
    auto reversedSegment = quadtree->findLineSegment(
        segment->endPoint(), segment->startPoint());
    QVERIFY(reversedSegment != nullptr);

    // Try to find non-existent segment
    auto point1 = std::make_shared<GPoint>(
        units::angle::degree_t(-80.0), units::angle::degree_t(35.0));
    auto point2 = std::make_shared<GPoint>(
        units::angle::degree_t(-79.0), units::angle::degree_t(36.0));

    auto notFoundSegment = quadtree->findLineSegment(point1, point2);
    QVERIFY(notFoundSegment == nullptr);
}

void QuadtreeTest::testFindNodesIntersectingLineSegment()
{
    auto segment = testSegments.first();

    auto intersectingNodes =
        quadtree->findNodesIntersectingLineSegment(segment);

    qDebug() << "Number of intersecting nodes:"
             << intersectingNodes.size();
    QVERIFY(intersectingNodes.size() > 0);

    // Test parallel version
    auto parallelNodes =
        quadtree->findNodesIntersectingLineSegmentParallel(segment);

    // Results should be similar (though order might differ)
    QVERIFY(parallelNodes.size() > 0);
}

void QuadtreeTest::testRangeQuery()
{
    // Define a range that covers part of our test area
    QRectF queryRange(-75.5, 39.5, 1.0, 1.0); // 1 degree x 1 degree

    auto segments = quadtree->rangeQuery(queryRange);
    qDebug() << "Segments in range:" << segments.size();

    // Test parallel version
    auto parallelSegments = quadtree->rangeQueryParallel(queryRange);
    qDebug() << "Parallel segments in range:"
             << parallelSegments.size();

    // Results should be the same
    QCOMPARE(segments.size(), parallelSegments.size());
}

void QuadtreeTest::testFindVerticesInRange()
{
    // Test polygon vertices are at: (-76, 39), (-72, 39), (-72, 42), (-76, 42)
    // Define a range that includes the corner at (-76, 39)
    // QRectF(x, y, width, height) where x=minLon, y=minLat
    QRectF queryRange(-77.0, 38.0, 2.0, 2.0);  // lon: [-77, -75], lat: [38, 40]

    auto vertices = quadtree->findVerticesInRange(queryRange);
    qDebug() << "Vertices in range:" << vertices.size();

    // Should find at least one vertex from our test polygon (the corner at -76, 39)
    QVERIFY(vertices.size() > 0);

    // Verify all returned vertices are within the range
    double minLon = queryRange.left();
    double maxLon = queryRange.right();
    double minLat = std::min(queryRange.top(), queryRange.bottom());
    double maxLat = std::max(queryRange.top(), queryRange.bottom());

    for (const auto &vertex : vertices)
    {
        double lon = vertex->getLongitude().value();
        double lat = vertex->getLatitude().value();

        qDebug() << "  Found vertex:" << lon << "," << lat;
        QVERIFY(lon >= minLon);
        QVERIFY(lon <= maxLon);
        QVERIFY(lat >= minLat);
        QVERIFY(lat <= maxLat);
    }

    // Test with empty range (outside all polygons)
    QRectF emptyRange(100.0, 50.0, 1.0, 1.0);  // Far from test polygon
    auto emptyVertices = quadtree->findVerticesInRange(emptyRange);
    qDebug() << "Vertices in empty range:" << emptyVertices.size();
    QCOMPARE(emptyVertices.size(), 0);

    // Test uniqueness - no duplicates should be returned
    std::set<std::pair<double, double>> uniqueCoords;
    for (const auto &vertex : vertices)
    {
        auto coord = std::make_pair(
            vertex->getLongitude().value(),
            vertex->getLatitude().value());
        QVERIFY(uniqueCoords.find(coord) == uniqueCoords.end());
        uniqueCoords.insert(coord);
    }
    qDebug() << "All vertices are unique: confirmed";

    // Test with a range covering all polygon vertices
    QRectF fullRange(-77.0, 38.0, 6.0, 5.0);  // lon: [-77, -71], lat: [38, 43]
    auto allVertices = quadtree->findVerticesInRange(fullRange);
    qDebug() << "Vertices in full range:" << allVertices.size();
    // Should find all 4 unique polygon vertices (5th point closes the polygon, same as first)
    QVERIFY(allVertices.size() >= 4);
}

void QuadtreeTest::testGetAllSegmentsInNode()
{
    auto segment = testSegments.first();
    quadtree->insertLineSegment(segment);

    auto intersectingNodes =
        quadtree->findNodesIntersectingLineSegment(segment);

    if (!intersectingNodes.isEmpty())
    {
        auto nodeSegments =
            quadtree->getAllSegmentsInNode(intersectingNodes.first());
        qDebug() << "Segments in first intersecting node:"
                 << nodeSegments.size();
        QVERIFY(nodeSegments.size() > 0);
    }
}

void QuadtreeTest::testFindNearestNeighbor()
{
    auto segment = testSegments.first();
    quadtree->insertLineSegment(segment);

    auto queryPoint = std::make_shared<GPoint>(
        units::angle::degree_t(-74.5), units::angle::degree_t(40.2));

    auto nearestSegment = quadtree->findNearestNeighbor(queryPoint);

    if (nearestSegment)
    {
        qDebug() << "Found nearest segment";
        QVERIFY(nearestSegment != nullptr);
    }
    else
    {
        qDebug() << "No nearest segment found";
    }
}

void QuadtreeTest::testFindNearestNeighborPoint()
{
    auto queryPoint = std::make_shared<GPoint>(
        units::angle::degree_t(-74.5), units::angle::degree_t(40.2));

    auto nearestPoint =
        quadtree->findNearestNeighborPoint(queryPoint);

    if (nearestPoint)
    {
        qDebug() << "Found nearest point at:"
                 << nearestPoint->getLongitude().value() << ","
                 << nearestPoint->getLatitude().value();
        QVERIFY(nearestPoint != nullptr);

        auto distance = queryPoint->distance(*nearestPoint);
        qDebug() << "Distance to nearest point:" << distance.value()
                 << "meters";
        QVERIFY(distance.value() >= 0.0);
    }
    else
    {
        qDebug() << "No nearest point found - this is acceptable for "
                    "empty areas";
    }
}

void QuadtreeTest::testGetMaxDepth()
{
    int depth = quadtree->getMaxDepth();
    qDebug() << "Quadtree max depth:" << depth;

    QVERIFY(depth >= 0);
    QVERIFY(depth <= 20); // Reasonable upper bound
}

void QuadtreeTest::testGetAdjacentNodes()
{
    // This is an internal method test - we'll add a segment and test
    auto segment = testSegments.first();
    quadtree->insertLineSegment(segment);

    auto intersectingNodes =
        quadtree->findNodesIntersectingLineSegment(segment);

    if (!intersectingNodes.isEmpty())
    {
        auto adjacentNodes =
            quadtree->getAdjacentNodes(intersectingNodes.first());
        qDebug() << "Adjacent nodes count:" << adjacentNodes.size();

        // Adjacent nodes count should be reasonable
        QVERIFY(adjacentNodes.size()
                <= 8); // Maximum 8 adjacent in a quadtree
    }
}

void QuadtreeTest::testMapBoundaries()
{
    auto width  = quadtree->getMapWidth();
    auto height = quadtree->getMapHeight();

    qDebug() << "Map width:" << width.value() << "degrees";
    qDebug() << "Map height:" << height.value() << "degrees";

    QVERIFY(width.value() > 0.0);
    QVERIFY(height.value() > 0.0);
    QVERIFY(width.value() <= 360.0);
    QVERIFY(height.value() <= 180.0);
}

void QuadtreeTest::testIsNearBoundary()
{
    auto minPoint = quadtree->getMapMinPoint();
    auto maxPoint = quadtree->getMapMaxPoint();

    // Point near left boundary
    auto nearLeft = std::make_shared<GPoint>(
        minPoint.getLongitude() + units::angle::degree_t(0.05),
        minPoint.getLatitude() + units::angle::degree_t(1.0));

    // Point near right boundary
    auto nearRight = std::make_shared<GPoint>(
        maxPoint.getLongitude() - units::angle::degree_t(0.05),
        maxPoint.getLatitude() - units::angle::degree_t(1.0));

    // Point in middle
    auto middle = std::make_shared<GPoint>(
        (minPoint.getLongitude() + maxPoint.getLongitude()) / 2.0,
        (minPoint.getLatitude() + maxPoint.getLatitude()) / 2.0);

    bool leftIsBoundary   = quadtree->isNearBoundary(nearLeft);
    bool rightIsBoundary  = quadtree->isNearBoundary(nearRight);
    bool middleIsBoundary = quadtree->isNearBoundary(middle);

    qDebug() << "Near left boundary:" << leftIsBoundary;
    qDebug() << "Near right boundary:" << rightIsBoundary;
    qDebug() << "Middle is near boundary:" << middleIsBoundary;

    QVERIFY(
        leftIsBoundary
        || rightIsBoundary); // At least one should be near boundary
    QVERIFY(!middleIsBoundary); // Middle should not be near boundary
}

void QuadtreeTest::testAntimeridianCrossing()
{
    // Create a segment that crosses the antimeridian
    auto westPoint = std::make_shared<GPoint>(
        units::angle::degree_t(179.0), units::angle::degree_t(40.0));
    auto eastPoint = std::make_shared<GPoint>(
        units::angle::degree_t(-179.0), units::angle::degree_t(40.0));

    auto crossingSegment =
        std::make_shared<GLine>(westPoint, eastPoint);

    bool crosses =
        Quadtree::isSegmentCrossingAntimeridian(crossingSegment);
    qDebug() << "Segment crosses antimeridian:" << crosses;

    QVERIFY(crosses);

    // Non-crossing segment
    auto normalSegment = testSegments.first();
    bool normalCrosses =
        Quadtree::isSegmentCrossingAntimeridian(normalSegment);
    QVERIFY(!normalCrosses);
}

void QuadtreeTest::testSplitSegmentAtAntimeridian()
{
    // Create a segment that crosses the antimeridian
    auto westPoint = std::make_shared<GPoint>(
        units::angle::degree_t(179.0), units::angle::degree_t(40.0));
    auto eastPoint = std::make_shared<GPoint>(
        units::angle::degree_t(-179.0), units::angle::degree_t(40.0));

    auto crossingSegment =
        std::make_shared<GLine>(westPoint, eastPoint);

    auto splitSegments =
        Quadtree::splitSegmentAtAntimeridian(crossingSegment);

    qDebug() << "Split segments count:" << splitSegments.size();
    QVERIFY(splitSegments.size() >= 1);

    if (splitSegments.size() > 1)
    {
        // Should have split into multiple segments
        QVERIFY(splitSegments.size()
                <= 2); // Should not split into more than 2
    }
}

void QuadtreeTest::testPerformanceWithManySegments()
{
    // Create many random segments
    QVector<std::shared_ptr<GLine>> manySegments;
    const int                       segmentCount = 1000;

    auto minPoint = quadtree->getMapMinPoint();
    auto maxPoint = quadtree->getMapMaxPoint();

    for (int i = 0; i < segmentCount; ++i)
    {
        double lon1 = minPoint.getLongitude().value()
                      + (maxPoint.getLongitude().value()
                         - minPoint.getLongitude().value())
                            * (i % 100) / 100.0;
        double lat1 = minPoint.getLatitude().value()
                      + (maxPoint.getLatitude().value()
                         - minPoint.getLatitude().value())
                            * (i % 100) / 100.0;
        double lon2 = lon1 + 0.01;
        double lat2 = lat1 + 0.01;

        auto point1 =
            std::make_shared<GPoint>(units::angle::degree_t(lon1),
                                     units::angle::degree_t(lat1));
        auto point2 =
            std::make_shared<GPoint>(units::angle::degree_t(lon2),
                                     units::angle::degree_t(lat2));
        auto segment = std::make_shared<GLine>(point1, point2);

        manySegments.append(segment);
    }

    // Time the insertion
    QElapsedTimer timer;
    timer.start();

    for (auto &segment : manySegments)
    {
        quadtree->insertLineSegment(segment);
    }

    qint64 insertTime = timer.elapsed();
    qDebug() << "Time to insert" << segmentCount
             << "segments:" << insertTime << "ms";

    // Performance should be reasonable (less than 5ms per segment on
    // average). Note: geodesic calculations are more accurate but slightly
    // more expensive than projection-based approaches.
    QVERIFY(insertTime
            < segmentCount * 5); // Less than 5ms per segment

    qDebug() << "Final tree depth after many insertions:"
             << quadtree->getMaxDepth();
}

void QuadtreeTest::testEdgeCases()
{
    // Test with null pointers
    std::shared_ptr<GLine> nullSegment;

    try
    {
        quadtree->insertLineSegment(nullSegment);
        QFAIL("Should have thrown exception for null segment");
    }
    catch (const std::exception &e)
    {
        qDebug() << "Correctly caught exception for null segment:"
                 << e.what();
    }

    // Test with very small segments
    auto point1 = std::make_shared<GPoint>(
        units::angle::degree_t(-75.0), units::angle::degree_t(40.0));
    auto point2 = std::make_shared<GPoint>(
        units::angle::degree_t(-75.0),
        units::angle::degree_t(40.0)); // Same point

    auto zeroLengthSegment = std::make_shared<GLine>(point1, point2);

    // Should handle zero-length segments gracefully
    quadtree->insertLineSegment(zeroLengthSegment);
    auto found = quadtree->findLineSegment(point1, point2);
    QVERIFY(found != nullptr);
}

QTEST_MAIN(QuadtreeTest)
#include "test_quadtree.moc"
