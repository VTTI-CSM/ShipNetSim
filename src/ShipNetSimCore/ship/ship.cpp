#include "ship.h"
#include <cmath>
#include "hydrology.h"
#include "ishipcalmresistancestrategy.h"
#include "ishipdynamicresistancestrategy.h"
#include "holtropmethod.h"
#include "langmaomethod.h"
#include <QDebug>
#include "../../third_party/units/units.h"
#include "../utils/utils.h"
#include "ishippropeller.h"
#include "shipgearbox.h"
#include "shippropeller.h"
#include "shipengine.h"
#include "defaults.h"
#include "../network/gpoint.h"
#include <QDebug>

namespace ShipNetSimCore
{
Ship::Ship(const QMap<QString, std::any>& parameters,
           QObject* parent) : QObject(parent)
{
#ifdef BUILD_SERVER_ENABLED
    try {
        mLoadedContainers =
            ContainerCore::ContainerMap(this);
    } catch (std::exception &e) {
        throw e;
    }

#endif

    if (parameters.contains("CalmWaterResistanceStrategy"))
    {
        try {
            // Try casting the std::any to the base type first
            if (parameters["CalmWaterResistanceStrategy"].type() ==
                typeid(HoltropMethod*)) {
                HoltropMethod* temp =
                    std::any_cast<HoltropMethod*>(
                        parameters["CalmWaterResistanceStrategy"]);
                mCalmResistanceStrategy = temp; // Upcast is implicit
            }
            else if (parameters["CalmWaterResistanceStrategy"].type() !=
                     typeid(nullptr))
            {
                qFatal("Calm water resistance strategy does "
                          "not match recognized strategies!");
            }
        }
        catch (const std::bad_any_cast&) {
            // Handle error: the std::any_cast didn't work
            mCalmResistanceStrategy = nullptr;
        }
    }
    else
    {
        // Setting default strategies or configurations
        mCalmResistanceStrategy = new HoltropMethod();

    }

    if (parameters.contains("DynamicResistanceStrategy"))
    {
        try {
            if (parameters["DynamicResistanceStrategy"].type() ==
                typeid(LangMaoMethod*)) {
                LangMaoMethod* temp =
                    std::any_cast<LangMaoMethod*>(
                    parameters["DynamicResistanceStrategy"]);
                mDynamicResistanceStrategy = temp;
            }
            else if (parameters["DynamicResistanceStrategy"].type() !=
                   typeid(nullptr))
            {
                qFatal("Dynamic resistance strategy does "
                       "not match recognized strategies!");
            }
        }
        catch (const std::bad_any_cast&) {
            // Handle error: the std::any_cast didn't work
            mDynamicResistanceStrategy = nullptr;
        }
    }
    else
    {
        mDynamicResistanceStrategy = new LangMaoMethod();
    }

    mShipUserID =
        Utils::getValueFromMap<QString>(
            parameters,
            "ID",
            "Not Defined");

    mWaterlineLength =
        Utils::getValueFromMap<units::length::meter_t>(
            parameters,
            "WaterlineLength",
            units::length::meter_t(std::nan("uninitialized")));

    mLengthBetweenPerpendiculars =
        Utils::getValueFromMap<units::length::meter_t>(
            parameters,
            "LengthBetweenPerpendiculars",
            units::length::meter_t(std::nan("uninitialized")));

    mBeam =
        Utils::getValueFromMap<units::length::meter_t>(
            parameters,
            "Beam",
            units::length::meter_t(std::nan("uninitialized")));

    mMeanDraft =
        Utils::getValueFromMap<units::length::meter_t>(
            parameters,
            "MeanDraft",
            units::length::meter_t(std::nan("uninitialized")));

    mDraftAtForward =
        Utils::getValueFromMap<units::length::meter_t>(
            parameters,
            "DraftAtForward",
            units::length::meter_t(std::nan("uninitialized")));

    mDraftAtAft =
        Utils::getValueFromMap<units::length::meter_t>(
            parameters,
            "DraftAtAft",
            units::length::meter_t(std::nan("uninitialized")));

    mVolumetricDisplacement =
        Utils::getValueFromMap<units::volume::cubic_meter_t>(
            parameters,
            "VolumetricDisplacement",
            units::volume::cubic_meter_t(std::nan("uninitialized")));

    mWettedHullSurface =
        Utils::getValueFromMap<units::area::square_meter_t>(
            parameters,
            "WettedHullSurface",
            units::area::square_meter_t(std::nan("uninitialized")));

    mWetSurfaceAreaMethod =
        Utils::getValueFromMap<WetSurfaceAreaCalculationMethod>(
            parameters,
            "WetSurfaceAreaMethod",
            WetSurfaceAreaCalculationMethod::Cargo);

    mBulbousBowTransverseAreaCenterHeight =
        Utils::getValueFromMap<units::length::meter_t>(
            parameters,
            "BulbousBowTransverseAreaCenterHeight",
            units::length::meter_t(std::nan("uninitialized")));

    mAppendagesWettedSurfaces =
        Utils::getValueFromMap<QMap<ShipAppendage,
                                    units::area::square_meter_t>>(
            parameters,
            "AppendagesWettedSurfaces",
            QMap<ShipAppendage, units::area::square_meter_t>());

    mBulbousBowTransverseArea =
        Utils::getValueFromMap<units::area::square_meter_t>(
            parameters,
            "BulbousBowTransverseArea",
            units::area::square_meter_t(std::nan("uninitialized")));

    mImmersedTransomArea =
        Utils::getValueFromMap<units::area::square_meter_t>(
            parameters,
            "ImmersedTransomArea",
            units::area::square_meter_t(std::nan("uninitialized")));

    mHalfWaterlineEntranceAngle =
        Utils::getValueFromMap<units::angle::degree_t>(
            parameters,
            "HalfWaterlineEntranceAngle",
            units::angle::degree_t(std::nan("uninitialized")));

    if (std::isnan(mHalfWaterlineEntranceAngle.value()))
    {
        mHalfWaterlineEntranceAngle = calc_i_E();
    }

    mLengthOfEntrance =
        Utils::getValueFromMap<units::length::meter_t>(
            parameters,
            "LengthOfEntrance",
            units::length::meter_t(std::nan("uninitialized")));

    if (std::isnan(mLengthOfEntrance.value()))
    {
        mLengthOfEntrance = calc_LE();
    }

    mSpeed = units::velocity::meters_per_second_t(0.0);

    mMaxSpeed =
        Utils::getValueFromMap<units::velocity::meters_per_second_t>(
            parameters, "MaxSpeed",
            units::velocity::knot_t(25.0)).    // if not defined, use 25 knts
        convert<units::velocity::meters_per_second>();

    mSurfaceRoughness =
        Utils::getValueFromMap<units::length::nanometer_t>(
            parameters,
            "SurfaceRoughness",
            units::length::nanometer_t(std::nan("uninitialized")));

    mRunLength =
        Utils::getValueFromMap<units::length::meter_t>(
            parameters,
            "RunLength",
            units::length::meter_t(std::nan("uninitialized")));

    mLongitudinalBuoyancyCenter =
        Utils::getValueFromMap<double>(
            parameters,
            "LongitudinalBuoyancyCenter",
            std::nan("uninitialized"));

    mSternShapeParam =
        Utils::getValueFromMap<CStern>(
            parameters,
            "SternShapeParam",
            CStern::None);

    mMidshipSectionCoef =
        Utils::getValueFromMap<double>(
            parameters,
            "MidshipSectionCoef",
            std::nan("uninitialized"));

    mWaterplaneAreaCoef =
        Utils::getValueFromMap<double>(
            parameters,
            "WaterplaneAreaCoef",
            std::nan("uninitialized"));

    mWaterplaneCoefMethod =
        Utils::getValueFromMap<WaterPlaneCoefficientMethod>(
            parameters,
            "WaterplaneCoefMethod",
            WaterPlaneCoefficientMethod::General_Cargo);

    mPrismaticCoef =
        Utils::getValueFromMap<double>(
            parameters,
            "PrismaticCoef",
            std::nan("uninitialized"));

    mBlockCoef =
        Utils::getValueFromMap<double>(
            parameters,
            "BlockCoef",
            std::nan("uninitialized"));

    mBlockCoefMethod =
        Utils::getValueFromMap<BlockCoefficientMethod>(
            parameters,
            "BlockCoefMethod",
            BlockCoefficientMethod::None);

    if ((std::isnan(mBlockCoef) && std::isnan(mPrismaticCoef)) ||
        (std::isnan(mBlockCoef) && std::isnan(mMidshipSectionCoef)) ||
        (std::isnan(mPrismaticCoef) && std::isnan(mMidshipSectionCoef)))
    {
        qFatal("More than one of these coefficients are not passed: "
               "Block, Prismatic, Midship Coefficients! "
               "Make sure at least two coefficients are defined!");
    }

    mLengthwiseProjectionArea =
        Utils::getValueFromMap<units::area::square_meter_t>(
        parameters,
        "ShipAndCargoAreaAboveWaterline",
        units::area::square_meter_t(0.0));

    mPropellers =
        Utils::getValueFromMap<QVector<IShipPropeller*>>(
            parameters,
            "Propellers",
            QVector<IShipPropeller*>());

    // Engine properties
    int engineCountPerPropeller =
        Utils::getValueFromMap<int>(parameters,
                                    "EnginesCountPerPropeller",
                                    Defaults::EngineCountPerPropeller);

    // Propeller Properties
    int propellerCount =
        Utils::getValueFromMap<int>(
            parameters,
            "PropellerCount",
            Defaults::PropellerCountPerShip);


    mEnergySources.clear();


    std::shared_ptr<IEnergySource> mainEnergySource =
        Utils::getValueFromMap<std::shared_ptr<IEnergySource>>(
            parameters,
            "EnergySource",
            nullptr);
    if (mainEnergySource == nullptr)
    {
        std::shared_ptr<Tank> tank = std::make_shared<Tank>();
        tank->initialize(this);
        QMap<QString, std::any> tankProperties;
        tankProperties = {
            {"FuelType", ShipFuel::FuelType::HFO},
            {"MaxCapacity", Defaults::TankSize},
            {"InitialCapacityPercentage",
             Defaults::TankInitialCapacityPercentage},
            {"DepthOfDischarge", Defaults::TankDepthOfDischarge}
        };
        tank->setCharacteristics(tankProperties);
        mEnergySources.push_back(std::static_pointer_cast<IEnergySource>(tank));
    }
    else {
        mEnergySources.push_back(mainEnergySource);
    }

    mStopIfNoEnergy =
        Utils::getValueFromMap<bool>(
            parameters,
            "StopIfNoEnergy",
            false);

    mRudderAngle =
        Utils::getValueFromMap<units::angle::degree_t>(
            parameters,
            "MaxRudderAngle",
            units::angle::degree_t(30));

    mVesselWeight =
        Utils::getValueFromMap<units::mass::metric_ton_t>(
            parameters,
            "VesselWeight",
            units::mass::metric_ton_t(0.0));

    mCargoWeight =
        Utils::getValueFromMap<units::mass::metric_ton_t>(
            parameters,
            "CargoWeight",
            units::mass::metric_ton_t(0.0));

    mDraggedVessels =
        Utils::getValueFromMap<QVector<Ship*>>(
            parameters,
            "DraggedVessels",
            QVector<Ship*>());

    QVector<std::shared_ptr<GPoint>> points =
        Utils::getValueFromMap<QVector<std::shared_ptr<GPoint>>>(
            parameters,
            "PathPoints",
            QVector<std::shared_ptr<GPoint>>());

    QVector<std::shared_ptr<GLine>> lines =
        Utils::getValueFromMap<QVector<std::shared_ptr<GLine>>>(
            parameters,
            "PathLines",
            QVector<std::shared_ptr<GLine>>());

    if (lines.empty() || points.empty() ||
        lines.size() < 1 || points.size() < 2)
    {
        qFatal("Path Lines and Points are not defined");
    }
    setPath(points, lines);
    setStartPoint(points.at(0));
    setEndPoint(points.back());


    initializeDefaults();
    reset();


    QVector<IEnergySource*> rawVector = [&](){ QVector<IEnergySource*> r; for(auto& sp : mEnergySources) r.push_back(sp.get()); return r; }();
    for (int i = 0; i < propellerCount; i++)
    {
        qDebug() << "--- > defining the propellers!";

        ShipGearBox* gearbox = new ShipGearBox();
        QVector<IShipEngine *> engines;
        engines.reserve(engineCountPerPropeller); // Reserve space

        for (int j = 0; j < engineCountPerPropeller; j++)
        {
            ShipEngine *engine = new ShipEngine();
            try {
                qDebug() << "----> Initting the engine!";
                engine->initialize(
                    this,
                    rawVector,
                    parameters);
            } catch (...) {
                delete engine;
                throw; // Rethrow the exception after cleaning up
            }
            engines.push_back(engine);
        }
        gearbox->initialize(this, engines, parameters);

        // Create, initialize, and add the propeller to the ship
        auto prop = new ShipPropeller();
        try {
            prop->initialize(this, gearbox, parameters);
        } catch (...) {
            delete prop;
            throw; // Rethrow the exception after cleaning up
        }
        mPropellers.push_back(prop); // add the propeller to the ship
    }

    for (auto& propeller : mPropellers) {
        // define the target point of the engine
        auto maxTotalRes = getCalmResistanceStrategy()->
                           getTotalResistance(*this, getMaxSpeed());
        units::velocity::meters_per_second_t Va =
            getCalmResistanceStrategy()->
            calc_SpeedOfAdvance(*this, getMaxSpeed());
        units::power::kilowatt_t maxEffectivePower = maxTotalRes * Va;
        double nH = getCalmResistanceStrategy()->getHullEffeciency(*this);

        auto p = maxEffectivePower/ nH;

        if (propeller->getGearBox()->getEngines().at(0)->getCurrentOperationalLoad() ==
            IShipEngine::EngineOperationalLoad::Default) {
            units::angular_velocity::revolutions_per_minute_t n =
                units::angular_velocity::revolutions_per_minute_t(
                    60.0 * Va.value() / (propeller->getPropellerPitch().value() *
                                         (1.0 - propeller->getPropellerSlip())));
            IShipEngine::EngineProperties ep;
            ep.RPM = n;
            ep.breakPower = p;

            propeller->getGearBox()->setEngineDefaultTargetState(ep);
            propeller->getGearBox()->setEngineTargetState(ep);
        }
    }



    QObject::connect(this, &Ship::stepDistanceChanged,
                     this, &Ship::handleStepDistanceChanged);
}

Ship::~Ship()
{
    // avoid memory leaks
    if (mCalmResistanceStrategy != nullptr)
    {
        delete mCalmResistanceStrategy;
        mCalmResistanceStrategy = nullptr;
    }
    if (mDynamicResistanceStrategy != nullptr)
    {
        delete mDynamicResistanceStrategy;
        mDynamicResistanceStrategy = nullptr;
    }
    for (IShipPropeller* propeller : mPropellers)
    {
        if (propeller != nullptr)
        {
            delete propeller;
            propeller = nullptr;
        }
    }
    for (Ship* vessel : mDraggedVessels)
    {
        if (vessel != nullptr)
        {
            delete vessel;
            vessel = nullptr;
        }
    }
}

void Ship::moveObjectToThread(QThread *thread)
{
    // Move Simulator object itself to the thread
    this->moveToThread(thread);

    for (auto& propeller: mPropellers) {
        if (propeller) {
            propeller->moveObjectToThread(thread);
        }
    }

}

QString Ship::getUserID()
{
    return mShipUserID;
}

units::force::newton_t Ship::calculateTotalResistance(
    units::velocity::meters_per_second_t customSpeed,
    units::force::newton_t* totalResistance)
{
    units::force::newton_t totalResis =
        mCalmResistanceStrategy->getTotalResistance(*this, customSpeed);

    if (mDynamicResistanceStrategy != nullptr)
    {
        totalResis += mDynamicResistanceStrategy->getTotalResistance(*this);
    }

    if (totalResistance) {
        *totalResistance = units::force::newton_t(totalResis.value());
    }

    for (auto &ship: this->mDraggedVessels)
    {
        // calculate the total calm and dynamic resistances
        totalResis += ship->calculateTotalResistance(customSpeed, totalResistance);
    }
    return totalResis;
}

units::force::newton_t Ship::getTotalResistance()
{
    return mTotalResistance;
}

units::area::square_meter_t Ship::calc_WetSurfaceAreaToHoltrop() const
{
    return (
        units::area::square_meter_t(
            getLengthInWaterline()
                * ((double)2.0 * getMeanDraft() + getBeam())
                * sqrt(getMidshipSectionCoef())
                * (
                    (double)0.453
                    + (double)0.4425 * getBlockCoef()
                    - (double)0.2862 * getMidshipSectionCoef()
                    - (double)0.003467 * getBeam() / getMeanDraft()
                    + (double)0.3696 * getWaterplaneAreaCoef()
                    )
            + ((double)2.38 * getBulbousBowTransverseArea() / getBlockCoef())
            )
        );
}

units::area::square_meter_t Ship::calc_WetSurfaceAreaToSchenzle() const
{
    double B = getWaterplaneAreaCoef() *
               getBeam().value() /
               getMeanDraft().value();
    double C = getLengthInWaterline().value() /
               getBeam().value() /
               getMidshipSectionCoef();
    double A1 = (
                    (double)1.0 +
                    (B / (double)2.0) -
                    sqrt(1.0 + B * B / 4.0)
                    ) *
                ((double)2.0 / B);
    double A2 = (double)1.0 + C - sqrt((double)1.0 + C * C);
    double CN1 = (double)0.8 + (double)0.2 * B;
    double CN2 = (double)1.15 + (double)0.2833 * C;
    double CPX = getBlockCoef() / getMidshipSectionCoef();
    double CPZ = getBlockCoef() / getWaterplaneAreaCoef();
    double C1 = (double)1.0 -
                A1 *
                    sqrt((double)1.0 -
                         pow((((double)2.0 *
                               CPZ) -
                              (double)1.0), CN1));
    double C2 = (double)1.0 -
                A2 * sqrt((double)1.0 -
                          pow((double)2.0 *
                                      CPX -
                                  (double)1.0,
                              CN2));
    return ((double)2.0 + C1 * B + (double)2.0 * C2 / C) *
           getLengthInWaterline() * getMeanDraft();
}

units::area::square_meter_t Ship::calc_WetSurfaceArea(
    WetSurfaceAreaCalculationMethod method) const
{
    if (method == WetSurfaceAreaCalculationMethod::Holtrop)
    {
        return calc_WetSurfaceAreaToHoltrop();
    }
    else if (method == WetSurfaceAreaCalculationMethod::Schenzle)
    {
        return calc_WetSurfaceAreaToSchenzle();
    }
    else if (method == WetSurfaceAreaCalculationMethod::Cargo)
    {
        return units::area::square_meter_t(
            (getVolumetricDisplacement().value() / getBeam().value()) *
                ((double)1.7 /
                 (getBlockCoef() -
                  ((double)0.2 * (getBlockCoef()- (double)0.65))
                  )) +
            (getBeam().value()/mMeanDraft.value())
            );
    }
    else if (method == WetSurfaceAreaCalculationMethod::Trawlers)
    {
        return units::area::square_meter_t(
            ((getVolumetricDisplacement().value() /
              getBeam().value()) * ((double)1.7/getBlockCoef())) +
            ((getBeam().value()/getMeanDraft().value())
             * ((double)0.92 + ((double)0.092/getBlockCoef())))
            );
    }

    qFatal("Wrong method selected!");

    return units::area::square_meter_t(0.0);

}

double Ship::calc_BlockCoefFromVolumetricDisplacement() const
{
    return (getVolumetricDisplacement() /
            (getBeam() * getMeanDraft() * getLengthInWaterline())).value();
}

double Ship::calc_BlockCoef(BlockCoefficientMethod method) const
{
    units::velocity::meters_per_second_t speed = getSpeed();

    double fn = hydrology::F_n(speed, getLengthInWaterline());
    if (method == BlockCoefficientMethod::Ayre)
    {
        return ((double)1.06 - (double)1.68 * fn);
    }
    else if (method == BlockCoefficientMethod::Jensen)
    {
        if (fn > 0.15 && fn < 0.32)
        {
            return ((double)-4.22 +
                    (double)27.8 * sqrt(fn) -
                    (double)39.1 * fn +
                    (double)46.6 * pow(fn, 3.0));
        }
        else
        {
            qWarning() << "Froud number is outside "
                          "the allowable range for Jensen Method. "
                          "Set to default 'Ayre Method' instead";
            mBlockCoefMethod = BlockCoefficientMethod::Ayre;
            return ((double)1.06 - (double)1.68 * fn);
        }
    }
    else if (method == BlockCoefficientMethod::Schneekluth)
    {
        if (fn > 0.14 && fn < 0.32)
        {
            if (fn > 0.3)
            {
                fn = (double)0.3;
            }
            double CB = ((double)0.14 / fn) *
                        (
                            (
                                (getLengthInWaterline().value() *
                                 getBeam().value())
                                + (double)20.0) /
                            (double)26.0);
            if (CB < 0.48)
            {
                return 0.48;
            }
            else if (CB > 0.85)
            {
                return 0.85;
            }
            else
            {
                return CB;
            }
        }
        else
        {
            qWarning() <<"Froud number is outside "
                          "the allowable range for Schneekluth Method. "
                          "Set to default 'Ayre Method' instead";
            mBlockCoefMethod = BlockCoefficientMethod::Ayre;
            return ((double)1.06 - (double)1.68 * fn);
        }
    }
    else
    {
        qWarning() << "Wrong method selected! "
                      "Set to default 'Ayre Method' instead";
        mBlockCoefMethod = BlockCoefficientMethod::Ayre;
        return ((double)1.06 - (double)1.68 * fn);
    }
}

double Ship::calc_MidshiSectionCoef()
{
    return getBlockCoef() / getPrismaticCoef();
}

double Ship::calc_PrismaticCoef() const
{
    return getBlockCoef()/ getMidshipSectionCoef();
}

double Ship::calc_blockCoefByVolumetricDisplacement() const
{
    return (getVolumetricDisplacement() /
           (getLengthBetweenPerpendiculars() *
            getBeam() *
            getMeanDraft())).value();
}
double Ship::calc_blockCoefByMidShipAndPrismaticCoefs() const
{
    return getMidshipSectionCoef() * getPrismaticCoef();
}

bool Ship::checkSelectedMethodAssumptions(
    IShipCalmResistanceStrategy *strategy)
{
    if (typeid(strategy) == typeid(HoltropMethod))
    {
        bool warning = false;
        double fn = hydrology::F_n(getSpeed(), getLengthInWaterline());
        if (fn > 0.45)
        {
            qWarning() << "Speed is outside the method range!"
                       << " Calculations may not be accurate";
            warning = true;
        }
        if (getPrismaticCoef() > 0.85 || getPrismaticCoef() < 0.55)
        {
            qWarning() << "Prismatic Coefficient is outside the method range!"
                       << " Calculations may not be accurate";
            warning = true;
        }
        double L_B = (getLengthInWaterline()/getBeam()).value();
        if (L_B > 9.5 || L_B < 3.9)
        {
            qWarning() << "Length/Beam is outside the method range!"
                       << " Calculations may not be accurate";
        }

        return !warning;
    }
    else if (typeid(strategy) != typeid(nullptr))
    {
        qFatal("Resistance Strategy is not recognized!");
    }
    return true;
}

units::volume::cubic_meter_t Ship::calc_VolumetricDisplacementByWeight() const
{
    auto env = mCurrentState.getEnvironment();
    auto water_rho =
        hydrology::get_waterDensity(env.salinity, env.temperature);
    return (
        getTotalVesselStaticWeight().convert<units::mass::kilogram>() /
        water_rho);
}

units::volume::cubic_meter_t Ship::calc_VolumetricDisplacement() const
{
    return (getLengthInWaterline() *
            getBeam() *
            getMeanDraft()) *
           getBlockCoef();
}

double Ship::calc_WaterplaneAreaCoef(WaterPlaneCoefficientMethod method) const
{
    if (method == WaterPlaneCoefficientMethod::U_Shape)
    {
        return (double)0.95 * getPrismaticCoef() +
               (double)0.17 *
                   pow(((double)1.0 - getPrismaticCoef()),
                       ((double)1.0 / (double)3.0));
    }
    else if (method == WaterPlaneCoefficientMethod::Average_Section)
    {
        return ((double)1.0 + (double)2.0 * getBlockCoef()) / (double)3.0;
    }
    else if (method == WaterPlaneCoefficientMethod::V_Section)
    {
        return sqrt(getBlockCoef()) - (double)0.025;
    }
    else if (method == WaterPlaneCoefficientMethod::General_Cargo)
    {
        return 0.763 * (getPrismaticCoef() + (double)0.34);
    }
    else if (method == WaterPlaneCoefficientMethod::Container)
    {
        return 3.226 * (getPrismaticCoef() - (double)0.36);
    }
    else
    {
        qFatal("Wrong method selected!");
        return 0.0;
    }
}

units::length::meter_t Ship::calc_RunLength() const
{
    return units::length::meter_t(getLengthInWaterline() * (
                                      1.0 - getPrismaticCoef() +
                                      0.06 * getPrismaticCoef() *
                                          getLongitudinalBuoyancyCenter() /
                                          (4.0 * getPrismaticCoef() - 1.0)
                                      ));
}

units::angle::degree_t Ship::calc_i_E() const
{
    return units::angle::degree_t(
        (double)1.0
        + (double)89.0
              * exp(
                  -pow((getLengthInWaterline() /
                        getBeam()).value(), (double)0.80856)
                  * pow(((double)1.0 - getWaterplaneAreaCoef()),
                        (double)0.30484)
                  * pow(((double)1.0 - getPrismaticCoef() -
                         (double)0.0225 * getLongitudinalBuoyancyCenter()),
                        (double)0.6367)
                  * pow((getRunLength() /
                         getBeam()).value(), (double)0.34574)
                  * pow((100.0 * getVolumetricDisplacement().value() /
                         pow(getLengthInWaterline().value(), 3)),
                        (double)0.16302)
                  )
        );
}

units::length::meter_t Ship::calc_LE() const
{
    // Convert angle from degrees to radians
    double angleERadians =
        getHalfWaterlineEntranceAngle().convert<units::angle::radians>().value();

    // Calculate LE using the rearranged tangent formula
    units::length::meter_t LE = getBeam() / tan(angleERadians);

    return LE;
}

/**
     * @brief calculate the surge added mass
     * @cite Zeraatgar, H., Moghaddas, A., & Sadati, K. (2020).
     *       Analysis of surge added mass of planing hulls by model experiment.
     *       Ships and Offshore Structures, 15(3), 310-317.
     */
units::mass::metric_ton_t Ship::calc_SurgeAddedMass() const
{

    // eccentricity
    double e =
        sqrt((double)1.0 -
             (((double)3.0 * getVolumetricDisplacement().value()) /
              ((double)2.0 * units::constants::pi.value() *
               getLengthInWaterline().value() * (double)0.5) ) /
                 pow(getLengthInWaterline().value() * (double)0.5,
                     (double)2.0));

    // alpha zero
    double alpha =
        (((double)2.0 * ((double)1.0 - pow(e, (double)2.0))) /
         (pow(e, (double)3.0))) *
        (0.5 * log(((double)1.0 + e)/ ((double)1.0 - e)) - e);

    // added mass coefficient
    double k1 =
        alpha / ((double)2.0 - alpha);

    auto env = mCurrentState.getEnvironment();
    auto water_rho = hydrology::get_waterDensity(env.salinity, env.temperature);

    return (water_rho *
            getVolumetricDisplacement() *
            k1).convert<units::mass::metric_ton>();

}

units::mass::metric_ton_t Ship::calc_addedWeight() const
{
    auto env = mCurrentState.getEnvironment();
    auto water_rho = hydrology::get_waterDensity(env.salinity, env.temperature);

    return ((units::constants::pi * water_rho *
             units::math::pow<2>(getMeanDraft()) *
             getBeam() * getMidshipSectionCoef())/ (double)2.0).
        convert<units::mass::metric_ton>();
}


double Ship::getPrismaticCoef() const
{
    if (std::isnan(mPrismaticCoef))
    {
        return calc_PrismaticCoef();
    }
    return mPrismaticCoef;
}

void Ship::setCalmResistanceStrategy(
    IShipCalmResistanceStrategy* newStrategy)
{
    if (mCalmResistanceStrategy)
    {
        delete mCalmResistanceStrategy;  // avoid memory leaks
    }
    mCalmResistanceStrategy = newStrategy;
}

IShipCalmResistanceStrategy *Ship::getCalmResistanceStrategy() const
{
    return mCalmResistanceStrategy;
}

void Ship::setDynamicResistanceStrategy(
    IShipDynamicResistanceStrategy *newStrategy)
{
    if (mDynamicResistanceStrategy)
    {
        delete mDynamicResistanceStrategy;  // avoid memory leaks
    }
    mDynamicResistanceStrategy = newStrategy;
}

IShipDynamicResistanceStrategy *Ship::getDynamicResistanceStrategy() const
{
    return mDynamicResistanceStrategy;
}

double Ship::getMainTankCurrentCapacity()
{
    return mPropellers[0]->getGearBox()->getEngines().at(0)->
        getCurrentEnergySource()->getCurrentCapacityState();
}

units::area::square_meter_t Ship::getLengthwiseProjectionArea() const
{
   if (std::isnan(mLengthwiseProjectionArea.value()))
   {
       throw ShipException("Lengthwise projection area of the ship "
                           "is not assigned yet!");
   }
   return mLengthwiseProjectionArea;
}

void Ship::setLengthwiseProjectionArea(
   const units::area::square_meter_t &newLengthwiseProjectionArea)
{
   mLengthwiseProjectionArea = newLengthwiseProjectionArea;
}

units::length::nanometer_t Ship::getSurfaceRoughness() const
{
    if (std::isnan(mSurfaceRoughness.value()))
    {
        throw ShipException("Surface roughness of the ship "
                            "is not assigned yet!");
    }
    return mSurfaceRoughness;
}

void Ship::setSurfaceRoughness(
    const units::length::nanometer_t &newSurfaceRoughness)
{
    mSurfaceRoughness = newSurfaceRoughness;
}

Ship::CStern Ship::getSternShapeParam() const
{
    return mSternShapeParam;
}

void Ship::setSternShapeParam(CStern newC_Stern)
{
    mSternShapeParam = newC_Stern;
}

units::length::meter_t Ship::getRunLength() const
{
    if (std::isnan(mRunLength.value()))
    {
        return calc_RunLength();
    }
    return mRunLength;
}

void Ship::setRunLength(const units::length::meter_t &newRunLength)
{
    mRunLength = newRunLength;
}

void Ship::addPropeller(IShipPropeller *newPropeller)
{
    mPropellers.push_back(newPropeller);
}

const QVector<IShipPropeller *> *Ship::getPropellers() const
{
    return &mPropellers;
}

QVector<Ship *> *Ship::draggedVessels()
{
    return &mDraggedVessels;
}

units::force::newton_t
Ship::CalculateTotalThrust(units::force::newton_t *totalThrust)
{
    units::force::newton_t totalThrustGen = units::force::newton_t(0.0);

    for (const auto propeller: mPropellers)
    {
        totalThrustGen += propeller->getThrust();
    }

    if (totalThrust) {
        *totalThrust = units::force::newton_t(totalThrustGen.value());
    };
    return totalThrustGen;
}

units::force::newton_t Ship::getTotalThrust() const
{
    return mTotalThrust;
}

units::mass::metric_ton_t Ship::getVesselWeight() const
{
    return mVesselWeight;
}

void Ship::setVesselWeight(const units::mass::metric_ton_t &newVesselWeight)
{
    mVesselWeight = newVesselWeight;
}

units::mass::metric_ton_t Ship::getCargoWeight() const
{
    return mCargoWeight;
}

void Ship::setCargoWeight(const units::mass::metric_ton_t &newCargoWeight)
{
    mCargoWeight = newCargoWeight;
}

units::mass::metric_ton_t Ship::getTotalVesselStaticWeight() const
{
    auto totalWeight = getCargoWeight() + getVesselWeight();

    for (auto& es : mEnergySources) {
        totalWeight += es->getCurrentWeight();
    }
    return totalWeight;
}

units::mass::metric_ton_t Ship::getTotalVesselDynamicWeight() const
{
    return getTotalVesselStaticWeight() +
                        calc_SurgeAddedMass(); // Calculate the added weights
}

units::angle::degree_t Ship::getHalfWaterlineEntranceAngle() const
{
    if (std::isnan(mHalfWaterlineEntranceAngle.value()))
    {
        return calc_i_E();
    }
    return mHalfWaterlineEntranceAngle;
}

void Ship::setHalfWaterlineEntranceAngle(
    const units::angle::degree_t &newHalfWaterlineEntranceAnlge)
{
    mHalfWaterlineEntranceAngle = newHalfWaterlineEntranceAnlge;
}

units::length::meter_t Ship::getLengthOfEntrance() const
{
    if (std::isnan(mLengthOfEntrance.value()))
    {
        return calc_LE();
    }
    return mLengthOfEntrance;
}

void Ship::setLengthOfEntrance(units::length::meter_t &newValue) {
    mLengthOfEntrance = newValue;
}

double Ship::getBlockCoef() const
{
    if (std::isnan(mBlockCoef))
    {
        return calc_blockCoefByVolumetricDisplacement(); //return calc_BlockCoef(mBlockCoefMethod);
    }
    return mBlockCoef;
}

void Ship::setWaterplaneAreaCoef(const double newC_WP)
{
    mWaterplaneAreaCoef = newC_WP;
}

units::volume::cubic_meter_t Ship::getVolumetricDisplacement() const
{
    if (std::isnan(mVolumetricDisplacement.value()))
    {
        return calc_VolumetricDisplacementByWeight();
    }
    return mVolumetricDisplacement;
}

void Ship::setVolumetricDisplacement(
    const units::volume::cubic_meter_t &newNab)
{
    mVolumetricDisplacement = newNab;
}

double Ship::getWaterplaneAreaCoef() const
{
    if (std::isnan(mWaterplaneAreaCoef))
    {
        return calc_WaterplaneAreaCoef();
    }
    return mWaterplaneAreaCoef;
}


units::length::meter_t Ship::getLengthInWaterline() const
{
    if (std::isnan(mWaterlineLength.value()))
    {
        throw ShipException("Length in waterline is not assigned yet!");
    }
    return mWaterlineLength;
}

void Ship::setLengthInWaterline(const units::length::meter_t &newL)
{
    mWaterlineLength = newL;
}

units::length::meter_t Ship::getLengthBetweenPerpendiculars() const
{
    if (std::isnan(mLengthBetweenPerpendiculars.value()))
    {
        throw ShipException("Length between perpendiculars "
                            "is not assigned yet!");
    }
    return mLengthBetweenPerpendiculars;
}

void Ship::setLengthBetweenPerpendiculars(const units::length::meter_t &newL)
{
    mLengthBetweenPerpendiculars = newL;
}

units::length::meter_t Ship::getBeam() const
{
    if (std::isnan(mBeam.value()))
    {
        throw ShipException("Molded beam is not assigned yet!");
    }
    return mBeam;
}

void Ship::setBeam(const units::length::meter_t &newB)
{
    mBeam = newB;
}

units::length::meter_t Ship::getMeanDraft() const
{
    if (std::isnan(mMeanDraft.value()))
    {
        throw ShipException("Mean draft is not assigned yet!");
    }
    return mMeanDraft;
}

void Ship::setMeanDraft(const units::length::meter_t &newT)
{
    mMeanDraft = newT;
}

void Ship::setMeanDraft(const units::length::meter_t &newT_A,
                        const units::length::meter_t &newT_F)
{
    mMeanDraft = ((newT_A + newT_F) / (double)2.0);
}

//units::length::meter_t Ship::getD() const
//{
//    return D;
//}

//void Ship::setD(const units::length::meter_t &newD)
//{
//    D = newD;
//}

units::length::meter_t Ship::getDraftAtForward() const
{
    if (std::isnan(mDraftAtForward.value()))
    {
        throw ShipException("Draft at forward perpendicular "
                            "is not assigned yet!");
    }
    return mDraftAtForward;
}

void Ship::setDraftAtForward(const units::length::meter_t &newT_F)
{
    mDraftAtForward = newT_F;
}

units::length::meter_t Ship::getDraftAtAft() const
{
    if (std::isnan(mDraftAtAft.value()))
    {
        throw ShipException("Draft at aft perpendicular "
                            "is not assigned yet!");
    }
    return mDraftAtAft;
}

void Ship::setDraftAtAft(const units::length::meter_t &newT_A)
{
    mDraftAtAft = newT_A;
}

units::area::square_meter_t Ship::getWettedHullSurface() const
{
    if (std::isnan(mWettedHullSurface.value()))
    {
        return calc_WetSurfaceArea(mWetSurfaceAreaMethod);
    }
    return mWettedHullSurface;
}

void Ship::setWettedHullSurface(const units::area::square_meter_t &newS)
{
    mWettedHullSurface = newS;
}

units::length::meter_t Ship::getBulbousBowTransverseAreaCenterHeight() const
{
    if (std::isnan(mBulbousBowTransverseAreaCenterHeight.value()))
    {
        throw ShipException("Bulbous Bow Transverse Area Center Height "
                            "is not assigned yet!");
    }
    return mBulbousBowTransverseAreaCenterHeight;
}

void Ship::setBulbousBowTransverseAreaCenterHeight(
    const units::length::meter_t &newH_b)
{
    mBulbousBowTransverseAreaCenterHeight = newH_b;
}

QMap<Ship::ShipAppendage, units::area::square_meter_t>
Ship::getAppendagesWettedSurfaces() const
{
    return mAppendagesWettedSurfaces;
}

units::area::square_meter_t Ship::getTotalAppendagesWettedSurfaces() const
{
    if (mAppendagesWettedSurfaces.empty())
    {
        return units::area::square_meter_t(0.0);
    }
    units::area::square_meter_t totalArea = units::area::square_meter_t(0.0);
    for (auto& area : getAppendagesWettedSurfaces())
    {
        totalArea += area;
    }
    return totalArea;
}

void Ship::setAppendagesWettedSurfaces(
    const QMap<ShipAppendage, units::area::square_meter_t> &newS_appList)
{
    mAppendagesWettedSurfaces = newS_appList;
}

void Ship::addAppendagesWettedSurface(
    const QPair<ShipAppendage, units::area::square_meter_t> &entry)
{
    mAppendagesWettedSurfaces.insert(entry.first, entry.second);
}

units::area::square_meter_t Ship::getBulbousBowTransverseArea() const
{
    if (std::isnan(mBulbousBowTransverseArea.value()))
    {
        throw ShipException("Bulbous Bow Transverse Area "
                            "is not assigned yet!");
    }
    return mBulbousBowTransverseArea;
}

void Ship::setBulbousBowTransverseArea(
    const units::area::square_meter_t &newA_BT)
{
    mBulbousBowTransverseArea = newA_BT;
}

units::velocity::meters_per_second_t Ship::getSpeed() const
{
    return mSpeed;
}

void Ship::setSpeed(const units::velocity::knot_t $newSpeed)
{
    mSpeed = $newSpeed.convert<velocity::meters_per_second>();
}

void Ship::setSpeed(const units::velocity::meters_per_second_t &newSpeed)
{
    mSpeed = newSpeed;
}

units::acceleration::meters_per_second_squared_t
Ship::getAcceleration() const
{
    return mAcceleration;
}

units::velocity::meters_per_second_t Ship::getPreviousSpeed() const
{
    return mPreviousSpeed;
}

double Ship::getLongitudinalBuoyancyCenter() const
{
    if (std::isnan(mLongitudinalBuoyancyCenter))
    {
        throw ShipException("Longitudinal buoyancy center of the ship "
                            "is not assigned yet!");
    }
    return mLongitudinalBuoyancyCenter;
}

void Ship::setLongitudinalBuoyancyCenter(const double newLcb)
{
    mLongitudinalBuoyancyCenter = newLcb;
}

double Ship::getMidshipSectionCoef() const
{
    if (std::isnan(mMidshipSectionCoef))
    {
        throw ShipException("Midship section coefficient"
                            "is not assigned yet!");
    }
    return mMidshipSectionCoef;
}

void Ship::setMidshipSectionCoef(const double newC_M)
{
    mMidshipSectionCoef = newC_M;
}

units::area::square_meter_t Ship::getImmersedTransomArea() const
{
    if (std::isnan(mImmersedTransomArea.value()))
    {
        throw ShipException("Immersed Transom Area "
                            "is not assigned yet!");
    }
    return mImmersedTransomArea;
}

void Ship::setImmersedTransomArea(const units::area::square_meter_t &newA_T)
{
    mImmersedTransomArea = newA_T;
}

void Ship::setBlockCoef(const double newC_B)
{
    if (std::isnan(mImmersedTransomArea.value()))
    {
        throw ShipException("Block Coefficient  "
                            "is not assigned yet!");
    }
    mBlockCoef = newC_B;
}

void Ship::setPrismaticCoef(const double newC_P)
{
    mPrismaticCoef = newC_P;
}

units::length::meter_t Ship::getTraveledDistance() const
{
    return mTraveledDistance;
}

units::length::meter_t Ship::getTotalPathLength() const
{
    return mTotalPathLength;
}

void Ship::initializeDefaults()
{
    // Constants or other defaults
    mSpeed = units::velocity::meters_per_second_t(0.0);

    // Handle the waterline length
    if (std::isnan(mWaterlineLength.value()))
    {
        throw ShipException("Waterline Length must be defined");
    }

    // Handle the beam
    if (std::isnan(mBeam.value()))
    {
        throw ShipException("Beam must be defined");
    }

    // Handle drafts
    if (std::isnan(mMeanDraft.value()) &&
        std::isnan(mDraftAtForward.value()) &&
        std::isnan(mDraftAtAft.value()))
    {
        throw ShipException("Draft must be defined");
    }

    if (std::isnan(mMeanDraft.value()) &&
        !std::isnan(mDraftAtForward.value()) &&
        !std::isnan(mDraftAtAft.value()))
    {
        mMeanDraft = (mDraftAtForward + mDraftAtAft) / ((double)2.0);
    }

    if (!std::isnan(mMeanDraft.value()) &&
        std::isnan(mDraftAtForward.value()))
    {
        mDraftAtForward = mMeanDraft;
    }

    if (!std::isnan(mMeanDraft.value()) &&
        std::isnan(mDraftAtAft.value()))
    {
        mDraftAtAft = mMeanDraft;
    }

    // Handling block coefficient method default
    if (mBlockCoefMethod == BlockCoefficientMethod::None)
    {
        qInfo() << "Block coefficient method is not defined. "
                   "Set to default 'Ayre Method'.";
        mBlockCoefMethod = BlockCoefficientMethod::Ayre;
    }

    // Handling waterplane coefficient method default
    if (mWaterplaneCoefMethod == WaterPlaneCoefficientMethod::None)
    {
        qInfo() << "Water plane coefficient method is not defined. "
                   "Set to default 'Average_Section Method'.";
        mWaterplaneCoefMethod = WaterPlaneCoefficientMethod::Average_Section;
    }

    // Handling wet surface area calculation method default
    if (mWetSurfaceAreaMethod == WetSurfaceAreaCalculationMethod::None)
    {
        qInfo() << "Wet surface area method is not defined. "
                   "Set to default 'Cargo Method'.";
        mWetSurfaceAreaMethod = WetSurfaceAreaCalculationMethod::Cargo;
    }

    // Handling CStern method
    if (mSternShapeParam == CStern::None)
    {
        qInfo() << "Stern shape is not defined. "
                   "Set to default 'Normal Section'.";
        mSternShapeParam = CStern::NormalSections;
    }

    // calculate V or CB
    if (std::isnan(mVolumetricDisplacement.value()) &&
        std::isnan(mBlockCoef))
    {
        throw ShipException("Volumetric displacement and block "
                            "coefficient are not defined!");
    }
    else if (std::isnan(mVolumetricDisplacement.value()) &&
             (! std::isnan(mBlockCoef))
             )
    {
        mVolumetricDisplacement = calc_VolumetricDisplacementByWeight();
    }
    else if (! std::isnan(mVolumetricDisplacement.value()) &&
             std::isnan(mBlockCoef)
             )
    {
        mBlockCoef = calc_BlockCoefFromVolumetricDisplacement();
    }

    // CM or CP
    if (std::isnan(mPrismaticCoef) &&
        std::isnan(mBlockCoef) &&
        std::isnan(mMidshipSectionCoef))
    {
        throw ShipException("Prismatic Coefficient, Block Coefficient, "
                            "and Midship coefficients are not defined!");
    }
    else if (! std::isnan(mPrismaticCoef) &&
             ! std::isnan(mBlockCoef) &&
             std::isnan(mMidshipSectionCoef))
    {
        mMidshipSectionCoef = calc_MidshiSectionCoef();
    }
    else if (std::isnan(mPrismaticCoef) &&
             ! std::isnan(mBlockCoef) &&
             ! std::isnan(mMidshipSectionCoef))
    {
        mPrismaticCoef = calc_PrismaticCoef();
    }

    // Setting default strategies or configurations
    if (!mCalmResistanceStrategy)
    {
        mCalmResistanceStrategy = new HoltropMethod();
    }

    // Handle areas and related values
    if (std::isnan(mBulbousBowTransverseArea.value()))
    {
        mBulbousBowTransverseArea = units::area::square_meter_t(0.0);
    }

    // Handle Immersed Transom Area
    if (std::isnan(mImmersedTransomArea.value()))
    {
        mImmersedTransomArea = units::area::square_meter_t(0.0);
    }


    // Handle the Longitudinal Buoyancy Center
    // Assuming it's halfway by default
    if (std::isnan(mLongitudinalBuoyancyCenter))
    {
        qInfo() << "Longitudinal Buoyancy Center is not defined. "
                   "Set to default of 0.5";
        mLongitudinalBuoyancyCenter = (double)0.5;
    }

    // Handle the bulbous bow transverse area center height
    if (std::isnan(mBulbousBowTransverseAreaCenterHeight.value()))
    {
        qInfo() << "The bulbous bow transverse area center"
                   " height is not defined. "
                   "Set to default of 0.6 x Draft at forward";
        mBulbousBowTransverseAreaCenterHeight =
            (double) 0.6 * mDraftAtForward;
    }

    if (std::isnan(mSurfaceRoughness.value()))
    {
        qInfo() << "Surface Roughness is not defined. "
                   "Set to default of 150 nanometer.";
        mSurfaceRoughness = units::length::nanometer_t(150.0);
    }

}

QVector<std::shared_ptr<GLine>>* Ship::getShipPathLines()
{
    return &mPathLines;
}

QVector<std::shared_ptr<GPoint>>* Ship::getShipPathPoints()
{
    return &mPathPoints;
}

void Ship::setPath(const QVector<std::shared_ptr<GPoint>> points,
                   const QVector<std::shared_ptr<GLine>> lines)
{
    if (mTraveledDistance > units::length::meter_t(0.0) ||
        isLoaded())
    {
        qWarning() << "Cannot set the ship path "
                      "in the middle of the trip!";
    }

    mPathPoints = points;
    mPathLines = lines;
    mLinksCumLengths = generateCumLinesLengths();
    mTotalPathLength = mLinksCumLengths.back();
    mCurrentState = GAlgebraicVector(*(mPathPoints.at(0)),
                                    *(mPathPoints.at(1)));
    computeStoppingPointIndices();
}

std::shared_ptr<GPoint> Ship::startPoint()
{
    return mStartCoordinates;
}

// this should be in the ship constructor only
void Ship::setStartPoint(std::shared_ptr<GPoint> startPoint)
{
    mStartCoordinates = startPoint;
}

std::shared_ptr<GPoint> Ship::endPoint()
{
    return mEndCoordinates;
}

void Ship::setEndPoint(std::shared_ptr<GPoint> endPoint)
{
    mEndCoordinates = endPoint;
}

GPoint Ship::getCurrentPosition()
{
    return mCurrentState.getCurrentPosition();
}

void Ship::restoreLatestGPSCorrectPosition() {
    mCurrentState.restoreLatestCorrectPosition();
}

void Ship::setCurrentPosition(GPoint newPosition) {
    mCurrentState.setCurrentPosition(newPosition);
}

void Ship::disableCommunications()
{
    mIsCommunicationActive = false;
    mCurrentState.setGPSUpdateState(false);
}

void Ship::enableCommunications()
{
    mIsCommunicationActive = true;
    mCurrentState.setGPSUpdateState(true);
}

units::angle::degree_t Ship::getCurrentHeading() const
{
    return mCurrentState.getVectorAzimuth();
}

GPoint Ship::getCurrentTarget()
{
    return mCurrentState.getTarget();
}

AlgebraicVector::Environment Ship::getCurrentEnvironment() const
{
    return mCurrentState.getEnvironment();
}

void Ship::setCurrentEnvironment(const AlgebraicVector::Environment newEnv)
{
    mCurrentState.setEnvironment(newEnv);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~ End of Setters/Getters ~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~ Start of Dynamics ~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

units::acceleration::meters_per_second_squared_t
Ship::calc_maxAcceleration(
    units::acceleration::meters_per_second_squared_t* maxAccel,
    units::force::newton_t* totalThrust,
    units::force::newton_t* totalResistance)
{
    auto thrust = CalculateTotalThrust(totalThrust);
    auto resistance =
        this->calculateTotalResistance(
        units::velocity::meters_per_second_t(std::nan("current")),
        totalResistance);

    auto acceleration = (thrust - resistance) /
           getTotalVesselDynamicWeight().convert<units::mass::kilogram>();
    if (maxAccel) {
        *maxAccel =
            units::acceleration::meters_per_second_squared_t(
            acceleration.value());
    }
    return acceleration;
}

units::acceleration::meters_per_second_squared_t
Ship::calc_decelerationAtSpeed(
    const units::velocity::meters_per_second_t customSpeed)
{
    units::force::newton_t resultantForces =
        this->calculateTotalResistance(customSpeed);

    if (mBrakingThrustAvailable)
    {
        resultantForces += CalculateTotalThrust();
    }

    units::acceleration::meters_per_second_squared_t acc =
        resultantForces /
        getTotalVesselDynamicWeight().convert<units::mass::kilogram>();

    return ((double)-1.0 * acc); // make it negative value

    // return -1.0L * units::acceleration::meters_per_second_squared_t(0.2);

}

double Ship::calc_frictionCoef(
    const units::velocity::meters_per_second_t customSpeed) const
{
    return mCalmResistanceStrategy->
        getCoefficientOfResistance(*this, customSpeed);
}

units::velocity::meters_per_second_t Ship::getMaxSpeed() const
{
    return mMaxSpeed;
}

units::acceleration::meters_per_second_squared_t
Ship::getMaxAcceleration()
{
    return mMaxAcceleration;
}

units::acceleration::meters_per_second_squared_t
Ship::getTripRunningAverageAcceleration() const
{
    return mRunningAvrAcceleration;
}

units::velocity::meters_per_second_t
    Ship::getTripRunningAvergageSpeed() const
{
    return mRunningAvrSpeed;
}

units::length::meter_t Ship::getSafeGap(const units::length::meter_t initialGap,
    const units::velocity::meters_per_second_t speed,
    const units::velocity::meters_per_second_t freeFlowSpeed,
    const units::time::second_t T_s,
    const units::time::second_t timeStep, bool estimate)
{
    units::length::meter_t gap_lad = units::length::meter_t(0.0);

    if (! estimate )
    {
        auto roundedSpeed = speed.round(1); // round the speed for faster calc

        // use memorization to speed up calculations
        if (mGapCache.contains(roundedSpeed))
        {
            return mGapCache[roundedSpeed];
        }

        units::velocity::meters_per_second_t currentSpeed = roundedSpeed;
        gap_lad = initialGap + currentSpeed * timeStep;

        // use a conversion limit of 0.05 for faster computations
        while(currentSpeed > units::velocity::meters_per_second_t(0.5))
        {
            // check if the memory has this value for faster calculations
            if (mGapCache.contains(roundedSpeed))
            {
                gap_lad += mGapCache[roundedSpeed];
                break;
            }

            auto d_des_internal = calc_decelerationAtSpeed(currentSpeed);
            currentSpeed += d_des_internal * timeStep; // the d_des is negative
            gap_lad += currentSpeed * timeStep;
        }

        // gap_lad *= 2.0L;

        mGapCache[roundedSpeed] = gap_lad;

        // d_des = units::math::min(
        //     units::math::abs(calc_decelerationAtSpeed(speed)), // average deceleration level
        //     mD_des);

        // gap_lad =
        //     initialGap + T_s * speed +
        //     (units::math::pow<2>(speed) / (2.0 * mD_des));
    }
    else
    {
        units::acceleration::meters_per_second_squared_t d_des =
        //  units::math::min(
            units::math::abs(calc_decelerationAtSpeed(freeFlowSpeed));//, // average deceleration level
            // mD_des);

        gap_lad = initialGap + T_s * freeFlowSpeed +
                  (units::math::pow<2>(freeFlowSpeed) /
                   (2.0 * d_des));
    }
    return gap_lad;
}

units::velocity::meters_per_second_t
Ship::getNextTimeStepSpeed(
    const units::length::meter_t gap,
    const units::length::meter_t minGap,
    const units::velocity::meters_per_second_t speed,
    const units::velocity::meters_per_second_t freeFlowSpeed,
    const units::acceleration::meters_per_second_squared_t aMax,
    const units::time::second_t T_s,
    const units::time::second_t deltaT)
{
    units::velocity::meters_per_second_t u_hat =
        units::math::min((gap - minGap) / T_s, freeFlowSpeed);

    if (u_hat < speed)
    {
        // consider average deceleration level
        u_hat = max(u_hat, speed + calc_decelerationAtSpeed(speed) * deltaT);
    }
    else if (u_hat > speed && u_hat != freeFlowSpeed)
    {
        u_hat = min(u_hat, speed + aMax * deltaT);
    }
    return u_hat;
}

units::time::second_t Ship::getTimeToCollision(
    const units::length::meter_t gap,
    const units::length::meter_t minGap,
    const units::velocity::meters_per_second_t speed,
    const units::velocity::meters_per_second_t leaderSpeed)
{
    return units::math::min(
        (gap - minGap) /
            units::math::max(speed - leaderSpeed,
                             units::velocity::meters_per_second_t(0.0001)),
        units::time::second_t(100.0));
}

units::acceleration::meters_per_second_squared_t
Ship::get_acceleration_an11(
    const units::velocity::meters_per_second_t u_hat,
    const units::velocity::meters_per_second_t speed,
    const units::time::second_t TTC_s)
{
    units::time::second_t denominator = units::time::second_t(0.0);
    if (TTC_s.value() > 0) {
        denominator = TTC_s;
    }
    else {
        denominator = units::time::second_t(0.0001);
    }
    // consider average deceleration level
    return units::math::max(((u_hat - speed) / (denominator)),
                            calc_decelerationAtSpeed(speed));
}


units::acceleration::meters_per_second_squared_t
Ship::get_acceleration_an12(
    const units::velocity::meters_per_second_t &u_hat,
    const units::velocity::meters_per_second_t &speed,
    const units::time::second_t &T_s,
    const units::acceleration::meters_per_second_squared_t &amax)
{
    units::time::second_t t_s = (T_s == units::time::second_t(0.0L)) ?
                                    units::time::second_t(0.0001L) : T_s;
    return units::math::min((u_hat - speed) / t_s,
                                amax);
}

double Ship::get_beta1(
    const units::acceleration::meters_per_second_squared_t &an11)
{
    if (an11.value() > 0)
    {
        return 1.0;
    }
    else {
        return 0.0;
    }
}

units::acceleration::meters_per_second_squared_t
Ship::get_acceleration_an13(
    const double beta1,
    const units::acceleration::meters_per_second_squared_t &an11,
    const units::acceleration::meters_per_second_squared_t &an12)
{
    return (1.0 - beta1) * an11 + beta1 * an12;
}

units::acceleration::meters_per_second_squared_t
Ship::get_acceleration_an14(
    const units::velocity::meters_per_second_t &speed,
    const units::velocity::meters_per_second_t &leaderSpeed,
    const units::time::second_t &T_s,
    const units::acceleration::meters_per_second_squared_t &amax)
{
    return units::math::max(
        units::math::min(
            (leaderSpeed - speed) / T_s, amax),
        calc_decelerationAtSpeed(speed)); // average deceleration level
}

double Ship::get_beta2()
{
    return 1.0;
}

units::acceleration::meters_per_second_squared_t
Ship::get_acceleration_an1(
    const double beta2,
    const units::acceleration::meters_per_second_squared_t &an13,
    const units::acceleration::meters_per_second_squared_t &an14)
{
    return beta2 * an13 + (1.0 - beta2) * an14;
}

double Ship::get_gamma(
    const units::velocity::meters_per_second_t &speedDiff)
{
    if (speedDiff.value() > 0.0) {
        return 1.0;
    }
    else {
        return 0.0;
    }
}

units::acceleration::meters_per_second_squared_t
Ship::get_acceleration_an2(
    const units::length::meter_t &gap,
    const units::length::meter_t &minGap,
    const units::velocity::meters_per_second_t &speed,
    const units::velocity::meters_per_second_t &leaderSpeed,
    const units::time::second_t &T_s)
{
    (void)T_s; // ignore it as we dont use it here

    units::acceleration::meters_per_second_squared_t d_des =
        // units::math::min(
        units::math::abs(calc_decelerationAtSpeed(speed));
            // mD_des);

    units::acceleration::meters_per_second_squared_t term;

    term = units::math::pow<2>(units::math::pow<2>(speed) -
                               units::math::pow<2>(leaderSpeed)) /
           (4.0 * d_des *
            units::math::pow<2>(
                units::math::max((gap - minGap),
                                 units::length::meter_t(0.0001)
                                 )
                )
            );
    return std::min(term, d_des);

    // consider average deceleration level
    // return min(term, calc_decelerationAtSpeed(speed) * (double)-1.0);
}

units::acceleration::meters_per_second_squared_t
Ship::accelerate(const units::length::meter_t &gap,
                 const units::length::meter_t &mingap,
                 const units::velocity::meters_per_second_t &speed,
                 const units::acceleration::meters_per_second_squared_t &acceleration,
                 const units::velocity::meters_per_second_t &leaderSpeed,
                 const units::velocity::meters_per_second_t &freeFlowSpeed,
                 const units::time::second_t &deltaT,
                 units::acceleration::meters_per_second_squared_t *maxAccel,
                 units::force::newton_t *totalThrust,
                 units::force::newton_t *totalResistance)
{
    (void) acceleration; // ignore it as we dnt use now
    //get the maximum acceleration that the ship can go by
    units::acceleration::meters_per_second_squared_t amax =
        calc_maxAcceleration(maxAccel, totalThrust, totalResistance);

    auto safeGap = this->getSafeGap(mingap, speed,
                                    freeFlowSpeed,
                                    mT_s, deltaT, false);

    if ((gap > safeGap) && (amax.value() > 0))
    {
        if (speed < freeFlowSpeed)
        {
            return amax;
        }
        else if ( speed == freeFlowSpeed)
        {
            return units::acceleration::meters_per_second_squared_t(0.0);
        }
    }
    units::velocity::meters_per_second_t u_hat =
        this->getNextTimeStepSpeed(gap, mingap, speed,
                                   freeFlowSpeed, amax, mT_s, deltaT);
    units::time::second_t TTC_s =
        this->getTimeToCollision(gap, mingap, speed, leaderSpeed);
    units::acceleration::meters_per_second_squared_t an11 =
        this->get_acceleration_an11(u_hat, speed, TTC_s);
    units::acceleration::meters_per_second_squared_t an12 =
        this->get_acceleration_an12(u_hat, speed, mT_s, amax);
    double beta1 = this->get_beta1(an11);
    units::acceleration::meters_per_second_squared_t an13 =
        this->get_acceleration_an13(beta1, an11, an12);
    units::acceleration::meters_per_second_squared_t an14 =
        this->get_acceleration_an14(speed, leaderSpeed, mT_s, amax);
    double beta2 = this->get_beta2();
    units::acceleration::meters_per_second_squared_t an1 =
        this->get_acceleration_an1(beta2, an13, an14);
    units::velocity::meters_per_second_t du = speed - leaderSpeed;
    double gamma = this->get_gamma(du);
    units::acceleration::meters_per_second_squared_t an2 =
        this->get_acceleration_an2(gap, mingap, speed, leaderSpeed, mT_s);
    units::acceleration::meters_per_second_squared_t a =
        an1 * (1.0 - gamma) +
        gamma * units::math::min(-1.0f * an2, amax); // todo: check

    return a;
}

units::acceleration::meters_per_second_squared_t
Ship::accelerateConsideringJerk(
    units::acceleration::meters_per_second_squared_t &acceleration,
    units::acceleration::meters_per_second_squared_t &previousAcceleration,
    units::jerk::meters_per_second_cubed_t &jerk,
    units::time::second_t &deltaT )
{

    units::acceleration::meters_per_second_squared_t an =
        units::math::min(units::math::abs(acceleration),
                         units::math::abs(previousAcceleration) + jerk * deltaT);
    return an * ((acceleration.value() > 0) ? 1 : -1);
}

units::acceleration::meters_per_second_squared_t
Ship::smoothAccelerate(
    units::acceleration::meters_per_second_squared_t &acceleration,
    units::acceleration::meters_per_second_squared_t &previousAccelerationValue,
    double &alpha,
    units::acceleration::meters_per_second_squared_t &maxAcceleration)
{
    if (maxAcceleration.value() > 0.0)
    {
        return units::math::min(
            maxAcceleration,
            alpha * acceleration + (1 - alpha) * previousAccelerationValue);
    }
    return alpha * acceleration + (1 - alpha) * previousAccelerationValue;
}

units::velocity::meters_per_second_t Ship::speedUpDown(
    units::velocity::meters_per_second_t &previousSpeed,
    units::acceleration::meters_per_second_squared_t &acceleration,
    units::time::second_t &deltaT,
    units::velocity::meters_per_second_t &freeFlowSpeed)
{
    units::velocity::meters_per_second_t u_next =
        units::math::min(previousSpeed + (acceleration * deltaT),
                         freeFlowSpeed);
    return units::math::max(u_next, units::velocity::meters_per_second_t(0.0));
}

units::acceleration::meters_per_second_squared_t Ship::adjustAcceleration(
    units::velocity::meters_per_second_t &speed,
    units::velocity::meters_per_second_t &previousSpeed,
    units::time::second_t &deltaT)
{
    return ((speed - previousSpeed) / deltaT);
}

bool Ship::checkSuddenAccChange(
    units::acceleration::meters_per_second_squared_t &previousAcceleration,
    units::acceleration::meters_per_second_squared_t &currentAcceleration,
    units::time::second_t &deltaT)
{
    if (units::math::abs((currentAcceleration -
                          previousAcceleration) / deltaT) >
        this->mMaxJerk)
    {
        emit suddenAccelerationOccurred(
            "sudden acceleration change!\n Report to the developer!");
        return true;
    }
    return false;
}


units::acceleration::meters_per_second_squared_t
Ship::getStepAcceleration(
    units::time::second_t &timeStep,
    units::velocity::meters_per_second_t &freeFlowSpeed,
    QVector<units::length::meter_t> &gapToNextCriticalPoint,
    QVector<bool> &isFollowingAnotherShip,
    QVector<units::velocity::meters_per_second_t> &leaderSpeeds,
    units::acceleration::meters_per_second_squared_t* maxAccel,
    units::force::newton_t* totalThrust,
    units::force::newton_t* totalResistance)
{
    units::length::meter_t minGap = units::length::meter_t(0.0);

    QVector<units::acceleration::meters_per_second_squared_t>
        allAccelerations;

    for (int i = 0; i < gapToNextCriticalPoint.size(); i++) {
        if (! isFollowingAnotherShip.at(i)) {
            allAccelerations.push_back(
                this->accelerate(gapToNextCriticalPoint.at(i),
                                 minGap, mSpeed,
                                 mAcceleration,
                                 leaderSpeeds.at(i),
                                 freeFlowSpeed,
                                 timeStep));
        }
    }

    units::acceleration::meters_per_second_squared_t nonSmoothedAcceleration =
        *std::min_element(allAccelerations.begin(), allAccelerations.end());

    size_t index = allAccelerations.indexOf(nonSmoothedAcceleration);
    this->accelerate(gapToNextCriticalPoint.at(index),
                     minGap, mSpeed,
                     mAcceleration,
                     leaderSpeeds.at(index),
                     freeFlowSpeed,
                     timeStep,
                     maxAccel,
                     totalThrust,
                     totalResistance);

    if (nonSmoothedAcceleration.value() < 0.0 &&
        mSpeed.value() <= 0.001 &&
        gapToNextCriticalPoint.back().value() > 50.0)
    {
        if (!mShowNoPowerMessage)
        {
            QString message;
            QTextStream stream(&message);
            stream << "Ship " << mShipUserID
                   << " Resistance is "
                   << "larger than ship tractive force at distance "
                   << mTraveledDistance.value() << "(m)\n";
            emit slowSpeedOrStopped(message);
            mShowNoPowerMessage = true;
        }

    }

    double alpha = (double)0.80;
    units::acceleration::meters_per_second_squared_t smoothedAcceleration =
        smoothAccelerate(nonSmoothedAcceleration,
                         mPreviousAcceleration, alpha, *maxAccel);
    units::acceleration::meters_per_second_squared_t jerkAcceleration =
        accelerateConsideringJerk(smoothedAcceleration,
                                  mPreviousAcceleration,
                                  mMaxJerk,
                                  timeStep);

    if (round(mSpeed.value()*1000.0)/1000.0 == 0.0
        && jerkAcceleration.value() < 0) {
        jerkAcceleration =
            units::acceleration::meters_per_second_squared_t(0.0);
    }
    return jerkAcceleration;

}

void Ship::sail(units::time::second_t &timeStep,
                    units::velocity::meters_per_second_t &freeFlowSpeed,
                    QVector<units::length::meter_t> &gapToNextCriticalPoint,
                    QVector<bool> &isFollowingAnotherShip,
                    QVector<units::velocity::meters_per_second_t> &leaderSpeeds,
                    AlgebraicVector::Environment currentEnvironment)
{
    // // update the engine state first
    // for (auto& propeller: mPropellers) {
    //     propeller->getGearBox()->updateGearboxOperationalState();
    // }

    // set the environment before doing anything
    setCurrentEnvironment(currentEnvironment);

    units::velocity::meters_per_second_t maxSpeed =
        units::math::min(freeFlowSpeed, mMaxSpeed);

    units::acceleration::meters_per_second_squared_t jerkAcceleration =
        this->getStepAcceleration(timeStep,
                                  maxSpeed,
                                  gapToNextCriticalPoint,
                                  isFollowingAnotherShip,
                                  leaderSpeeds,
                                  &mMaxAcceleration,
                                  &mTotalThrust,
                                  &mTotalResistance);

    mAcceleration = jerkAcceleration;
    mPreviousSpeed = this->mSpeed;
    mSpeed = this->speedUpDown(this->mPreviousSpeed,
                               this->mAcceleration,
                               timeStep, maxSpeed);
    mAcceleration = this->adjustAcceleration(this->mSpeed,
                                             this->mPreviousSpeed,
                                             timeStep);
    checkSuddenAccChange(this->mPreviousAcceleration,
                         this->mAcceleration,
                         timeStep);
    setStepTravelledDistance(this->mSpeed * timeStep, timeStep);

    QSet<int> uniqueEngineIDs; // holds ids of engines
    bool anyEngineOn = false;  // if any engine is still running

    // loop over all propellers
    // 2 propellers can share an engine,
    // so do not consume twice the energy!
    for (auto propeller : mPropellers)
    {
        // get the propeller engine
        auto engines = propeller->getGearBox()->getEngines();

        // if a propeller has 2 engines (rare case)
        for (auto engine: engines)
        {
            // check duplicates
            if (!uniqueEngineIDs.contains(engine->getEngineID()))
            {
                // consume required energy
                auto ecr = engine->consumeUsedEnergy(timeStep);
                mCumConsumedFuel[ecr.fuelConsumed.first] +=
                    ecr.fuelConsumed.second;

                addToCummulativeConsumedEnergy(
                    ecr.energyConsumed);
                uniqueEngineIDs.insert(engine->getEngineID());

                // if any engine is still running
                if (engine->isEngineWorking())
                {
                    anyEngineOn = true; // set to true
                }
            }
        }
    }

    // update the outofenergy and ison variables
    mOutOfEnergy = !anyEngineOn; // the inverse of anyEngineOn (not the diff)
    mIsOn = anyEngineOn;

    units::length::meter_t lastStepDistance = mSpeed * timeStep;

    // if the ship is within 10m from its destination,
    // count it as reached destination
    if (mPathPoints.back()->distance(mCurrentState.getCurrentPosition()) <=
        lastStepDistance)
    {
        immediateStop(timeStep);
        mReachedDestination = true;

        auto json = getCurrentStateAsJson();
        emit reachedDestination(json);
    }

    // update operational power of the engine
    // if (mTotalThrust < mTotalResistance &&
    //     getSpeed() < getMaxSpeed() * 0.8) {
    //     for (auto& propeller: mPropellers) {
    //         propeller->requestHigherEnginePower();
    //     }
    // }
    // else if (mTotalThrust.value() > 1.25 * mTotalResistance.value() &&
    //          mPropellers[0]->getCurrentOperationalLoad() >
    //              IShipEngine::EngineOperationalLoad::Economic) {
    //     for (auto& propeller: mPropellers) {
    //         propeller->requestLowerEnginePower();
    //     }
    // }
}

void Ship::calculateGeneralStats(units::time::second_t timeStep) {
    // count the timeStep;
    mTripTime += timeStep;


    // calculate running averages
    mRunningAvrAcceleration =
        units::acceleration::meters_per_second_squared_t(
            calculateRunningAverage(mRunningAvrAcceleration.value(),
                                    mAcceleration.value(), timeStep.value()));
    mRunningAvrSpeed =
        units::velocity::meters_per_second_t(
            calculateRunningAverage(mRunningAvrSpeed.value(),
                                    mSpeed.value(), timeStep.value()));
}

QVector<units::length::meter_t> Ship::getLinksCumLengths() const
{
    return mLinksCumLengths;
}


bool Ship::isLoaded() const
{
    return mLoaded;
}

void Ship::load()
{
    reset();
    mLoaded = true;
}

void Ship::reset()
{
    mAcceleration = units::acceleration::meters_per_second_squared_t(0.0);
    mPreviousAcceleration =
        units::acceleration::meters_per_second_squared_t(0.0);
    mRunningAvrAcceleration =
        units::acceleration::meters_per_second_squared_t(0.0);
    mSpeed = units::velocity::meters_per_second_t(0.0);
    mPreviousSpeed = units::velocity::meters_per_second_t(0.0);
    mRunningAvrSpeed = units::velocity::meters_per_second_t(0.0);
    mTraveledDistance = units::length::meter_t(0.0);
    mTripTime = units::time::second_t(0.0);
    mCumConsumedEnergy = units::energy::kilowatt_hour_t(0.0);

    mCumConsumedFuel.clear();
    for (auto& ft: ShipFuel::getFuelTypes()) {
        mCumConsumedFuel.insert({ft, units::volume::liter_t(0.0)});
    }

    mIsOn = true;
    mOffLoaded = false;
    mReachedDestination = false;
    mOutOfEnergy = false;
    mLoaded = false;

    GPoint sp = *(mPathPoints.at(0));
    GPoint ep = *(mPathPoints.at(1));
    mCurrentState = GAlgebraicVector(sp, ep);

    mPreviousPathPointIndex = 0;
    mTotalResistance = units::force::newton_t(0.0);

    // reset the energy source
    for (auto& propeller: mPropellers) {
        for (auto& engine : propeller->getGearBox()->getEngines()) {
            engine->getCurrentEnergySource()->reset();
        }
    }
}

std::size_t Ship::getPreviousPathPointIndex() const
{
    return mPreviousPathPointIndex;
}

units::time::second_t Ship::getStartTime() const
{
    return mStartTime;
}

void Ship::setStartTime(const units::time::second_t &newStartTime)
{
    mStartTime = newStartTime;
}

void Ship::addToCummulativeConsumedEnergy(
    units::energy::kilowatt_hour_t consumedkWh)
{
    mCumConsumedEnergy += consumedkWh;
}

units::unit_t<units::compound_unit<units::mass::metric_tons,
                                   units::length::kilometers>>
Ship::getTotalCargoTonKm() const
{
    return mTotalCargoTonKm;
}

units::time::second_t Ship::getTripTime()
{
    return mTripTime;
}

double Ship::calculateRunningAverage(
    double previousAverage, double currentTimeStepData, double timeStep) {
    double n = this->mTripTime.value() / timeStep;
    return (previousAverage * ((n - 1)/n)) + (currentTimeStepData / n);
}

units::energy::kilowatt_hour_t Ship::getCumConsumedEnergy() const
{
    return mCumConsumedEnergy;
}

std::map<ShipFuel::FuelType, units::volume::liter_t>
Ship::getCumConsumedFuel() const
{
    return mCumConsumedFuel;
}

units::unit_t<
    units::compound_unit<units::energy::kilowatt_hour,
                         units::inverse<units::mass::metric_ton>>>
Ship::getEnergyConsumptionPerTon() const
{
    return getCumConsumedEnergy() / getCargoWeight();
}

units::unit_t<
    units::compound_unit<units::energy::kilowatt_hour,
                         units::inverse<
                             units::compound_unit<
                                 units::length::meter,
                                 units::mass::metric_ton>>>>
Ship::getEnergyConsumptionPerTonKM() const
{
    return getCumConsumedEnergy() / getTotalCargoTonKm();
}

units::volume::liter_t Ship::getOverallCumFuelConsumption() const
{
    units::volume::liter_t total = units::volume::liter_t(0.0);
    for (auto& f: this->mCumConsumedFuel) {
        total += f.second;
    }
    return total;
}

units::unit_t<
    units::compound_unit<units::volume::liter,
                         units::inverse<units::mass::metric_ton>>>
Ship::getOverallCumFuelConsumptionPerTon() const
{
    return getOverallCumFuelConsumption() / getCargoWeight();
}

units::unit_t<
    units::compound_unit<units::volume::liter,
                         units::inverse<
                             units::compound_unit<
                                 units::length::meter,
                                 units::mass::metric_ton>>>>
Ship::getOverallCumFuelConsumptionPerTonKM() const
{
    return getOverallCumFuelConsumption() / getTotalCargoTonKm();
}

QJsonObject Ship::getCurrentStateAsJson() const
{
    QJsonObject json;

    // Add simple attributes
    json["ShipID"] = mShipUserID;
    json["TravelledDistance"] = mTraveledDistance.value();
    json["CurrentAcceleration"] = mAcceleration.value();
    json["PreviousAcceleration"] = mPreviousAcceleration.value();
    json["CurrentSpeed"] = mSpeed.value();
    json["PreviousSpeed"] = mPreviousSpeed.value();
    json["TotalThrust"] = mTotalThrust.value();
    json["TotalResistance"] = mTotalResistance.value();
    json["VesselWeight"] = mVesselWeight.value();
    json["CargoWeight"] = mCargoWeight.value();
    json["IsOn"] = mIsOn;
    json["OutOfEnergy"] = mOutOfEnergy;
    json["Loaded"] = mLoaded;
    json["ReachedDestination"] = mReachedDestination;
    QJsonObject stateJson;
    stateJson["EenergyConsumption"] = mCumConsumedEnergy.value();
    QJsonArray fuelConsumptionArray;
    for (const auto& fuelEntry : mCumConsumedFuel) {
        QJsonObject fuelJson;
        fuelJson["FuelType"] =
            ShipFuel::convertFuelTypeToString(fuelEntry.first);
        fuelJson["ConsumedVolumeLiters"] = fuelEntry.second.value();
        fuelConsumptionArray.append(fuelJson);
    }
    stateJson["FuelConsumption"] = fuelConsumptionArray;
    json["Consumption"] = stateJson;

    // Add energy sources (as an array)
    QJsonArray energySourcesArray;
    for (const auto& energySource : mEnergySources) {
        // You can expand this if `IEnergySource` has more
        // detailed information to add
        QJsonObject energySourceJson;
        energySourceJson["Capacity"] =
            energySource->getCurrentCapacityState();
        energySourceJson["FuelType"] =
            ShipFuel::convertFuelTypeToString(energySource->getFuelType());
        energySourceJson["EnergyConsumed"] =
            energySource->getTotalEnergyConsumed().value();
        energySourceJson["Weight"] = energySource->getCurrentWeight().value();
        energySourcesArray.append(energySourceJson);
    }
    json["EnergySources"] = energySourcesArray;

    QJsonObject posJson;
    auto cp = mCurrentState.getCurrentPosition();
    posJson["Latitude"] = cp.getLatitude().value();
    posJson["Longitude"] = cp.getLongitude().value();
    QJsonArray xyPosition;
    xyPosition.append(cp.getLatitude().value());
    xyPosition.append(cp.getLongitude().value());
    posJson["Position"] = xyPosition;
    json["Position"] = posJson;

    // Environment conditions
    QJsonObject envJson;
    envJson["WaterDepth"] = mCurrentState.getEnvironment().waterDepth.value();
    envJson["Salinity"] = mCurrentState.getEnvironment().salinity.value();
    envJson["Temperature"] = mCurrentState.getEnvironment().temperature.value();
    envJson["WaveHeight"] = mCurrentState.getEnvironment().waveHeight.value();
    envJson["WaveLength"] = mCurrentState.getEnvironment().waveLength.value();
    envJson["WaveAngularFrequency"] =
        mCurrentState.getEnvironment().waveAngularFrequency.value();
    json["Environment"] = envJson;

#ifdef BUILD_SERVER_ENABLED
    json["ContainerMap"] = mLoadedContainers.toJson();
#endif

    // Convert to JSON document
    return json;
}

#ifdef BUILD_SERVER_ENABLED
QVector<ContainerCore::Container*> Ship::getLoadedContainers() const {
    QVector<ContainerCore::Container*> containerList;
    for (auto &container : mLoadedContainers.containers()) {
        containerList.append(container);
    }
    return containerList;
}

void Ship::addContainer(ContainerCore::Container* container) {
    if (container) {
        mLoadedContainers.addContainer(container->getContainerID(), container);
    }
}
#endif

QVector<units::length::meter_t> Ship::generateCumLinesLengths()
{
    qsizetype n = mPathLines.size();

    if (n < 1)
    {
        qFatal("Ship number of links should "
               "be greater than zero!");
    }

    QVector<units::length::meter_t>
        linksCumLengths(n, units::length::meter_t(0.0));

    linksCumLengths[0] =
        units::length::meter_t(
            mPathLines.at(0)->length());

    for (qsizetype i = 1; i < n; i++)
    {
        linksCumLengths[i] = linksCumLengths[i-1] +
                             units::length::meter_t(
                                 mPathLines.at(i)->length());
    }

    return linksCumLengths;
}

units::length::meter_t Ship::distanceToFinishFromPathNodeIndex(qsizetype i)
{
    if (i < 0 || i > mLinksCumLengths.size())
    {
        qFatal("Node index should "
               "be within zero and node path size!");
    }
    if (i == mLinksCumLengths.size())
    {
        return units::length::meter_t(0.0);
    }

    auto passedLength = mLinksCumLengths[i-1];
    return mLinksCumLengths.back() - passedLength;
}

units::length::meter_t
Ship::distanceToNodePathIndexFromPathNodeIndex(qsizetype startIndex,
                                               qsizetype endIndex)
{
    if (endIndex < startIndex)
    {
        qFatal("Start index is greater than end index");
    }
    if (startIndex < 0 || endIndex >= mLinksCumLengths.size())
    {
        qFatal("Node indices should "
                            "be within zero and node path size!");
    }

    if (startIndex == endIndex) { return units::length::meter_t(0.0); }

    auto passedLength =
        (startIndex > 0) ? mLinksCumLengths[startIndex-1] :
            units::length::meter_t(0);
    return mLinksCumLengths[endIndex] - passedLength;
}

units::length::meter_t Ship::distanceFromCurrentPositionToNodePathIndex(
    qsizetype endIndex)
{
    if (endIndex > mLinksCumLengths.size() || endIndex < 0)
    {
        qFatal("End index should be between "
               "zero and node path size!");
    }
    qsizetype nextIndex = mPreviousPathPointIndex + 1;
    auto rest =
        (nextIndex == endIndex) ? units::length::meter_t(0.0) :
            distanceToNodePathIndexFromPathNodeIndex(nextIndex, endIndex);
    return rest +
           mCurrentState.getCurrentPosition().
           distance(*mPathPoints[mPreviousPathPointIndex+1]);

}

double Ship::progress()
{
    // if the ship has not yet been loaded, return 0.0
    if (! mLoaded) return 0.0;
    // if the ship reached its destination, return 1.0
    if (mReachedDestination) return 1.0;

    // if neither, calculate the progress

    // calculate distance to finish from next node
    auto cumToFinish =
        distanceToFinishFromPathNodeIndex(mPreviousPathPointIndex + 1);

    // calculate distance to next point
    cumToFinish +=
        mCurrentState.getCurrentPosition().
        distance(*mPathPoints[mPreviousPathPointIndex + 1]);

    // return the progress proportion
    return ((mLinksCumLengths.back().value() - cumToFinish.value()) /
            mLinksCumLengths.back().value());
}

// units::velocity::meters_per_second_t Ship::getCurrentMaxSpeed()
// {
//     return mPathLines[mPreviousPathPointIndex]->getMaxSpeed();
// }

// QHash<qsizetype, units::velocity::meters_per_second_t>
// Ship::getAheadLowerSpeeds(qsizetype nextStopIndex)
// {
//     if (mLowerSpeedLinkIndex.isEmpty() ||
//         mLowerSpeedLinkIndex[mPreviousPathPointIndex].isEmpty() ||
//         mLowerSpeedLinkIndex[mPreviousPathPointIndex][nextStopIndex].empty())
//     {
//         QHash<qsizetype, units::velocity::meters_per_second_t>
//             lowerSpeedMapping;

//         for(qsizetype i = mPreviousPathPointIndex + 1;
//              i < mPathLines.size(); ++i)
//         {
//             const auto& currentLine = mPathLines[i];
//             const auto& previousLine = mPathLines[i - 1];

//             // if(currentLine->getMaxSpeed() < previousLine->getMaxSpeed())
//             // {
//             //     lowerSpeedMapping.insert(i, currentLine->getMaxSpeed());
//             // }
//         }

//         // resize mLowerSpeedLinkIndex to store the calculated values
//         if (mLowerSpeedLinkIndex.size() - 1 <= mPreviousPathPointIndex)
//         {
//             mLowerSpeedLinkIndex.resize(mPreviousPathPointIndex + 1);
//         }
//         if (mLowerSpeedLinkIndex[mPreviousPathPointIndex].size() - 1 <=
//             nextStopIndex)
//         {
//             mLowerSpeedLinkIndex[mPreviousPathPointIndex].resize(
//                 nextStopIndex + 1);
//         }
//         mLowerSpeedLinkIndex[mPreviousPathPointIndex][nextStopIndex] =
//             lowerSpeedMapping;
//         return lowerSpeedMapping;
//     }
//     else
//     {
//         return mLowerSpeedLinkIndex[mPreviousPathPointIndex][nextStopIndex];
//     }

// }

void Ship::computeStoppingPointIndices()
{
    mStoppingPointIndices.clear();
    for (qsizetype i = 0; i < mPathPoints.size(); ++i)
    {
        if (mPathPoints[i]->isPort())
        {
            mStoppingPointIndices.append(i);
        }
    }
}

Ship::stopPointDefinition Ship::getNextStoppingPoint()
{
    Ship::stopPointDefinition result;

    // find the point index where the point is identified as a port
    // and its index is larger than the previous ship index
    auto it = std::lower_bound(mStoppingPointIndices.begin(),
                               mStoppingPointIndices.end(),
                               mPreviousPathPointIndex);
    if(it != mStoppingPointIndices.end())
    {
        result.pointIndex = *it;
        result.point = mPathPoints[*it];
        return result;
    }
    result.pointIndex = mPathPoints.size() - 1;
    result.point = mPathPoints.back();
    return result;
}


void Ship::unload()
{
    mLoaded = false;
}


bool Ship::isOutOfEnergy() const
{
    return mOutOfEnergy;
}

bool Ship::isReachedDestination() const
{
    return mReachedDestination;
}

void Ship::immediateStop(units::time::second_t &timestep)
{
    (void)timestep; // unused for now
    this->mPreviousAcceleration = this->mAcceleration;
    this->mPreviousSpeed = this->mSpeed;
    this->mSpeed =
        units::velocity::meters_per_second_t(0.0);
    this->mAcceleration =
        units::acceleration::meters_per_second_squared_t(0.0);
}

void Ship::kickForwardADistance(
    units::length::meter_t &distance,
    units::time::second_t timeStep)
{
    this->mPreviousAcceleration =
        units::acceleration::meters_per_second_squared_t(0.0);
    this->mAcceleration =
        units::acceleration::meters_per_second_squared_t(0.0);
    this->mPreviousSpeed = units::velocity::meters_per_second_t(0.0);
    this->mSpeed = units::velocity::meters_per_second_t(0.0);
    setStepTravelledDistance(distance,
                             timeStep);
}

void Ship::setStepTravelledDistance(units::length::meter_t distance,
                                    units::time::second_t timeStep)
{

    if(distance != units::length::meter_t(0.0))
    {
        mTraveledDistance += distance;
        emit stepDistanceChanged(distance, timeStep);
    }
}

// Point Ship::getPositionByTravelledDistance(
//     units::length::meter_t newTotalDistance)
// {
//     if (newTotalDistance >= mTotalPathLength)
//     {
//         return *(mPathPoints.back());
//     }
//     // Initialize a Point object, assuming Point is default-constructible.
//     Point result;

//     for (qsizetype i = mPreviousPathPointIndex;
//          i < mLinksCumLengths.size();
//          ++i)
//     {
//         if (mLinksCumLengths[i] >= newTotalDistance)
//         {
//             // Update previousPathPointIndex to the index of the highest value
//             // that is lower than newTotalDistance.
//             mPreviousPathPointIndex = (i == 0) ? 0 : i - 1;
//             break;
//         }
//     }

//     if (mPreviousPathPointIndex >= mLinksCumLengths.size())
//     {
//         // If no such value exists, then all elements in
//         // mLinksCumLengths are lower than newTotalDistance.
//         // In this case, set previousPathPointIndex to the
//         // last index in mLinksCumLengths.
//         if (!mLinksCumLengths.empty())
//         {
//             mPreviousPathPointIndex = mLinksCumLengths.size() - 1;
//         }
//     }

//     units::length::meter_t remainingDistance;
//     if (mPreviousPathPointIndex == 0)
//     {
//         remainingDistance = mTraveledDistance;
//     }
//     else
//     {
//         remainingDistance =
//             mTraveledDistance - mLinksCumLengths[mPreviousPathPointIndex - 1];
//     }

//     // Calculate the current coordinates based on the
//     // line and traveled distance. This assumes that
//     // getPointByDistance returns a Point when provided with
//     // the remaining distance and a starting point.
//     result = mPathLines[mPreviousPathPointIndex]->getPointByDistance(
//         remainingDistance,
//         mPathPoints[mPreviousPathPointIndex]
//         );

//     return result;
// }

bool Ship::isShipOnCorrectPath()
{
    return true;
    // // No path or single point path, consider it on path
    // if (mPathPoints.size() < 2) return true;

    // units::length::meter_t positionTolerance = units::length::meter_t(10.0);
    // units::angle::degree_t orientationTolerance = units::angle::degree_t(5.0);
    // auto currentPosition = mCurrentState.getCurrentPosition();
    // auto currentOrientation = mCurrentState.getOrientationWithRespectToTarget();

    // for(int i = 0; i < mPathPoints.size() - 1; i++)
    // {
    //     auto startPoint = mPathPoints[i];
    //     auto endPoint = mPathPoints[i + 1];

    //     auto l = Line(startPoint, endPoint);
    //     // Check Positional Deviation
    //     auto distance = l.getPerpendicularDistance(currentPosition);


    //     if (distance > positionTolerance) {
    //         return false; // Positional deviation is too high
    //     }

    //     // Check Orientation Deviation
    //     auto pathSegmentOrientation =
    //         l.toAlgebraicVector(startPoint).getOrientationWithRespectToTarget();
    //     auto orientationDifference =
    //         units::math::abs((currentOrientation - pathSegmentOrientation));
    //     if (orientationDifference > orientationTolerance) {
    //         return false; // Orientation deviation is too high
    //     }
    // }

    // return true; // The ship is on path
}


// this has a problem here. if the ship needs to turn in
// raduis that is large and the line length is very short,
// the ship may not be able to rotate appropiately
void Ship::handleStepDistanceChanged(units::length::meter_t newTotalDistance,
                                     units::time::second_t timeStep)
{
    // Check if the path has less than two points,
    // if so, the ship cannot move.
    if (mPathPoints.size() < 2) {
        qWarning() << "Path is empty or has only one point."
                      " No movement will occur.";
        return;
    }

    // update the ton.km
    mTotalCargoTonKm += mCargoWeight * newTotalDistance;

    // Obtain the current target point from the path.
    std::shared_ptr<GPoint> currentTarget =
        mPathPoints[mPreviousPathPointIndex + 1];

    // Calculate the distance at which the ship should start turning
    auto r = calcTurningRadius();

    // get the next target
    if (mPreviousPathPointIndex + 2 > mPathPoints.size() - 1)
    {

        auto maxROT = calcMaxROT(r);
        mCurrentState.setTargetAndMaxROT(*currentTarget, maxROT);
        mCurrentState.moveByDistance(newTotalDistance, timeStep);
        return;
    }
    std::shared_ptr<GPoint> nextTarget =
        mPathPoints[mPreviousPathPointIndex + 2];

    // Calculate the remaining distance to the target point.
    units::length::meter_t distanceToTarget =
        mCurrentState.getCurrentPosition().distance(*currentTarget);


    // calculate the angle between the ship orientation and next target
    units::angle::degree_t turningAngleInDegrees =
        mCurrentState.angleTo(*nextTarget);
    while (turningAngleInDegrees.value() > 180.0)
    {
        turningAngleInDegrees -= units::angle::degree_t(180);
    }
    while (turningAngleInDegrees.value() < 0.0)
    {
        turningAngleInDegrees += units::angle::degree_t(180);
    }
    auto turningAngleInRad =
        turningAngleInDegrees.convert<units::angle::radian>();

    // Check if turningAngleInDegrees is within [150, 210] degrees range
    if(turningAngleInDegrees.value() > (180.0 - mRudderAngle.value()) &&
        turningAngleInDegrees.value() < (180.0 + mRudderAngle.value()))
    {
        // If ship is moving almost in straight line,
        // set turning radius to zero
        r = units::length::meter_t(0.0);
    }

    auto distanceToStartTurning =
        r * std::tan(turningAngleInRad.value() / 2.0);


    // if the distance to target is less than
    // the required distance to turn, do the turn immediately
    if (distanceToTarget <= distanceToStartTurning)
    {
        // Move to the next segment
        mPreviousPathPointIndex++;

        // If there are more points in the path,
        // update the orientation before moving to the next segment
        if (mPreviousPathPointIndex < mPathPoints.size() - 2)
        {
            std::shared_ptr<GPoint> newTarget =
                mPathPoints[mPreviousPathPointIndex + 1];
            auto maxROT = calcMaxROT(r);
            mCurrentState.setTargetAndMaxROT(*newTarget, maxROT);
        }

        // If there is enough distance remaining to reach the target,
        // update orientation and move the ship to the target
        mCurrentState.moveByDistance(newTotalDistance, timeStep);

    }
    else
    {
        // Move only by the remaining distance,
        // and adjust the position and orientation accordingly
        mCurrentState.moveByDistance(newTotalDistance, timeStep);
        newTotalDistance = units::length::meter_t(0.0);
    }


    // Similar logic applies for the last point in the path
    if (mPreviousPathPointIndex == mPathPoints.size() - 2)
    {
        std::shared_ptr<GPoint> lastPoint = mPathPoints.last();
        units::length::meter_t distanceToLast =
            mCurrentState.getCurrentPosition().distance(*lastPoint);

        if (newTotalDistance >= distanceToLast)
        {
            mCurrentState.moveByDistance(distanceToLast, timeStep);
        }
        else
        {
            mCurrentState.moveByDistance(newTotalDistance, timeStep);
        }
    }

    // Emit a signal if the ship is deviating from the correct path
    if (!isShipOnCorrectPath())
    {
        emit pathDeviation("Ship is deviating from Path");
    }
}




//void Ship::handleStepDistanceChanged(units::length::meter_t newTotalDistance,
//                                     units::time::second_t timeStep)
//{
//    // Check for empty path or single-point path
//    if (mPathPoints.size() < 2) {
//        qWarning() << "Path is empty or has only one point. "
//                      "No movement will occur.";
//        return;
//    }

//    while (newTotalDistance > units::length::meter_t(0.0) &&
//           mPreviousPathPointIndex < mPathPoints.size() - 2)
//    {
//        // Calculate remaining distance to the target
//        std::shared_ptr<Point> currentTarget =
//            mPathPoints[mPreviousPathPointIndex + 1];

//        units::length::meter_t distanceToTarget =
//            mCurrentState.getCurrentPosition().distance(*currentTarget);

//        if (newTotalDistance >= distanceToTarget) {
//            // Move to the current target point
//            mCurrentState.moveByDistance(distanceToTarget, timeStep);

//            // Update to next point in path
//            mPreviousPathPointIndex++;

//            // Deduct the distance to current target from remaining distance
//            newTotalDistance -= distanceToTarget;

//            // Update orientation before moving, if there is another target
//            if (mPreviousPathPointIndex < mPathPoints.size() - 2)
//            {
//                std::shared_ptr<Point> newTarget =
//                    mPathPoints[mPreviousPathPointIndex + 1];
//                // assuming the ship does not reduce speed while turning
//                auto r = calcTurningRaduis();
//                auto maxROT = calcMaxROT(r);
//                mCurrentState.setTargetAndMaxROT(*newTarget, maxROT);
//            }
//        }
//        else
//        {
//            // If remaining distance is less than the distance to the target,
//            // move only by the remaining distance
//            mCurrentState.moveByDistance(newTotalDistance, timeStep);
//            newTotalDistance = units::length::meter_t(0.0);
//        }
//    }

//    // If mPreviousPathPointIndex is size() - 2,
//    // then simply move to the last point
//    if (mPreviousPathPointIndex == mPathPoints.size() - 2)
//    {
//        std::shared_ptr<Point> lastPoint = mPathPoints.last();
//        units::length::meter_t distanceToLast =
//            mCurrentState.getCurrentPosition().distance(*lastPoint);

//        if (newTotalDistance >= distanceToLast)
//        {
//            mCurrentState.moveByDistance(distanceToLast, timeStep);
//        }
//        else
//        {
//            // In this scenario, we shouldn't be here but just to be safe.
//            mCurrentState.moveByDistance(newTotalDistance, timeStep);
//        }
//    }

//    if (!isShipOnCorrectPath())
//    {
//        emit pathDeviation("Ship is deviating from Path");
//    }
//}

//units::velocity::meters_per_second_t
//Ship::calc_shallowWaterSpeedReduction(
//    units::velocity::meters_per_second_t speed,
//    units::length::meter_t waterDepth)
//{
//    return units::velocity::meters_per_second_t(
//        ((0.1242 *
//              (mMidshipSectionCoef /
//               std::pow(waterDepth.value(),2.0) ) -
//          0.05) +
//         1.0 -
//         std::sqrt(
//             std::tanh(mCurrentLink->depth().value() *
//                       hydrology::G.value() /
//                       std::pow(speed.value(),2.0)))) * speed.value());
//}

units::angle::degree_t Ship::calcMaxROT(units::length::meter_t turnRaduis)
{
    return units::angle::degree_t(mSpeed.value()/turnRaduis.value()/60.0);
}
units::length::meter_t Ship::calcTurningRadius()
{
    return getLengthInWaterline() /
           units::math::tan(
               mRudderAngle.convert<units::angle::radian>()).value();
}

};
