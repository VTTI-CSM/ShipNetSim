#include "shipengine.h"
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
    // empty
}

void ShipEngine::initialize(Ship *host, QVector<IEnergySource*> energySources,
                const QMap<QString, std::any> &parameters)
{
    mHost = host;

    mEnergySources = energySources;
    mCurrentEnergySource = energySources[0];

    // set the engines parameters
    setParameters(parameters);

    counter++; // increment the counter
}

void ShipEngine::setParameters(const QMap<QString, std::any> &parameters)
{
    mId =
        Utils::getValueFromMap<unsigned int>(parameters,
                                             "EngineID",
                                             counter);

    mEngineOperationalPowerSettings =
        Utils::getValueFromMap<QVector<units::power::kilowatt_t>>
        (parameters, "EngineOperationalPowerSettings", {});

    if (mEngineOperationalPowerSettings.size() != 4)
    {
        qCritical() << "Engine max power properties is not defined! "
                       "Engine Properties (BrakePower, RPM, Efficiency) "
                       "must be defined at the"
                       " corners of the engine layout!";
    }

    mEngineDefaultTierPropertiesPoints = Utils::getValueFromMap<QVector<EngineProperties>>
        (parameters, "EngineTierIIPropertiesPoints", {});

    if (mEngineDefaultTierPropertiesPoints.size() < 2)
    {
        qFatal("Engine Properties points are not defined! "
               "2 Engine Properties (BrakePower, RPM, Efficiency) "
               "must be defined at least!");
    }

    mEngineNOxReducedTierPropertiesPoints = Utils::getValueFromMap<QVector<EngineProperties>>
        (parameters, "EngineTierIIIPropertiesPoints", {});

    // sort the engine properties
    std::sort(mEngineOperationalPowerSettings.begin(),
              mEngineOperationalPowerSettings.end());
    std::sort(mEngineDefaultTierPropertiesPoints.begin(),
              mEngineDefaultTierPropertiesPoints.end(),
              EngineProperties::compareByBreakPower);
    if (!mEngineDefaultTierPropertiesPoints.empty())
    {
        std::sort(mEngineNOxReducedTierPropertiesPoints.begin(),
                  mEngineNOxReducedTierPropertiesPoints.end(),
                  EngineProperties::compareByBreakPower);
    }

    setCurrentEngineProperties(EngineOperationalLoad::Economic);

    // set initial value
    mEfficiency = 0.001f;
    mRPM = units::angular_velocity::revolutions_per_minute_t(0.0f);
    mCurrentOutputPower = units::power::kilowatt_t(0.0);
}

void ShipEngine::setEngineMaxPowerLoad(double targetRatio)
{
    mMaxPowerRatio = targetRatio;

    // get the power
    updateCurrentStep();
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
        (mCurrentOutputPower / mEfficiency) *
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
    return mEfficiency;
}


void ShipEngine::updateCurrentStep()
{
    // update the previous power
    mPreviousOutputPower = mCurrentOutputPower;

    units::power::kilowatt_t mRawPower;  ///< power without considering eff
    // ensure the RPM is up to date
    if (!mIsWorking) {
        mRPM = units::angular_velocity::revolutions_per_minute_t(0.0);
        mRawPower = units::power::kilowatt_t(0.0);
        mCurrentOutputPower = mRawPower;
        mEfficiency = 0.0;
        return;
    }

    double lambda = getHyperbolicThrottleCoef(mHost->getSpeed());

    // Calculating power without considering efficiency
    // as the efficiency is only for calculating fuel consumption
    mRawPower = lambda * mCurrentOperationalPowerSetting;
    if (mRawPower > mCurrentOperationalPowerSetting)
    {
        mRawPower = mCurrentOperationalPowerSetting;
    }

    auto r = getEnginePropertiesAtPower(mRawPower, mCurrentOperationalTier);

    // Getting efficiency based on raw power without
    // checks or using stored values
    mEfficiency = (r.efficiency <= 0.0) ? 0.0001 : r.efficiency;
    mRPM = r.RPM;

    // the engine output power is what is reported by the manufacturer
    mCurrentOutputPower = mRawPower;

}


units::power::kilowatt_t ShipEngine::getBrakePower()
{
    return mCurrentOutputPower;
}

units::torque::newton_meter_t ShipEngine::getBrakeTorque()
{
    units::torque::newton_meter_t result =
        units::torque::newton_meter_t(
        (mCurrentOutputPower.convert<units::power::watt>().value()) /
            mRPM.convert<
                units::angular_velocity::radians_per_second>().value());

    return result;
}

units::angular_velocity::revolutions_per_minute_t ShipEngine::getRPM()
{
    return mRPM;
}

QVector<units::angular_velocity::revolutions_per_minute_t>
ShipEngine::getRPMRange()
{
    return {mEngineDefaultTierPropertiesPoints.front().RPM,
            mEngineDefaultTierPropertiesPoints.back().RPM};
}

double ShipEngine::getHyperbolicThrottleCoef(
    units::velocity::meters_per_second_t ShipSpeed)
{
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

    return lambda;

}

int ShipEngine::getEngineID()
{
    return mId;
}

void ShipEngine::setEngineRPM(
    units::angular_velocity::revolutions_per_minute_t targetRPM)
{

    // set the new RPM
    mRPM =
        units::math::max(mEngineDefaultTierPropertiesPoints.front().RPM,
                         units::math::min(targetRPM,
                                          mEngineDefaultTierPropertiesPoints.back().RPM));

    // get the power
    updateCurrentStep();

}

units::power::kilowatt_t ShipEngine::getPreviousBrakePower()
{
    return mPreviousOutputPower;
}

bool ShipEngine::isEngineWorking()
{
    return mIsWorking;
}
};
