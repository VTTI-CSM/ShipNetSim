#include "holtropresistancemethodtest.h"
#include "../ShipNetSim/ship/ship.h"
#include "../ShipNetSim/ship/holtropmethod.h"
#include "../ShipNetSim/ship/hydrology.h"

QMap<QString, std::any> HoltropResistanceMethodTest::createSampleParameters()
{
    QMap<QString, std::any> parameters;
    try {

        parameters.insert("ResistanceStrategy", new HoltropMethod);
        parameters.insert("WaterlineLength", units::length::meter_t(147.7));
        parameters.insert("Beam", units::length::meter_t(24.0));
        parameters.insert("MeanDraft", units::length::meter_t(8.2));
        parameters.insert("DraftAtForward", units::length::meter_t(8.2));
        parameters.insert("DraftAtAft", units::length::meter_t(8.2));
        parameters.insert("VolumetricDisplacement",
                          units::volume::cubic_meter_t(18872.0));
        parameters.insert("WettedHullSurface",
                          units::area::square_meter_t(4400.0));
        parameters.insert("WetSurfaceAreaMethod",
                          Ship::WetSurfaceAreaCalculationMethod::None);
//        parameters.insert("BulbousBowTransverseAreaCenterHeight",
//                          units::length::meter_t(3.0));
        QMap<Ship::ShipAppendage, units::area::square_meter_t> appendages;
        appendages.insert(Ship::ShipAppendage::bilge_keels,
                          units::area::square_meter_t(52.0));
        parameters.insert("AppendagesWettedSurfaces", appendages);
        parameters.insert("BulbousBowTransverseArea",
                          units::area::square_meter_t(14.0));
        parameters.insert("ImmersedTransomArea",
                          units::area::square_meter_t(0.0));
        parameters.insert("HalfWaterlineEntranceAngle",
                          units::angle::degree_t(19.231));
        parameters.insert("LongitudinalBuoyancyCenter", 0.4);
        parameters.insert("SternShapeParam", Ship::CStern::NormalSections);
        parameters.insert("MidshipSectionCoef", 0.984);
        parameters.insert("WaterplaneAreaCoef", 0.7675);
        parameters.insert("WaterplaneCoefMethod",
                          Ship::WaterPlaneCoefficientMethod::None);
        parameters.insert("PrismaticCoef", 0.665898);
        parameters.insert("BlockCoef", 0.6492);
        parameters.insert("BlockCoefMethod",
                          Ship::BlockCoefficientMethod::None);

    }
    catch (const std::exception& e)
    {
        qDebug() << e.what();
        return parameters;
    }
    catch (...)
    {
        qDebug() << ("An unknown exception was thrown");
        return parameters;
    }
    return parameters;
}


void HoltropResistanceMethodTest::initTestCase()
{
    try{
        QMap<QString, std::any> params = createSampleParameters();
        m_method =  new HoltropMethod();
        m_ship = new Ship(params);
        units::velocity::knot_t newSpeed = units::velocity::knot_t(15.0);
        m_ship->setSpeed(newSpeed);
    }
    catch (const std::bad_any_cast& e) {
        QFAIL(e.what());
    }
    catch (const std::exception& e)
    {
        QFAIL(e.what());
    }
    catch (...)
    {
        QFAIL("An unknown exception was thrown");
    }
}

void HoltropResistanceMethodTest::cleanupTestCase()
{
    delete m_method;
    delete m_ship;
}

void HoltropResistanceMethodTest::testCoefs_c1()
{
    qDebug() << "R_N: " <<
        hydrology::R_n(m_ship->getSpeed(), m_ship->getLengthInWaterline());

    qDebug() << "FR: " <<
        hydrology::F_n(m_ship->getSpeed(), m_ship->getLengthInWaterline());

    auto c_1 = m_method->calc_c_1(*m_ship);
    qDebug() << "C_1: " << QString::number(c_1, 'f', 10);
    QVERIFY(qAbs(c_1 - 2.0454963077) < tolerance);
}
void HoltropResistanceMethodTest::testCoefs_c2()
{
    auto c_2 = m_method->calc_c_2(*m_ship);
    qDebug() << "C_2: " << QString::number(c_2, 'f', 10);
    QVERIFY(qAbs(c_2 - 0.7073005263) < tolerance);
}

void HoltropResistanceMethodTest::testCoefs_c3()
{
    auto c_3 = m_method->calc_c_3(*m_ship);
    qDebug() << "C_3: " << QString::number(c_3, 'f', 10);
    QVERIFY(qAbs(c_3 - 0.03357225) < tolerance);
}

void HoltropResistanceMethodTest::testCoefs_c4()
{
    auto c_4 = m_method->calc_c_4(*m_ship);
    qDebug() << "C_4: " << QString::number(c_4, 'f', 10);
    QVERIFY(qAbs(c_4 - 0.040000) < tolerance);
}

