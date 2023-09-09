#include "ship.h"
#include <cmath>
#include "hydrology.h"
#include "ishipresistancepropulsionstrategy.h"
#include "holtropmethod.h"
#include <QDebug>
#include "../../third_party/units/units.h"

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
        getValueFromMap<units::length::meter_t>(
            parameters,
            "WaterlineLength",
            units::length::meter_t(std::nan("uninitialized")));

    mBeam =
        getValueFromMap<units::length::meter_t>(
            parameters,
            "Beam",
            units::length::meter_t(std::nan("uninitialized")));

    mMeanDraft =
        getValueFromMap<units::length::meter_t>(
            parameters,
            "MeanDraft",
            units::length::meter_t(std::nan("uninitialized")));

    mDraftAtForward =
        getValueFromMap<units::length::meter_t>(
            parameters,
            "DraftAtForward",
            units::length::meter_t(std::nan("uninitialized")));

    mDraftAtAft =
        getValueFromMap<units::length::meter_t>(
            parameters,
            "DraftAtAft",
            units::length::meter_t(std::nan("uninitialized")));

    mVolumetricDisplacement =
        getValueFromMap<units::volume::cubic_meter_t>(
            parameters,
            "VolumetricDisplacement",
            units::volume::cubic_meter_t(std::nan("uninitialized")));

    mWettedHullSurface =
        getValueFromMap<units::area::square_meter_t>(
            parameters,
            "WettedHullSurface",
            units::area::square_meter_t(std::nan("uninitialized")));

    mWetSurfaceAreaMethod =
        getValueFromMap<WetSurfaceAreaCalculationMethod>(
            parameters,
            "WetSurfaceAreaMethod",
            WetSurfaceAreaCalculationMethod::None);

    mBulbousBowTransverseAreaCenterHeight =
        getValueFromMap<units::length::meter_t>(
            parameters,
            "BulbousBowTransverseAreaCenterHeight",
            units::length::meter_t(std::nan("uninitialized")));

    mAppendagesWettedSurfaces =
        getValueFromMap<QMap<ShipAppendage, units::area::square_meter_t>>(
            parameters,
            "AppendagesWettedSurfaces",
            QMap<ShipAppendage, units::area::square_meter_t>());

    mBulbousBowTransverseArea =
        getValueFromMap<units::area::square_meter_t>(
            parameters,
            "BulbousBowTransverseArea",
            units::area::square_meter_t(std::nan("uninitialized")));

    mImmersedTransomArea =
        getValueFromMap<units::area::square_meter_t>(
            parameters,
            "ImmersedTransomArea",
            units::area::square_meter_t(std::nan("uninitialized")));

    mHalfWaterlineEntranceAngle =
        getValueFromMap<units::angle::degree_t>(
            parameters,
            "HalfWaterlineEntranceAngle",
            units::angle::degree_t(std::nan("uninitialized")));

    mSpeed =
        getValueFromMap<units::velocity::meters_per_second_t>(
            parameters,
            "Speed",
            units::velocity::meters_per_second_t(0.0));

    mSurfaceRoughness =
        getValueFromMap<units::length::nanometer_t>(
            parameters,
            "SurfaceRoughness",
            units::length::nanometer_t(std::nan("uninitialized")));

    mRunLength =
        getValueFromMap<units::length::meter_t>(
            parameters,
            "RunLength",
            units::length::meter_t(std::nan("uninitialized")));

    mLongitudinalBuoyancyCenter =
        getValueFromMap<double>(
            parameters,
            "LongitudinalBuoyancyCenter",
            std::nan("uninitialized"));

    mSternShapeParam =
        getValueFromMap<CStern>(
            parameters,
            "SternShapeParam",
            CStern::None);

    mMidshipSectionCoef =
        getValueFromMap<double>(
            parameters,
            "MidshipSectionCoef",
            std::nan("uninitialized"));

    mWaterplaneAreaCoef =
        getValueFromMap<double>(
            parameters,
            "WaterplaneAreaCoef",
            std::nan("uninitialized"));

    mWaterplaneCoefMethod =
        getValueFromMap<WaterPlaneCoefficientMethod>(
            parameters,
            "WaterplaneCoefMethod",
            WaterPlaneCoefficientMethod::None);

    mPrismaticCoef =
        getValueFromMap<double>(
            parameters,
            "PrismaticCoef",
            std::nan("uninitialized"));

    mBlockCoef =
        getValueFromMap<double>(
            parameters,
            "BlockCoef",
            std::nan("uninitialized"));

    mBlockCoefMethod =
        getValueFromMap<BlockCoefficientMethod>(
            parameters,
            "BlockCoefMethod",
            BlockCoefficientMethod::None);

    mBreakPower = getValueFromMap<units::power::kilowatt_t>(
        parameters, "gearEfficiency",
        units::power::kilowatt_t(std::nan("uninitialized")));

    mGearEfficiency = getValueFromMap<double>(
        parameters, "gearEfficiency", std::nan("uninitialized"));
    mShaftEfficiency = getValueFromMap<double>(
        parameters, "shaftEfficiency", std::nan("uninitialized"));
    mPropellerEfficiency = getValueFromMap<double>(
        parameters, "propellerEfficiency", std::nan("uninitialized"));
    mHullEfficiency = getValueFromMap<double>(
        parameters, "hullEfficiency", std::nan("uninitialized"));
    mPropulsiveEfficiency = getValueFromMap<double>(
        parameters, "propulsiveEfficiency", std::nan("uninitialized"));

    initializeDefaults();
}

