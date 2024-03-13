#include "shippropeller.h"
#include "ishipcalmresistancestrategy.h"
#include "ship.h"
#include "hydrology.h"
#include "../utils/utils.h"
#include <cmath>

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

    double j = 0.0;
    double bestEff = 0.0, bestJ = 0.0;
    while (j < 1.0)
    {
        double approxKT = KT.getResult(j, PD, mPropellerExpandedAreaRatio,
                                       mNumberOfblades, 2000000.0);
        double approxKQ = KQ.getResult(j, PD, mPropellerExpandedAreaRatio,
                                       mNumberOfblades, 2000000.0);
        double eff = getOpenWaterEfficiency(j, approxKT, approxKQ);
        if (eff > bestEff)
        {
            bestEff = eff;
            bestJ = j;
        }
        j += 0.05;
    }

    for (int i = 0; i < mGearBox->getEngines().size(); i++)
    {
        mGearBox->getEngines()[0]->setEngineMaxSpeedRatio(bestJ);
    }
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
        JRatio = getAdvanceRatio();
        K_T = getThrustCoefficient();
        K_Q = getTorqueCoefficient();
    }

    return (JRatio / 2.0 * units::constants::pi.value()) * (K_T / K_Q);
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
    if (getAdvanceRatio() < 0.3)
    {
        return PROPELLER_EFFICIENCY_AT_ZERO_SPEED;
    }
    else
    {
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
    // mGearBox->getOutputPower() *
    //                           getPropellerEfficiency() *
    //                           mShaftEfficiency *
    //                           getHullEfficiency();
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

double ShipPropeller::getThrustCoefficient()
{

    double J = getAdvanceRatio();
    double PD = (getPropellerPitch()/getPropellerDiameter()).value();
    auto env = mHost->getCurrentEnvironment();
    double RN = hydrology::R_n(mHost->getSpeed(),
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
        // return 0.0;
        qFatal("Thrust Coefficient cannot be a negative value!"
               " Use a custom efficiency curve for the propeller "
               "instead of the B-Series!");
        return result;
    }

//     auto shaftThrust = getShaftThrust(); // Calculate using gearbox output power
//     return (shaftThrust.value() /
//             (hydrology::WATER_RHO.value() *
//              pow(
//                  getRPM().
//                  convert<units::angular_velocity::revolutions_per_second>().
//                  value(), (double)2.0) *
//              pow(getPropellerDiameter().value(),(double)4.0)));
}

double ShipPropeller::getTorqueCoefficient()
{
    double J = getAdvanceRatio();
    double PD = (getPropellerPitch()/getPropellerDiameter()).value();
    auto env = mHost->getCurrentEnvironment();
    double RN = hydrology::R_n(mHost->getSpeed(),
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
    // auto shaftTorque = getShaftTorque(); // Calculated using gearbox output power
    // return (shaftTorque.value() /
    //         (hydrology::WATER_RHO.value() *
    //          pow(
    //              getRPM().
    //              convert<units::angular_velocity::revolutions_per_second>().
    //              value(), (double)2.0) *
    //          pow(getPropellerDiameter().value(),(double)5.0)));
}

double ShipPropeller::getPropellerSlip()
{
    return 1.0 - (mHost->getCalmResistanceStrategy()->
                  calc_SpeedOfAdvance(*mHost).value() /
                  getIdealAdvanceSpeed().value());
}

units::velocity::meters_per_second_t
ShipPropeller::getIdealAdvanceSpeed()
{
    return units::velocity::meters_per_second_t(
        getRPM().
        convert<units::angular_velocity::revolutions_per_second>().
        value() * mPropellerPitch.value());
}

double ShipPropeller::getAdvanceRatio()
{
    double speedOfAdvance = mHost->getCalmResistanceStrategy()->
                          calc_SpeedOfAdvance(*mHost).value();
    double n = getRPM().
               convert<units::angular_velocity::revolutions_per_second>().
               value();

    double j = ( speedOfAdvance /
            ( n * getPropellerDiameter().value()));

    // confine j to be with [0, 1]
    // if (j < 0) { return 0.0;}
    // else if (j > 0) { return 1.0;}
    return j;
}

