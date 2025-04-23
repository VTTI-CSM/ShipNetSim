#ifndef SEAPORT_H
#define SEAPORT_H

#include "../export.h"
#include "gpoint.h"
#include <QString>
#include <qobject.h>

#include <QMetaType>

namespace ShipNetSimCore
{
class SHIPNETSIM_EXPORT SeaPort : public QObject
{
    Q_OBJECT
public:
    // SeaPort();

    SeaPort(GPoint newCoordinate);

    void setPortCoordinate(GPoint newCoordinate);
    void setClosestPointOnWaterPolygon(
        std::shared_ptr<GPoint> newCoordinate);
    void setCountryName(QString country);
    void setPortName(QString portName);
    void setPortCode(QString portCode);
    void setHasRailTerminal(bool access);
    void setHasRoadTerminal(bool access);
    void setStatusOfEntry(QString status);

    GPoint                  getPortCoordinate();
    std::shared_ptr<GPoint> getClosestPointOnWaterPolygon();
    QString                 getCountryName();
    QString                 getPortName();
    QString                 getPortCode();
    bool                    getHasRailTerminal();
    bool                    getHasRoadTerminal();
    QString                 getStatusOfEntry();

private:
    QString                 mPortCountry;
    QString                 mPortName;
    QString                 mPortCode;
    GPoint                  mPortCoordinate;
    bool                    mHasRailTerminal;
    bool                    mHasRoadTerminal;
    QString                 mStatusOfEntry;
    std::shared_ptr<GPoint> mClosestPointOnWaterPolygon;
};
}; // namespace ShipNetSimCore

Q_DECLARE_METATYPE(ShipNetSimCore::SeaPort *)
Q_DECLARE_METATYPE(std::shared_ptr<ShipNetSimCore::SeaPort>)
#endif // SEAPORT_H
