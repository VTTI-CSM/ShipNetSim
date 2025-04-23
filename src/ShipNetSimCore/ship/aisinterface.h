#ifndef AISINTERFACE_H
#define AISINTERFACE_H

#include "../network/gpoint.h"
#include "iaisinterface.h"
#include "ship.h"
#include <QMap>
#include <QSet>
#include <QString>
#include <QVector>
#include <functional>

namespace ShipNetSimCore
{

struct Status
{
    int    mmsi;                     // MMSI of the ship
    GPoint position;                 // Position (latitude, longitude)
    units::velocity::knot_t speed;   // Speed over ground (knots)
    units::angle::degree_t  heading; // Heading (degrees)
    units::angle::degree_t  course;  // Course (degrees)
    Ship::NavigationStatus  navStatus; // Navigation status
    QString                 shipName;  // Name of the ship

    Status()
        : mmsi(0)
        , position()
        , speed(units::velocity::knot_t(0))
        , heading(units::angle::degree_t(0))
        , course(units::angle::degree_t(0))
        , navStatus(Ship::NavigationStatus::Undefined)
        , shipName("")
    {
    }
};

class AISInterface : public IAISInterface
{
public:
    AISInterface();
    AISInterface(const Ship *host);
    ~AISInterface();

    // Implementations of IAISInterface methods
    void             setHost(const Ship *host) override;
    QVector<QString> encodeChunks() override;
    void decodeChunks(const QVector<QString> &chunks) override;
    void step(std::function<void(const QString &)> sendFunction,
              units::time::second_t                timeStep) override;
    void resetTransmission() override;
    bool receiveAISData(const QString &chunk,
                        const Ship    *transmittingShip) override;
    bool isMessageComplete() const override;

    // Additional methods for handling multiple ships
    QVector<QString> getReceivedChunks(QString &shipID)
        const; // Retrieve received chunks for a specific ship
    void clearReceivedChunks(
        QString &shipID); // Clear received chunks for a specific ship
    QMap<QString, QVector<QString>>
    getAllReceivedData() const; // Retrieve all received chunks
    QMap<QString, Status>
    getAllStatuses() const; // Retrieve all decoded statuses

private:
    const Ship      *mHost;   // The host ship
    QVector<QString> mChunks; // Encoded AIS message chunks
    size_t nextChunkIndex;    // Index of the next chunk to send
    units::time::second_t
        mTimeSinceLastTransmission; // Time elapsed since the last
                                    // transmission
    units::time::second_t
        mCurrentTransmissionInterval; // Current interval based on
                                      // speed/state

    // Variables for receiving and decoding
    QMap<QString, QVector<QString>>
        mReceivedChunksMap; // Received chunks grouped by ship
    QSet<QString>
        mCompletedMessages; // Set of ships with complete messages
    QMap<QString, Status>
        mDecodedStatuses; // Decoded statuses for each ship user id
    QMap<QString, units::time::second_t>
        mLastUpdateTime; // Time since last update for each ship

    // Constants for AIS ranges (in meters)
    static units::length::meter_t SHIP_RANGE; // Ship-to-Ship range
    static units::time::second_t
        MAX_INACTIVE_TIME; // Max time before removing a ship

    // Helper methods
    QString encodeBinary()
        const; // Helper to encode ship data into binary string
    void decodeBinary(
        const QString &binaryData,
        const Ship
            *transmittingShip); // Decode binary for a specific ship
    static QString toBinary(int value, int bits);
    static int     fromBinary(const QString &binaryData);

    // Determine transmission interval based on speed/state
    units::time::second_t determineTransmissionInterval() const;

    // Helper to calculate distance between two ships
    static units::length::meter_t
    calculateDistance(const Ship *ship1, const Ship *ship2);
};

} // namespace ShipNetSimCore

#endif // AISINTERFACE_H
