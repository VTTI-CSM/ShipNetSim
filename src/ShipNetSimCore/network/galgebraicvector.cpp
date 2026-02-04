#include "galgebraicvector.h"
#include "../utils/utils.h"
#include <cmath>

namespace ShipNetSimCore
{

// =========================================================================
// Constructors
// =========================================================================

GAlgebraicVector::GAlgebraicVector()
    : mPosition(GPoint(units::angle::degree_t(0.0),
                        units::angle::degree_t(0.0)))
    , mTarget(GPoint(units::angle::degree_t(1.0),
                      units::angle::degree_t(0.0)))
    , mHeading(units::angle::degree_t(0.0))
    , mMaxROTPerSec(units::angle::degree_t(0.0))
    , mIsRotating(false)
    , mBackupPosition(mPosition)
    , mBackupHeading(mHeading)
    , mIsGPSUpdating(true)
{
}

GAlgebraicVector::GAlgebraicVector(const GPoint startPoint,
                                   const GPoint &endPoint)
    : mPosition(startPoint)
    , mTarget(endPoint)
    , mMaxROTPerSec(units::angle::degree_t(0.0))
    , mIsRotating(false)
    , mBackupPosition(startPoint)
    , mIsGPSUpdating(true)
{
    // Set initial heading toward the target
    constexpr double MIN_DISTANCE_M = 1.0;
    if (mPosition.distance(mTarget).value() >= MIN_DISTANCE_M)
    {
        mHeading = mPosition.forwardAzimuth(mTarget);
    }
    else
    {
        mHeading = units::angle::degree_t(0.0);
    }
    mBackupHeading = mHeading;
}

// =========================================================================
// Position & Heading
// =========================================================================

GPoint GAlgebraicVector::getCurrentPosition() const
{
    return mPosition;
}

void GAlgebraicVector::setCurrentPosition(GPoint newPosition)
{
    mPosition = newPosition;
}

units::angle::degree_t GAlgebraicVector::getCourse() const
{
    return mHeading;
}

bool GAlgebraicVector::isRotating() const
{
    return mIsRotating;
}

// =========================================================================
// Target & Navigation
// =========================================================================

GPoint GAlgebraicVector::getTarget() const
{
    return mTarget;
}

void GAlgebraicVector::setTargetAndMaxROT(
    const GPoint          &target,
    units::angle::degree_t maxROTPerSec)
{
    mTarget       = target;
    mMaxROTPerSec = maxROTPerSec;
}

units::angle::degree_t GAlgebraicVector::getAngleToTarget() const
{
    constexpr double MIN_DISTANCE_M = 1.0;
    if (mPosition.distance(mTarget).value() < MIN_DISTANCE_M)
        return units::angle::degree_t(0.0);

    double bearingToTarget = mPosition.forwardAzimuth(mTarget).value();
    double diff = bearingToTarget - mHeading.value();
    return units::angle::degree_t(
        AngleUtils::normalizeAngleDifference(diff));
}

units::angle::degree_t
GAlgebraicVector::angleTo(const GPoint &otherPoint) const
{
    double azimuthToPoint =
        getCurrentPosition().forwardAzimuth(otherPoint).value();
    double diff = azimuthToPoint - mHeading.value();
    return units::angle::degree_t(
        AngleUtils::normalizeAngleDifference(diff));
}

// =========================================================================
// Movement
// =========================================================================

void GAlgebraicVector::moveByDistance(units::length::meter_t distance,
                                      units::time::second_t  timeStep)
{
    // Step 1: Rotate toward target (limited by maxROT)
    rotateToTarget(timeStep);

    // Step 2: Move forward along current heading
    GPoint newPos = mPosition.pointAtDistanceAndHeading(
        distance, mHeading);

    if (mIsGPSUpdating)
    {
        mPosition       = newPos;
        mBackupPosition = newPos;
        mBackupHeading  = mHeading;
    }
    else
    {
        mBackupPosition = newPos;
        mBackupHeading  = mHeading;
    }
}

// =========================================================================
// Environment
// =========================================================================

AlgebraicVector::Environment GAlgebraicVector::getEnvironment() const
{
    return mEnvironment;
}

void GAlgebraicVector::setEnvironment(
    const AlgebraicVector::Environment env)
{
    mEnvironment = env;
}

// =========================================================================
// GPS State
// =========================================================================

void GAlgebraicVector::setGPSUpdateState(bool isUpdating)
{
    if (isUpdating && !mIsGPSUpdating)
    {
        // Recovering from GPS spoofing — restore backup
        mPosition = mBackupPosition;
        mHeading  = mBackupHeading;
    }
    mIsGPSUpdating = isUpdating;
}

void GAlgebraicVector::restoreLatestCorrectPosition()
{
    mPosition = mBackupPosition;
    mHeading  = mBackupHeading;
}

// =========================================================================
// Private: Rotation
// =========================================================================

void GAlgebraicVector::rotateToTarget(units::time::second_t deltaTime)
{
    // Skip rotation if target is at same location (e.g. portal points)
    constexpr double MIN_ROTATION_DISTANCE_M = 1.0;
    if (mPosition.distance(mTarget).value() < MIN_ROTATION_DISTANCE_M)
    {
        mIsRotating = false;
        return;
    }

    auto angleDiff = getAngleToTarget();
    auto maxChange = mMaxROTPerSec * deltaTime.value();

    // If we can reach target heading this timestep, align exactly
    if (std::abs(angleDiff.value()) <= maxChange.value())
    {
        mHeading = mPosition.forwardAzimuth(mTarget);
        mHeading = units::angle::degree_t(
            AngleUtils::normalizeLongitude360(mHeading.value()));
        mIsRotating = false;
        return;
    }

    // Rotate by maximum allowable amount toward target
    mIsRotating = true;
    if (angleDiff.value() > 0)
    {
        // Target is to starboard (right) — increase heading
        mHeading += maxChange;
    }
    else
    {
        // Target is to port (left) — decrease heading
        mHeading -= maxChange;
    }

    // Normalize heading to [0, 360)
    mHeading = units::angle::degree_t(
        AngleUtils::normalizeLongitude360(mHeading.value()));
}

}; // namespace ShipNetSimCore
