#include "ship.h"
#include <cmath>
#include "hydrology.h"
#include "ishipresistancepropulsionstrategy.h"
#include "holtropmethod.h"
#include <QDebug>
#include "../../third_party/units/units.h"
#include "../utils/utils.h"

Ship::Ship(units::length::meter_t lengthInWaterline,
           units::length::meter_t moldedBeam,
           double midshipCoef,
           double LongitudinalBuoyancyCenter,
           units::length::nanometer_t roughness,
           units::area::square_meter_t bulbousBowTransverseArea,
           units::length::meter_t bulbousBowTransverseAreaCenterHeight,
           units::area::square_meter_t immersedTransomArea,
           units::length::meter_t moldedMeanDraft,
           units::length::meter_t moldedDraftAtAft,
           units::length::meter_t moldedDraftAtForward,
           double blockCoef,
           BlockCoefficientMethod blockCoefMethod,
           double prismaticCoef,
           units::length::meter_t runLength,
           double waterplaneAreaCoef,
           WaterPlaneCoefficientMethod waterplaneCoefMethod,
           units::volume::cubic_meter_t volumetricDisplacement,
           units::area::square_meter_t wettedHullSurface,
           WetSurfaceAreaCalculationMethod wetSurfaceAreaMethod,
           units::angle::degree_t halfWaterlineEntranceAngle,
           IShipResistancePropulsionStrategy* initialStrategy) :
    mStrategy(initialStrategy),
    mWaterlineLength(lengthInWaterline),
    mBeam(moldedBeam),
    mMeanDraft(moldedMeanDraft),
    mDraftAtForward(moldedDraftAtForward),
    mDraftAtAft(moldedDraftAtAft),
    mVolumetricDisplacement(volumetricDisplacement),
    mWettedHullSurface(wettedHullSurface),
    mWetSurfaceAreaMethod(wetSurfaceAreaMethod),
    mBulbousBowTransverseAreaCenterHeight(bulbousBowTransverseAreaCenterHeight),
    mAppendagesWettedSurfaces(),
    mBulbousBowTransverseArea(bulbousBowTransverseArea),
    mImmersedTransomArea(immersedTransomArea),
    mHalfWaterlineEntranceAngle(halfWaterlineEntranceAngle),
    mSpeed(units::velocity::meters_per_second_t(0.0)),
    mSurfaceRoughness(roughness),
    mRunLength(runLength),
    mLongitudinalBuoyancyCenter(LongitudinalBuoyancyCenter),
    mSternShapeParam(CStern::NormalSections),
    mMidshipSectionCoef(midshipCoef),
    mWaterplaneAreaCoef(waterplaneAreaCoef),
    mWaterplaneCoefMethod(waterplaneCoefMethod),
    mPrismaticCoef(prismaticCoef),
    mBlockCoef(blockCoef),
    mBlockCoefMethod(blockCoefMethod)
{
    initializeDefaults();
}

