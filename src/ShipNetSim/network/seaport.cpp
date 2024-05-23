#include "seaport.h"

namespace ShipNetSimCore
{
SeaPort::SeaPort(GPoint newCoordinate) : mPortCoordinate(newCoordinate) {}

void SeaPort::setPortCoordinate(GPoint newCoordinate) {
    mPortCoordinate = newCoordinate;
}

void SeaPort::setCountryName(QString country) { mPortCountry = country; }
void SeaPort::setPortName(QString portName) { mPortName = portName; }
void SeaPort::setPortCode(QString portCode) { mPortCode = portCode; }

void SeaPort::setClosestPointOnWaterPolygon(
    std::shared_ptr<GPoint> newCoordinate)
{
    mClosestPointOnWaterPolygon = newCoordinate;
    if (!mClosestPointOnWaterPolygon->isPort())
    {
        mClosestPointOnWaterPolygon->MarkAsPort(units::time::second_t(0.0));
    }
}


GPoint SeaPort::getPortCoordinate() { return mPortCoordinate; }
std::shared_ptr<GPoint> SeaPort::getClosestPointOnWaterPolygon() {
    return mClosestPointOnWaterPolygon;
}
QString SeaPort::getCountryName() { return mPortCountry; }
QString SeaPort::getPortName() { return mPortName; }
QString SeaPort::getPortCode() { return mPortCode; }

};
