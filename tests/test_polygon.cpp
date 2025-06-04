#include "network/gline.h"
#include "network/gpoint.h"
#include "network/polygon.h"
#include <QDebug>
#include <QTest>

using namespace ShipNetSimCore;

class PolygonTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    // Basic functionality tests
    void testDefaultConstructor();
    void testParameterizedConstructor();
    void testAccessors();

    // Geometric calculations
    void testArea();
    void testPerimeter();

    // Point containment tests
    void testPointWithinExteriorRing();
    void testPointWithinInteriorRings();
    void testPointWithinPolygon();
    void testContains();

    // Line intersection tests
    void testIntersects();

    // Clear width tests
    void testGetMaxClearWidth();

    // String representation
    void testToString();

    // Edge cases
    void testEdgeCases();

private:
    QVector<std::shared_ptr<GPoint>>
    createSquareBoundary(double centerLon, double centerLat,
                         double size);
    QVector<std::shared_ptr<GPoint>>          outerBoundary;
    QVector<QVector<std::shared_ptr<GPoint>>> innerHoles;
    std::shared_ptr<Polygon>                  testPolygon;
};

void PolygonTest::initTestCase()
{
    qDebug() << "Starting Polygon tests...";

    // Create a square outer boundary (1 degree x 1 degree)
    outerBoundary = createSquareBoundary(-75.0, 40.0, 0.5);

    // Create a small square hole inside
    QVector<std::shared_ptr<GPoint>> hole =
        createSquareBoundary(-75.0, 40.0, 0.1);
    innerHoles.append(hole);

    // Create test polygon
    testPolygon = std::make_shared<Polygon>(outerBoundary, innerHoles,
                                            "TestPolygon");
}

void PolygonTest::cleanupTestCase()
{
    qDebug() << "Polygon tests completed.";
}

QVector<std::shared_ptr<GPoint>>
PolygonTest::createSquareBoundary(double centerLon, double centerLat,
                                  double size)
{
    QVector<std::shared_ptr<GPoint>> boundary;

    // Create square vertices (clockwise from bottom-left)
    boundary.append(std::make_shared<GPoint>(
        units::angle::degree_t(centerLon - size),
        units::angle::degree_t(centerLat - size),
        QString("Point_%1_%2_BL").arg(centerLon).arg(centerLat)));

    boundary.append(std::make_shared<GPoint>(
        units::angle::degree_t(centerLon + size),
        units::angle::degree_t(centerLat - size),
        QString("Point_%1_%2_BR").arg(centerLon).arg(centerLat)));

    boundary.append(std::make_shared<GPoint>(
        units::angle::degree_t(centerLon + size),
        units::angle::degree_t(centerLat + size),
        QString("Point_%1_%2_TR").arg(centerLon).arg(centerLat)));

    boundary.append(std::make_shared<GPoint>(
        units::angle::degree_t(centerLon - size),
        units::angle::degree_t(centerLat + size),
        QString("Point_%1_%2_TL").arg(centerLon).arg(centerLat)));

    // Close the polygon by repeating the first point
    boundary.append(boundary.first());

    return boundary;
}

void PolygonTest::testDefaultConstructor()
{
    Polygon defaultPolygon;
    QVERIFY(defaultPolygon.outer().isEmpty());
    QVERIFY(defaultPolygon.inners().isEmpty());
}

void PolygonTest::testParameterizedConstructor()
{
    QCOMPARE(testPolygon->outer().size(), outerBoundary.size());
    QCOMPARE(testPolygon->inners().size(), 1);
    QCOMPARE(testPolygon->inners().first().size(),
             innerHoles.first().size());
}

void PolygonTest::testAccessors()
{
    // Test setters
    auto newBoundary = createSquareBoundary(-76.0, 41.0, 0.3);

    Polygon modifiablePolygon;
    modifiablePolygon.setOuterPoints(newBoundary);
    QCOMPARE(modifiablePolygon.outer().size(), newBoundary.size());

    QVector<QVector<std::shared_ptr<GPoint>>> newHoles;
    newHoles.append(createSquareBoundary(-76.0, 41.0, 0.05));
    modifiablePolygon.setInnerHolesPoints(newHoles);
    QCOMPARE(modifiablePolygon.inners().size(), 1);
}

void PolygonTest::testArea()
{
    auto area = testPolygon->area();
    qDebug() << "Polygon area:" << area.value() << "square meters";

    QVERIFY(area.value() > 0.0);

    // For a roughly 1 degree x 1 degree square at 40°N,
    // the area should be approximately 9,000-12,000 km²
    QVERIFY(area.value() > 8000000000.0);  // 8,000 km²
    QVERIFY(area.value() < 15000000000.0); // 15,000 km²
}

void PolygonTest::testPerimeter()
{
    auto perimeter = testPolygon->perimeter();
    qDebug() << "Polygon perimeter:" << perimeter.value() << "meters";

    QVERIFY(perimeter.value() > 0.0);

    // For a roughly 1 degree x 1 degree square at 40°N,
    // the perimeter should be approximately 400-450 km
    QVERIFY(perimeter.value() > 350000.0); // 350 km
    QVERIFY(perimeter.value() < 500000.0); // 500 km
}

void PolygonTest::testPointWithinExteriorRing()
{
    // Point clearly inside the outer boundary
    GPoint insidePoint(units::angle::degree_t(-75.0),
                       units::angle::degree_t(40.0));
    QVERIFY(testPolygon->isPointWithinExteriorRing(insidePoint));

    // Point clearly outside the outer boundary
    GPoint outsidePoint(units::angle::degree_t(-76.0),
                        units::angle::degree_t(41.0));
    QVERIFY(!testPolygon->isPointWithinExteriorRing(outsidePoint));

    // Point on the boundary
    GPoint boundaryPoint(units::angle::degree_t(-74.5),
                         units::angle::degree_t(40.5));
    // Boundary points should return true
    QVERIFY(testPolygon->isPointWithinExteriorRing(boundaryPoint));
}

