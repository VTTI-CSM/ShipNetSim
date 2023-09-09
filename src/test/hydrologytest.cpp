#include "../ShipNetSim/ship/hydrology.h"
#include "hydrologytest.h"

void HydrologyTest::test_get_nue() {
    double salin = 35.0;  // example salinityAA
    units::temperature::celsius_t temp(25); // example temperature
    auto result = hydrology::get_nue(salin, temp);
    qDebug() << "The nue is:" << result.value();
    QVERIFY(qAbs(result.value() - 1.38563E-06) < tolerance);
}

void HydrologyTest::test_get_nue_lowSalin() {
    double salin = 5.0;
    units::temperature::celsius_t temp(25);
    auto result = hydrology::get_nue(salin, temp);
    qDebug() << "The nue with low salinity is:" << result.value();
    QVERIFY(qAbs(result.value() - 9.65625E-7) < tolerance);
}

void HydrologyTest::test_get_nue_highTemp() {
    double salin = 35.0;
    units::temperature::celsius_t temp(60); // High temperature
    auto result = hydrology::get_nue(salin, temp);
    qDebug() << "The nue with high temperature is:" << result.value();
    QVERIFY(qAbs(result.value() - 1.544E-6) < tolerance);
}

void HydrologyTest::test_F_n() {
    units::velocity::knot_t ship_speed(17);
    units::length::meter_t ship_length(245.5);
    double result = hydrology::F_n(ship_speed, ship_length);
    qDebug() << "The F_n is:" << QString::number(result, 'f', 10);
    QVERIFY(qAbs(result - 0.1782079815) < tolerance);
}

void HydrologyTest::test_F_n_negativeSpeed() {
    units::velocity::knot_t ship_speed(-17);
    units::length::meter_t ship_length(245.5);
    double result = hydrology::F_n(ship_speed, ship_length);
    qDebug() << "The F_n is:" << QString::number(result, 'f', 10);
    QVERIFY(qAbs(result - 0.0) < tolerance);
}

void HydrologyTest::test_F_n_lowSpeed() {
    units::velocity::knot_t ship_speed(1); // Low speed
    units::length::meter_t ship_length(245.5);
    double result = hydrology::F_n(ship_speed, ship_length);
    qDebug() << "The F_n with low speed is:" <<
        QString::number(result, 'f', 10);
    QVERIFY(qAbs(result - 0.0104828224) < tolerance);
}

void HydrologyTest::test_F_n_highLength() {
    units::velocity::knot_t ship_speed(17);
    units::length::meter_t ship_length(1000); // High ship length
    double result = hydrology::F_n(ship_speed, ship_length);
    qDebug() << "The F_n with high length is:" <<
        QString::number(result, 'f', 10);
    QVERIFY(qAbs(result - 0.0882984133) < tolerance);
}

void HydrologyTest::test_R_n() {
    units::velocity::knot_t ship_speed(17);
    units::length::meter_t ship_length(245.5);
    double result = hydrology::R_n(ship_speed, ship_length);
    qDebug() << "The R_n is:" << QString::number(result, 'f', 10);
    QVERIFY(qAbs(result - 1881522799.094647646) < tolerance);
}

void HydrologyTest::test_R_n_lowSpeed() {
    units::velocity::knot_t ship_speed(1); // Low speed
    units::length::meter_t ship_length(245.5);
    double result = hydrology::R_n(ship_speed, ship_length);
    qDebug() << "The R_n with low speed is:" <<
        QString::number(result, 'f', 10);
    QVERIFY(qAbs(result - 110677811.7114498764) < tolerance);
}

void HydrologyTest::test_R_n_highLength() {
    units::velocity::knot_t ship_speed(17);
    units::length::meter_t ship_length(1000); // High ship length
    double result = hydrology::R_n(ship_speed, ship_length);
    qDebug() << "The R_n with high length is:" <<
        QString::number(result, 'f', 10);
    QVERIFY(qAbs(result - 7664043988.1655702591) < tolerance);
}


//QTEST_MAIN(HydrologyTest)
#include "hydrologytest.moc"
