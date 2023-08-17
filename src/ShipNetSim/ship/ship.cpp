#include "ship.h"
#include <cmath>
#include "hydrology.h"
#include "ishipresistancestrategy.h"
#include <QDebug>

Ship::Ship(units::length::meter_t lengthInWaterline,
           units::length::meter_t moldedBeam,
           units::length::meter_t moldedMeanDraft,
           units::length::meter_t moldedDraftAtAft,
           units::length::meter_t moldedDraftAtForward,
           units::volume::cubic_meter_t volumetricDisplacement,
           units::area::square_meter_t wettedSurface,
           IShipResistanceStrategy* initialStrategy) :
    mResistanceStrategy(initialStrategy)
{

}

Ship::~Ship()
{
    if (mResistanceStrategy) {
        delete mResistanceStrategy;  // avoid memory leaks
    }
}

void Ship::setResistanceStrategy(IShipResistanceStrategy* newStrategy)
{
    if (mResistanceStrategy) {
        delete mResistanceStrategy;  // avoid memory leaks
    }
    mResistanceStrategy = newStrategy;
}

units::force::kilonewton_t Ship::calculateTotalResistance() const
{
    return mResistanceStrategy->getTotalResistance(*this);
}

units::area::square_meter_t Ship::calc_WetSurfaceAreaToHoltrop()
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

units::area::square_meter_t Ship::calc_WetSurfaceAreaToSchenzle()
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
    WetSurfaceAreaCalculationMethod method)
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



double Ship::calc_BlockCoef(units::velocity::meters_per_second_t &speed,
                     BlockCoefficientMethod method)
{
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
                                "the allowable range for Jensen Method");
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

double Ship::calc_PrismaticCoef()
{
    return getBlockCoef()/ getMidshipSectionCoef();
}

bool Ship::checkSelectedMethodAssumptions(IShipResistanceStrategy *strategy)
{
    if (strategy->getMethodName() ==
        "Holtrop and Mennen Resistance Prediction Method")
    {
        bool warning = false;
        double fn = hydrology::F_n(getSpeed(), getLengthInWaterline());
        if (fn > 0.45)
        {
            qWarning() << "Speed is outside the method range!"
                       << " Calculations maynot be accurate";
            warning = true;
        }
        if (getPrismaticCoef() > 0.85 || getPrismaticCoef() < 0.55)
        {
            qWarning() << "Prismatic Coefficient is outside the method range!"
                       << " Calculations maynot be accurate";
            warning = true;
        }
        double L_B = (getLengthInWaterline()/getBeam()).value();
        if (L_B > 9.5 || L_B < 3.9)
        {
            qWarning() << "Length/Beam is outside the method range!"
                       << " Calculations maynot be accurate";
        }

        return !warning;
    }
    return true;
}


units::volume::cubic_meter_t Ship::calc_VolumetricDisplacement()
{
    return (getLengthInWaterline() *
            getBeam() *
            getMeanDraft()) *
           getBlockCoef();
}

double Ship::calc_WaterplaneAreaCoef(WaterPlaneCoefficientMethod method)
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

units::length::meter_t Ship::calc_RunLength()
{
    return units::length::meter_t(getLengthInWaterline() * (
                                      1.0 - getPrismaticCoef() +
                                      0.06 * getPrismaticCoef() *
                                          getLongitudinalBuoyancyCenter() /
                                          (4.0 * getPrismaticCoef() - 1.0)
                                      ));
}

units::angle::degree_t Ship::calc_i_E()
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
    return mPrismaticCoef;
}

IShipResistanceStrategy *Ship::getResistanceStrategy() const
{
    return mResistanceStrategy;
}

units::area::square_meter_t Ship::getLengthwiseProjectionArea() const
{
    return mLengthwiseProjectionArea;
}

void Ship::setLengthwiseProjectionArea(
    const units::area::square_meter_t &newLengthwiseProjectionArea)
{
    mLengthwiseProjectionArea = newLengthwiseProjectionArea;
}

units::length::nanometer_t Ship::getSurfaceRoughness() const
{
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
    return mRunLength;
}

void Ship::setRunLength(const units::length::meter_t &newRunLength)
{
    mRunLength = newRunLength;
}

units::angle::degree_t Ship::getHalfWaterlineEntranceAngle() const
{
    return mHalfWaterlineEntranceAngle;
}

void Ship::setHalfWaterlineEntranceAngle(
    const units::angle::degree_t &newHalfWaterlineEntranceAnlge)
{
    mHalfWaterlineEntranceAngle = newHalfWaterlineEntranceAnlge;
}

double Ship::getBlockCoef() const
{
    return mBlockCoef;
}

void Ship::setWaterplaneAreaCoef(double newC_WP)
{
    mWaterplaneAreaCoef = newC_WP;
}

units::volume::cubic_meter_t Ship::getVolumetricDisplacement() const
{
    return mVolumetricDisplacement;
}

void Ship::setVolumetricDisplacement(
    const units::volume::cubic_meter_t &newNab)
{
    mVolumetricDisplacement = newNab;
}

double Ship::getWaterplaneAreaCoef() const
{
    return mWaterplaneAreaCoef;
}


units::length::meter_t Ship::getLengthInWaterline() const
{
    return mWaterlineLength;
}

void Ship::setLengthInWaterline(const units::length::meter_t &newL)
{
    mWaterlineLength = newL;
}

units::length::meter_t Ship::getBeam() const
{
    return mBeam;
}

void Ship::setBeam(const units::length::meter_t &newB)
{
    mBeam = newB;
}

units::length::meter_t Ship::getMeanDraft() const
{
    return mMeanDraft;
}

void Ship::setMeanDraft(const units::length::meter_t &newT)
{
    mMeanDraft = newT;
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
    return mDraftAtForward;
}

void Ship::setDraftAtForward(const units::length::meter_t &newT_F)
{
    mDraftAtForward = newT_F;
}

units::length::meter_t Ship::getDraftAtAft() const
{
    return mDraftAtAft;
}

void Ship::setDraftAtAft(const units::length::meter_t &newT_A)
{
    mDraftAtAft = newT_A;
}

units::area::square_meter_t Ship::getWettedHullSurface() const
{
    return mWettedHullSurface;
}

void Ship::setWettedHullSurface(const units::area::square_meter_t &newS)
{
    mWettedHullSurface = newS;
}

units::length::meter_t Ship::getBulbousBowTransverseAreaCenterHeight() const
{
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
    return mBulbousBowTransverseArea;
}

void Ship::setBulbousBowTransverseArea(
    const units::area::square_meter_t &newA_BT)
{
    mBulbousBowTransverseArea = newA_BT;
}

units::velocity::knot_t Ship::getSpeed() const
{
    return mSpeed.convert<velocity::knot>();
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
    return mLongitudinalBuoyancyCenter;
}

void Ship::setLongitudinalBuoyancyCenter(double newLcb)
{
    mLongitudinalBuoyancyCenter = newLcb;
}

double Ship::getMidshipSectionCoef() const
{
    return mMidshipSectionCoef;
}

void Ship::setMidshipSectionCoef(double newC_M)
{
    mMidshipSectionCoef = newC_M;
}

units::area::square_meter_t Ship::getImmersedTransomArea() const
{
    return mImmersedTransomArea;
}

void Ship::setImmersedTransomArea(const units::area::square_meter_t &newA_T)
{
    mImmersedTransomArea = newA_T;
}

void Ship::setBlockCoef(double newC_B)
{
    mBlockCoef = newC_B;
}

void Ship::setPrismaticCoef(double newC_P)
{
    mPrismaticCoef = newC_P;
}

