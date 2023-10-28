#include "shipengine.h"
#include "ship.h"
#include <QFile>
#include <QStringList>
#include <QTextStream>
#include <QDebug>
#include <QMap>
#include "../utils/utils.h"

ShipEngine::ShipEngine()
{
    mHost = nullptr;
    mEnergySource = nullptr;
}

void ShipEngine::initialize(Ship *host, IEnergySource *energySource,
                const QMap<QString, std::any> &parameters)
{
    mHost = host;

    mEnergySource = energySource;

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
    mBrakePowerToRPMMap =
        Utils::getValueFromMap<
            QMap<units::power::kilowatt_t,
                 units::angular_velocity::revolutions_per_minute_t>>(
            parameters, "BrakePowerToRPM",
            QMap<units::power::kilowatt_t,
                 units::angular_velocity::revolutions_per_minute_t>());

    mBrakePowerToEfficiencyMap =
        Utils::getValueFromMap<QMap<units::power::kilowatt_t,
                                    double>>(
            parameters, "BrakePowerToEfficiency",
            QMap<units::power::kilowatt_t, double>());

    if (mBrakePowerToRPMMap.size() < 1)
    {
        qCritical() << "Power-To-RPM Mapping is not defined!";
    }

    if (mBrakePowerToEfficiencyMap.size() < 1)
    {
        qCritical() << "Power-To-Efficiency Mapping is not defined!";
    }
}

EnergyConsumptionData
ShipEngine::energyConsumed(units::time::second_t timeStep)
{
    // the brake power should be increased to consider the losses
    units::energy::kilowatt_hour_t energy =
        mCurrentOutputPower * timeStep;
    // consume the energy and return not consumed energy
    auto result = mEnergySource->consume(timeStep, energy);
    if (!result.isEnergySupplied)
    {
        // no power is provided
        mIsWorking = false;
    }
    return result;
}

void ShipEngine::readPowerEfficiency(QString filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "Unable to open" << filePath;
        return;
    }

    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine();
        QStringList parts = line.split(' ', Qt::SkipEmptyParts);
        if (parts.size() != 2) {
            qDebug() << "Invalid line:" << line;
            continue;
        }

        bool ok1, ok2;
        double power = parts[0].toDouble(&ok1);
        double efficiency = parts[1].toDouble(&ok2);

        if (!ok1 || !ok2) {
            qDebug() << "Invalid numbers in line:" << line;
            continue;
        }

        mBrakePowerToEfficiencyMap.insert(
            units::power::kilowatt_t(power), efficiency);
    }

    file.close();
}


void ShipEngine::readPowerRPM(QString filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "Unable to open" << filePath;
        return;
    }

    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine();
        QStringList parts = line.split(' ', Qt::SkipEmptyParts);
        if (parts.size() != 2) {
            qDebug() << "Invalid line:" << line;
            continue;
        }

        bool ok1, ok2;
        double power = parts[0].toDouble(&ok1);
        double speed = parts[1].toDouble(&ok2);

        if (!ok1 || !ok2) {
            qDebug() << "Invalid numbers in line:" << line;
            continue;
        }

        mBrakePowerToRPMMap.insert(
            units::power::kilowatt_t(power),
            units::angular_velocity::revolutions_per_minute_t(speed));
    }

    file.close();
}


double ShipEngine::getEfficiency()
{
    return mEfficiency;
}


void ShipEngine::updateCurrentStep()
{
    mPreviousOutputPower = mCurrentOutputPower;

    double lambda = getHyperbolicThrottleCoef(mHost->getSpeed());

    units::power::kilowatt_t min_power = mBrakePowerToRPMMap.begin().key();
    units::power::kilowatt_t max_power = mBrakePowerToRPMMap.end().key();

    // Calculating power without considering efficiency
    mRawPower = lambda * max_power * mIsWorking;
    if (mRawPower < min_power)
    {
        mRawPower = min_power;
    }
    else if (mRawPower > max_power)
    {
        mRawPower = max_power;
    }

    // Getting efficiency based on raw power without
    // checks or using stored values
    mEfficiency = (mIsWorking)?
        Utils::interpolate(mBrakePowerToEfficiencyMap, mRawPower) : 0.0;

    // Calculating the final power by applying efficiency
    mCurrentOutputPower = mRawPower * mEfficiency;

    // ensure the RPM is up to date
    mRPM = (mIsWorking)?
               Utils::interpolate(mBrakePowerToRPMMap, mRawPower) :
               units::angular_velocity::revolutions_per_minute_t(0.0);
}



units::power::kilowatt_t ShipEngine::getBrakePower()
{
    updateCurrentStep();

    return mCurrentOutputPower;
}

units::angular_velocity::revolutions_per_minute_t ShipEngine::getRPM()
{
    return mRPM;
}

double ShipEngine::getHyperbolicThrottleCoef(
    units::velocity::meters_per_second_t ShipSpeed)
{
    double dv, um;
    // ratio of current speed by the max speed
    dv = (ShipSpeed / mHost->getMaxSpeed()).value();
    double lambda = (double)1.0 / (1.0 + exp(-7.82605 * (dv - 0.42606)));

    auto min = mBrakePowerToRPMMap.begin().key();
    auto max = mBrakePowerToRPMMap.end().key();

    if (lambda < 0.0)
    {
        return 0.0;
    }
    else if (lambda > 1.0)
    {
        return 1.0;
    }

    return lambda;

}

//void ShipEngine::setBrakePowerFractionToFullPower(double fractionToFullPower)
//{

//};

units::power::kilowatt_t ShipEngine::getPreviousBrakePower()
{
    return mPreviousOutputPower;
}
