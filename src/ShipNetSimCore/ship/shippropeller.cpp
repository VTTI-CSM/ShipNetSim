#include "shippropeller.h"
#include "ishipcalmresistancestrategy.h"
#include "ship.h"
#include "hydrology.h"
#include "../utils/utils.h"
#include <cmath>

namespace ShipNetSimCore
{


#define PROPELLER_EFFICIENCY_AT_ZERO_SPEED 0.8

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~ Generals ~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


double ShipPropeller::KCoef::getResult(double J, double PD,
                                       double AreaRatio, double Z, double Rn)
{
    auto power = [](double base, int exp)
    {
        return (exp == 0)? 1.0 : (exp == 1)? base : std::pow(base, exp);
    };

    double result = 0;
    if (J == 0)
    {
        J = 0.0001;
    }

    for (int i = 0; i < C.size(); i++)
    {
        // TODO: This gives negative value at some instances
        result += C[i] * power(J, s[i]) * power(PD, t[i]) *
                  power(AreaRatio, u[i]) * power(Z, v[i]);
    }

    if (Rn > 2000000.0)
    {
        double logR = std::log(Rn - 0.301);
        double logR2 = std::pow(logR, 2);
        double PD6 = power(PD, 6);
        double PD2 = power(PD, 2);
        if (type == KType::Thrust)
        {
            double J2 = std::pow(J,2);
            result +=
                0.000353485
                - 0.00333758 * AreaRatio * J
                - 0.00478125 * AreaRatio * PD * J
                + 0.000257792 * logR2 * AreaRatio * J2
                + 0.0000643192 * logR * PD6 * J2
                - 0.0000110636 * logR2 * PD6 * J2
                - 0.0000276305 * logR2 * Z * AreaRatio * J2
                + 0.0000954 * logR * Z * AreaRatio * PD * J
                + 0.0000032049 * logR * power(Z, 2) * AreaRatio *
                      power(PD, 3) * J;
        }
        else
        {
            result +=
                - 0.000591412
                + 0.00696898 * PD
                - 0.0000666654 * Z * PD2
                + 0.0160818 * power(AreaRatio, 2)
                - 0.000938091 * logR * PD
                - 0.00059593 * logR * PD2
                + 0.0000782099 * logR2 * PD2
                + 0.0000052199 * logR * Z * AreaRatio * power(J, 2)
                - 0.00000088528 * logR2 * Z * AreaRatio * J
                + 0.0000230171 * logR * Z * PD6
                - 0.00000184341 * logR2 * Z * PD6
                - 0.00400252 * logR * power(AreaRatio, 2)
                + 0.000220915 * logR2 * power(AreaRatio, 2);
        }
    }

    return result;
}

bool ShipPropeller::KCoef::checkInputs(double PD, double AreaRatio, int Z)
{
    if (PD < 0.5 ||
        PD > 1.4)
    {
        qWarning() << "B Series does not support propellers "
                      "with P/D ratio of " << PD << "!";
        return false;
    }
    if (Z > 7 || Z < 2)
    {
        qWarning() << "B Series does not support propellers "
                      "with blades number of " << Z << "!";
        return false;
    }

    if (AreaRatio < 0.3 ||
        AreaRatio > 1.05)
    {
        qWarning() << "B Series does not support Propellers "
                      "with expanded area ratio of " << AreaRatio << "!";
        return false;
    }
    return true;
}

void ShipPropeller::initialize(Ship *ship, IShipGearBox *gearbox,
                               const QMap<QString, std::any> &parameters)
{
    mHost = ship;
    mGearBox = gearbox;
    setParameters(parameters);

    KT.C = {0.008805, -0.204554, 0.166351, 0.158114,
            -0.147581, -0.481497, 0.415437, 0.0144043,
            -0.0530054, 0.0143481, 0.0606826,-0.0125894,
            0.0109689, -0.133698, 0.0063841, -0.0013272,
            0.168496, -0.0507214, 0.0854559, -0.0504475,
            0.010465, -0.0064827, -0.0084173, 0.0168424,
            -0.001023, -0.0317791, 0.018604, -0.004108,
            -0.0006068, -0.0049819, 0.0025983, -0.0005605,
            -0.0016365, -0.0003288, 0.0001165, 0.0006909,
            0.0042175, 0.00005652, -0.0014656};

    KT.s = {0, 1, 0, 0, 2, 1, 0, 0, 2, 0, 1, 0, 1, 0, 0, 2, 3, 0,
            2, 3, 1, 2, 0, 1, 3, 0, 1, 0, 0, 1, 2, 3, 1, 1, 2, 0,
            0, 3, 0};

    KT.t = {0, 0, 1, 2, 0, 1, 2, 0, 0, 1, 1, 0, 0, 3, 6, 6, 0, 0,
            0, 0, 6, 6, 3, 3, 3, 3, 0, 2, 0, 0, 0, 0, 2, 6, 6, 0,
            3, 6, 3};

    KT.u = {0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 1, 1, 0, 0, 0, 1, 2,
            2, 2, 2, 2, 0, 0, 0, 1, 2, 2, 0, 0, 0, 0, 0, 0, 0, 1,
            1, 1, 2};

    KT.v = {0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2,
            2, 2, 2};

    KT.type = KCoef::KType::Thrust;


    KQ.C = {0.0037937, 0.0088652, -0.032241, 0.0034478,
            -0.0408811, -0.108009, -0.0885381, 0.188561,
            -0.0037087, 0.005137, 0.0209449, 0.0047432,
            -0.0072341, 0.0043839, -0.0269403, 0.0558082,
            0.0161886, 0.0031809, 0.015896, 0.0471729,
            0.0196283, -0.0502782, -0.030055, 0.0417122,
            -0.0397722, -0.0035002, -0.0106854, 0.001109,
            -0.0003139, 0.0035985, -0.0014212, -0.0038364,
            0.0126803, -0.0031828, 0.0033427, -0.0018349,
            0.0001125, -0.00002972, 0.0002696, 0.0008327,
            0.0015533, 0.0003027, -0.0001843, -0.0004254,
            0.00008692, -0.0004659, 0.00005542};

    KQ.s = {0, 2, 1, 0, 0, 1, 2, 0, 1, 0, 1, 2, 2, 1, 0, 3, 0, 1,
            0, 1, 3, 0, 3, 2, 0, 0, 3, 3, 0, 3, 0, 1, 0, 2, 0, 1,
            3, 3, 1, 2, 0, 0, 0, 0, 3, 0, 1};

    KQ.t = {0, 0, 1, 2, 1, 1, 1, 2, 0, 1, 1, 1, 0, 1, 2, 0, 3, 3,
             0, 0, 0, 1, 1, 2, 3, 6, 0, 3, 6, 0, 6, 0, 2, 3, 6, 1,
            2, 6, 0, 0, 2, 6, 0, 3, 3, 6, 6};

    KQ.u = {0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1,
            2, 2, 2, 2, 2, 2, 2, 2, 0, 0, 0, 1, 1, 2, 2, 2, 2, 0,
            0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 2};

    KQ.v = {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2,
            2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2};

    KQ.type = KCoef::KType::Torque;

    double PD = (getPropellerPitch()/getPropellerDiameter()).value();

    if (! KT.checkInputs(PD, mPropellerExpandedAreaRatio, mNumberOfblades))
    {
        qFatal("Propeller efficiency cannot be "
               "calculated with the current propeller "
               "characteristics!");
    }

    // get the max effeciency of the propeller to set the
    // max speed of the engines

    // double j = 0.0;
    // double bestEff = 0.0, bestJ = 0.0;
    // while (j < 1.0)
    // {
    //     double approxKT = KT.getResult(j, PD, mPropellerExpandedAreaRatio,
    //                                    mNumberOfblades, 2000000.0);
    //     double approxKQ = KQ.getResult(j, PD, mPropellerExpandedAreaRatio,
    //                                    mNumberOfblades, 2000000.0);
    //     double eff = getOpenWaterEfficiency(j, approxKT, approxKQ);
    //     if (eff > bestEff)
    //     {
    //         bestEff = eff;
    //         bestJ = j;
    //     }
    //     j += 0.05;
    // }

    // for (int i = 0; i < mGearBox->getEngines().size(); i++)
    // {
    //     mGearBox->getEngines()[0]->setEngineMaxSpeedRatio(bestJ);
    // }
}


void ShipPropeller::setParameters(const QMap<QString, std::any> &parameters)
{
    // Process Shaft Efficiency
    mShaftEfficiency =
        Utils::getValueFromMap<double>(parameters, "ShaftEfficiency", -1.0);
    if (mShaftEfficiency < 0.0)
    {
        qFatal("Shaft efficiency is not defined!");
    }

    mPropellerSlip = Utils::getValueFromMap<double>(parameters,
                                                    "PropellerSlip", 0.1);

    // Process Propeller Diameter & Disk Area
    mPropellerDiameter =
        Utils::getValueFromMap<units::length::meter_t>(
            parameters, "PropellerDiameter", units::length::meter_t(-1.0));
    if (mPropellerDiameter.value() < 0.0)
    {
        qFatal("Propeller diameter is not defined!");
    }

    mPropellerPitch = Utils::getValueFromMap<units::length::meter_t>(
        parameters, "PropellerPitch", units::length::meter_t(-1.0));
    if (mPropellerPitch.value() < 0.0)
    {
        qFatal("Propeller pitch is not defined!");
    }

    mNumberOfblades = Utils::getValueFromMap<int>(
        parameters, "PropellerBladesCount", -1);
    if (mNumberOfblades < 0)
    {
        mNumberOfblades = 4;
        qWarning() << "Propeller number of blades is not defined! "
                      "Set to default '4 blades'!";
    }

    mPropellerDiskArea =
        units::constants::pi *
        units::math::pow<2>(mPropellerDiameter) / 4.0;

    // Process Expanded Area Ratio & Blade Area
    mPropellerExpandedAreaRatio =
        Utils::getValueFromMap<double>(parameters,
                                       "PropellerExpandedAreaRatio", -1.0);
    if (mPropellerExpandedAreaRatio < 0.0)
    {
        qFatal("Propeller expanded area ratio is not defined!");
    }

    mAllowPropellerEngineOptimization =
        Utils::getValueFromMap<bool>(parameters,
        "AllowPropellerEngineOptimization", false);

    mExpandedBladeArea =
        mPropellerExpandedAreaRatio * mPropellerDiskArea;

}

const QVector<IShipEngine *> ShipPropeller::getDrivingEngines() const
{
    return mGearBox->getEngines();
}

double ShipPropeller::getHullEfficiency()
{
    return mHost->getCalmResistanceStrategy()->getHullEffeciency(*mHost);
}

double ShipPropeller::getRelativeEfficiency()
{
    return mHost->getCalmResistanceStrategy()->
        getPropellerRotationEfficiency(*mHost);

}

double ShipPropeller::getOpenWaterEfficiency(
    double JRatio, double K_T, double K_Q)
{
    if (std::isnan(JRatio) || std::isnan(K_T) || std::isnan(K_Q))
    {
        JRatio = getAdvanceRatio(getRPM());
        K_T = getThrustCoefficient(getRPM());
        K_Q = getTorqueCoefficient(getRPM());
    }

    return (JRatio / (2.0 * units::constants::pi.value())) * (K_T / K_Q);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~ Calculations ~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~



// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~ Shaft ~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

double ShipPropeller::getShaftEfficiency()
{
    return mShaftEfficiency;
}

units::power::kilowatt_t ShipPropeller::getShaftPower()
{
    return mGearBox->getOutputPower() * mShaftEfficiency;
}

units::force::newton_t ShipPropeller::getShaftThrust()
{
    return getShaftPower().convert<units::power::watt>() /
           mHost->getCalmResistanceStrategy()->calc_SpeedOfAdvance(*mHost);
}

units::torque::newton_meter_t ShipPropeller::getShaftTorque()
{
    return units::torque::newton_meter_t(
        getShaftPower().convert<units::power::watt>().value() /
        getRPM().convert<units::angular_velocity::
                         radians_per_second>().value());
}


// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~ Propeller ~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


void ShipPropeller::setShaftEfficiency(double newShaftEfficiency)
{
    mShaftEfficiency = newShaftEfficiency;
}

double ShipPropeller::getPropellerEfficiency()
{
    if (getAdvanceRatio(getRPM()) < 0.3)
    {
        return PROPELLER_EFFICIENCY_AT_ZERO_SPEED;
    }
    else
    {
        if (mAllowPropellerEngineOptimization) {
            // find intersection of engine curve and propeller curve.
            auto newP = solveEnginePropellerIntersection();
            for (auto& engine : mGearBox->getEngines()) {
                auto newPR =
                    newP.breakPower.value() /
                    engine->getEngineRatingProperties().breakPower.value();
                engine->setEngineMaxPowerLoad(newPR);
            }
            // mGearBox->setEngineTargetState(newP);
        }
        else {
            // check if engine power at this rpm is sufficient to rotate the
            // propeller
            if (calcPowerDifferenceBetweenEngineAndPropellerPowerAtRPM(
                    getRPM()) < 0)
            {
                auto newP =
                    getMinEngineCharacteristicsForPropellerAtRPM(getRPM());
                for (auto& engine : mGearBox->getEngines()) {
                    auto newPR =
                        newP.breakPower.value() /
                        engine->getEngineRatingProperties().breakPower.value();
                    engine->setEngineMaxPowerLoad(newPR);
                }
            } else {
                mGearBox->setEngineMaxPowerLoad(1.0);
            }

            // keep the engine updated
            mGearBox->updateGearboxOperationalState();
        }
        // mGearBox->setEngineMaxPowerLoad(getOptimumJ(mHost->getSpeed()));
        return getOpenWaterEfficiency() * getRelativeEfficiency();
    }
}

units::power::kilowatt_t ShipPropeller::getEffectivePower()
{
    auto gearboxPower = mGearBox->getOutputPower();
    auto propellerEff = getPropellerEfficiency();
    auto shaftEff = mShaftEfficiency;
    auto hullEff = getHullEfficiency();
    mPreviousEffectivePower = gearboxPower * propellerEff * shaftEff * hullEff;
    return mPreviousEffectivePower;
}

units::power::kilowatt_t ShipPropeller::getPreviousEffectivePower()
{
    return mPreviousEffectivePower;
}


units::force::newton_t ShipPropeller::getThrust()
{
    auto PE = getEffectivePower().convert<units::power::watt>();
    
    auto Va = mHost->getCalmResistanceStrategy()->
              calc_SpeedOfAdvance(*mHost);

    units::force::newton_t T =
        units::force::newton_t(PE.value()/Va.value());

    auto env = mHost->getCurrentEnvironment();
    auto water_rho = hydrology::get_waterDensity(env.salinity, env.temperature);

    // check Basic ship propulsion book page 34
    units::force::newton_t maxThrust =
        units::force::newton_t(std::cbrt(
            2.0 * water_rho.value() *
        mPropellerDiskArea.value() * std::pow(PE.value(),2)));

    return units::math::min(T, maxThrust);
}

units::angular_velocity::revolutions_per_minute_t ShipPropeller::getRPM()
{
    return mGearBox->getOutputRPM();
}

units::torque::newton_meter_t ShipPropeller::getTorque()
{
    return units::torque::newton_meter_t(
        getEffectivePower().convert<units::power::watt>().value() /
        getRPM().convert<units::angular_velocity::
                         radians_per_second>().value());
}

double ShipPropeller::getThrustCoefficient(
    units::angular_velocity::revolutions_per_minute_t rpm,
    units::velocity::meters_per_second_t speed)
{

    double J = getAdvanceRatio(rpm, speed);
    double PD = (getPropellerPitch()/getPropellerDiameter()).value();
    auto env = mHost->getCurrentEnvironment();
    units::velocity::meters_per_second_t spd;
    if (!isnan(speed.value())) {
        spd = speed;
    }
    else {
        spd = mHost->getSpeed();
    }
    double RN = hydrology::R_n(spd,
                               mHost->getLengthInWaterline(),
                               env.salinity, env.temperature);
    double result =  KT.getResult(J, PD, getPropellerExpandedAreaRatio(),
                                 getPropellerBladesCount(), RN);
    if (result >= 0)
    {
        return result;
    }
    else
    {
        double speedOfAdvance = mHost->getCalmResistanceStrategy()->
                                calc_SpeedOfAdvance(*mHost, speed).value();
        double n = rpm.
                   convert<units::angular_velocity::revolutions_per_second>().
                   value();
        std::cout << "ERROR AT " << "J: " << J
                  << ", PD: " <<  PD
                  << ", Va: " << speedOfAdvance
                  << ", N: " << n
                  << ", A/A: " << getPropellerExpandedAreaRatio()
                  << ", salinity: " << env.salinity.value()
                  << ", temp: " << env.temperature.value()
                  << ", Z: " << getPropellerBladesCount()
                  << ", RN: " << RN << "\n";
        // return 0.0;
        qFatal("Thrust Coefficient cannot be a negative value!"
               " Use a custom efficiency curve for the propeller "
               "instead of the B-Series!");
        return result;
    }
}

double ShipPropeller::getTorqueCoefficient(
    units::angular_velocity::revolutions_per_minute_t rpm,
    units::velocity::meters_per_second_t speed)
{
    double J = getAdvanceRatio(rpm);
    double PD = (getPropellerPitch()/getPropellerDiameter()).value();
    auto env = mHost->getCurrentEnvironment();
    units::velocity::meters_per_second_t spd;
    if (!isnan(speed.value())) {
        spd = speed;
    }
    else {
        spd = mHost->getSpeed();
    }
    double RN = hydrology::R_n(spd,
                               mHost->getLengthInWaterline(),
                               env.salinity, env.temperature);
    double result = KQ.getResult(J, PD, getPropellerExpandedAreaRatio(),
                                 getPropellerBladesCount(), RN);

    if (result >= 0)
    {
        return result;
    }
    else
    {
        qFatal("Torque Coefficient cannot be a negative value!"
               " Use a custom efficiency curve for the propeller "
               "instead of the B-Series!");
        return result;
    }
}

double ShipPropeller::getPropellerSlipToIdeal(
    units::velocity::meters_per_second_t customSpeed,
    units::angular_velocity::revolutions_per_minute_t customRPM)
{
    return 1.0 - (mHost->getCalmResistanceStrategy()->
                  calc_SpeedOfAdvance(*mHost, customSpeed).value() /
                  getIdealAdvanceSpeed(customRPM).value());
}

units::velocity::meters_per_second_t
ShipPropeller::getIdealAdvanceSpeed(
    units::angular_velocity::revolutions_per_minute_t customRPM)
{
    units::angular_velocity::revolutions_per_second_t rps;
    if (!std::isnan(customRPM.value())) {
        rps =
            customRPM
                  .convert<units::angular_velocity::revolutions_per_second>();
    }
    else {
        rps =
            getRPM()
                  .convert<units::angular_velocity::revolutions_per_second>();
    }
    return units::velocity::meters_per_second_t(
        rps.value() * mPropellerPitch.value());
}

double ShipPropeller::getAdvanceRatio(
    units::angular_velocity::revolutions_per_minute_t rpm,
    units::velocity::meters_per_second_t speed)
{
    double speedOfAdvance = mHost->getCalmResistanceStrategy()->
                          calc_SpeedOfAdvance(*mHost, speed).value();
    double n = rpm.
               convert<units::angular_velocity::revolutions_per_second>().
               value();

    double j = (n != 0.0) ? ( speedOfAdvance /
                             ( n * getPropellerDiameter().value())) : 0.0;

    // confine j to be with [0, 1]
    if (j < 0.0) { return 0.0;}
    else if (j > 1.0) { return 1.0;}
    return j;
}

units::angular_velocity::revolutions_per_minute_t
ShipPropeller::getRPMFromAdvanceRatioAndMaxShipSpeed(double advanceRatio)
{
    double speedofAdvance =
        mHost->getCalmResistanceStrategy()->
        calc_SpeedOfAdvance(*mHost, mHost->getMaxSpeed()).value();

    double n =
        speedofAdvance / (advanceRatio * getPropellerDiameter().value());

    return units::angular_velocity::revolutions_per_minute_t(n);
}

units::angular_velocity::revolutions_per_minute_t
ShipPropeller::getRPMFromAdvanceRatioAndShipSpeed(
    double advanceRatio, units::velocity::meters_per_second_t speed)
{
    double speedofAdvance =
        mHost->getCalmResistanceStrategy()->
        calc_SpeedOfAdvance(*mHost, speed).value();

    double n =
        speedofAdvance / (advanceRatio * getPropellerDiameter().value());

    return units::angular_velocity::revolutions_per_minute_t(n);
}


double ShipPropeller::getOptimumJ(units::velocity::meters_per_second_t speed) {
    double PD = (getPropellerPitch()/getPropellerDiameter()).value();
    auto env = mHost->getCurrentEnvironment();
    double RN = hydrology::R_n(speed, mHost->getLengthBetweenPerpendiculars(),
                               env.salinity, env.temperature);

    /// calculate the eff at J
    auto getEfficiencyAtJ = [&](double jV, double PDV, double RNV) {
        double approxKT = KT.getResult(jV, PDV, mPropellerExpandedAreaRatio,
                                       mNumberOfblades, RNV);
        double approxKQ = KQ.getResult(jV, PDV, mPropellerExpandedAreaRatio,
                                       mNumberOfblades, RNV);
        return getOpenWaterEfficiency(jV, approxKT, approxKQ);
    };

    double step = 0.05; // Step size for evaluation around bestJ
    double bestEff = getEfficiencyAtJ(mLastBestJ, PD, RN);
    double j = mLastBestJ;

    bool searchPositive = true; // Initially, let's assume we
                                // search in the positive direction.

    bool updated = false;       // If the algorithm found a better eff,
                                // then keep searching.

    do {
        updated = false;
        double newJ = j + (searchPositive ? step : -step);
        if ((newJ <= 1.0 && searchPositive) ||
            (newJ >= 0.0 && !searchPositive))
        {
            double newEff = getEfficiencyAtJ(newJ, PD, RN);
            if (newEff > bestEff) {
                bestEff = newEff;
                j = newJ;
                updated = true; // Found a better efficiency,
                                // continue in this direction
            }
            else
            {
                // If no improvement, switch the search direction
                // for the next iteration
                searchPositive = !searchPositive;
                newJ = j + (searchPositive ? step : -step);

                if ((newJ <= 1.0 && searchPositive) ||
                    (newJ >= 0.0 && !searchPositive))
                {
                    newEff = getEfficiencyAtJ(newJ, PD, RN);

                    if (newEff > bestEff) {
                        bestEff = newEff;
                        j = newJ;
                        updated = true;  // Found a better efficiency,
                                         // continue in this direction
                    }
                }
            }
        }
    } while (updated);

    mLastBestJ = j; // Update the last known best J
    return j;
}

units::angular_velocity::revolutions_per_minute_t
ShipPropeller::getOptimumRPM(units::velocity::meters_per_second_t speed)
{
    double bestJ = getOptimumJ(speed);

    return getRPMFromAdvanceRatioAndShipSpeed(bestJ, speed);
}

units::power::kilowatt_t ShipPropeller::getRequiredShaftPowerAtRPM(
    units::angular_velocity::revolutions_per_minute_t rpm,
    units::velocity::meters_per_second_t speed)
{
    // Calculate advance ratio (J) based on RPM and ship's speed
    // double J = getAdvanceRatio(rpm, speed);

    // Get thrust and torque coefficients for the given RPM (J ratio)
    // double KT_v = getThrustCoefficient(rpm, speed);
    double KQ_v = getTorqueCoefficient(rpm, speed);

    // Calculate open water efficiency based on these coefficients
    //double openWaterEfficiency = getOpenWaterEfficiency(J, KT_v, KQ_v);

    auto waterDensity = hydrology::get_waterDensity(
                            mHost->getCurrentEnvironment().salinity,
                            mHost->getCurrentEnvironment().temperature).value();
    auto prop5 = std::pow(getPropellerDiameter().value(), 5);
    auto rps2 = std::pow(rpm.convert<units::angular_velocity::
                                    revolutions_per_second>().value(), 2);

    // Torque is related to power and RPM
    units::torque::newton_meter_t torque =
        units::torque::newton_meter_t(
            KQ_v *
            waterDensity *
            prop5 *
            rps2);

    // Calculate shaft power (P = 2π * n * torque)  ==> rad = rev * 2π
    units::power::kilowatt_t shaftPower =
        units::power::watt_t(
            rpm.convert<units::angular_velocity::radians_per_second>().value() *
            torque.convert<units::torque::newton_meter>().value()).
                                          convert<units::power::kilowatt>();

    // std::cout << KQ_v << ", " <<waterDensity << ", " << prop5 << ", " << rps2
    //           << ", " << torque.value() << ", " << shaftPower.value() << "\n";

    // Return the required shaft power, adjusted by propeller
    // efficiency and shaft efficiency
    return shaftPower; // * openWaterEfficiency * getShaftEfficiency();
}

double ShipPropeller::calcPowerDifferenceBetweenEngineAndPropellerPowerAtRPM(
    units::angular_velocity::revolutions_per_minute_t rpm)
{
    IShipEngine::EngineProperties enginePropertiesAtShaft =
        mGearBox->getGearboxOperationalPropertiesAtRPM(rpm);
    units::power::kilowatt_t enginePowerAtShaft =
        enginePropertiesAtShaft.breakPower * mShaftEfficiency;

    // Get the propeller's required shaft power at the same RPM
    units::power::kilowatt_t propellerPower =
        getRequiredShaftPowerAtRPM(rpm);

    // Calculate the absolute difference between engine and propeller power
    return enginePowerAtShaft.value() -
           propellerPower.value();
}

IShipEngine::EngineProperties ShipPropeller::
    getMinEngineCharacteristicsForPropellerAtRPM(
    units::angular_velocity::revolutions_per_minute_t rpm)
{
    // Get the min and max RPM values from the gearbox
    auto [minRPM, maxRPM] = mGearBox->getOutputRPMRange();

    const double step = 1.0;  // Step size for the search

    std::vector<std::pair<double, double>> mtemp;
    for (double rpm = minRPM.value(); rpm < maxRPM.value(); rpm += step) {
        auto r = units::angular_velocity::revolutions_per_minute_t(rpm);
        mtemp.emplace_back(
            rpm, calcPowerDifferenceBetweenEngineAndPropellerPowerAtRPM(r));
    }
    auto it = std::min_element(mtemp.begin(), mtemp.end(),
                               [](const std::pair<double, double>& a, const std::pair<double, double>& b) {
                                   // Compare only non-negative differences
                                   if (a.second < 0 && b.second < 0) return false; // Ignore both if negative
                                   if (a.second < 0) return false; // Ignore a if negative
                                   if (b.second < 0) return true;  // Ignore b if negative
                                   return a.second < b.second;    // Standard comparison otherwise
                               });
    if (it != mtemp.end() && it->second >= 0) {
        return mGearBox->getGearboxOperationalPropertiesAtRPM(
            units::angular_velocity::revolutions_per_minute_t(it->first));
    } else {
        QString errorMessage =
            QString("The required power to rotate the propeller within "
                    "the RPM range (%1, %2) exceeds the engine's "
                    "available power at these RPMs.")
                .arg(minRPM.value())
                .arg(maxRPM.value());
        qFatal(errorMessage.toLocal8Bit().constData());
    }
}

IShipEngine::EngineProperties ShipPropeller::solveEnginePropellerIntersection()
{
    // Get the min and max RPM values from the gearbox
    auto [minRPM, maxRPM] = mGearBox->getOutputRPMRange();

    const double step = 1.0;  // Step size for the search

    auto calculateDifference = [this](double rpm) {
        auto r = units::angular_velocity::revolutions_per_minute_t(rpm);
        return calcPowerDifferenceBetweenEngineAndPropellerPowerAtRPM(r);
    };

    // Start within the valid range
    double n = std::clamp(mLastBestN, minRPM.value(), maxRPM.value());

    double bestDiff = calculateDifference(n);
    bool validRPMFound = bestDiff >= 0; // Track if a valid RPM has been found

    // Initially, let's assume we search in the positive direction.
    bool searchPositive = true;
    // If the algorithm finds a better diff, it will keep searching.
    bool updated = false;

    do {
        updated = false;
        double newN = n + (searchPositive ? step : -step);

        // Ensure the new RPM is within the bounds of minRPM and maxRPM
        if (newN <= maxRPM.value() && newN >= minRPM.value()) {
            double newDiff = calculateDifference(newN);

            // Only consider newDiff if it's >= 0
            if (newDiff >= 0 && newDiff < bestDiff) {
                bestDiff = newDiff;
                n = newN;
                // Found a better diff, continue searching
                // in the same direction
                updated = true;
                validRPMFound = true; // Mark that a valid RPM was found
            } else {
                // Switch search direction if no improvement
                searchPositive = !searchPositive;
                newN = n + (searchPositive ? step : -step);

                if (newN <= maxRPM.value() && newN >= minRPM.value()) {
                    newDiff = calculateDifference(newN);

                    // Only accept newDiff if it's >= 0
                    if (newDiff >= 0 && newDiff < bestDiff) {
                        bestDiff = newDiff;
                        n = newN;
                        // Found a better diff, continue searching
                        // in this new direction
                        updated = true;
                        // Mark that a valid RPM was found
                        validRPMFound = true;
                    }
                }
            }
        }
    } while (updated);

    // If no valid RPM was found, throw an error or handle it appropriately
    if (!validRPMFound) {
        QString errorMessage =
            QString("The required power to rotate the propeller within "
                    "the RPM range (%1, %2) exceeds the engine's "
                    "available power at these RPMs.")
                .arg(minRPM.value())
                .arg(maxRPM.value());
        qFatal(errorMessage.toLocal8Bit().constData());
    }

    // Update the last known best RPM
    mLastBestN = n;

    // Return the engine's operational properties at the best RPM
    return mGearBox->getGearboxOperationalPropertiesAtRPM(
        units::angular_velocity::revolutions_per_minute_t(n));
}





};
