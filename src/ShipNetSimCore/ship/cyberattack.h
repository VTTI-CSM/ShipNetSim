// cyberattack.h
#ifndef CYBERATTACK_H
#define CYBERATTACK_H

#include <QMap>
#include <QObject>
#include <QString>

namespace ShipNetSimCore
{

class Ship;

class CyberAttack : public QObject
{
    Q_OBJECT

public:
    enum AttackType
    {
        GPS_Spoofing,
        CommunicationJamming,
        EngineControlHijack
    };

    explicit CyberAttack(AttackType type, QObject *parent = nullptr);

    // Set the attack target (e.g., a specific subsystem on the ship)
    void setTarget(Ship *target);

    // Execute the attack on the ship
    void executeAttack();

    // stop the attack
    void stopAttack();

    // Accessors for the attack type and status
    AttackType getAttackType() const;
    bool       isAttackActive() const;

signals:
    void attackStarted();
    void attackEnded();

private:
    AttackType mAttackType;
    bool       mAttackActive;
    Ship      *mTarget; // Pointer to the ship being attacked
};

} // namespace ShipNetSimCore

#endif // CYBERATTACK_H
