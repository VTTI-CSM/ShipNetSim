#include <QCoreApplication>
#include "utils/logger.h"

//#include "../third_party/units/units.h"
//#include "ship/ship.h"
//#include "ship/holtropmethod.h"

//QMap<QString, std::any> createSampleParameters()
//{
//    QMap<QString, std::any> parameters;
//    try {

//        parameters.insert("ResistanceStrategy", new HoltropMethod);
//        parameters.insert("WaterlineLength", units::length::meter_t(100.0));
//        parameters.insert("Beam", units::length::meter_t(15.0));
//        parameters.insert("MeanDraft", units::length::meter_t(5.0));
//        parameters.insert("DraftAtForward", units::length::meter_t(4.5));
//        parameters.insert("DraftAtAft", units::length::meter_t(5.5));
//        parameters.insert("VolumetricDisplacement",
//                          units::volume::cubic_meter_t(1500.0));
//        parameters.insert("WettedHullSurface",
//                          units::area::square_meter_t(200.0));
//        parameters.insert("WetSurfaceAreaMethod",
//                          Ship::WetSurfaceAreaCalculationMethod::None);
//        parameters.insert("BulbousBowTransverseAreaCenterHeight",
//                          units::length::meter_t(1.0));
//        QMap<Ship::ShipAppendage, units::area::square_meter_t> appendages;
//        appendages.insert(Ship::ShipAppendage::rudder_behind_skeg,
//                          units::area::square_meter_t(10.0));
//        parameters.insert("AppendagesWettedSurfaces", appendages);
//        parameters.insert("BulbousBowTransverseArea",
//                          units::area::square_meter_t(5.0));
//        parameters.insert("ImmersedTransomArea",
//                          units::area::square_meter_t(20.0));
//        parameters.insert("HalfWaterlineEntranceAngle",
//                          units::angle::degree_t(20.0));
//        parameters.insert("LongitudinalBuoyancyCenter", 35.0);
//        parameters.insert("SternShapeParam", Ship::CStern::None);
//        parameters.insert("MidshipSectionCoef", 0.9);
//        parameters.insert("WaterplaneAreaCoef", 0.85);
//        parameters.insert("WaterplaneCoefMethod",
//                          Ship::WaterPlaneCoefficientMethod::None);
//        parameters.insert("PrismaticCoef", 0.7);
//        parameters.insert("BlockCoef", 0.8);
//        parameters.insert("BlockCoefMethod",
//                          Ship::BlockCoefficientMethod::None);

//    }
//    catch (const std::exception& e)
//    {
//        qDebug() << e.what();
//        return parameters;
//    }
//    catch (...)
//    {
//        qDebug() << ("An unknown exception was thrown");
//        return parameters;
//    }
//    return parameters;
//}

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    Logger::attach();

//    QMap<QString, std::any> params = createSampleParameters();
//    m_method =  new HoltropResistanceMethod();
//    Ship* m_ship = new Ship(params);




    Logger::detach();
    return a.exec();
}