Ship::Ship(units::length::meter_t lengthInWaterline,
           units::length::meter_t moldedBeam,
           units::length::meter_t moldedMeanDraft) :
    mStrategy(nullptr),
    mWaterlineLength(lengthInWaterline),
    mBeam(moldedBeam),
    mMeanDraft(moldedMeanDraft),
    mDraftAtForward(units::length::meter_t(std::nan("uninitialized"))),
    mDraftAtAft(units::length::meter_t(std::nan("uninitialized"))),
    mVolumetricDisplacement(
        units::volume::cubic_meter_t(std::nan("uninitialized"))),
    mWettedHullSurface(units::area::square_meter_t(std::nan("uninitialized"))),
    mWetSurfaceAreaMethod(WetSurfaceAreaCalculationMethod::None),
    mBulbousBowTransverseAreaCenterHeight(
        units::length::meter_t(std::nan("uninitialized"))),
    mAppendagesWettedSurfaces(),
    mBulbousBowTransverseArea(
        units::area::square_meter_t(std::nan("uninitialized"))),
    mImmersedTransomArea(
        units::area::square_meter_t(std::nan("uninitialized"))),
    mHalfWaterlineEntranceAngle(
        units::angle::degree_t(std::nan("uninitialized"))),
    mSpeed(units::velocity::meters_per_second_t(0.0)),
    mSurfaceRoughness(
        units::length::nanometer_t(std::nan("uninitialized"))),
    mRunLength(units::length::meter_t(std::nan("uninitialized"))),
    mLongitudinalBuoyancyCenter(std::nan("uninitialized")),
    mSternShapeParam(CStern::None),
    mMidshipSectionCoef(std::nan("uninitialized")),
    mWaterplaneAreaCoef(std::nan("uninitialized")),
    mWaterplaneCoefMethod(WaterPlaneCoefficientMethod::None),
    mPrismaticCoef(std::nan("uninitialized")),
    mBlockCoef(std::nan("uninitialized")),
    mBlockCoefMethod(BlockCoefficientMethod::None)
{
    initializeDefaults();
}

Ship::Ship(units::length::meter_t lengthInWaterline,
           units::length::meter_t moldedBeam,
           units::length::meter_t moldedDraftAtAft,
           units::length::meter_t moldedDraftAtForward) :
    mStrategy(nullptr),
    mWaterlineLength(lengthInWaterline),
    mBeam(moldedBeam),
    mMeanDraft(std::nan("uninitialized")),
    mDraftAtForward(units::length::meter_t(moldedDraftAtForward)),
    mDraftAtAft(units::length::meter_t(moldedDraftAtAft)),
    mVolumetricDisplacement(
        units::volume::cubic_meter_t(std::nan("uninitialized"))),
    mWettedHullSurface(units::area::square_meter_t(std::nan("uninitialized"))),
    mWetSurfaceAreaMethod(WetSurfaceAreaCalculationMethod::None),
    mBulbousBowTransverseAreaCenterHeight(
          units::length::meter_t(std::nan("uninitialized"))),
    mAppendagesWettedSurfaces(),
    mBulbousBowTransverseArea(
        units::area::square_meter_t(std::nan("uninitialized"))),
    mImmersedTransomArea(
        units::area::square_meter_t(std::nan("uninitialized"))),
    mHalfWaterlineEntranceAngle(
        units::angle::degree_t(std::nan("uninitialized"))),
    mSpeed(units::velocity::meters_per_second_t(0.0)),
    mSurfaceRoughness(
        units::length::nanometer_t(std::nan("uninitialized"))),
    mRunLength(units::length::meter_t(std::nan("uninitialized"))),
    mLongitudinalBuoyancyCenter(std::nan("uninitialized")),
    mSternShapeParam(CStern::None),
    mMidshipSectionCoef(std::nan("uninitialized")),
    mWaterplaneAreaCoef(std::nan("uninitialized")),
    mWaterplaneCoefMethod(WaterPlaneCoefficientMethod::None),
    mPrismaticCoef(std::nan("uninitialized")),
    mBlockCoef(std::nan("uninitialized")),
    mBlockCoefMethod(BlockCoefficientMethod::None)

{
    initializeDefaults();
}

