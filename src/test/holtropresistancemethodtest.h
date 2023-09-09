#ifndef HOLTROPRESISTANCETEST_H
#define HOLTROPRESISTANCETEST_H
#include <QtTest/QTest>
#include "../ShipNetSim/ship/holtropmethod.h"
#include "../ShipNetSim/ship/Ship.h"

class HoltropResistanceMethodTest : public QObject
{
    Q_OBJECT

private slots:

    // Will be called before the first test function is executed.
    void initTestCase();
    // Will be called after the last test function is executed.
    void cleanupTestCase();

    void testCoefs_c1();
    void testCoefs_c2();
    void testCoefs_c3();
    void testCoefs_c4();
    void testCoefs_c5();
    void testCoefs_c7();
    void testCoefs_c14();
    void testCoefs_c15();
    void testCoefs_c16();
    void testCoefs_c17();
    void testCoefs_lambda();
    void testCoefs_m1();
    void testCoefs_m3();
    void testCoefs_pb();
    void testCoefs_Fri();
    void testCoefs_Frt();
    void testCoefs_c6();
    void testCoefs_m3Frd();
    void testCoefs_m4();
    void testCoefs_m4Cos();
    void testFrictionalResistance();
    void testAppendageResistance();
    void testWaveResistance();
    void testBulbousBowResistance();
    void testImmersedTransomPressureResistance();
    void testModelShipCorrelationResistance();
    void testAirResistance();
    void testTotalResistance();
    void testMethodName();

private:
    HoltropMethod *m_method;
    Ship *m_ship;
    QMap<QString, std::any> createSampleParameters();
    const double tolerance = 1e-9;  // difference between code vs excel
};


#endif // HOLTROPRESISTANCETEST_H
