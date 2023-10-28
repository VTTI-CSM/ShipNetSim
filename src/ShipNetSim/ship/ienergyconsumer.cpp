#include "ienergyconsumer.h"
#include "ship.h"
IEnergyConsumer::IEnergyConsumer()
{
    mHost = nullptr;
    mEnergySource = nullptr;
}

IEnergyConsumer::~IEnergyConsumer()
{
    mHost = nullptr;
    if (mEnergySource) delete mEnergySource;
}

void IEnergyConsumer::setHost(Ship *host)
{
    mHost = host;
}

void IEnergyConsumer::setEnergySource(IEnergySource *energySource)
{
    mEnergySource = energySource;
}

const Ship* IEnergyConsumer::getHost() const
{
    return mHost;
}

IEnergySource *IEnergyConsumer::getEnergySource() const
{
    return mEnergySource;
}