Ship::Ship(const QMap<QString, std::any>& parameters)
{
//    mResistanceStrategy =
//        getValueFromMap<IShipResistanceStrategy*>(
//            parameters,
//            "ResistanceStrategy",
//            nullptr);

    if (parameters.contains("ResistanceStrategy")) {
        try {
            // Try casting the std::any to the base type first
            if (parameters["ResistanceStrategy"].type() ==
                typeid(HoltropMethod*)) {
                HoltropMethod* temp =
                    std::any_cast<HoltropMethod*>(
                    parameters["ResistanceStrategy"]);
                mStrategy = temp; // Upcast is implicit
            }
            else if (parameters["ResistanceStrategy"].type() !=
                       typeid(nullptr))
            {
                throw ShipException("Resistance strategy does "
                                    "not match recognized strategies!");
            }
        }
        catch (const std::bad_any_cast&) {
            // Handle error: the std::any_cast didn't work
            mStrategy = nullptr;
        }
    }

    mWaterlineLength =
        Utils::getValueFromMap<units::length::meter_t>(
            parameters,
            "WaterlineLength",
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
            WetSurfaceAreaCalculationMethod::None);

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

    mSpeed =
        Utils::getValueFromMap<units::velocity::meters_per_second_t>(
            parameters,
            "Speed",
            units::velocity::meters_per_second_t(0.0));

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
            WaterPlaneCoefficientMethod::None);

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

    mPropellers =
        Utils::getValueFromMap<QVector<IShipPropeller*>>(
        parameters,
        "propellers",
        QVector<IShipPropeller*>());


    initializeDefaults();
}

Ship::~Ship()
{
    // avoid memory leaks
    if (mStrategy) {
        delete mStrategy;
    }
    for (IShipPropeller* propeller : mPropellers)
    {
        if (propeller)
            delete propeller;
    }
    for (Ship* vessel : mDraggedVessels)
    {
        if (vessel)
            delete vessel;
    }
    if (mBattery)
    {
        delete mBattery;
    }

    if (mTank)
    {
        delete mTank;
    }
}

void Ship::setResistancePropulsionStrategy(
    IShipResistancePropulsionStrategy* newStrategy)
{
    if (mStrategy) {
        delete mStrategy;  // avoid memory leaks
    }
    mStrategy = newStrategy;
}

units::force::newton_t Ship::calculateTotalResistance() const
{
    return mStrategy->getTotalResistance(*this);
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
            (getVolumetricDisplacement().value() /
             getBeam().value()) *
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
    else
    {
        throw ShipException("Wrong method selected!");
    }
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
            throw ShipException("Froud number is outside "
                                "the allowable range for Jensen Method. "
                                "Consider using Ayre Method instead");
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
            throw ShipException("Froud number is outside "
                                "the allowable range for Schneekluth Method");
        }
    }
    else
    {
        throw ShipException("Wrong method selected!");
    }
}

double Ship::calc_MidshiSectionCoef()
{
    return mBlockCoef / mPrismaticCoef;
}

double Ship::calc_PrismaticCoef() const
{
    return getBlockCoef()/ getMidshipSectionCoef();
}

bool Ship::checkSelectedMethodAssumptions(
    IShipResistancePropulsionStrategy *strategy)
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
        throw ShipException("Resistance Strategy is not recognized!");
    }
    return true;
}

units::volume::cubic_meter_t Ship::calc_VolumetricDisplacementByWeight() const
{
    return (getTotalVesselWeight().convert<units::mass::kilogram>() /
            hydrology::WATER_RHO);
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
        throw ShipException("Wrong method selected!");
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
                  * pow((calc_RunLength() /
                         getBeam()).value(), (double)0.34574)
                  * pow((100.0 * getVolumetricDisplacement().value() /
                         pow(getLengthInWaterline().value(), 3)),
                        (double)0.16302)
                  )
        );
}



double Ship::getPrismaticCoef() const
{
    if (std::isnan(mPrismaticCoef))
    {
        return calc_PrismaticCoef();
    }
    return mPrismaticCoef;
}

IShipResistancePropulsionStrategy *Ship::getResistanceStrategy() const
{
    return mStrategy;
}

//units::area::square_meter_t Ship::getLengthwiseProjectionArea() const
//{
//    if (std::isnan(mLengthwiseProjectionArea))
//    {
//        throw ShipException("Lengthwise projection area of the ship "
//                            "is not assigned yet!");
//    }
//    return mLengthwiseProjectionArea;
//}

//void Ship::setLengthwiseProjectionArea(
//    const units::area::square_meter_t &newLengthwiseProjectionArea)
//{
//    mLengthwiseProjectionArea = newLengthwiseProjectionArea;
//}

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

