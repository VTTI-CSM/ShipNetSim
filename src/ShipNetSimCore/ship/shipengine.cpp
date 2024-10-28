#include "shipengine.h"
#include "ishipcalmresistancestrategy.h"
#include "ship.h"
#include <QFile>
#include <QStringList>
#include <QTextStream>
#include <QDebug>
#include <QMap>
#include "../utils/utils.h"

namespace ShipNetSimCore
{


ShipEngine::ShipEngine()
{
    mHost = nullptr;
}

ShipEngine::~ShipEngine()
{
}

void ShipEngine::moveObjectToThread(QThread *thread)
{
    this->moveToThread(thread);
}

void ShipEngine::initialize(Ship *host, QVector<IEnergySource*> energySources,
                const QMap<QString, std::any> &parameters)
{

    mHost = host;

    mEnergySources = energySources;
    mCurrentEnergySource = energySources[0];

    connect(this, &ShipEngine::operationalLoadChanged,
            this, &ShipEngine::handleOperationalDetailsChange,
            Qt::QueuedConnection);

    connect(this, &ShipEngine::engineTargetStateChanged,
            this, &ShipEngine::handleTargetStateChange,
            Qt::QueuedConnection);

    connect(this, &ShipEngine::engineOperationalTierChanged,
            this, &ShipEngine::handleOperationalDetailsChange,
            Qt::QueuedConnection);

    // set the engines parameters
    setParameters(parameters);

    counter++; // increment the counter

}

void ShipEngine::handleTargetStateChange() {

    if (getCurrentOperationalLoad() ==
        IShipEngine::EngineOperationalLoad::UserDefined)
    {
        if (getCurrentOperationalTier() ==
            IShipEngine::EngineOperationalTier::tierII)
        {
            mEngineCurve = mUserEngineCurveInDefaultTier;
        }
        else
        {
            mEngineCurve = mUserEngineCurveInNOxReducedTier;
        }
        setEngineProperitesSetting(mEngineCurve);

    }
    else
    {
        mEngineCurve = estimateEnginePowerCurve();
        setEngineProperitesSetting(mEngineCurve);
    }
}

void ShipEngine::handleOperationalDetailsChange()
{
    // change target state

    if (getCurrentOperationalLoad() == EngineOperationalLoad::Default)
    {
        setEngineTargetState(getEngineDefaultTargetState());
    }
    else if (getCurrentOperationalLoad() ==
             EngineOperationalLoad::UserDefined)
    {
        if (getCurrentOperationalTier() ==
            IShipEngine::EngineOperationalTier::tierII)
        {
            setEngineTargetState(mUserEngineCurveInDefaultTier.back());
        }
        else
        {
            setEngineTargetState(mUserEngineCurveInNOxReducedTier.back());
        }
    }
    else
    {
        auto props = (getCurrentOperationalTier() ==
                      EngineOperationalTier::tierII) ?
                         mEngineDefaultTierPropertiesPoints :
                         mEngineNOxReducedTierPropertiesPoints;
        int index = static_cast<int>(getCurrentOperationalLoad()) + 1;
        setEngineTargetState(props.at(index));
    }
}

void ShipEngine::setParameters(const QMap<QString, std::any> &parameters)
{
    mId =
        Utils::getValueFromMap<unsigned int>(parameters,
                                             "EngineID",
                                             counter);


    mEngineDefaultTierPropertiesPoints =
        Utils::getValueFromMap<QVector<EngineProperties>>
        (parameters, "EngineTierIIPropertiesPoints", {});
    if (mEngineDefaultTierPropertiesPoints.size() != 4)
    {
        qFatal("Engine safe operational zone properties is not defined! "
               "Engine Properties (BrakePower, RPM, Efficiency) "
               "must be defined at the"
               " corners of the engine layout!");
    }

    // set initial value
    setEngineCurrentState(
        {units::power::kilowatt_t(0.0),
         units::angular_velocity::revolutions_per_minute_t(0.0f),
         0.001f});

    mEngineNOxReducedTierPropertiesPoints =
        Utils::getValueFromMap<QVector<EngineProperties>>
        (parameters, "EngineTierIIIPropertiesPoints", {});

    setEngineTierIICurve(Utils::getValueFromMap<QVector<EngineProperties>>
                         (parameters, "EngineTierIICurve", {}));
    setEngineTierIIICurve(Utils::getValueFromMap<QVector<EngineProperties>>
                          (parameters, "EngineTierIIICurve", {}));

    // sort the engine properties (L1, L2, L3, L4 points)
    std::sort(mEngineDefaultTierPropertiesPoints.begin(),
              mEngineDefaultTierPropertiesPoints.end(),
              [](const EngineProperties& a, const EngineProperties& b) {
                  return EngineProperties::compareByBreakPower(a, b, true);
              });

    if (!mEngineNOxReducedTierPropertiesPoints.empty())
    {
        std::sort(mEngineNOxReducedTierPropertiesPoints.begin(),
                  mEngineNOxReducedTierPropertiesPoints.end(),
                  [](const EngineProperties& a, const EngineProperties& b) {
                      return EngineProperties::compareByBreakPower(a, b, true);
                  });
    }


    if (!mUserEngineCurveInDefaultTier.empty()) {

        setEngineOperationalLoad(EngineOperationalLoad::UserDefined);

        std::sort(mUserEngineCurveInDefaultTier.begin(),
                  mUserEngineCurveInDefaultTier.end(),
                  [](const EngineProperties& a, const EngineProperties& b) {
                      return EngineProperties::compareByBreakPower(a, b, true);
                  });

        setEngineOperationalTier(EngineOperationalTier::tierII);
        setEngineTargetState(mUserEngineCurveInDefaultTier.back());
    }
    else if (mUserEngineCurveInDefaultTier.empty() &&
               !mUserEngineCurveInNOxReducedTier.empty()) {
        setEngineOperationalLoad(EngineOperationalLoad::UserDefined);

        std::sort(mUserEngineCurveInNOxReducedTier.begin(),
                  mUserEngineCurveInNOxReducedTier.end(),
                  [](const EngineProperties& a, const EngineProperties& b) {
                      return EngineProperties::compareByBreakPower(a, b, true);
                  });

        setEngineOperationalTier(EngineOperationalTier::TierIII);
        setEngineTargetState(mUserEngineCurveInNOxReducedTier.back());
    }
    else {
        setEngineOperationalLoad(EngineOperationalLoad::Default);
        setEngineOperationalTier(EngineOperationalTier::tierII);
    }


}

void ShipEngine::setEngineMaxPowerLoad(double targetRatio)
{
    mMaxPowerRatio = targetRatio;

    // get the power
    updateEngineOperationalState();
}

double ShipEngine::getEngineMaxPowerRatio()
{
    return mMaxPowerRatio;
}

bool
ShipEngine::selectCurrentEnergySourceByFuelType(ShipFuel::FuelType fuelType)
{
    for (auto& ES : mEnergySources)
    {
        if (ES->getFuelType() == fuelType)
        {
            mCurrentEnergySource = ES;
            return true;
        }
    }
    return false;
}

EnergyConsumptionData
ShipEngine::consumeUsedEnergy(units::time::second_t timeStep)
{
    // the brake power should be increased to consider the losses due to the
    // efficiency of the engine.
    // this value should be calculated by the SOF as reported by
    // the manufacturer
    units::energy::kilowatt_hour_t energy =
        (getEngineCurrentState().breakPower /
         getEngineCurrentState().efficiency) *
        timeStep.convert<units::time::hour>();

    // consume the energy and return not consumed energy
    auto result = mCurrentEnergySource->consume(timeStep, energy);
    mCumFuelConsumption[result.fuelConsumed.first] +=
        result.fuelConsumed.second;
    // check there is enough energy
    if (!result.isEnergySupplied)
    {
        // no power is provided
        mIsWorking = false;
    }

    mCumEnergyConsumption += result.energyConsumed;

    return result;
}

units::energy::kilowatt_hour_t ShipEngine::getCumEnergyConsumption() {
    return mCumEnergyConsumption;
}


double ShipEngine::getEfficiency()
{
    return getEngineCurrentState().efficiency;
}


void ShipEngine::updateEngineOperationalState()
{
    // update the previous power
    setEnginePreviousState(getEngineCurrentState());

    // ensure the RPM is up to date
    if (!mIsWorking) {
        IShipEngine::EngineProperties ep;
        ep.RPM =
            units::angular_velocity::revolutions_per_minute_t(0.0);
        ep.breakPower = units::power::kilowatt_t(0.0);
        ep.efficiency = 0.0f;
        setEngineCurrentState(ep);
        return;
    }

    bool expHgRes = mHost->isExperiencingHighResistance();
    double lambda = getHyperbolicThrottleCoef(mHost->getSpeed(), expHgRes);

    // Calculating power without considering efficiency
    // as the efficiency is only for calculating fuel consumption
    units::power::kilowatt_t mRawPower = lambda * getEngineTargetState().breakPower;
    if (mRawPower > getEngineTargetState().breakPower)
    {
        mRawPower = getEngineTargetState().breakPower;
    }

    // Getting efficiency based on raw power without
    // checks or using stored values
    auto r = getEnginePropertiesAtPower(mRawPower, getCurrentOperationalTier());

    setEngineCurrentState({mRawPower, r.RPM,
                           (r.efficiency <= 0.0) ? 0.0001 : r.efficiency});

}

units::power::kilowatt_t ShipEngine::getBrakePower()
{
    return getEngineCurrentState().breakPower;
}

units::torque::newton_meter_t ShipEngine::getBrakeTorque()
{
    units::torque::newton_meter_t result =
        units::torque::newton_meter_t(
            (getEngineCurrentState().breakPower
             .convert<units::power::watt>().value()) /
            getEngineCurrentState().RPM.
            convert<units::angular_velocity::radians_per_second>().value());

    return result;
}

units::angular_velocity::revolutions_per_minute_t ShipEngine::getRPM()
{
    return getEngineCurrentState().RPM;
}

QPair<units::angular_velocity::revolutions_per_minute_t,
      units::angular_velocity::revolutions_per_minute_t>
ShipEngine::getRPMRange()
{
    return {mEngineCurve.front().RPM, mEngineCurve.back().RPM};
    // return {mEngineDefaultTierPropertiesPoints.front().RPM,
    //         mEngineDefaultTierPropertiesPoints.back().RPM};
}

double ShipEngine::getHyperbolicThrottleCoef(
    units::velocity::meters_per_second_t ShipSpeed,
    bool isExperiencingHighResistance)
{
    if (isExperiencingHighResistance) {
        return mNormalLambda;
    }

    double dv;
    // ratio of current speed by the max speed
    dv = (ShipSpeed / mHost->getMaxSpeed()).value();
    double lambda = (double)1.0 / (1.0 + exp(-7.82605 * (dv - 0.42606)));

    if (lambda <= 0.2)
    {
        lambda = 0.2;
    }
    else if (lambda > 1.0)
    {
        lambda = 1.0;
    }

    if (lambda > mMaxPowerRatio)
    {
        lambda = mMaxPowerRatio;
    }

    mNormalLambda = lambda;
    return lambda;

}

int ShipEngine::getEngineID()
{
    return mId;
}

void ShipEngine::setEngineTargetState(EngineProperties newState)
{

    // set the new state
    IShipEngine::setEngineTargetState(newState);

    // qDebug() << "----> Engine Target State changed!!!!";
    handleTargetStateChange();

    // get the power
    updateEngineOperationalState();

}

units::power::kilowatt_t ShipEngine::getPreviousBrakePower()
{
    return getEnginePreviousState().breakPower;
}

bool ShipEngine::isEngineWorking()
{
    return mIsWorking;
}

QVector<IShipEngine::EngineProperties> ShipEngine::estimateEnginePowerCurve() {


    double P_M = getEngineTargetState().breakPower.value();
    double omega_M = getEngineTargetState().RPM.
                     convert<units::angular_velocity::radians_per_second>().
                     value();
    double P1 = 0.87 * P_M / omega_M;
    double P2 = 1.13 * P_M / std::powf(omega_M, 2);
    double P3 = -P_M / std::powf(omega_M, 3);

    QVector<EngineProperties> engineProps;
    std::vector<double> omegas = Utils::linspace_step(0.0, omega_M, 10);


    for (const auto &omega : omegas) {
        EngineProperties ep;
        /// This estimation is from:
        /// Yehia, W., & Moustafa, M. M. (2014, October).
        /// Practical considerations for marine propeller sizing.
        /// In International Marine and Offshore Engineering Conference,
        /// Portugal.
        ep.breakPower =
            units::power::kilowatt_t(P1 * omega +
                                     P2 * std::powf(omega, 2) +
                                     P3 * std::powf(omega, 3));

        ep.RPM = units::angular_velocity::radians_per_second_t(omega).
                 convert<units::angular_velocity::revolutions_per_minute>();

        // Estimation using the simple equation proposed by
        // Institute of Marine Engineering, Science and Technology (IMarEST)
        ep.efficiency = (ep.breakPower / P_M).value() *
                        getEngineTargetState().efficiency;

        engineProps.push_back(ep);
    }

    return engineProps;
}

void ShipEngine::turnOffEngine() {
    mIsWorking = false;
}
void ShipEngine::turnOnEngine() {
    mIsWorking = true;
}

};
