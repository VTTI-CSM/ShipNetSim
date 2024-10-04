#include "ienergyconsumer.h"
#include "ship.h"

namespace ShipNetSimCore
{

IEnergyConsumer::IEnergyConsumer(QObject* parent) : QObject(parent)
{
    mHost = nullptr;
    mCurrentEnergySource = nullptr;
    mCumFuelConsumption = initializeFuelConsumption();
}

IEnergyConsumer::~IEnergyConsumer()
{
    mHost = nullptr;
    mCurrentEnergySource = nullptr;
}

void IEnergyConsumer::moveObjectToThread(QThread *thread) {
    this->moveToThread(thread);
}

void IEnergyConsumer::setHost(Ship *host)
{
    mHost = host;
}

void IEnergyConsumer::setEnergySources(QVector<IEnergySource*> energySources)
{
    mEnergySources = energySources;
}

const Ship* IEnergyConsumer::getHost() const
{
    return mHost;
}

IEnergySource *IEnergyConsumer::getCurrentEnergySource() const
{
    return mCurrentEnergySource;
}
};
