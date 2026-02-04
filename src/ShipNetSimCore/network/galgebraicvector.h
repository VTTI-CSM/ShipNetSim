/**
 * @file galgebraicvector.h
 * @brief Declaration of the GAlgebraicVector class for geodesic ship navigation.
 * @author Ahmed Aredah
 * @date 10.12.2023
 */

#ifndef GALGEBRAICVECTOR_H
#define GALGEBRAICVECTOR_H

#include "../../third_party/units/units.h"
#include "algebraicvector.h"
#include "gpoint.h"

namespace ShipNetSimCore
{

/**
 * @class GAlgebraicVector
 * @brief Represents the navigation state of a ship in geodesic coordinates.
 *
 * Tracks position (lat/lon), heading, target waypoint, and environment.
 * The ship cannot turn instantly — each timestep it rotates toward its
 * target (limited by max rate of turn) then moves forward along its
 * current heading.
 *
 * Angle convention: angleTo() and getAngleToTarget() both return
 * (bearingToTarget - heading), normalized to [-180, 180].
 * Positive = target is to starboard (right).
 * Negative = target is to port (left).
 *
 * GPS spoofing simulation: when GPS is disabled via setGPSUpdateState(),
 * position updates go to a backup. When re-enabled, backup is restored.
 */
class GAlgebraicVector
{
public:
    // =====================================================================
    // Constructors
    // =====================================================================

    /**
     * @brief Default constructor. Initializes at origin (0,0) facing north.
     */
    GAlgebraicVector();

    /**
     * @brief Constructs with given position and initial target.
     * @param startPoint Initial position of the ship.
     * @param endPoint Initial target (sets initial heading toward it).
     */
    GAlgebraicVector(const GPoint startPoint, const GPoint &endPoint);

    // =====================================================================
    // Position & Heading
    // =====================================================================

    /**
     * @brief Gets the ship's current geographic position.
     */
    GPoint getCurrentPosition() const;

    /**
     * @brief Sets the ship's position.
     * @param newPosition New position to set.
     */
    void setCurrentPosition(GPoint newPosition);

    /**
     * @brief Gets the ship's current heading (course).
     * @return Azimuth in degrees [0, 360).
     */
    units::angle::degree_t getCourse() const;

    /**
     * @brief Whether the ship is currently rotating toward target.
     */
    bool isRotating() const;

    // =====================================================================
    // Target & Navigation
    // =====================================================================

    /**
     * @brief Gets the current target waypoint.
     */
    GPoint getTarget() const;

    /**
     * @brief Sets the target waypoint and maximum rate of turn.
     * @param target Target position to navigate toward.
     * @param maxROTPerSec Maximum rate of turn in degrees per second.
     */
    void setTargetAndMaxROT(const GPoint          &target,
                            units::angle::degree_t maxROTPerSec);

    /**
     * @brief Angle from current heading to the current target.
     * @return (bearingToTarget - heading), normalized to [-180, 180].
     *         Positive = target to starboard. Negative = target to port.
     */
    units::angle::degree_t getAngleToTarget() const;

    /**
     * @brief Angle from current heading to an arbitrary point.
     * @param otherPoint Point to calculate angle to.
     * @return (bearingToPoint - heading), normalized to [-180, 180].
     *         Positive = point to starboard. Negative = point to port.
     */
    units::angle::degree_t angleTo(const GPoint &otherPoint) const;

    // =====================================================================
    // Movement
    // =====================================================================

    /**
     * @brief Advances the ship by given distance.
     * @param distance Distance to travel in meters.
     * @param timeStep Time elapsed (used to calculate rotation amount).
     *
     * Steps: rotate toward target (limited by maxROT × timeStep),
     * then move forward along current heading.
     */
    void moveByDistance(units::length::meter_t distance,
                        units::time::second_t  timeStep);

    // =====================================================================
    // Environment
    // =====================================================================

    AlgebraicVector::Environment getEnvironment() const;
    void setEnvironment(const AlgebraicVector::Environment env);

    // =====================================================================
    // GPS State (Cyber Attack Simulation)
    // =====================================================================

    /**
     * @brief Enables/disables GPS updates for cyber attack simulation.
     * @param isUpdating True = normal, false = GPS spoofed.
     */
    void setGPSUpdateState(bool isUpdating);

    /**
     * @brief Restores position from backup (GPS recovery).
     */
    void restoreLatestCorrectPosition();

private:
    // Core navigation state
    GPoint                 mPosition;
    GPoint                 mTarget;
    units::angle::degree_t mHeading;
    units::angle::degree_t mMaxROTPerSec;
    bool                   mIsRotating = false;

    // GPS spoofing backup
    GPoint                 mBackupPosition;
    units::angle::degree_t mBackupHeading;
    bool                   mIsGPSUpdating = true;

    // Environment
    AlgebraicVector::Environment mEnvironment;

    // Private helpers
    void rotateToTarget(units::time::second_t deltaTime);
};

}; // namespace ShipNetSimCore

#endif // GALGEBRAICVECTOR_H
