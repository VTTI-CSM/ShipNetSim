#ifndef HYDROLOGYTEST_H
#define HYDROLOGYTEST_H
#include <QtTest>
#include <QDebug>
#include <QObject>

class HydrologyTest : public QObject {
    Q_OBJECT

private slots:
    void test_get_nue();
    void test_get_nue_lowSalin();
    void test_get_nue_highTemp();

    void test_F_n();
    void test_F_n_negativeSpeed();
    void test_F_n_lowSpeed();
    void test_F_n_highLength();

    void test_R_n();
    void test_R_n_lowSpeed();
    void test_R_n_highLength();


private:
    const double tolerance = 1e-9;  // difference between code vs excel
};
#endif // HYDROLOGYTEST_H
