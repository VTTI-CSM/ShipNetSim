#include "ishipengine.h"

namespace ShipNetSimCore
{

QVector<IShipEngine::EngineOperationalLoad>
    IShipEngine::mEngineOperationalLoad = {
    IShipEngine::EngineOperationalLoad::Low,
    IShipEngine::EngineOperationalLoad::Economic,
    IShipEngine::EngineOperationalLoad::ReducedMCR,
    IShipEngine::EngineOperationalLoad::MCR
};

QVector<IShipEngine::EngineOperationalTier>
    IShipEngine::mEngineOperationalTier = {
    IShipEngine::EngineOperationalTier::tierII,
    IShipEngine::EngineOperationalTier::TierIII
};

bool
IShipEngine::requestHigherEnginePower()
{
    if (mCurrentOperationalLoad == EngineOperationalLoad::UserDefined) {
        qWarning() << "Engine is operating in user defined engine curve. "
                      "Cannot provide higher operational load!";
        return false;
    }
    QVector<EngineProperties> currentProps =
        (mCurrentOperationalTier == EngineOperationalTier::tierII) ?
            mEngineDefaultTierPropertiesPoints :
            mEngineNOxReducedTierPropertiesPoints;

    std::sort(currentProps.begin(), currentProps.end(),
              [](const EngineProperties& a, const EngineProperties& b) {
                  return EngineProperties::compareByBreakPower(a, b, true);
              });

    // Find the first element that has a breakPower greater than targetState
    for (const auto& prop : currentProps) {
        if (prop.breakPower > mEngineTargetState.breakPower) {
            setEngineTargetState(prop);
            return true;
        }
    }

    return false;
}

bool
IShipEngine::requestLowerEnginePower()
{
    if (mCurrentOperationalLoad == EngineOperationalLoad::UserDefined) {
        qWarning() << "Engine is operating in user defined engine curve. "
                      "Cannot provide higher operational load!";
        return false;
    }

    QVector<EngineProperties> currentProps =
        (mCurrentOperationalTier == EngineOperationalTier::tierII) ?
            mEngineDefaultTierPropertiesPoints :
            mEngineNOxReducedTierPropertiesPoints;

    std::sort(currentProps.begin(), currentProps.end(),
              [](const EngineProperties& a, const EngineProperties& b) {
                  return EngineProperties::compareByBreakPower(a, b, false);
              });

    // Find the first element that has a breakPower greater than targetState
    for (const auto& prop : currentProps) {
        if (prop.breakPower < mEngineTargetState.breakPower) {
            setEngineTargetState(prop);
            return true;
        }
    }

    return false;
}



IShipEngine::EngineProperties
IShipEngine::getEnginePropertiesAtRPM(
    units::angular_velocity::revolutions_per_minute_t rpm)
{
    IShipEngine::EngineProperties ep;
    ep.RPM = rpm;
    ep.breakPower =
        units::power::kilowatt_t(
        Utils::linearInterpolateAtX(mRPMList, mEnginePowerList, rpm.value()));

    ep.efficiency =
        Utils::linearInterpolateAtX(mRPMList,
                                    mEfficiencyList, rpm.value());

    return ep;
}

IShipEngine::EngineProperties
IShipEngine::getEnginePropertiesAtPower(
    units::power::kilowatt_t p,
    IShipEngine::EngineOperationalTier tier)
{
    setEngineOperationalTier(tier);


    IShipEngine::EngineProperties ep;
    ep.breakPower = p;
    ep.RPM =
        units::angular_velocity::revolutions_per_minute_t(
            Utils::linearInterpolateAtX(mEnginePowerList,
                                        mRPMList, p.value()));
    ep.efficiency =
        Utils::linearInterpolateAtX(mEnginePowerList,
                                    mEfficiencyList, p.value());

    return ep;
}

units::torque::newton_meter_t IShipEngine::getEngineTorqueByRPM(
    units::angular_velocity::revolutions_per_minute_t rpm)
{
    auto breakPower =
        units::power::kilowatt_t(
            Utils::linearInterpolateAtX(mRPMList, mEnginePowerList, rpm.value()))
            .convert<units::power::watt>();

    auto rps = rpm.convert<units::angular_velocity::radians_per_second>();

    return units::torque::newton_meter_t(breakPower.value() / rps.value());
}


void IShipEngine::setEngineProperitesSetting(
    QVector<IShipEngine::EngineProperties> engineSettings)
{
    mEnginePowerList.clear();
    mRPMList.clear();
    mEfficiencyList.clear();
    for (auto& le: engineSettings)
    {
        mEnginePowerList.push_back(le.breakPower.value());
        mRPMList.push_back(le.RPM.value());
        mEfficiencyList.push_back(le.efficiency);
    }
}

// -----------------------------------------------------------------------------
// ---------------------------- Getters & Setters ------------------------------
// -----------------------------------------------------------------------------

IShipEngine::EngineOperationalLoad
IShipEngine::getCurrentOperationalLoad()
{
    return mCurrentOperationalLoad;
}

IShipEngine::EngineOperationalTier
IShipEngine::getCurrentOperationalTier()
{
    return mCurrentOperationalTier;
}

QVector<IShipEngine::EngineOperationalLoad>
IShipEngine::getEngineOperationalLoads() {
    return mEngineOperationalLoad;
}

QVector<IShipEngine::EngineOperationalTier>
IShipEngine::getEngineOperationalTiers() {
    return mEngineOperationalTier;
}

IShipEngine::EngineProperties IShipEngine::getEngineTargetState() {
    return mEngineTargetState;
}

void IShipEngine::setEnginePreviousState(EngineProperties newState)
{
    mEnginePreviousState = newState;
}

IShipEngine::EngineProperties IShipEngine::getEnginePreviousState()
{
    return mEnginePreviousState;
}

IShipEngine::EngineProperties IShipEngine::getEngineCurrentState() {
    return mEngineCurrentState;
}

bool IShipEngine::isRPMWithinOperationalRange(
    units::angular_velocity::revolutions_per_minute_t rpm)
{
    return (rpm.value() >= 0.0 && rpm.value() <= mRPMList.back());
}

bool IShipEngine::isPowerWithinOperationalRange(units::power::kilowatt_t power)
{
    return (power.value() >= 0.0 &&
            power.value() <= mEnginePowerList.back());
}

// -----------------------------------------------------------------------------

void IShipEngine::setEngineTierIICurve(QVector<EngineProperties> newCurve)
{
    std::sort(newCurve.begin(), newCurve.end(),
              [](const EngineProperties& a, const EngineProperties& b) {
                  return EngineProperties::compareByBreakPower(a, b, true);
              });
    mUserEngineCurveInDefaultTier = newCurve;
}

void IShipEngine::setEngineTierIIICurve(QVector<EngineProperties> newCurve)
{
    std::sort(newCurve.begin(), newCurve.end(),
              [](const EngineProperties& a, const EngineProperties& b) {
                  return EngineProperties::compareByBreakPower(a, b, true);
              });
    mUserEngineCurveInNOxReducedTier = newCurve;
}

void IShipEngine::setEngineDefaultTargetState(EngineProperties newState)
{
    mEngineDefaultTargetState = newState;
}

IShipEngine::EngineProperties IShipEngine::getEngineDefaultTargetState()
{
    return mEngineDefaultTargetState;
}

void IShipEngine::setEngineTargetState(EngineProperties newState) {
    if (mEngineTargetState != newState) {
        mEngineTargetState = newState;
        emit engineTargetStateChanged(mEngineTargetState);
    }

}

void IShipEngine::setEngineCurrentState(EngineProperties newState) {
    if (mEngineCurrentState != newState) {
        mEngineCurrentState = newState;
        emit engineCurrentStateChanged(newState);
    }

}

void IShipEngine::setEngineOperationalLoad(
    IShipEngine::EngineOperationalLoad targetLoad)
{
    if (targetLoad != mCurrentOperationalLoad) {
        mCurrentOperationalLoad = targetLoad;
        emit operationalLoadChanged(targetLoad);
    }
}

bool IShipEngine::setEngineOperationalTier(
    IShipEngine::EngineOperationalTier targetTier)
{
    if (mCurrentOperationalTier != targetTier) {
        mCurrentOperationalTier = targetTier;
        emit engineOperationalTierChanged(targetTier);
        return true;
    }
    return false;
}

}
