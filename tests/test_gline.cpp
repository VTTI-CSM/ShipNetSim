#include "network/gline.h"
#include "network/gpoint.h"
#include <QDebug>
#include <QTest>

using namespace ShipNetSimCore;

class GLineTest : public QObject
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
    void testLength();
    void testAzimuth();
    void testMidpoint();
    void testReverse();

    // Point operations
    void testGetPointByDistance();
    void testPerpendicularDistance();
    void testDistanceToPoint();

    // Line relationships
    void testIntersection();
    void testAngleWith();
    void testOrientation();

    // String representation
    void testToString();

    // Edge cases
    void testEdgeCases();

private:
    std::shared_ptr<GPoint> point1;
    std::shared_ptr<GPoint> point2;
    std::shared_ptr<GPoint> point3;
    std::shared_ptr<GLine>  line1;
    std::shared_ptr<GLine>  line2;
};

void GLineTest::initTestCase()
{
    qDebug() << "Starting GLine tests...";

    // Create test points
    point1 = std::make_shared<GPoint>(
        units::angle::degree_t(-77.0369),
        units::angle::degree_t(38.9072), "Washington_DC");

    point2 = std::make_shared<GPoint>(
        units::angle::degree_t(-74.0060),
        units::angle::degree_t(40.7128), "New_York");

    point3 = std::make_shared<GPoint>(
        units::angle::degree_t(-75.1652),
        units::angle::degree_t(39.9526), "Philadelphia");

    // Create test lines
    line1 = std::make_shared<GLine>(point1, point2);
    line2 = std::make_shared<GLine>(point2, point3);
}

void GLineTest::cleanupTestCase()
{
    qDebug() << "GLine tests completed.";
}

void GLineTest::testDefaultConstructor()
{
    GLine defaultLine;
    QVERIFY(defaultLine.startPoint() != nullptr);
    QVERIFY(defaultLine.endPoint() != nullptr);
    QCOMPARE(defaultLine.length().value(), 0.0);
}

void GLineTest::testParameterizedConstructor()
{
    QVERIFY(line1->startPoint() == point1);
    QVERIFY(line1->endPoint() == point2);
    QVERIFY(line1->length().value() > 0.0);
}

void GLineTest::testAccessors()
{
    // Test setters
    auto newPoint = std::make_shared<GPoint>(
        units::angle::degree_t(-80.0), units::angle::degree_t(35.0),
        "TestPoint");

    GLine testLine(point1, point2);
    testLine.setStartPoint(newPoint);
    QVERIFY(testLine.startPoint() == newPoint);

    testLine.setEndPoint(newPoint);
    QVERIFY(testLine.endPoint() == newPoint);
}

void GLineTest::testLength()
{
    auto length = line1->length();
    qDebug() << "Line length DC to NYC:" << length.value()
             << "meters";

    // Should be approximately 328 km
    QVERIFY(length.value() > 295000.0);
    QVERIFY(length.value() < 361000.0);

    // Zero-length line
    GLine zeroLine(point1, point1);
    QVERIFY(zeroLine.length().value() < 1.0);
}

void GLineTest::testAzimuth()
{
    auto forwardAz  = line1->forwardAzimuth();
    auto backwardAz = line1->backwardAzimuth();

    qDebug() << "Forward azimuth:" << forwardAz.value();
    qDebug() << "Backward azimuth:" << backwardAz.value();

    QVERIFY(forwardAz.value() >= 0.0);
    QVERIFY(forwardAz.value() <= 360.0);
    QVERIFY(backwardAz.value() >= -180.0);
    QVERIFY(backwardAz.value() <= 180.0);
}

void GLineTest::testMidpoint()
{
    auto midpoint = line1->midpoint();
    qDebug() << "Midpoint coordinates:"
             << midpoint.getLongitude().value()
             << midpoint.getLatitude().value();

    // Midpoint should be roughly between DC and NYC
    QVERIFY(midpoint.getLongitude().value() > -77.0);
    QVERIFY(midpoint.getLongitude().value() < -74.0);
    QVERIFY(midpoint.getLatitude().value() > 38.9);
    QVERIFY(midpoint.getLatitude().value() < 40.8);
}

void GLineTest::testReverse()
{
    auto reversedLine = line1->reverse();

    QVERIFY(*reversedLine.startPoint() == *line1->endPoint());
    QVERIFY(*reversedLine.endPoint() == *line1->startPoint());
    QCOMPARE(reversedLine.length().value(), line1->length().value());
}

void GLineTest::testGetPointByDistance()
{
    auto halfwayPoint = line1->getPointByDistance(
        units::length::meter_t(line1->length() / 2.0),
        Line::LineEnd::Start);

    // Should be close to the midpoint
    auto midpoint = line1->midpoint();
    auto distance = halfwayPoint.distance(midpoint);
    QVERIFY(distance.value() < 1000.0); // Within 1 km
}

void GLineTest::testPerpendicularDistance()
{
    // Create a point slightly off the line
    GPoint offLinePoint(units::angle::degree_t(-75.5),
                        units::angle::degree_t(39.8));

    auto perpDistance = line1->getPerpendicularDistance(offLinePoint);
    qDebug() << "Perpendicular distance:" << perpDistance.value()
             << "meters";

    QVERIFY(perpDistance.value() >= 0.0);
}

