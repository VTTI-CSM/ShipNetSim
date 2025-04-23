#ifndef DYNAMICRESISTANCE_H
#define DYNAMICRESISTANCE_H

#include "ishipdynamicresistancestrategy.h"

namespace ShipNetSimCore
{
class LangMaoMethod : public IShipDynamicResistanceStrategy
{
public:
    LangMaoMethod();

    // IDynamicResistance interface
    units::force::newton_t getWaveResistance(const Ship &ship);
    units::force::newton_t getWindResistance(const Ship &ship);
    units::force::newton_t
    getTotalResistance(const Ship &ship) override;

private:
    units::force::newton_t
    getWaveReflectionResistance(const Ship &ship);
    units::force::newton_t getWaveMotionResistance(const Ship &ship);

    double getDragCoef(units::angle::degree_t angleOfAttach =
                           units::angle::degree_t(0.0));
};
}; // namespace ShipNetSimCore
#endif // DYNAMICRESISTANCE_H
