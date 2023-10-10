#ifndef ISHIPGEARBOX_H
#define ISHIPGEARBOX_H

#include <QMap>
#include <any>
#include "../../third_party/units/units.h"

class IShipEngine; // Forward declaration of engine
class Ship; // Forward declaration of ship class

class IShipGearBox
{
public:
    IShipGearBox();
    ~IShipGearBox();
    virtual void initialize(Ship *host, QVector<IShipEngine*> engines,
                            QMap<QString, std::any> &parameters) = 0;

    void setHost(Ship *host);
    void setEngines(QVector<IShipEngine*> engines);
    const QVector<IShipEngine*> getEngines() const;
    const Ship* getHost() const;

    virtual void setParameters(QMap<QString, std::any> &parameters) = 0;
    virtual units::angular_velocity::revolutions_per_minute_t
    getOutputRPM() const = 0;

    virtual units::power::kilowatt_t getOutputPower() = 0;
    virtual units::power::kilowatt_t getPreviousOutputPower() const = 0;
protected:
    Ship *mHost;
    QVector<IShipEngine *> mEngines;
};

#endif // ISHIPGEARBOX_H
