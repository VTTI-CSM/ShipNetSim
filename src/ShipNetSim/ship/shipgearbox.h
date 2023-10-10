#ifndef SHIPGEARBOX_H
#define SHIPGEARBOX_H

#include "ishipgearbox.h"



class ShipGearBox : IShipGearBox
{
public:
    ShipGearBox();

    // IShipGearBox interface
    void initialize(Ship *host, QVector<IShipEngine *> engines,
                    QMap<QString, std::any> &parameters) override;
    void setParameters(QMap<QString, std::any> &parameters) override;

    units::angular_velocity::revolutions_per_minute_t
    getOutputRPM() const override;

    units::power::kilowatt_t getOutputPower() override;
    units::power::kilowatt_t getPreviousOutputPower() const override;

private:

    double mEfficiency;// the efficiency of the gearbox in the range [0, 1].
    double mGearRationTo1;
    units::power::kilowatt_t mOutputPower;
};

#endif // SHIPGEARBOX_H