void HoltropResistanceMethodTest::testCoefs_c5()
{
    auto c_5 = m_method->calc_c_5(*m_ship);
    qDebug() << "C_5: " << QString::number(c_5, 'f', 10);
    QVERIFY(qAbs(c_5 - 1.000000) < tolerance);
}

void HoltropResistanceMethodTest::testCoefs_c7()
{
    auto c_7 = m_method->calc_c_7(*m_ship);
    qDebug() << "C_7: " << QString::number(c_7, 'f', 10);
    QVERIFY(qAbs(c_7 - 0.1624915369) < tolerance);
}

void HoltropResistanceMethodTest::testCoefs_c14()
{
    auto c_14 = m_method->calc_c_14(*m_ship);
    qDebug() << "C_14: " << QString::number(c_14, 'f', 10);
    QVERIFY(qAbs(c_14 - 1.0000) < tolerance);
}

void HoltropResistanceMethodTest::testCoefs_c15()
{
    auto c_15 = m_method->calc_c_15(*m_ship);
    qDebug() << "c_15: " << QString::number(c_15, 'f', 10);
    QVERIFY(qAbs(c_15 + 1.693850) < tolerance);

}
void HoltropResistanceMethodTest::testCoefs_c16()
{
    qDebug() << "C_P = " << m_ship->getPrismaticCoef();
    auto c_16 = m_method->calc_c_16(*m_ship);
    qDebug() << "c_16: " << QString::number(c_16, 'f', 10);
    QVERIFY(qAbs(c_16 - 1.293587448) < tolerance);

}
void HoltropResistanceMethodTest::testCoefs_c17()
{
    qDebug() << "CM = " << m_ship->getMidshipSectionCoef();
    qDebug() << "V = " << m_ship->getVolumetricDisplacement().value();
    auto C_17 = m_method->calc_c_17(*m_ship);
    qDebug() << "C_17: " << QString::number(C_17, 'f', 10);
    QVERIFY(qAbs(C_17 - 1.7104539134) < tolerance);

}
void HoltropResistanceMethodTest::testCoefs_lambda()
{
    qDebug() << "d: " << m_method->d;

    auto lambda = m_method->calc_lambda(*m_ship);
    qDebug() << "lambda: " << QString::number(lambda, 'f', 10);
    QVERIFY(qAbs(lambda - 0.778263508) < tolerance);
}
void HoltropResistanceMethodTest::testCoefs_m1()
{
    auto m_1 = m_method->calc_m_1(*m_ship);
    qDebug() << "m_1: " << QString::number(m_1, 'f', 10);
    QVERIFY(qAbs(m_1 + 2.1354505383) < tolerance);
}
void HoltropResistanceMethodTest::testCoefs_m3()
{
    auto m_3 = m_method->calc_m_3(*m_ship);
    qDebug() << "m_3: " << QString::number(m_3, 'f', 10);
    QVERIFY(qAbs(m_3 + 2.0760761352) < tolerance);
}
void HoltropResistanceMethodTest::testCoefs_pb()
{
    qDebug() << "A_BT = " << m_ship->getBulbousBowTransverseArea().value();
    qDebug() << "T_F = " << m_ship->getDraftAtForward().value();
    qDebug() << "h_B = " <<
        m_ship->getBulbousBowTransverseAreaCenterHeight().value();
    qDebug() << "h_F" << m_method->calc_h_F(*m_ship).value();

    auto p_b = m_method->calc_p_B(*m_ship);
    qDebug() << "p_b: " << QString::number(p_b, 'f', 10);
    QVERIFY(qAbs(p_b - 2.555278) < tolerance);
}

void HoltropResistanceMethodTest::testCoefs_Fri()
{
    auto fri = m_method->calc_F_n_i(*m_ship);
    qDebug() << "fri: " << QString::number(fri, 'f', 10);
    QVERIFY(qAbs(fri - 1.36572) < tolerance);
}

void HoltropResistanceMethodTest::testCoefs_Frt()
{
    auto frt = m_method->calc_F_n_T(*m_ship);
    qDebug() << "frt: " << QString::number(frt, 'f', 10);
    QVERIFY(qAbs(frt - 0.0) < tolerance);

}

void HoltropResistanceMethodTest::testCoefs_c6()
{
    auto c6 = m_method->calc_c_6(*m_ship);
    qDebug() << "c6: " << QString::number(c6, 'f', 10);
    QVERIFY(qAbs(c6 - 0.0) < tolerance);
}

