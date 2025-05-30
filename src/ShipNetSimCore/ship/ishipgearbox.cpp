#include "ishipgearbox.h"
#include "ship.h"

namespace ShipNetSimCore
{
IShipGearBox::IShipGearBox()
{
    mHost = nullptr;
}

IShipGearBox::~IShipGearBox()
{
    mHost = nullptr;

    foreach (auto &e, mEngines)
    {
        if (e != nullptr)
        {
            delete e;
        }
    }
}

void IShipGearBox::moveObjectToThread(QThread *thread)
{
    this->moveToThread(thread);

    for (auto &engine : mEngines)
    {
        if (engine)
        {
            engine->moveObjectToThread(thread);
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

QVector<IShipEngine::EngineProperties>
IShipGearBox::getEnginesRatingProperties()
{
    QVector<IShipEngine::EngineProperties> results;
    for (auto &engine : mEngines)
    {
        results.push_back(engine->getEngineRatingProperties());
    }
    return results;
}

bool IShipGearBox::requestHigherEnginePower()
{
    bool result = true;
    for (auto &engine : mEngines)
    {
        result = result && engine->requestHigherEnginePower();
    }

    return result;
}

bool IShipGearBox::requestLowerEnginePower()
{
    bool result = true;
    for (auto &engine : mEngines)
    {
        result = result && engine->requestLowerEnginePower();
    }

    return result;
}

IShipEngine::EngineOperationalLoad
IShipGearBox::getCurrentOperationalLoad()
{
    return mEngines[0]->getCurrentOperationalLoad();
}
}; // namespace ShipNetSimCore