void PolygonTest::testPointWithinInteriorRings()
{
    // Point inside the hole
    GPoint holePoint(units::angle::degree_t(-75.0),
                     units::angle::degree_t(40.0));
    QVERIFY(testPolygon->isPointWithinInteriorRings(holePoint));

    // Point outside all holes
    GPoint outsideHolePoint(units::angle::degree_t(-74.8),
                            units::angle::degree_t(40.2));
    QVERIFY(
        !testPolygon->isPointWithinInteriorRings(outsideHolePoint));
}

void PolygonTest::testPointWithinPolygon()
{
    // Point inside polygon but outside holes
    GPoint validPoint(units::angle::degree_t(-74.8),
                      units::angle::degree_t(40.2));
    QVERIFY(testPolygon->isPointWithinPolygon(validPoint));

    // Point inside hole (should be false)
    GPoint holePoint(units::angle::degree_t(-75.0),
                     units::angle::degree_t(40.0));
    QVERIFY(!testPolygon->isPointWithinPolygon(holePoint));

    // Point outside polygon
    GPoint outsidePoint(units::angle::degree_t(-76.0),
                        units::angle::degree_t(41.0));
    QVERIFY(!testPolygon->isPointWithinPolygon(outsidePoint));
}

void PolygonTest::testContains()
{
    // Create points that are vertices of the polygon
    auto vertexPoint = outerBoundary.first();
    QVERIFY(testPolygon->ringsContain(vertexPoint));

    // Create point that's not part of the polygon structure
    auto randomPoint = std::make_shared<GPoint>(
        units::angle::degree_t(-76.0), units::angle::degree_t(41.0));
    QVERIFY(!testPolygon->ringsContain(randomPoint));
}

void PolygonTest::testIntersects()
{
    // Line that crosses the polygon
    auto startPoint = std::make_shared<GPoint>(
        units::angle::degree_t(-76.0), units::angle::degree_t(40.0));
    auto endPoint = std::make_shared<GPoint>(
        units::angle::degree_t(-74.0), units::angle::degree_t(40.0));
    auto crossingLine = std::make_shared<GLine>(startPoint, endPoint);

    QVERIFY(testPolygon->intersects(crossingLine));

    // Line that doesn't intersect
    auto farStartPoint = std::make_shared<GPoint>(
        units::angle::degree_t(-77.0), units::angle::degree_t(42.0));
    auto farEndPoint = std::make_shared<GPoint>(
        units::angle::degree_t(-76.0), units::angle::degree_t(42.0));
    auto nonCrossingLine =
        std::make_shared<GLine>(farStartPoint, farEndPoint);

    QVERIFY(!testPolygon->intersects(nonCrossingLine));

    auto diagLine =
        std::make_shared<GLine>(outerBoundary[0], outerBoundary[2]);
    QVERIFY(testPolygon->intersects(diagLine));

    auto touchingLine =
        std::make_shared<GLine>(outerBoundary[0], startPoint);
    QVERIFY(!testPolygon->intersects(touchingLine));
}

void PolygonTest::testGetMaxClearWidth()
{
    // Create a line through the polygon
    auto startPoint = std::make_shared<GPoint>(
        units::angle::degree_t(-74.9), units::angle::degree_t(40.0));
    auto endPoint = std::make_shared<GPoint>(
        units::angle::degree_t(-74.1), units::angle::degree_t(40.0));
    GLine testLine(startPoint, endPoint);

    auto clearWidth = testPolygon->getMaxClearWidth(testLine);
    qDebug() << "Max clear width:" << clearWidth.value() << "meters";

    QVERIFY(clearWidth.value() > 0.0);
}

void PolygonTest::testToString()
{
    QString defaultFormat = testPolygon->toString();
    QVERIFY(!defaultFormat.isEmpty());
    QVERIFY(defaultFormat.contains("Polygon"));
    QVERIFY(defaultFormat.contains("Perimeter"));
    QVERIFY(defaultFormat.contains("Area"));

    QString customFormat = testPolygon->toString("Area: %area m²");
    QVERIFY(customFormat.contains("Area:"));
    QVERIFY(customFormat.contains("m²"));
}

void PolygonTest::testEdgeCases()
{
    // Test polygon with minimum valid points (triangle)
    QVector<std::shared_ptr<GPoint>> triangleBoundary;
    triangleBoundary.append(std::make_shared<GPoint>(
        units::angle::degree_t(-75.0), units::angle::degree_t(40.0)));
    triangleBoundary.append(std::make_shared<GPoint>(
        units::angle::degree_t(-74.0), units::angle::degree_t(40.0)));
    triangleBoundary.append(std::make_shared<GPoint>(
        units::angle::degree_t(-74.5), units::angle::degree_t(41.0)));
    triangleBoundary.append(
        triangleBoundary.first()); // Close polygon

    try
    {
        Polygon trianglePolygon(triangleBoundary);
        QVERIFY(trianglePolygon.area().value() > 0.0);
        QVERIFY(trianglePolygon.perimeter().value() > 0.0);
    }
    catch (const std::exception &e)
    {
        QFAIL(QString("Triangle polygon creation failed: %1")
                  .arg(e.what())
                  .toLocal8Bit());
    }

    // Test empty polygon
    Polygon emptyPolygon;
    // Empty polygon operations should handle gracefully
    QVERIFY(emptyPolygon.outer().isEmpty());
}

QTEST_MAIN(PolygonTest)
#include "test_polygon.moc"
