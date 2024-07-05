#include "ishipengine.h"

namespace ShipNetSimCore
{

QVector<IShipEngine::EngineOperationalLoad> IShipEngine::mEngineOperationalLoad = {
    IShipEngine::EngineOperationalLoad::Low,
    IShipEngine::EngineOperationalLoad::Economic,
    IShipEngine::EngineOperationalLoad::ReducedMCR,
    IShipEngine::EngineOperationalLoad::MCR
};

QVector<IShipEngine::EngineOperationalTier> IShipEngine::mEngineOperationalTier = {
    IShipEngine::EngineOperationalTier::tierII,
    IShipEngine::EngineOperationalTier::TierIII
};

bool
IShipEngine::requestHigherEnginePower()
{
    int newIndex = static_cast<int>(mCurrentOperationalLoad) + 1;
    if (mCurrentOperationalLoad <
            IShipEngine::EngineOperationalLoad::MCR &&
         newIndex < mEngineOperationalPowerSettings.size())
    {
        mCurrentOperationalLoad =
            static_cast<EngineOperationalLoad>(newIndex);

        mCurrentOperationalPowerSetting =
            mEngineOperationalPowerSettings[newIndex];

        return true;
    }
    return false;
}

bool
IShipEngine::requestLowerEnginePower()
{
    int newIndex = static_cast<int>(mCurrentOperationalLoad) - 1;
    if (mCurrentOperationalLoad >
            IShipEngine::EngineOperationalLoad::Low &&
        newIndex < mEngineOperationalPowerSettings.size())
    {
        mCurrentOperationalLoad =
            static_cast<EngineOperationalLoad>(newIndex);

        mCurrentOperationalPowerSetting =
            mEngineOperationalPowerSettings[newIndex];

        return true;
    }
    return false;
}

void IShipEngine::setCurrentEngineProperties(
    IShipEngine::EngineOperationalLoad targetLoad)
{
    mCurrentOperationalLoad = targetLoad;
    mCurrentOperationalPowerSetting =
        mEngineOperationalPowerSettings.at(static_cast<int>(targetLoad));
}

IShipEngine::EngineProperties
IShipEngine::getEnginePropertiesAtPower(units::power::kilowatt_t p,
                                        IShipEngine::EngineOperationalTier tier)
{
    if (mEnginePowerList.empty() ||
        mRPMList.empty() ||
        mEfficiencyList.empty())
    {
        setEngineProperitesSetting(mEngineDefaultTierPropertiesPoints);
    }

    if (mCurrentOperationalTier != tier)
    {
        changeTier(tier);
    }

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

IShipEngine::EngineOperationalLoad
IShipEngine::getCurrentOperationalLoad()
{
    return mCurrentOperationalLoad;
}

void IShipEngine::setEngineProperitesSetting(
    QVector<IShipEngine::EngineProperties> engineSettings)
{
    for (auto& le: engineSettings)
    {
        mEnginePowerList.push_back(le.breakPower.value());
        mRPMList.push_back(le.RPM.value());
        mEfficiencyList.push_back(le.efficiency);
    }
}

bool IShipEngine::changeTier(IShipEngine::EngineOperationalTier targetTier)
{
    if (mCurrentOperationalTier != targetTier) {
        mCurrentOperationalTier = targetTier;
        switch(targetTier)
        {
        case IShipEngine::EngineOperationalTier::tierII:
            setEngineProperitesSetting(mEngineDefaultTierPropertiesPoints);
            break;
        case IShipEngine::EngineOperationalTier::TierIII:
            setEngineProperitesSetting(mEngineNOxReducedTierPropertiesPoints);
            break;
        }
        return true;
    }
    return false;
}

QVector<IShipEngine::EngineOperationalLoad>
IShipEngine::getEngineOperationalLoads() {
    return mEngineOperationalLoad;
}

QVector<IShipEngine::EngineOperationalTier>
IShipEngine::getEngineOperationalTiers() {
    return mEngineOperationalTier;
}



}
