#ifndef IAISINTERFACE_H
#define IAISINTERFACE_H

#include "../network/gpoint.h"
#include "ship.h"

namespace ShipNetSimCore
{

class IAISInterface {
public:
    // Enum for AIS types
    // enum class Type { SHIP, SHORE, SATELLITE };

    virtual ~IAISInterface() = 0;

    // Set ship details

    ///< Set the host
    virtual void setHost(const Ship *ship) = 0;

    // Encode and decode methods
    ///< Encode AIS data into binary chunks
    virtual QVector<QString> encodeChunks() = 0;

    ///< Decode binary chunks into AIS data
    virtual void decodeChunks(const QVector<QString>& chunks) = 0;

    ///< Simulate a time step and send AIS data if needed
    virtual void step(std::function<void(const QString&)> sendFunction,
                      units::time::second_t timeStep) = 0;

    ///< Reset transmission state
    virtual void resetTransmission() = 0;

    ///< Receive chunks based on distance and decode
    virtual bool receiveAISData(const QString& chunk,
                                const Ship *transmittingShip) = 0;

    ///< Check if the received message is complete
    virtual bool isMessageComplete() const = 0;

};
};

#endif // IAISINTERFACE_H
