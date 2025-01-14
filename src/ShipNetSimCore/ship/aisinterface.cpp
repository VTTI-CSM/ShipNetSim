#include "aisinterface.h"
#include <bitset>
#include <cmath>
#include <QDebug>

namespace ShipNetSimCore {

// Initialize static constants
units::length::meter_t AISInterface::SHIP_RANGE = units::length::meter_t(37040.0); // 20 nautical miles
units::time::second_t AISInterface::MAX_INACTIVE_TIME = units::time::second_t(60); // 60 seconds

AISInterface::AISInterface()
    : mHost(nullptr), nextChunkIndex(0),
    mTimeSinceLastTransmission(0), mCurrentTransmissionInterval(10) {}

AISInterface::AISInterface(const Ship *host)
    : mHost(host), nextChunkIndex(0),
    mTimeSinceLastTransmission(0), mCurrentTransmissionInterval(10) {}

AISInterface::~AISInterface() {}

void AISInterface::setHost(const Ship *host) {
    mHost = host;
}

QVector<QString> AISInterface::encodeChunks() {
    mChunks.clear();
    QString binaryData = encodeBinary();

    constexpr int CHUNK_SIZE = 256; // Define chunk size in bits
    for (int i = 0; i < binaryData.size(); i += CHUNK_SIZE) {
        mChunks.append(binaryData.mid(i, CHUNK_SIZE));
    }

    return mChunks;
}

void AISInterface::decodeChunks(const QVector<QString>& chunks) {
    QString reassembledData;
    for (const auto& chunk : chunks) {
        reassembledData += chunk;
    }

    // Decode and update status
    decodeBinary(reassembledData, mHost);
}

void AISInterface::step(std::function<void(const QString&)> sendFunction,
                        units::time::second_t timeStep) {
    mTimeSinceLastTransmission += timeStep;

    // Check for inactive ships and remove them
    for (auto it = mLastUpdateTime.begin(); it != mLastUpdateTime.end();) {
        it.value() += timeStep;
        if (it.value() > MAX_INACTIVE_TIME) {
            const QString shipID = it.key();
            mDecodedStatuses.remove(shipID);
            mReceivedChunksMap.remove(shipID);
            it = mLastUpdateTime.erase(it); // Remove the ship from the map
        } else {
            ++it;
        }
    }

    if (mTimeSinceLastTransmission >= mCurrentTransmissionInterval) {
        if (!mChunks.isEmpty()) {
            sendFunction(mChunks[nextChunkIndex]);
            nextChunkIndex = (nextChunkIndex + 1) % mChunks.size();
        }

        mTimeSinceLastTransmission = units::time::second_t(0); // Reset time since last transmission
    }
}

void AISInterface::resetTransmission() {
    mChunks.clear();
    nextChunkIndex = 0;
    mTimeSinceLastTransmission = units::time::second_t(0);
    mReceivedChunksMap.clear();
    mCompletedMessages.clear();
    mDecodedStatuses.clear();
}

bool AISInterface::receiveAISData(const QString& chunk,
                                  const Ship *transmittingShip) {
    if (mHost == nullptr || transmittingShip == nullptr) {
        return false;
    }

    // Calculate the distance between the host and transmitting ship
    auto distance = calculateDistance(mHost, transmittingShip);

    if (distance <= SHIP_RANGE) {
        // Store received chunk for the transmitting ship
        mReceivedChunksMap[transmittingShip->getUserID()].append(chunk);

        // Update the last update time
        mLastUpdateTime[transmittingShip->getUserID()] =
            units::time::second_t(0);

        // Check if the message is complete
        if (mReceivedChunksMap[transmittingShip->getUserID()].size() ==
            mChunks.size())
        {
            QString reassembledData;
            for (const auto& receivedChunk :
                 mReceivedChunksMap[transmittingShip->getUserID()]) {
                reassembledData += receivedChunk;
            }

            decodeBinary(reassembledData, transmittingShip);
            mCompletedMessages.insert(transmittingShip->getUserID());
        }

        return true;
    }

    return false;
}

bool AISInterface::isMessageComplete() const {
    return !mCompletedMessages.isEmpty();
}

QVector<QString> AISInterface::getReceivedChunks(QString &shipID) const {
    if (mReceivedChunksMap.contains(shipID)) {
        return mReceivedChunksMap[shipID];
    }
    return {};
}

void AISInterface::clearReceivedChunks(QString &shipID) {
    mReceivedChunksMap.remove(shipID);
}

QMap<QString, QVector<QString> > AISInterface::getAllReceivedData() const {
    return mReceivedChunksMap;
}

QMap<QString, Status> AISInterface::getAllStatuses() const {
    return mDecodedStatuses;
}

// Helper: Encode ship data into binary string
QString AISInterface::encodeBinary() const {
    auto pos = mHost->getCurrentPosition();
    auto speed = mHost->getSpeed().convert<units::velocity::knot>();
    QString shipName = mHost->getName();

    QString binaryData;

    // Encode MMSI (30 bits)
    binaryData += toBinary(mHost->getMMSI(), 30);

    // Encode Latitude (27 bits, scaled by 600,000)
    int latBits = static_cast<int>(pos.getLatitude().value() * 600000);
    binaryData += toBinary(latBits, 27);

    // Encode Longitude (28 bits, scaled by 600,000)
    int lonBits = static_cast<int>(pos.getLongitude().value() * 600000);
    binaryData += toBinary(lonBits, 28);

    // Encode Speed Over Ground (10 bits, scaled by 10)
    int speedBits = static_cast<int>(speed.value() * 10);
    binaryData += toBinary(speedBits, 10);

    // Encode Course Over Ground (12 bits, scaled by 10)
    // TODO: The course is equal to heading now since we do not have side waves
    int cogBits = static_cast<int>(mHost->getCurrentHeading().value() * 10);
    binaryData += toBinary(cogBits, 12);

    // Encode True Heading (9 bits)
    int headingBits = static_cast<int>(mHost->getCurrentHeading().value());
    binaryData += toBinary(headingBits, 9);

    // Encode Navigation Status (4 bits)
    binaryData += toBinary(static_cast<int>(mHost->getNavigationStatus()), 4);

    // Encode Ship Name (20 characters, 6 bits each)
    for (size_t i = 0; i < 20; ++i) {
        if (i < shipName.length()) {
            binaryData += toBinary(static_cast<int>(shipName[i].unicode()), 6);
        } else {
            binaryData += toBinary(0, 6); // Padding
        }
    }

    return binaryData;
}

// Helper: Decode binary data into ship status
void AISInterface::decodeBinary(const QString& binaryData,
                                const Ship *transmittingShip) {
    Status status;
    int pos = 0;

    // Decode MMSI (30 bits)
    status.mmsi = fromBinary(binaryData.mid(pos, 30));
    pos += 30;

    // Decode Latitude (27 bits, scaled by 600,000)
    units::angle::degree_t latitude =
        units::angle::degree_t(fromBinary(binaryData.mid(pos, 27)) / 600000.0);
    pos += 27;

    // Decode Longitude (28 bits, scaled by 600,000)
    units::angle::degree_t longitude =
        units::angle::degree_t(fromBinary(binaryData.mid(pos, 28)) / 600000.0);
    pos += 28;

    // Decode Speed Over Ground (10 bits, scaled by 10)
    status.speed =
        units::velocity::knot_t(fromBinary(binaryData.mid(pos, 10)) / 10.0);
    pos += 10;

    // Decode Course Over Ground (12 bits, scaled by 10)
    status.course =
        units::angle::degree_t(fromBinary(binaryData.mid(pos, 12)) / 10.0);
    pos += 12;

    // Decode True Heading (9 bits)
    status.heading = units::angle::degree_t(fromBinary(binaryData.mid(pos, 9)));
    pos += 9;

    // Decode Navigation Status (4 bits)
    status.navStatus =
        static_cast<Ship::NavigationStatus>(fromBinary(binaryData.mid(pos, 4)));
    pos += 4;

    // Decode Ship Name (20 characters, 6 bits each)
    QString shipName;
    for (int i = 0; i < 20; ++i) {
        int charCode = fromBinary(binaryData.mid(pos, 6));
        if (charCode != 0) {
            shipName += QChar(charCode);
        }
        pos += 6;
    }
    status.shipName = shipName;

    // Update the position and decoded statuses
    status.position = GPoint(latitude, longitude);
    mDecodedStatuses[transmittingShip->getUserID()] = status;
}

// Helper: Convert integer to binary string
QString AISInterface::toBinary(int value, int bits) {
    QString binary = QString::number(value, 2); // Convert to binary
    return binary.rightJustified(bits, '0');  // Right-justify with zero-padding
}

// Helper: Convert binary string to integer
int AISInterface::fromBinary(const QString& binaryData) {
    return binaryData.toInt(nullptr, 2); // Parse binary string to integer
}

// Helper: Calculate distance between two ships
units::length::meter_t AISInterface::calculateDistance(const Ship *ship1,
                                                       const Ship *ship2) {
    auto pos1 = ship1->getCurrentPosition();
    auto pos2 = ship2->getCurrentPosition();
    return pos1.distance(pos2);
}

} // namespace ShipNetSimCore