units::length::meter_t Ship::getPropellerDiameter() const
{
    if (std::isnan(mPropellerDiameter.value()))
    {
        throw ShipException("Ship propeller diameter is not set!");
    }
    return mPropellerDiameter;
}

void Ship::setPropellerDiameter(
    const units::length::meter_t &newPropellerDiameter)
{
    mPropellerDiameter = newPropellerDiameter;
}

units::area::square_meter_t Ship::getExpandedBladeArea() const
{
    return mExpandedBladeArea;
}

void Ship::setExpandedBladeArea(
    const units::area::square_meter_t &newExpandedBladeArea)
{
    mExpandedBladeArea = newExpandedBladeArea;
}

units::area::square_meter_t Ship::getPropellerDiskArea() const
{
    return mPropellerDiskArea;
}

void Ship::setPropellerDiskArea(
    const units::area::square_meter_t &newPropellerDiskArea)
{
    mPropellerDiskArea = newPropellerDiskArea;
}

double Ship::getPropellerExpandedAreaRatio() const
{

    return mPropellerExpandedAreaRation;
}

void Ship::setPropellerExpandedAreaRation(
    double newPropellerExpandedAreaRation)
{
    mPropellerExpandedAreaRation = newPropellerExpandedAreaRation;
}

Ship::ScrewVesselType Ship::getScrewVesselType() const
{
    return mScrewVesselType;
}

void Ship::setScrewVesselType(ScrewVesselType newScrewVesselType)
{
    mScrewVesselType = newScrewVesselType;
}

void Ship::addPropeller(IShipPropeller *newPropeller)
{
    mPropellers.push_back(newPropeller);
}

QVector<IShipPropeller *> *Ship::propellers()
{
    return &mPropellers;
}

QVector<Ship *> *Ship::draggedVessels()
{
    return &mDraggedVessels;
}

