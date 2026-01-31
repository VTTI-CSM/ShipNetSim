/**
 * @file galgebraicvector.cpp
 * @brief Implementation of the GAlgebraicVector class.
 * @author Ahmed Aredah
 * @date 10.12.2023
 */

#include "galgebraicvector.h"
#include "gline.h"
#include "gpoint.h"
#include "../utils/utils.h"
#include <QDebug>
#include <cmath>

namespace ShipNetSimCore
{

// =============================================================================
// Constructors
// =============================================================================

GAlgebraicVector::GAlgebraicVector()
{
    // Initialize at origin facing north (toward lat 1°)
    auto currentPos = std::make_shared<GPoint>(
        GPoint(units::angle::degree_t(0.0), units::angle::degree_t(0.0)));
    auto targetPos = std::make_shared<GPoint>(
        GPoint(units::angle::degree_t(1.0), units::angle::degree_t(0.0)));

    mNavigationLine        = GLine(currentPos, targetPos);
    mNavigationLine_backup = mNavigationLine;
    mCurrentCourse         = units::angle::degree_t(0.0);
    mIsRotating            = false;
}

GAlgebraicVector::GAlgebraicVector(const GPoint  startPoint,
                                   const GPoint &endPoint)
{
    auto currentPos = std::make_shared<GPoint>(startPoint);
    auto targetPos  = std::make_shared<GPoint>(endPoint);

    mNavigationLine        = GLine(currentPos, targetPos);
    mNavigationLine_backup = mNavigationLine;
    mCurrentCourse         = mNavigationLine.forwardAzimuth();
    mIsRotating            = false;
}

// =============================================================================
// Position & Heading
// =============================================================================

GPoint GAlgebraicVector::getCurrentPosition() const
{
    return *mNavigationLine.startPoint();
}

void GAlgebraicVector::setCurrentPosition(GPoint newPosition)
{
    auto newStartPoint     = std::make_shared<GPoint>(newPosition);
    auto currentStartPoint = mNavigationLine.startPoint();
    auto currentEndPoint   = mNavigationLine.endPoint();

    // Calculate direction vector (end - start) to preserve target direction
    GPoint directionVector = *currentEndPoint - *currentStartPoint;

    // Set new start point
    mNavigationLine.setStartPoint(newStartPoint);

    // Update end point to maintain relative target position
    GPoint newEndPoint    = *newStartPoint + directionVector;
    auto   newEndPointPtr = std::make_shared<GPoint>(newEndPoint);
    mNavigationLine.setEndPoint(newEndPointPtr);
}

units::angle::degree_t GAlgebraicVector::getCourse() const
{
    return mCurrentCourse;
}

bool GAlgebraicVector::isRotating() const
{
    return mIsRotating;
}

// =============================================================================
// Target & Navigation
// =============================================================================

GPoint GAlgebraicVector::getTarget()
{
    return *mNavigationLine.endPoint();
}

void GAlgebraicVector::setTargetAndMaxROT(const GPoint          &target,
                                          units::angle::degree_t maxROTPerSec)
{
    auto targetPos = std::make_shared<GPoint>(target);
    mNavigationLine.setEndPoint(targetPos);
    mNavigationLine_backup.setEndPoint(targetPos);
    mMaxROTPerSec = maxROTPerSec;
}

units::angle::degree_t GAlgebraicVector::getAngleToTarget() const
{
    // Calculate raw angle difference between course and direction to target
    double diff = mCurrentCourse.value() -
                  mNavigationLine.forwardAzimuth().value();

    // Normalize to [-180, 180] for correct antimeridian handling
    return units::angle::degree_t(AngleUtils::normalizeAngleDifference(diff));
}

units::angle::degree_t
GAlgebraicVector::angleTo(const GPoint &otherPoint) const
{
    GLine lineToPoint = GLine(mNavigationLine.startPoint(),
                              std::make_shared<GPoint>(otherPoint));
    return mNavigationLine.smallestAngleWith(lineToPoint);
}

// =============================================================================
// Movement
// =============================================================================

void GAlgebraicVector::moveByDistance(units::length::meter_t distance,
                                      units::time::second_t  timeStep)
{
    // Step 1: Rotate toward target (limited by max ROT)
    rotateToTargetByMaxROT(*mNavigationLine.endPoint(), mMaxROTPerSec, timeStep);
    mNavigationLine_backup = mNavigationLine;

    // Step 2: Move forward along current course
    auto newCurrentPos = std::make_shared<GPoint>(
        mNavigationLine.startPoint()->pointAtDistanceAndHeading(
            distance, mCurrentCourse));

    if (mIsUpdating)
    {
        mNavigationLine.setStartPoint(newCurrentPos);
    }
    else
    {
        mNavigationLine_backup.setStartPoint(newCurrentPos);
    }
}

// =============================================================================
// Environment
// =============================================================================

AlgebraicVector::Environment GAlgebraicVector::getEnvironment() const
{
    return mStateEnv;
}

void GAlgebraicVector::setEnvironment(const AlgebraicVector::Environment env)
{
    mStateEnv = env;
}

// =============================================================================
// GPS State (Cyber Attack Simulation)
// =============================================================================

void GAlgebraicVector::setGPSUpdateState(bool isUpdating)
{
    if (isUpdating && !mIsUpdating)
    {
        // Recovering from GPS spoofing - restore backup
        mNavigationLine = mNavigationLine_backup;
    }
    mIsUpdating = isUpdating;
}

void GAlgebraicVector::restoreLatestCorrectPosition()
{
    mNavigationLine = mNavigationLine_backup;
}

// =============================================================================
// Private Methods
// =============================================================================

void GAlgebraicVector::setHeadingByEndPoint(const GPoint &endPoint)
{
    auto ePoint = std::make_shared<GPoint>(endPoint);
    mNavigationLine.setEndPoint(ePoint);
    mNavigationLine_backup.setEndPoint(ePoint);

    // Skip course update if endpoint is at same location (antimeridian portal)
    constexpr double MIN_HEADING_DISTANCE_M = 1.0;
    auto distanceToEndPoint = getCurrentPosition().distance(endPoint);
    if (distanceToEndPoint.value() < MIN_HEADING_DISTANCE_M)
    {
        return;  // Maintain current heading
    }

    mCurrentCourse = mNavigationLine.forwardAzimuth();
}

void GAlgebraicVector::rotateToTargetByMaxROT(const GPoint          &target,
                                              units::angle::degree_t maxROTPerSec,
                                              units::time::second_t  deltaTime)
{
    // Skip rotation if target is at same location (antimeridian portal points)
    constexpr double MIN_ROTATION_DISTANCE_M = 1.0;
    auto distanceToTarget = getCurrentPosition().distance(target);
    if (distanceToTarget.value() < MIN_ROTATION_DISTANCE_M)
    {
        mIsRotating = false;
        return;
    }

    auto angleDiff            = getAngleToTarget();
    auto maxOrientationChange = maxROTPerSec * deltaTime.value();

    // Check if we can reach target heading this timestep
    if (std::abs(angleDiff.value()) < maxOrientationChange.value())
    {
        setHeadingByEndPoint(target);
        mIsRotating = false;
        return;
    }

    // Rotate by maximum allowable amount
    mIsRotating = true;
    if (angleDiff.value() > 0)
    {
        mCurrentCourse -= maxOrientationChange;  // Turn port (left)
    }
    else
    {
        mCurrentCourse += maxOrientationChange;  // Turn starboard (right)
    }

    // Normalize course to [0, 360)
    mCurrentCourse = units::angle::degree_t(
        AngleUtils::normalizeLongitude360(mCurrentCourse.value()));
}

}; // namespace ShipNetSimCore
