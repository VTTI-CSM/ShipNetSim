#include "shiptest.h"
#include "../ShipNetSim/ship/holtropmethod.h"

void ShipTest::initTestCase()
{
    m_strategy = new HoltropMethod();
    // Initialization shared across test cases (if any)
}

void ShipTest::cleanupTestCase()
{
    // Cleanup shared across test cases (if any)
}

void ShipTest::init()
{
    QMap<QString, std::any> params;
    params.insert("WaterlineLength", units::length::meter_t(100.0));
    params.insert("Beam", units::length::meter_t(15.0));
    params.insert("MeanDraft", units::length::meter_t(5.0));
    m_ship = new Ship(params);
}

void ShipTest::cleanup()
{
    delete m_ship;
    // Cleanup after each test
}

void ShipTest::testFullConstructor()
{
    QMap<QString, std::any> params;
    params.insert("WaterlineLength", units::length::meter_t(100.0));
    params.insert("Beam", units::length::meter_t(15.0));
    params.insert("DraftAtForward", units::length::meter_t(4.0));
    params.insert("MeanDraft", units::length::meter_t(5.0));
    params.insert("DraftAtAft", units::length::meter_t(6.0));
    params.insert("VolumetricDisplacement", units::volume::cubic_meter_t(8000.0));
    params.insert("WettedHullSurface", units::area::square_meter_t(400.0));
    params.insert("BulbousBowTransverseAreaCenterHeight", units::length::meter_t(3.0));
    params.insert("BulbousBowTransverseArea", units::area::square_meter_t(5.0));
    params.insert("ImmersedTransomArea", units::area::square_meter_t(4.0));
    params.insert("HalfWaterlineEntranceAngle", units::length::meter_t(15.0));
    params.insert("MaxSpeed", units::length::meter_t(5.0));
    params.insert("SurfaceRoughness", units::length::nanometer_t(200));
    params.insert("RunLength", units::length::meter_t(75.0));
    params.insert("LongitudinalBuoyancyCenter", 0.6);
    params.insert("MidshipSectionCoef", 0.5);
    params.insert("WaterplaneAreaCoef", 0.9);
    params.insert("PrismaticCoef", 0.8);
    params.insert("BlockCoef", 0.7);
    params.insert("BlockCoefMethod", Ship::BlockCoefficientMethod::Ayre);
    params.insert("WaterplaneCoefMethod", Ship::WaterPlaneCoefficientMethod::U_Shape);
    params.insert("PropellerCount", 1);
    QMap<units::power::kilowatt_t,
         units::angular_velocity::revolutions_per_minute_t> engineBrakePowerRPM;
    engineBrakePowerRPM.insert(units::power::kilowatt_t(50000), units::angular_velocity::revolutions_per_minute_t(120));
    params.insert("EngineBrakePowerToRPMMap", engineBrakePowerRPM);
    QMap<units::power::kilowatt_t, double> engineBrakePowerEff;
    engineBrakePowerEff.insert(units::power::kilowatt_t(50000), 0.9);
    params.insert("EngineBrakePowerToEfficiency", engineBrakePowerEff);
    params.insert("GearboxRatio", 1);
    params.insert("gearboxEfficiency", 1.0);
    params.insert("ShaftEfficiency", 1.0);
    params.insert("PropellerDiameter", units::length::meter_t(5.0));
    params.insert("PropellerPitch", units::length::meter_t(4.8));
    params.insert("PropellerExpandedAreaRatio", 0.9);
    Ship ship(params);

    QCOMPARE(ship.getLengthInWaterline(), units::length::meter_t(100.0));
    QCOMPARE(ship.getBeam(), units::length::meter_t(15.0));
    QCOMPARE(ship.getMidshipSectionCoef(), 0.5);
    QCOMPARE(ship.getLongitudinalBuoyancyCenter(), 0.6);
    QCOMPARE(ship.getSurfaceRoughness(),
             units::length::nanometer_t(200));
    QCOMPARE(ship.getBulbousBowTransverseArea(),
             units::area::square_meter_t(5.0));
    QCOMPARE(ship.getBulbousBowTransverseAreaCenterHeight(),
             units::length::meter_t(3.0));
    QCOMPARE(ship.getImmersedTransomArea(),
             units::area::square_meter_t(4.0));
    QCOMPARE(ship.getMeanDraft(), units::length::meter_t(5.0));
    QCOMPARE(ship.getDraftAtAft(), units::length::meter_t(6.0));
    QCOMPARE(ship.getDraftAtForward(),
             units::length::meter_t(4.0));
    QCOMPARE(ship.getBlockCoef(), 0.7);
    QCOMPARE(ship.getPrismaticCoef(), 0.8);
    QCOMPARE(ship.getRunLength(), units::length::meter_t(75.0));
    QCOMPARE(ship.getWaterplaneAreaCoef(), 0.9);
    QCOMPARE(ship.getVolumetricDisplacement(),
             units::volume::cubic_meter_t(8000.0));
    QCOMPARE(ship.getWettedHullSurface(),
             units::area::square_meter_t(400.0));
    QCOMPARE(ship.getHalfWaterlineEntranceAngle(),
             units::angle::degree_t(15.0));
}