units::force::newton_t Ship::getTotalThrust() const
{
    units::force::newton_t totalThrust = units::force::newton_t(0.0);

    for (const auto propeller: mPropellers)
    {
        totalThrust += propeller->getThrust() *
                       getHyperbolicThrottleCoef(mSpeed);
    }

    return totalThrust;
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

units::mass::metric_ton_t Ship::getTotalVesselWeight() const
{
    return (mCargoWeight + mVesselWeight + mAddedWeight);
}

units::mass::metric_ton_t Ship::calc_addedWeight() const
{
    return ((units::constants::pi * hydrology::WATER_RHO *
             units::math::pow<2>(getMeanDraft()) *
             getBeam() * getMidshipSectionCoef())/ (double)2.0).
        convert<units::mass::metric_ton>();
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

double Ship::getBlockCoef() const
{
    if (std::isnan(mBlockCoef))
    {
        return calc_BlockCoef(mBlockCoefMethod);
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
        return calc_VolumetricDisplacement();
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
    for (auto area : getAppendagesWettedSurfaces())
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
    if (std::isnan(mSpeed.value()))
    {
        throw ShipException("Ship speed "
                            "is not assigned yet!");
    }
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
                   "Set to default 'Holtrop Method'.";
        mWetSurfaceAreaMethod = WetSurfaceAreaCalculationMethod::Holtrop;
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
        mVolumetricDisplacement = calc_VolumetricDisplacement();
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
    if (!mStrategy)
    {
        mStrategy = new HoltropMethod();
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

    if (std::isnan(mPropellerExpandedAreaRation))
    {
        mPropellerExpandedAreaRation = getExpandedBladeArea().value() /
                                       getPropellerDiskArea().value();
    }

    mCurrentCoordinates = mStartCoordinates;

}

std::vector<std::shared_ptr<Line>> Ship::shipPath()
{
    return mPath;
}

void Ship::setShipPath(std::vector<std::shared_ptr<Line>> &path)
{
    mPath = path;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~~~ Dynamics ~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

units::acceleration::meters_per_second_squared_t
Ship::calc_maxAcceleration() const
{
    return (getTotalThrust() - mStrategy->getTotalResistance(*this)) /
           getTotalVesselWeight().convert<units::mass::kilogram>();
}

units::acceleration::meters_per_second_squared_t
Ship::calc_decelerationAtSpeed(
    const units::velocity::meters_per_second_t customSpeed) const
{
    return ((double)-1.0 * mStrategy->getTotalResistance(*this, customSpeed)) /
           getTotalVesselWeight().convert<units::mass::kilogram>();
}


double Ship::getHyperbolicThrottleCoef(
    const units::velocity::meters_per_second_t &ShipSpeed) const
{
    double dv, um;
    // ratio of current train speed by the max loco speed
    dv = (ShipSpeed / mMaxSpeed).value();
    double lambda = (double)1.0 / (1.0 + exp(-7.82605 * (dv - 0.42606)));

    if (lambda < 0.0){
        return 0.0;
    }
    else if (lambda > 1.0) {
        return 1.0;
    }

    return lambda;

};



units::length::meter_t Ship::getSafeGap(
    const units::length::meter_t initialGap,
    const units::velocity::meters_per_second_t speed,
    const units::velocity::meters_per_second_t freeFlowSpeed,
    const units::time::second_t T_s, bool estimate)
{
    units::length::meter_t gap_lad = units::length::meter_t(0.0);

    units::acceleration::meters_per_second_squared_t d_des;

    if (! estimate )
    {
        d_des = units::math::min(
            units::math::abs(calc_decelerationAtSpeed(speed)),
            mD_des);

        gap_lad =
            initialGap + T_s * speed +
            (units::math::pow<2>(speed) / (2.0 * mD_des));
    }
        else
    {
        d_des = units::math::min(
            units::math::abs(calc_decelerationAtSpeed(freeFlowSpeed)),
            mD_des);
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
        u_hat = max(u_hat, speed - calc_decelerationAtSpeed(speed) * deltaT);
    }
    else if (u_hat > speed && u_hat != freeFlowSpeed) {
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
    return units::math::min((u_hat - speed) / t_s, amax);
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
               calc_decelerationAtSpeed(speed));
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
    units::acceleration::meters_per_second_squared_t d_des =
        units::math::min(
            units::math::abs(calc_decelerationAtSpeed(speed)),
            mD_des);

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
    return min(term, calc_decelerationAtSpeed(speed) * (double)-1.0);
}

units::acceleration::meters_per_second_squared_t
Ship::accelerate(
    const units::length::meter_t &gap,
    const units::length::meter_t &mingap,
    const units::velocity::meters_per_second_t &speed,
    const units::acceleration::meters_per_second_squared_t &acceleration,
    const units::velocity::meters_per_second_t &leaderSpeed,
    const units::velocity::meters_per_second_t &freeFlowSpeed,
    const units::time::second_t &deltaT)
{
    //get the maximum acceleration that the train can go by
    units::acceleration::meters_per_second_squared_t amax =
        calc_maxAcceleration();

    if ((gap > this->getSafeGap(mingap, speed, freeFlowSpeed, mT_s, false)) &&
        (amax.value() > 0))
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
        an1 * (1.0 - gamma) - gamma * an2;
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
    double &alpha)
{
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
    QVector<units::length::meter_t> *gapToNextCriticalPoint,
    QVector<bool> *gapToNextCrticalPointType,
    QVector<units::velocity::meters_per_second_t> *leaderSpeeds)
{
    units::length::meter_t minGap = units::length::meter_t(0.0);

    QVector<units::acceleration::meters_per_second_squared_t>
        allAccelerations;

    for (int i = 0; i < gapToNextCriticalPoint->size(); i++) {
        if (! gapToNextCrticalPointType->at(i)) {
            allAccelerations.push_back(
                this->accelerate(gapToNextCriticalPoint->at(i),
                                 minGap, mSpeed,
                                 mAcceleration,
                                 leaderSpeeds->at(i),
                                 freeFlowSpeed,
                                 timeStep));
        }
    }

    units::acceleration::meters_per_second_squared_t nonSmoothedAcceleration =
        *std::min_element(allAccelerations.begin(), allAccelerations.end());

    if (nonSmoothedAcceleration.value() < 0.0 &&
        mSpeed.value() <= 0.001 &&
        gapToNextCriticalPoint->back().value() > 50.0)
    {
        if (!mShowNoPowerMessage)
        {
            std::stringstream message;
            message << "Ship " << mShipUserID.toStdString()
                    << " Resistance is "
                    << "larger than train tractive force at distance "
                    << mTraveledDistance.value() << "(m)\n";
            emit slowSpeedOrStopped(message.str());
            mShowNoPowerMessage = true;
        }

    }

    double alpha = (double)1.0;
    units::acceleration::meters_per_second_squared_t smoothedAcceleration =
        smoothAccelerate(nonSmoothedAcceleration,
                         mPreviousAcceleration, alpha);
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

void Ship::moveShip(
    units::time::second_t &timeStep,
    units::velocity::meters_per_second_t &freeFlowSpeed,
    QVector<units::length::meter_t> *gapToNextCriticalPoint,
    QVector<bool> *gapToNextCrticalPointType,
    QVector<units::velocity::meters_per_second_t> *leaderSpeeds)
{

    units::acceleration::meters_per_second_squared_t jerkedAcceleration =
        this->getStepAcceleration(timeStep,
                                  freeFlowSpeed,
                                  gapToNextCriticalPoint,
                                  gapToNextCrticalPointType,
                                  leaderSpeeds);

    mAcceleration = jerkedAcceleration;
    mPreviousSpeed = this->mSpeed;
    mSpeed = this->speedUpDown(this->mPreviousSpeed,
                               this->mAcceleration,
                               timeStep, freeFlowSpeed);
    mAcceleration = this->adjustAcceleration(this->mSpeed,
                                             this->mPreviousSpeed,
                                             timeStep);
    checkSuddenAccChange(this->mPreviousAcceleration,
                         this->mAcceleration,
                         timeStep);
    mTraveledDistance += this->mSpeed * timeStep;
}

std::vector<units::length::meter_t> Ship::linksCumLengths() const
{
    return mLinksCumLengths;
}

void Ship::setLinksCumLengths(
    const std::vector<units::length::meter_t> &newLinksCumLengths)
{
    mLinksCumLengths = newLinksCumLengths;
}

bool Ship::loaded() const
{
    return mLoaded;
}

void Ship::setLoaded(bool newLoaded)
{
    mLoaded = newLoaded;
}

bool Ship::outOfEnergy() const
{
    return mOutOfEnergy;
}

void Ship::setOutOfEnergy(bool newOutOfEnergy)
{
    mOutOfEnergy = newOutOfEnergy;
}

bool Ship::reachedDestination() const
{
    return mReachedDestination;
}

void Ship::setReachedDestination(bool newReachedDestination)
{
    mReachedDestination = newReachedDestination;
}

void Ship::immediateStop(units::time::second_t &timestep)
{
    this->mPreviousAcceleration = this->mAcceleration;
    this->mPreviousSpeed = this->mSpeed;
    this->mSpeed =
        units::velocity::meters_per_second_t(0.0);
    this->mAcceleration =
        units::acceleration::meters_per_second_squared_t(0.0);
}

void Ship::kickForwardADistance(
    units::length::meter_t &distance)
{
    this->mPreviousAcceleration =
        units::acceleration::meters_per_second_squared_t(0.0);
    this->mAcceleration =
        units::acceleration::meters_per_second_squared_t(0.0);
    this->mPreviousSpeed = units::velocity::meters_per_second_t(0.0);
    this->mSpeed = units::velocity::meters_per_second_t(0.0);
    this->mTraveledDistance += distance;
}


