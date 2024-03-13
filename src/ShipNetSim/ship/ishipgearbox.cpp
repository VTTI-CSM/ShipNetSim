#include "ishipgearbox.h"
#include "ship.h"

IShipGearBox::IShipGearBox()
{
    mHost = nullptr;
}

IShipGearBox::~IShipGearBox()
{
    mHost = nullptr;


    foreach(auto &e, mEngines)
    {
        if (e != nullptr)
        {
            delete e;
        }
    }
}

void IShipGearBox::setHost(Ship *host)
{
    mHost = host;
}

void IShipGearBox::setEngines(QVector<IShipEngine *> engines)
{
    mEngines = engines;
}

const QVector<IShipEngine *> IShipGearBox::getEngines() const
{
    return mEngines;
}

const Ship *IShipGearBox::getHost() const
{
    return mHost;
}