void HoltropResistanceMethodTest::testCoefs_m3Frd()
{
    auto m3 = m_method->calc_m_3(*m_ship);
    qDebug() << "m3: " << QString::number(m3, 'f', 10);
    auto fr = hydrology::F_n(m_ship->getSpeed(),
                             m_ship->getLengthInWaterline());
    qDebug() << "fr: " << QString::number(fr, 'f', 10);
    auto result = m3 * pow(fr, m_method->d);
    qDebug() << "result: " << QString::number(result, 'f', 10);

    QVERIFY(qAbs(result + 8.72909) < tolerance);
}

void HoltropResistanceMethodTest::testCoefs_m4()
{
    auto m4 = m_method->calc_m_4(*m_ship);
    qDebug() << "m4: " << QString::number(m4, 'f', 10);
    QVERIFY(qAbs(m4 + 0.00104) < tolerance);
}

void HoltropResistanceMethodTest::testCoefs_m4Cos()
{
    auto m4 = m_method->calc_m_4(*m_ship);
    qDebug() << "m4: " << QString::number(m4, 'f', 10);

    auto lambda = m_method->calc_lambda(*m_ship);
    qDebug() << "lambda: " << QString::number(lambda, 'f', 10);

    auto fr = hydrology::F_n(m_ship->getSpeed(),
                             m_ship->getLengthInWaterline());
    qDebug() << "fr: " << QString::number(fr, 'f', 10);

    auto result = m4 * cos((lambda/ pow(fr, 2.0)));
    qDebug() << "result: " << QString::number(result, 'f', 10);

    QVERIFY(qAbs(result + 0.00104) < tolerance);

}

void HoltropResistanceMethodTest::testFrictionalResistance()
{
    qDebug() << "R_N: " <<
        hydrology::R_n(m_ship->getSpeed(), m_ship->getLengthInWaterline());
    auto result = m_method->getfrictionalResistance(*m_ship).
                  convert<units::force::kilonewton>();
    qDebug() << "The frictional Resistance is: " << result.value();
    QCOMPARE(result, units::force::kilonewton_t(10));
}

void HoltropResistanceMethodTest::testAppendageResistance()
{
    auto k2 = m_method->calc_equivalentAppendageFormFactor(*m_ship);
    qDebug() << "k2: " << QString::number(k2, 'f', 10);


    auto result = m_method->getAppendageResistance(*m_ship).
                  convert<units::force::kilonewton>();
    qDebug() << "The Appendage Resistance is: " << result.value();
    QCOMPARE(result, units::force::kilonewton_t(10));
}

void HoltropResistanceMethodTest::testWaveResistance()
{
    auto result = m_method->getWaveResistance(*m_ship).
                  convert<units::force::kilonewton>();
    qDebug() << "The Wave Resistance is: " << result.value();
    QCOMPARE(result, units::force::kilonewton_t(34.6));
}

void HoltropResistanceMethodTest::testBulbousBowResistance()
{
    auto result = m_method->getBulbousBowResistance(*m_ship).
                  convert<units::force::kilonewton>();
    qDebug() << "The BulbousBow Resistance is: " << result.value();
    QCOMPARE(result, units::force::kilonewton_t(32.56));
}

void HoltropResistanceMethodTest::testImmersedTransomPressureResistance()
{
    auto result = m_method->getImmersedTransomPressureResistance(*m_ship).
                  convert<units::force::kilonewton>();
    qDebug() << "The ImmersedTransomPressure Resistance is: " <<
        result.value();
    QCOMPARE(result, units::force::kilonewton_t(10));
}

void HoltropResistanceMethodTest::testModelShipCorrelationResistance()
{
    auto ca = m_method->calc_c_A(*m_ship);
    qDebug() << "ca: " << QString::number(ca, 'f', 10);

    auto result = m_method->getModelShipCorrelationResistance(*m_ship).
                  convert<units::force::kilonewton>();
    qDebug() << "The ModelShipCorrelation Resistance is: " << result.value();
    QCOMPARE(result, units::force::kilonewton_t(10));
}

void HoltropResistanceMethodTest::testAirResistance()
{
    auto result = m_method->getAirResistance(*m_ship).
                  convert<units::force::kilonewton>();
    qDebug() << "The Air Resistance Resistance is: " << result.value();
    QCOMPARE(result, units::force::kilonewton_t(11.2));
}

void HoltropResistanceMethodTest::testTotalResistance()
{
    auto result = m_method->getTotalResistance(*m_ship).
                  convert<units::force::kilonewton>();;
    qDebug() << "The Total Resistance is: " << result.value();
    QCOMPARE(result, units::force::kilonewton_t(10));
}


void HoltropResistanceMethodTest::testMethodName()
{
    auto result = m_method->getMethodName();
    qDebug() << "The Holtrop Resistance name is: " <<
        QString::fromStdString(result);
    QCOMPARE(result, std::string("Holtrop and Mennen Resistance "
                                 "Prediction Method"));
}

//QTEST_APPLESS_MAIN(HoltropResistanceMethodTest)
//#include "HoltropResistanceMethodTest.moc"