Ship::~Ship()
{
    if (mStrategy) {
        delete mStrategy;  // avoid memory leaks
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

units::power::kilowatt_t Ship::getBreakPower() const
{
    return mBreakPower;
}

void Ship::setBreakPower(const units::power::kilowatt_t newPower)
{
    mBreakPower = newPower;
}

units::power::kilowatt_t Ship::getShaftPower() const
{
    return getBreakPower() * mGearEfficiency;
}

units::power::kilowatt_t Ship::getDeliveredPower() const
{
    return getShaftPower() * mShaftEfficiency;
}

units::power::kilowatt_t Ship::getThrustPower() const
{
    return getDeliveredPower() * mPropellerEfficiency;
}

units::power::kilowatt_t Ship::getEffectivePower() const
{
    return getThrustPower() * mHullEfficiency;
}

units::force::newton_t Ship::getThrust() const
{
    return units::math::min(
        (getEffectivePower() /
         units::math::max(
             mStrategy->calc_SpeedOfAdvance(*this),
             units::velocity::meters_per_second_t(0.0001))).
        convert<units::force::newton>(),

        units::force::kilonewton_t(
            (double)2.0 * hydrology::WATER_RHO.value() *
            getPropellerDiskArea().value() *
            pow(getDeliveredPower().convert<units::power::watts>().value(),
                (double)2.0) / (double)1000.0
            ).convert<units::force::newton>());
}

double Ship::getRPM() const
{
    return pow(pow(getShaftPower().
                   convert<units::power::horsepower>().value(),
                   (double)0.2) * ((double)632.7/
                      getPropellerDiameter().
                      convert<units::length::inch>().value()),
               ((double)1 / (double)0.6));
}

units::torque::newton_meter_t Ship::getTorque() const
{
    return units::torque::newton_meter_t(
        getDeliveredPower().convert<units::power::watt>().value() /
            ((double)2.0 * units::constants::pi.value() * getRPM()));

//    return units::torque::foot_pound_t(
//               getEffectivePower().convert<units::power::horsepower>().value() /
//                                       (getRPM() / (double)5252.0)).
//        convert<units::torque::newton_meter>();
}

double Ship::getThrustCoefficient() const
{
    return (getThrust().value() /
            (hydrology::WATER_RHO.value() *
             pow(getRPM(), (double)2.0) *
             pow(getPropellerDiameter().value(),(double)4.0)));
}

double Ship::getTorqueCoefficient() const
{
    return (getTorque().value() /
            (hydrology::WATER_RHO.value() *
             pow(getRPM(), (double)2.0) *
             pow(getPropellerDiameter().value(),(double)5.0)));
}

double Ship::getAdvancedRatio() const
{
    return (mStrategy->calc_SpeedOfAdvance(*this).value() /
            (getRPM() * getPropellerDiameter().value()));
}

//double Ship::calc_propellerEfficiency()
//{
//    return ((getThrustCoefficient() / getTorqueCoefficient()) *
//            (getAdvancedRatio() / ((double)2.0 *
//                                   units::constants::pi.value())));
//}

double Ship::getPropellerEfficiency() const
{
    return mPropellerEfficiency;
}

void Ship::setPropellerEfficiency(double newPropellerEfficiency)
{
    mPropellerEfficiency = newPropellerEfficiency;
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

units::acceleration::meters_per_second_squared_t
Ship::calc_maxAcceleration() const
{
    return (getThrust() - mStrategy->getTotalResistance(*this)) /
           getTotalVesselWeight().convert<units::mass::kilogram>();
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

    if (std::isnan(mGearEfficiency))
    {
        mGearEfficiency = (double)0.99;
    }

    if (std::isnan(mShaftEfficiency))
    {
        mShaftEfficiency = (double)0.00;
    }

    if (std::isnan(mPropellerEfficiency))
    {
        mPropellerEfficiency = (double)0.75;
    }

    if (std::isnan(mHullEfficiency))
    {
        mHullEfficiency = 0.99;
    }

    if (std::isnan(mPropulsiveEfficiency))
    {
        mPropulsiveEfficiency = mGearEfficiency * mShaftEfficiency *
                                mPropellerEfficiency * mHullEfficiency;
    }

//    mPropellerDeliveredPower = mPropellerBreakPower * mPropulsiveEfficiency;
}


units::force::newton_t Ship::calc_Torque()
{
    return units::force::kilonewton_t(
               cbrt((double)2.0 *
                    hydrology::WATER_RHO.value() *
                    getPropellerDiskArea().value() *
                    pow(getDeliveredPower().
                        convert<units::power::watts>().value(), (double)2.0)
                    )/(double)1000.0).convert<units::force::newton>();
}

