#include "network/gpoint.h"
#include <QDebug>
#include <QTest>

using namespace ShipNetSimCore;

class GPointTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    
    // Basic functionality tests
    void testDefaultConstructor();
    void testParameterizedConstructor();
    void testCoordinateSettersGetters();
    void testCoordinateNormalization();
    
    // Geodetic calculations
    void testDistanceCalculation();
    void testAzimuthCalculation();
    void testPointAtDistanceAndHeading();
    
    // Port functionality
    void testPortFunctionality();
    
    // Serialization
    void testSerialization();
    
    // Equality and operators
    void testEqualityOperators();
    void testArithmeticOperators();
    
    // String representation
    void testToString();
    
    // Edge cases and error handling
    void testEdgeCases();

private:
    std::shared_ptr<GPoint> point1;
    std::shared_ptr<GPoint> point2;
    std::shared_ptr<GPoint> point3;
};

void GPointTest::initTestCase()
{
    qDebug() << "Starting GPoint tests...";
    
    // Create test points
    point1 = std::make_shared<GPoint>(
        units::angle::degree_t(-77.0369), 
        units::angle::degree_t(38.9072), 
        "Washington_DC"
    );
    
    point2 = std::make_shared<GPoint>(
        units::angle::degree_t(-74.0060), 
        units::angle::degree_t(40.7128), 
        "New_York"
    );
    
    point3 = std::make_shared<GPoint>(
        units::angle::degree_t(0.0), 
        units::angle::degree_t(0.0), 
        "Origin"
    );
}

void GPointTest::cleanupTestCase()
{
    qDebug() << "GPoint tests completed.";
}

void GPointTest::testDefaultConstructor()
{
    GPoint defaultPoint;
    QCOMPARE(defaultPoint.getLongitude().value(), 0.0);
    QCOMPARE(defaultPoint.getLatitude().value(), 0.0);
    QVERIFY(!defaultPoint.isPort());
}

void GPointTest::testParameterizedConstructor()
{
    QCOMPARE(point1->getLongitude().value(), -77.0369);
    QCOMPARE(point1->getLatitude().value(), 38.9072);
    QVERIFY(point1->getUserID() == "Washington_DC");
}

void GPointTest::testCoordinateSettersGetters()
{
    GPoint testPoint;
    
    testPoint.setLongitude(units::angle::degree_t(45.0));
    testPoint.setLatitude(units::angle::degree_t(-30.0));
    
    QCOMPARE(testPoint.getLongitude().value(), 45.0);
    QCOMPARE(testPoint.getLatitude().value(), -30.0);
}

void GPointTest::testCoordinateNormalization()
{
    GPoint testPoint;
    
    // Test longitude normalization (should wrap to [-180, 180])
    testPoint.setLongitude(units::angle::degree_t(270.0));
    QCOMPARE(testPoint.getLongitude().value(), -90.0);
    
    testPoint.setLongitude(units::angle::degree_t(-270.0));
    QCOMPARE(testPoint.getLongitude().value(), 90.0);
    
    // Test latitude normalization (should stay within [-90, 90])
    testPoint.setLatitude(units::angle::degree_t(100.0));
    QVERIFY(testPoint.getLatitude().value() <= 90.0);
    QVERIFY(testPoint.getLatitude().value() >= -90.0);
}

void GPointTest::testDistanceCalculation()
{
    // Distance between Washington DC and New York (approximately 328 km)
    auto distance = point1->distance(*point2);
    qDebug() << "Distance DC to NYC:" << distance.value() << "meters";
    
    // Should be approximately 328,000 meters (allow 10% tolerance)
    QVERIFY(distance.value() > 295000.0);  // 295 km
    QVERIFY(distance.value() < 361000.0);  // 361 km
    
    // Distance from point to itself should be 0
    auto selfDistance = point1->distance(*point1);
    QVERIFY(selfDistance.value() < 1.0);  // Should be essentially 0
}