void GLineTest::testDistanceToPoint()
{
    auto distance = line1->distanceToPoint(point3);
    qDebug() << "Distance to Philadelphia:" << distance.value()
             << "meters";

    QVERIFY(distance.value() >= 0.0);
}

void GLineTest::testIntersection()
{
    // Test 1: Lines that intersect at an interior point
    auto point4 = std::make_shared<GPoint>(
        units::angle::degree_t(-76.0), units::angle::degree_t(40.0),
        "Point4");
    auto point5 = std::make_shared<GPoint>(
        units::angle::degree_t(-75.0), units::angle::degree_t(40.0),
        "Point5");

    GLine crossLine(point4, point5);
    bool  intersects = line1->intersects(crossLine);
    qDebug() << "Test 1: Lines intersect at interior point:"
             << intersects;
    QVERIFY(intersects); // Expect intersection between DC-NYC and
                         // Point4-Point5

    // Test 2: Non-intersecting lines
    auto point6 = std::make_shared<GPoint>(
        units::angle::degree_t(-80.0), units::angle::degree_t(35.0),
        "Point6");
    auto point7 = std::make_shared<GPoint>(
        units::angle::degree_t(-79.0), units::angle::degree_t(34.0),
        "Point7");

    GLine separateLine(point6, point7);
    bool  noIntersection = line1->intersects(separateLine);
    qDebug() << "Test 2: Non-intersecting lines:" << noIntersection;
    QVERIFY(!noIntersection); // No intersection expected

    // Test 3: Lines sharing the same start point
    auto point8 = std::make_shared<GPoint>(
        units::angle::degree_t(-78.0), units::angle::degree_t(38.0),
        "Point8");
    GLine sharedStartLine(
        point1,
        point8); // Shares start point with line1 (Washington_DC)
    bool sharedStartIntersects = line1->intersects(sharedStartLine);
    qDebug() << "Test 3: Lines sharing start point:"
             << sharedStartIntersects;
    QVERIFY(!sharedStartIntersects); // Should not intersect, only
                                     // share endpoint

    // Test 4: Lines sharing the same end point
    auto point9 = std::make_shared<GPoint>(
        units::angle::degree_t(-73.0), units::angle::degree_t(41.0),
        "Point9");
    GLine sharedEndLine(
        point9, point2); // Shares end point with line1 (New_York)
    bool sharedEndIntersects = line1->intersects(sharedEndLine);
    qDebug() << "Test 4: Lines sharing end point:"
             << sharedEndIntersects;
    QVERIFY(!sharedEndIntersects); // Should not intersect, only share
                                   // endpoint

    // Test 5: Lines sharing both start and end points (coincident)
    GLine coincidentLine(point1, point2); // Same as line1
    bool  coincidentIntersects = line1->intersects(coincidentLine);
    qDebug() << "Test 5: Coincident lines:" << coincidentIntersects;
    QVERIFY(!coincidentIntersects); // Coincident lines should not
                                    // intersect

    // Test 6: Zero-length line with shared point
    GLine zeroLine(point2, point2); // Zero-length line at New_York
    bool  zeroLineIntersects = line1->intersects(zeroLine);
    qDebug() << "Test 6: Zero-length line at shared point:"
             << zeroLineIntersects;
    QVERIFY(
        !zeroLineIntersects); // Zero-length line should not intersect
}

void GLineTest::testAngleWith()
{
    // Lines that share point2
    auto angle = line1->smallestAngleWith(*line2);
    qDebug() << "Angle between lines:"
             << angle.convert<units::angle::degree>().value()
             << "degrees";

    QVERIFY(angle.value() >= 0.0);
    QVERIFY(angle.value() <= units::constants::pi);
}

void GLineTest::testOrientation()
{
    auto orientation = GLine::orientation(point1, point2, point3);
    qDebug() << "Orientation:" << static_cast<int>(orientation);

    // Should be one of the enum values
    QVERIFY(orientation == Line::Orientation::Clockwise
            || orientation == Line::Orientation::CounterClockwise
            || orientation == Line::Orientation::Collinear);
}

void GLineTest::testToString()
{
    QString defaultFormat = line1->toString();
    QVERIFY(!defaultFormat.isEmpty());
    QVERIFY(defaultFormat.contains("Start Point"));
    QVERIFY(defaultFormat.contains("End Point"));

    QString customFormat = line1->toString("%start -> %end");
    QVERIFY(customFormat.contains("->"));
}

void GLineTest::testEdgeCases()
{
    // Test line with same start and end points
    GLine zeroLine(point1, point1);
    QCOMPARE(zeroLine.length().value(), 0.0);

    // Test line with extreme coordinates
    auto extremePoint1 = std::make_shared<GPoint>(
        units::angle::degree_t(-179.9), units::angle::degree_t(89.9));
    auto extremePoint2 = std::make_shared<GPoint>(
        units::angle::degree_t(179.9), units::angle::degree_t(-89.9));

    GLine extremeLine(extremePoint1, extremePoint2);
    QVERIFY(extremeLine.length().value() > 0.0);
}

QTEST_MAIN(GLineTest)
#include "test_gline.moc"
