#ifndef SHIPTEST_H
#define SHIPTEST_H
#include "../ShipNetSim/ship/ship.h"
#include <QtTest>
#include <QObject>

class ShipTest : public QObject
{
    Q_OBJECT

private slots:
    // This will be executed before any test function is executed.
    void initTestCase();
        // This will be executed after all test functions have been executed.
    void cleanupTestCase();
    // This will be executed before each test function is executed.
    void init();
    void testFullConstructor();
    void testConstructorWithMeanDraft();
    void testConstructorWithAftAndForwardDraft();
    // This will be executed after each test function is executed.
    void cleanup();

    void testLengthInWaterline();
    void testBeam();
    void testMeanDraftMethods_SingleParam();
    void testMeanDraftMethods_MultipleParams();
    void testDraftAtAftMethods();
    void testDraftAtForwardMethods();
    void testVolumetricDisplacementMethods();
    void testBlockCoefMethods();
    void testPrismaticCoefMethods();
    void testMidshipSectionCoefMethods();
    void testWaterplaneAreaCoefMethods();
    void testLongitudinalBuoyancyCenterMethods();
    void testWettedHullSurfaceMethods();
    void testBulbousBowTransverseAreaCenterHeightMethods();
    void testAppendagesWettedSurfacesMethods();
    void testBulbousBowTransverseAreaMethods();
    void testHalfWaterlineEntranceAngleMethods();
    void testSpeedMethods();
    void testImmersedTransomAreaMethods();
    void testResistanceStrategyMethods();
    void testSurfaceRoughnessMethods();
    void testSternShapeParamMethods();
    void testRunLengthMethods();


private:
    Ship* m_ship;
    IShipResistancePropulsionStrategy* m_strategy;
    const double tolerance = 1e-9;  // difference between code vs excel
};
#endif