void ShipTest::testConstructorWithMeanDraft()
{
    QMap<QString, std::any> params;
    params.insert("WaterlineLength", units::length::meter_t(120.0));
    params.insert("Beam", units::length::meter_t(16.0));
    params.insert("DraftAtForward", units::length::meter_t(4.0));
    params.insert("MeanDraft", units::length::meter_t(7.0));
    params.insert("DraftAtAft", units::length::meter_t(6.0));
    params.insert("VolumetricDisplacement", units::volume::cubic_meter_t(8000.0));
    params.insert("WettedHullSurface", units::area::square_meter_t(400.0));
    params.insert("BulbousBowTransverseAreaCenterHeight", units::length::meter_t(3.0));
    params.insert("BulbousBowTransverseArea", units::area::square_meter_t(5.0));
    params.insert("ImmersedTransomArea", units::area::square_meter_t(4.0));
    params.insert("HalfWaterlineEntranceAngle", units::length::meter_t(15.0));
    params.insert("MaxSpeed", units::length::meter_t(5.0));
    params.insert("SurfaceRoughness", units::length::nanometer_t(200));
    params.insert("RunLength", units::length::meter_t(75.0));
    params.insert("LongitudinalBuoyancyCenter", 0.6);
    params.insert("MidshipSectionCoef", 0.5);
    params.insert("WaterplaneAreaCoef", 0.9);
    params.insert("PrismaticCoef", 0.8);
    params.insert("BlockCoef", 0.7);
    params.insert("BlockCoefMethod", Ship::BlockCoefficientMethod::Ayre);
    params.insert("WaterplaneCoefMethod", Ship::WaterPlaneCoefficientMethod::U_Shape);
    params.insert("PropellerCount", 1);
    QMap<units::power::kilowatt_t,
         units::angular_velocity::revolutions_per_minute_t> engineBrakePowerRPM;
    engineBrakePowerRPM.insert(units::power::kilowatt_t(50000), units::angular_velocity::revolutions_per_minute_t(120));
    params.insert("EngineBrakePowerToRPMMap", engineBrakePowerRPM);
    QMap<units::power::kilowatt_t, double> engineBrakePowerEff;
    engineBrakePowerEff.insert(units::power::kilowatt_t(50000), 0.9);
    params.insert("EngineBrakePowerToEfficiency", engineBrakePowerEff);
    params.insert("GearboxRatio", 1);
    params.insert("gearboxEfficiency", 1.0);
    params.insert("ShaftEfficiency", 1.0);
    params.insert("PropellerDiameter", units::length::meter_t(5.0));
    params.insert("PropellerPitch", units::length::meter_t(4.8));
    params.insert("PropellerExpandedAreaRatio", 0.9);
    Ship ship(params);

    QCOMPARE(ship.getLengthInWaterline(), units::length::meter_t(120.0));
    QCOMPARE(ship.getBeam(), units::length::meter_t(16.0));
    QCOMPARE(ship.getMeanDraft(), units::length::meter_t(7.0));
}

