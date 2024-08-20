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
void SeaPort::setHasRailTerminal(bool access) { mHasRailTerminal = access; }
void SeaPort::setHasRoadTerminal(bool access) { mHasRoadTerminal = access; }
void SeaPort::setStatusOfEntry(QString status) { mStatusOfEntry = status; }

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
bool SeaPort::getHasRailTerminal() { return mHasRailTerminal; }
bool SeaPort::getHasRoadTerminal() { return mHasRoadTerminal; }
QString SeaPort::getStatusOfEntry() { return mStatusOfEntry; }

};
