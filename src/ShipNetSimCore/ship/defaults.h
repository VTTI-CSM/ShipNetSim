/**
 * @file defaults.h
 * @brief This file containes the defaults values for the simulator.
 *
 * This class holds the default values for all the simulator
 * variables.
 *
 * @author Ahmed Aredah
 * @date 11.6.2023
 */

#ifndef DEFAULTS_H
#define DEFAULTS_H

#include "../../third_party/units/units.h"
#include <QString>

namespace Defaults
{
units::volume::liter_t TankSize = units::volume::liter_t(11356235.35);
double                 TankInitialCapacityPercentage = 0.9;
double                 TankDepthOfDischarge          = 0.9;

int PropellerCountPerShip   = 1;
int EngineCountPerPropeller = 1;
} // namespace Defaults
#endif // DEFAULTS_H