void ShipTest::testConstructorWithAftAndForwardDraft()
{
    QMap<QString, std::any> params;
    params.insert("WaterlineLength", units::length::meter_t(130.0));
    params.insert("Beam", units::length::meter_t(17.0));
    params.insert("DraftAtForward", units::length::meter_t(6.0));
    params.insert("MeanDraft", units::length::meter_t(7.0));
    params.insert("DraftAtAft", units::length::meter_t(8.0));
    params.insert("VolumetricDisplacement", units::volume::cubic_meter_t(8000.0));
    params.insert("WettedHullSurface", units::area::square_meter_t(400.0));
    params.insert("BulbousBowTransverseAreaCenterHeight", units::length::meter_t(3.0));
    params.insert("BulbousBowTransverseArea", units::area::square_meter_t(5.0));
    params.insert("ImmersedTransomArea", units::area::square_meter_t(4.0));
    params.insert("HalfWaterlineEntranceAngle", units::length::meter_t(15.0));
    params.insert("MaxSpeed", units::length::meter_t(5.0));
    params.insert("SurfaceRoughness", units::length::nanometer_t(200));
    params.insert("RunLength", units::length::meter_t(75.0));
    params.insert("LongitudinalBuoyancyCenter", 0.6);
    params.insert("MidshipSectionCoef", 0.5);
    params.insert("WaterplaneAreaCoef", 0.9);
    params.insert("PrismaticCoef", 0.8);
    params.insert("BlockCoef", 0.7);
    params.insert("BlockCoefMethod", Ship::BlockCoefficientMethod::Ayre);
    params.insert("WaterplaneCoefMethod", Ship::WaterPlaneCoefficientMethod::U_Shape);
    params.insert("PropellerCount", 1);
    QMap<units::power::kilowatt_t,
         units::angular_velocity::revolutions_per_minute_t> engineBrakePowerRPM;
    engineBrakePowerRPM.insert(units::power::kilowatt_t(50000), units::angular_velocity::revolutions_per_minute_t(120));
    params.insert("EngineBrakePowerToRPMMap", engineBrakePowerRPM);
    QMap<units::power::kilowatt_t, double> engineBrakePowerEff;
    engineBrakePowerEff.insert(units::power::kilowatt_t(50000), 0.9);
    params.insert("EngineBrakePowerToEfficiency", engineBrakePowerEff);
    params.insert("GearboxRatio", 1);
    params.insert("gearboxEfficiency", 1.0);
    params.insert("ShaftEfficiency", 1.0);
    params.insert("PropellerDiameter", units::length::meter_t(5.0));
    params.insert("PropellerPitch", units::length::meter_t(4.8));
    params.insert("PropellerExpandedAreaRatio", 0.9);
    Ship ship(params);

    QCOMPARE(ship.getLengthInWaterline(), units::length::meter_t(130.0));
    QCOMPARE(ship.getBeam(), units::length::meter_t(17.0));
    QCOMPARE(ship.getDraftAtAft(), units::length::meter_t(8.0));
    QCOMPARE(ship.getDraftAtForward(), units::length::meter_t(6.0));
}



void ShipTest::testLengthInWaterline()
{
    QCOMPARE(m_ship->getLengthInWaterline(), units::length::meter_t(100.0));
    m_ship->setLengthInWaterline(units::length::meter_t(110.0));
    QCOMPARE(m_ship->getLengthInWaterline(), units::length::meter_t(110.0));
}

