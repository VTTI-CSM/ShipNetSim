#ifndef IDYNAMICRESISTANCE_H
#define IDYNAMICRESISTANCE_H

#include "../../third_party/units/units.h"

class Ship;  // Forward declaration of the class ship

class IShipDynamicResistanceStrategy {
public:
    virtual ~IShipDynamicResistanceStrategy() = default;

    /**
     * @brief Calculates the total resistances of the ship.
     *          this includes wave and wind effects.
     *
     * This function determines the resistance caused by dynamic
     * nature of the environment. this could include the wave
     * and wind resistances.
     *
     * @param ship A constant reference to the Ship object.
     * @return total resistance in kilonewtons.
     */
    virtual units::force::newton_t
    getTotalResistance(const Ship &ship) = 0;
};

#endif // IDYNAMICRESISTANCE_H