void GPointTest::testAzimuthCalculation()
{
    // Test forward azimuth
    auto forwardAz = point1->forwardAzimuth(*point2);
    qDebug() << "Forward azimuth DC to NYC:" << forwardAz.value() << "degrees";
    
    // Should be approximately northeast (between 0 and 90 degrees)
    QVERIFY(forwardAz.value() >= 0.0);
    QVERIFY(forwardAz.value() <= 90.0);
    
    // Test backward azimuth
    auto backwardAz = point1->backwardAzimuth(*point2);
    qDebug() << "Backward azimuth DC to NYC:" << backwardAz.value() << "degrees";
}

void GPointTest::testPointAtDistanceAndHeading()
{
    // Create a point 1000m north of the origin
    auto newPoint = point3->pointAtDistanceAndHeading(
        units::length::meter_t(1000.0),
        units::angle::degree_t(0.0)  // North
    );
    
    // Should be approximately at (0, 0.009) degrees
    QVERIFY(std::abs(newPoint.getLongitude().value()) < 0.001);
    QVERIFY(newPoint.getLatitude().value() > 0.008);
    QVERIFY(newPoint.getLatitude().value() < 0.010);
}

void GPointTest::testPortFunctionality()
{
    GPoint portPoint;
    
    // Initially not a port
    QVERIFY(!portPoint.isPort());
    QCOMPARE(portPoint.getDwellTime().value(), 0.0);
    
    // Mark as port
    portPoint.MarkAsPort(units::time::second_t(3600.0));  // 1 hour
    QVERIFY(portPoint.isPort());
    QCOMPARE(portPoint.getDwellTime().value(), 3600.0);
    
    // Mark as non-port
    portPoint.MarkAsNonPort();
    QVERIFY(!portPoint.isPort());
    QCOMPARE(portPoint.getDwellTime().value(), 0.0);
}

void GPointTest::testSerialization()
{
    // Test serialization/deserialization
    std::stringstream ss;
    
    // Serialize point1
    point1->serialize(ss);
    
    // Create new point and deserialize
    GPoint deserializedPoint;
    ss.seekg(0);  // Reset stream position
    deserializedPoint.deserialize(ss);
    
    // Verify coordinates match
    QCOMPARE(deserializedPoint.getLongitude().value(), point1->getLongitude().value());
    QCOMPARE(deserializedPoint.getLatitude().value(), point1->getLatitude().value());
    QCOMPARE(deserializedPoint.getUserID(), point1->getUserID());
}

void GPointTest::testEqualityOperators()
{
    GPoint point1Copy(units::angle::degree_t(-77.0369),
                      units::angle::degree_t(38.9072), "Copy");

    QVERIFY(*point1 == point1Copy);  // Same coordinates
    QVERIFY(!(*point1 == *point2));  // Different coordinates
}

void GPointTest::testArithmeticOperators()
{
    auto sum = *point1 + *point2;
    auto diff = *point1 - *point2;
    
    QVERIFY(sum.getLongitude().value() == (point1->getLongitude().value() + point2->getLongitude().value()));
    QVERIFY(diff.getLongitude().value() == (point1->getLongitude().value() - point2->getLongitude().value()));
}

void GPointTest::testToString()
{
    QString defaultFormat = point1->toString();
    QVERIFY(defaultFormat.contains("77.0369"));
    QVERIFY(defaultFormat.contains("38.9072"));
    
    QString customFormat = point1->toString("Lat: %y, Lon: %x, ID: %id");
    QVERIFY(customFormat.contains("Lat: 38.9072"));
    QVERIFY(customFormat.contains("Lon: -77.0369"));
    QVERIFY(customFormat.contains("ID: Washington_DC"));
}

void GPointTest::testEdgeCases()
{
    // Test extreme coordinates
    GPoint extremePoint(units::angle::degree_t(179.9), units::angle::degree_t(89.9));
    QVERIFY(extremePoint.getLongitude().value() <= 180.0);
    QVERIFY(extremePoint.getLatitude().value() <= 90.0);
    
    // Test null coordinate operations
    GPoint nullPoint1(units::angle::degree_t(0.0), units::angle::degree_t(0.0));
    GPoint nullPoint2(units::angle::degree_t(0.0), units::angle::degree_t(0.0));
    
    auto distance = nullPoint1.distance(nullPoint2);
    QVERIFY(distance.value() < 1.0);  // Should be essentially 0
}

QTEST_MAIN(GPointTest)
#include "test_gpoint.moc"