void ShipTest::testBeam()
{
    QCOMPARE(m_ship->getBeam(), units::length::meter_t(15.0));
    m_ship->setBeam(units::length::meter_t(16.0));
    QCOMPARE(m_ship->getBeam(), units::length::meter_t(16.0));
}

void ShipTest::testMeanDraftMethods_SingleParam()
{
    QCOMPARE(m_ship->getMeanDraft(), units::length::meter_t(5.0));
    m_ship->setMeanDraft(units::length::meter_t(6.0));
    QCOMPARE(m_ship->getMeanDraft(), units::length::meter_t(6.0));
}

void ShipTest::testMeanDraftMethods_MultipleParams()
{
    units::length::meter_t newDraft_A(6.0);
    units::length::meter_t newDraft_F(4.0);

    m_ship->setMeanDraft(newDraft_A, newDraft_F);

    // Assuming that the mean draft is the average of newDraft_A and newDraft_F
    units::length::meter_t expectedMeanDraft = (newDraft_A + newDraft_F) / 2.0;
    QCOMPARE(expectedMeanDraft, m_ship->getMeanDraft());
}

void ShipTest::testDraftAtAftMethods()
{
    units::length::meter_t newDraft_A(6.0);
    m_ship->setDraftAtAft(newDraft_A);
    QCOMPARE(newDraft_A, m_ship->getDraftAtAft());
}

void ShipTest::testDraftAtForwardMethods()
{
    units::length::meter_t newDraft_F(4.0);
    m_ship->setDraftAtForward(newDraft_F);
    QCOMPARE(newDraft_F, m_ship->getDraftAtForward());
}

void ShipTest::testVolumetricDisplacementMethods()
{
    units::volume::cubic_meter_t newDisplacement(5000.0); // Example value
    m_ship->setVolumetricDisplacement(newDisplacement);
    QCOMPARE(newDisplacement, m_ship->getVolumetricDisplacement());
}

void ShipTest::testBlockCoefMethods()
{
    double newBlockCoef = 0.85; // Example value
    m_ship->setBlockCoef(newBlockCoef);
    QCOMPARE(newBlockCoef, m_ship->getBlockCoef());
}

void ShipTest::testPrismaticCoefMethods()
{
    double newC_P = 0.68;  // Example coefficient value
    m_ship->setPrismaticCoef(newC_P);
    QCOMPARE(newC_P, m_ship->getPrismaticCoef());
}

void ShipTest::testMidshipSectionCoefMethods()
{
    double newC_M = 0.95;  // Example coefficient value
    m_ship->setMidshipSectionCoef(newC_M);
    QCOMPARE(newC_M, m_ship->getMidshipSectionCoef());
}

void ShipTest::testWaterplaneAreaCoefMethods()
{
    double newC_WP = 0.72;  // Example coefficient value
    m_ship->setWaterplaneAreaCoef(newC_WP);
    QCOMPARE(newC_WP, m_ship->getWaterplaneAreaCoef());
}

void ShipTest::testLongitudinalBuoyancyCenterMethods()
{
    double newLcb = 5.2;  // Example value for buoyancy center
    m_ship->setLongitudinalBuoyancyCenter(newLcb);
    QCOMPARE(newLcb, m_ship->getLongitudinalBuoyancyCenter());
}

void ShipTest::testWettedHullSurfaceMethods()
{
    units::area::square_meter_t newS(300.0); // Example value
    m_ship->setWettedHullSurface(newS);
    QCOMPARE(newS, m_ship->getWettedHullSurface());
}

void ShipTest::testBulbousBowTransverseAreaCenterHeightMethods()
{
    units::length::meter_t newH_b(1.5); // Example value
    m_ship->setBulbousBowTransverseAreaCenterHeight(newH_b);
    QCOMPARE(newH_b, m_ship->getBulbousBowTransverseAreaCenterHeight());
}

