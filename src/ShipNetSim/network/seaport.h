#ifndef SEAPORT_H
#define SEAPORT_H

#include "gpoint.h"
#include <QString>

namespace ShipNetSimCore
{
class SeaPort
{
public:
    // SeaPort();

    SeaPort(GPoint newCoordinate);

    void setPortCoordinate(GPoint newCoordinate);
    void setClosestPointOnWaterPolygon(std::shared_ptr<GPoint> newCoordinate);
    void setCountryName(QString country);
    void setPortName(QString portName);
    void setPortCode(QString portCode);

    GPoint getPortCoordinate();
    std::shared_ptr<GPoint> getClosestPointOnWaterPolygon();
    QString getCountryName();
    QString getPortName();
    QString getPortCode();

private:
    QString mPortCountry;
    QString mPortName;
    QString mPortCode;
    GPoint mPortCoordinate;
    std::shared_ptr<GPoint> mClosestPointOnWaterPolygon;
};
};
#endif // SEAPORT_H
