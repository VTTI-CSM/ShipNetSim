// cyberattack.cpp
#include "cyberattack.h"
#include "ship.h"
#include <QDebug>

namespace ShipNetSimCore
{

CyberAttack::CyberAttack(AttackType type, QObject *parent)
    : QObject(parent)
    , mAttackType(type)
    , mAttackActive(false)
    , mTarget(nullptr)
{
}

void CyberAttack::setTarget(Ship *target)
{
    mTarget = target;
}

void CyberAttack::executeAttack()
{
    if (!mTarget)
    {
        qWarning() << "No target specified for cyber attack!";
        return;
    }

    mAttackActive = true;
    emit attackStarted();

    if (mAttackType == GPS_Spoofing)
    {
        qDebug() << "Executing GPS spoofing attack on the ship!";

        auto position = mTarget->getCurrentPosition();
        units::angle::degree_t spoofedLatitude =
            position.getLatitude(); // Initialize with real position
        units::angle::degree_t spoofedLongitude =
            position.getLongitude(); // Initialize with real position

        // Apply an offset to simulate the spoofing only if under
        // attack
        spoofedLatitude =
            position.getLatitude()
            + units::angle::degree_t(
                (rand() % 10 - 5)
                * 0.001); // Random shift by up to 0.005 degrees
        spoofedLongitude =
            position.getLongitude()
            + units::angle::degree_t((rand() % 10 - 5) * 0.001);
        GPoint newP = GPoint(spoofedLongitude, spoofedLatitude);

        // Modify the ship's GPS data
        mTarget->setCurrentPosition(newP); // spoof position
    }
    else if (mAttackType == CommunicationJamming)
    {
        qDebug() << "Executing communication jamming attack!";
        // Disable communication between systems
        mTarget->disableCommunications();
    }
    else if (mAttackType == EngineControlHijack)
    {
        qDebug() << "Executing engine control hijack!";
        auto props = mTarget->getPropellers();
        for (auto &prop : *props)
        {
            auto engs = prop->getDrivingEngines();

            for (auto eng : engs)
            {
                eng->turnOffEngine();
            }
        }
        // TODO: Take control of the ship's engine
    }
}

void CyberAttack::stopAttack()
{
    emit attackEnded();
    mAttackActive = false;

    if (mAttackType == GPS_Spoofing)
    {
        mTarget->restoreLatestGPSCorrectPosition();
    }
    else if (mAttackType == CommunicationJamming)
    {
        mTarget->enableCommunications();
    }
    else if (mAttackType == EngineControlHijack)
    {
        auto props = mTarget->getPropellers();
        for (auto &prop : *props)
        {
            auto engs = prop->getDrivingEngines();

            for (auto eng : engs)
            {
                eng->turnOnEngine();
            }
        }
    }
}

CyberAttack::AttackType CyberAttack::getAttackType() const
{
    return mAttackType;
}

bool CyberAttack::isAttackActive() const
{
    return mAttackActive;
}

} // namespace ShipNetSimCore
