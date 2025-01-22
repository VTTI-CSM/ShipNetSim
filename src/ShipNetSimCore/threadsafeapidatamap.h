#include <QReadWriteLock>
#include <QMap>
#include <QString>
#include <memory>
#include <stdexcept>
#include "network/optimizednetwork.h"
#include "simulatorworker.h"
#include "shiploaderworker.h"
#include "simulator.h"
#include "ship/ship.h"

/**
* @struct APIData
* @brief Container for network-specific simulation components
*
* @details Holds all components required for a single network simulation,
* including network, simulator, workers, and ships. Each network in the
* simulation has its own APIData instance.
*/
struct SHIPNETSIM_EXPORT APIData {
    ShipNetSimCore::OptimizedNetwork* network = nullptr;  ///< Network instance for routing and geography
    SimulatorWorker* simulatorWorker = nullptr;           ///< Worker for simulation processing
    ShipNetSimCore::Simulator* simulator = nullptr;       ///< Main simulator instance
    ShipLoaderWorker* shipLoaderWorker = nullptr;        ///< Worker for loading ship data
    QThread *workerThread = nullptr;                      ///< Dedicated thread for this network
    QMap<QString, std::shared_ptr<ShipNetSimCore::Ship>> ships;  ///< Map of ship ID to ship instance
    bool isBusy = false;  ///< Indicates if network is currently processing
};


class ThreadSafeAPIDataMap
{
public:
    ThreadSafeAPIDataMap() = default;
    ~ThreadSafeAPIDataMap() = default;

    // Add or update APIData
    void addOrUpdate(const QString& networkName, const APIData& data)
    {
        QWriteLocker locker(&mDataLock);
        mData.insert(networkName, data);
    }

    // Remove APIData
    void remove(const QString& networkName)
    {
        QWriteLocker locker(&mDataLock);
        mData.remove(networkName);
    }

    // Get APIData (returns a copy for thread safety)
    APIData get(const QString& networkName) const
    {
        QReadLocker locker(&mDataLock);
        if (!mData.contains(networkName))
        {
            throw std::runtime_error(
                QString("Network not found in APIData: %1").
                arg(networkName).toStdString());
        }
        return mData.value(networkName);
    }

    // Check if a network exists
    bool contains(const QString& networkName) const
    {
        QReadLocker locker(&mDataLock);
        return mData.contains(networkName);
    }

    // Get all network names
    QList<QString> getNetworkNames() const
    {
        QReadLocker locker(&mDataLock);
        return mData.keys();
    }

    // Set busy state for a network
    void setBusy(const QString& networkName, bool busy)
    {
        QWriteLocker locker(&mDataLock);
        if (mData.contains(networkName)) {
            mData[networkName].isBusy = busy;
        }
    }

    // Check if a network is busy
    bool isBusy(const QString& networkName) const
    {
        QReadLocker locker(&mDataLock);
        return mData.contains(networkName) ? mData[networkName].isBusy : false;
    }

    // Get the simulator for a network
    ShipNetSimCore::Simulator* getSimulator(const QString& networkName) const
    {
        QReadLocker locker(&mDataLock);
        if (!mData.contains(networkName))
        {
            throw std::runtime_error(
                QString("Network not found in APIData: %1").arg(networkName)
                    .toStdString());
        }
        return mData[networkName].simulator;
    }

    // Get the network for a network name
    ShipNetSimCore::OptimizedNetwork*
    getNetwork(const QString& networkName) const
    {
        QReadLocker locker(&mDataLock);
        if (!mData.contains(networkName)) {
            throw std::runtime_error(
                QString("Network not found in APIData: %1")
                    .arg(networkName).toStdString());
        }
        return mData[networkName].network;
    }

    // Add a ship to a network
    void addShip(const QString& networkName,
                 const std::shared_ptr<ShipNetSimCore::Ship>& ship)
    {
        QWriteLocker locker(&mDataLock);
        if (mData.contains(networkName))
        {
            mData[networkName].ships.insert(ship->getUserID(), ship);
        }
    }

    // Get all ships for a network
    QVector<std::shared_ptr<ShipNetSimCore::Ship>>
    getAllShips(const QString& networkName) const
    {
        QReadLocker locker(&mDataLock);
        if (!mData.contains(networkName))
        {
            throw std::runtime_error(
                QString("Network not found in APIData: %1")
                    .arg(networkName).toStdString());
        }
        return mData[networkName].ships.values().toVector();
    }

    // Get a ship by ID
    std::shared_ptr<ShipNetSimCore::Ship>
    getShipByID(const QString& networkName, const QString& shipID) const
    {
        QReadLocker locker(&mDataLock);
        if (!mData.contains(networkName))
        {
            throw std::runtime_error(
                QString("Network not found in APIData: %1")
                    .arg(networkName).toStdString());
        }
        if (mData[networkName].ships.contains(shipID))
        {
            return mData[networkName].ships.value(shipID);
        }
        return nullptr;
    }

    // Clear all data
    void clear()
    {
        QWriteLocker locker(&mDataLock);
        mData.clear();
    }

private:
    QMap<QString, APIData> mData;  // The shared data
    mutable QReadWriteLock mDataLock;  // Protects mData
};
