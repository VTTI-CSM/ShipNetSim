#ifndef SHIPENGINE_H
#define SHIPENGINE_H


#include "ishipengine.h"
#include "IEnergySource.h"
#include <QMap>

class ShipEngine : IShipEngine
{
public:

    ShipEngine();

    // IEnergyConsumer interface
    void initialize(Ship *host, IEnergySource *energySource,
                    QMap<QString, std::any> &parameters) override;
    void setParameters(QMap<QString, std::any> &parameters) override;

    EnergyConsumptionData
    energyConsumed(units::time::second_t timeStep) override;

    // IEngine interface
    void readPowerEfficiency(QString filePath) override;
    void readPowerRPM(QString filePath) override;

    double getEfficiency() override;
    units::power::kilowatt_t getBrakePower() override;
    units::angular_velocity::revolutions_per_minute_t getRPM() override;

    units::power::kilowatt_t getPreviousBrakePower() override;

    double getHyperbolicThrottleCoef(
        units::velocity::meters_per_second_t ShipSpeed);



private:
    unsigned int mId;
    bool mIsWorking;
    // for interpolation
    QMap<units::power::kilowatt_t,
         units::angular_velocity::revolutions_per_minute_t>
        mBrakePowerToRPMMap;
    QMap<units::power::kilowatt_t, double> mBrakePowerToEfficiencyMap;

    unsigned int counter = 0;

    double mEfficiency;
    units::angular_velocity::revolutions_per_minute_t mRPM;
    units::power::kilowatt_t mRawPower;
    units::power::kilowatt_t mCurrentOutputPower;
    units::power::kilowatt_t mPreviousOutputPower;

    void updateCurrentStep();

};

#endif // SHIPENGINE_H
