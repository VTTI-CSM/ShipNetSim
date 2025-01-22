#ifndef SEAPORTLOADER_H
#define SEAPORTLOADER_H

#include <QString>
#include <qthreadstorage.h>
#include "seaport.h"
#include "../export.h"

namespace ShipNetSimCore {
// class SeaPort;
class GPoint;

class SHIPNETSIM_EXPORT SeaPortLoader {
public:
    // Delete copy constructor and assignment operator
    SeaPortLoader(const SeaPortLoader&) = delete;
    SeaPortLoader& operator=(const SeaPortLoader&) = delete;

    // Load ports from file
    static bool loadPortsFromFile(const QString& filePath);

    // Get closest port to point (static for direct access)
    static std::shared_ptr<SeaPort>
    getClosestPortToPoint(const std::shared_ptr<GPoint> point,
                          units::length::meter_t maxDistance =
                          units::length::meter_t(
                              std::numeric_limits<double>::infinity()));

    // reads the seaports as vector
    static QVector<std::shared_ptr<SeaPort>> readSeaPorts(const char* filename);

    /**
     * @brief loads the sea ports from the default geojson file
     * @return a vector of all sea ports in the network that support
     *         cargo ships.
     */
    static QVector<std::shared_ptr<SeaPort>>
    loadFirstAvailableSeaPorts();

    // load the GEOJSON file
    static bool loadFirstAvailableSeaPortsFile(QVector<QString> locations);

    static QVector<std::shared_ptr<SeaPort>> getPorts();

private:
    // Private constructor for singleton
    SeaPortLoader();

    // Container for loaded ports
    static QThreadStorage<QVector<std::shared_ptr<SeaPort>>> mPorts;
};

};  // namespace ShipNetSimCore
#endif // SEAPORTLOADER_H