void ShipTest::testAppendagesWettedSurfacesMethods()
{
    // Set up example data
    QMap<Ship::ShipAppendage, units::area::square_meter_t> appendageData;
    appendageData[Ship::ShipAppendage::dome] =
        units::area::square_meter_t(10.0);
    appendageData[Ship::ShipAppendage::bilge_keels] =
        units::area::square_meter_t(15.0);

    m_ship->setAppendagesWettedSurfaces(appendageData);

    QCOMPARE(appendageData, m_ship->getAppendagesWettedSurfaces());

    // Adding a new appendage surface
    QPair<Ship::ShipAppendage, units::area::square_meter_t>
        newEntry(Ship::ShipAppendage::hull_bossing,
                 units::area::square_meter_t(20.0));
    m_ship->addAppendagesWettedSurface(newEntry);

    QCOMPARE(newEntry.second,
             m_ship->getAppendagesWettedSurfaces().value(newEntry.first));

    units::area::square_meter_t totalSurface =
        appendageData.value(Ship::ShipAppendage::dome) +
        appendageData.value(Ship::ShipAppendage::bilge_keels) +
        newEntry.second;
    QCOMPARE(totalSurface, m_ship->getTotalAppendagesWettedSurfaces());
}

void ShipTest::testBulbousBowTransverseAreaMethods()
{
    units::area::square_meter_t newA_BT(40.0); // Example value
    m_ship->setBulbousBowTransverseArea(newA_BT);
    QCOMPARE(newA_BT, m_ship->getBulbousBowTransverseArea());
}

void ShipTest::testHalfWaterlineEntranceAngleMethods()
{
    units::angle::degree_t newAngle(12.5); // Example angle
    m_ship->setHalfWaterlineEntranceAngle(newAngle);
    QCOMPARE(newAngle, m_ship->getHalfWaterlineEntranceAngle());
}

void ShipTest::testSpeedMethods()
{
    // Test with knots
    units::velocity::knot_t speedInKnots(20.0); // Example value
    m_ship->setSpeed(speedInKnots);
    QCOMPARE(speedInKnots, m_ship->getSpeed());

    // Test with meters per second
    units::velocity::meters_per_second_t speedInMPS(10.29); // 20 knots to m/s
    m_ship->setSpeed(speedInMPS);
    QCOMPARE(speedInMPS, m_ship->getSpeed());
}

void ShipTest::testImmersedTransomAreaMethods()
{
    units::area::square_meter_t newA_T(30.0); // Example value
    m_ship->setImmersedTransomArea(newA_T);
    QCOMPARE(newA_T, m_ship->getImmersedTransomArea());
}

void ShipTest::testResistanceStrategyMethods()
{
    // Just set the strategy
    qDebug() << m_strategy;
    m_ship->setResistancePropulsionStrategy(m_strategy);

    // Separate retrieval test
    IShipResistancePropulsionStrategy* retrievedStrategy =
        m_ship->getResistanceStrategy();
    QVERIFY(retrievedStrategy != nullptr); // Ensure that we have a strategy
}

void ShipTest::testSurfaceRoughnessMethods()
{
    units::length::nanometer_t newSurfaceRoughness(200);
    m_ship->setSurfaceRoughness(newSurfaceRoughness);
    QCOMPARE(newSurfaceRoughness, m_ship->getSurfaceRoughness());
}

void ShipTest::testSternShapeParamMethods()
{
    Ship::CStern newC_Stern = Ship::CStern::UShapedSections;
    m_ship->setSternShapeParam(newC_Stern);
    QCOMPARE(newC_Stern, m_ship->getSternShapeParam());
}

void ShipTest::testRunLengthMethods()
{
    units::length::meter_t newRunLength(100.0); // Example value
    m_ship->setRunLength(newRunLength);
    QCOMPARE(newRunLength, m_ship->getRunLength());
}



//QTEST_MAIN(ShipTest)
#include "shiptest.moc"
