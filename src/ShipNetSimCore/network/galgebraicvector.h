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
#include "gline.h"
#include "gpoint.h"
#include <QVector>

namespace ShipNetSimCore
{

/**
 * @class GAlgebraicVector
 * @brief Represents the navigation state of a ship in geodesic coordinates.
 *
 * This class tracks:
 * - **Position**: Current location (latitude/longitude)
 * - **Course**: Direction the ship is facing (azimuth, 0-360°)
 * - **Target**: Next waypoint the ship is navigating toward
 * - **Environment**: Water conditions (temperature, salinity, waves, depth)
 *
 * ## Navigation Model
 * The ship cannot turn instantly. Each timestep:
 * 1. Ship rotates toward target (limited by max rate of turn)
 * 2. Ship moves forward along its current course
 *
 * ## Key Distinction
 * - `getCourse()`: Direction the ship is FACING (may differ from target)
 * - `getAngleToTarget()`: Angle between course and direction to target
 *
 * ## Antimeridian Handling
 * When target is at same physical location (e.g., portal points at ±180°),
 * rotation is skipped to avoid undefined azimuth calculations.
 *
 * ## GPS Spoofing Simulation
 * Supports simulating GPS attacks via `setGPSUpdateState()`. When disabled,
 * position updates go to backup; when re-enabled, backup is restored.
 */
class GAlgebraicVector
{
public:
    // =========================================================================
    // Constructors
    // =========================================================================

    /**
     * @brief Default constructor. Initializes at origin (0,0) facing north.
     */
    GAlgebraicVector();

    /**
     * @brief Constructs with given position and initial target.
     * @param startPoint Initial position of the ship.
     * @param endPoint Initial target waypoint (sets initial course toward it).
     */
    GAlgebraicVector(const GPoint startPoint, const GPoint &endPoint);

    // =========================================================================
    // Position & Heading (Ship's Current State)
    // =========================================================================

    /**
     * @brief Gets the ship's current geographic position.
     * @return Current position as GPoint (latitude/longitude).
     */
    GPoint getCurrentPosition() const;

    /**
     * @brief Sets the ship's position (used for GPS spoofing simulation).
     * @param newPosition New position to set.
     */
    void setCurrentPosition(GPoint newPosition);

    /**
     * @brief Gets the ship's current course (heading).
     * @return Azimuth in degrees [0, 360) that the ship is facing.
     * @note This is the actual direction of travel, not the direction to target.
     */
    units::angle::degree_t getCourse() const;

    /**
     * @brief Checks whether the ship is currently rotating toward target.
     * @return True if ship hasn't aligned with target direction yet.
     */
    bool isRotating() const;

    // =========================================================================
    // Target & Navigation
    // =========================================================================

    /**
     * @brief Gets the current target waypoint.
     * @return Target position as GPoint.
     */
    GPoint getTarget();

    /**
     * @brief Sets the target waypoint and maximum rate of turn.
     * @param target Target position to navigate toward.
     * @param maxROTPerSec Maximum rate of turn in degrees per second.
     */
    void setTargetAndMaxROT(const GPoint          &target,
                            units::angle::degree_t maxROTPerSec);

    /**
     * @brief Gets angle difference between current course and target direction.
     * @return Angle in degrees [-180, 180]. Positive = target is to port (left).
     * @note Normalized to handle antimeridian crossing correctly.
     */
    units::angle::degree_t getAngleToTarget() const;

    /**
     * @brief Calculates the smallest angle to another point from current position.
     * @param otherPoint Point to calculate angle to.
     * @return Smallest angle between current heading and direction to point.
     */
    units::angle::degree_t angleTo(const GPoint &otherPoint) const;

    // =========================================================================
    // Movement
    // =========================================================================

    /**
     * @brief Advances the ship by given distance along its current course.
     * @param distance Distance to travel in meters.
     * @param timeStep Time elapsed (used to calculate rotation amount).
     *
     * Internally:
     * 1. Rotates toward target (limited by maxROT × timeStep)
     * 2. Moves forward along mCurrentCourse (not directly toward target)
     *
     * @note If target is at same location (distance < 1m), rotation is skipped.
     */
    void moveByDistance(units::length::meter_t distance,
                        units::time::second_t  timeStep);

    // =========================================================================
    // Environment
    // =========================================================================

    /**
     * @brief Gets the environmental conditions at ship's location.
     * @return Environment struct with water properties.
     */
    AlgebraicVector::Environment getEnvironment() const;

    /**
     * @brief Sets the environmental conditions at ship's location.
     * @param env New environment conditions.
     */
    void setEnvironment(const AlgebraicVector::Environment env);

    // =========================================================================
    // GPS State (Cyber Attack Simulation)
    // =========================================================================

    /**
     * @brief Enables/disables GPS updates for cyber attack simulation.
     * @param isUpdating True for normal operation, false to simulate GPS spoofing.
     *
     * When disabled, position updates go to backup vector.
     * When re-enabled, backup is restored (simulating GPS recovery).
     */
    void setGPSUpdateState(bool isUpdating);

    /**
     * @brief Restores position from backup (GPS recovery after attack).
     */
    void restoreLatestCorrectPosition();

private:
    // =========================================================================
    // Navigation State
    // =========================================================================

    /// Line from current position to target waypoint.
    /// startPoint = current position, endPoint = target.
    GLine mNavigationLine;

    /// Backup of navigation line for GPS spoofing recovery.
    GLine mNavigationLine_backup;

    /// Ship's actual heading/course in degrees [0, 360).
    /// This is the direction the ship is facing, updated during rotation.
    units::angle::degree_t mCurrentCourse;

    /// Maximum rate of turn in degrees per second.
    units::angle::degree_t mMaxROTPerSec;

    /// True if ship is currently rotating toward target.
    bool mIsRotating;

    // =========================================================================
    // Environment & GPS State
    // =========================================================================

    /// Environmental conditions at ship's current location.
    AlgebraicVector::Environment mStateEnv;

    /// GPS update state. False during simulated cyber attacks.
    bool mIsUpdating = true;

    // =========================================================================
    // Private Methods
    // =========================================================================

    /**
     * @brief Sets course to point directly at the given endpoint.
     * @param endPoint Point to face toward.
     * @note If endpoint is at same location (< 1m), course is unchanged.
     */
    void setHeadingByEndPoint(const GPoint &endPoint);

    /**
     * @brief Rotates ship toward target, limited by max rate of turn.
     * @param target Target position to rotate toward.
     * @param maxROTPerSec Maximum rotation rate in degrees/second.
     * @param deltaTime Time step for this rotation.
     * @note If target is at same location (< 1m), rotation is skipped.
     */
    void rotateToTargetByMaxROT(const GPoint          &target,
                                units::angle::degree_t maxROTPerSec,
                                units::time::second_t  deltaTime);
};

}; // namespace ShipNetSimCore

#endif // GALGEBRAICVECTOR_H
