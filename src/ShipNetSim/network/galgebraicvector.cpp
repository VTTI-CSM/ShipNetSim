/**
 * @file galgebraicvector.cpp
 *
 * @brief Implementation of the GAlgebraicVector class.
 *
 * This file contains the implementation of the GAlgebraicVector class,
 * which is used to represent a vector in a 3D space with additional
 * functionalities such as rotation and translation.
 *
 * @author Ahmed Aredah
 * @date 10.12.2023
 */

#include "galgebraicvector.h"  // Include GAlgebraicVector class definition
#include <cmath>  // Include mathematical functions
#include <QDebug>  // Include debugging utilities
#include "gline.h"
#include "gpoint.h"

// Default constructor
GAlgebraicVector::GAlgebraicVector()
{
    // Initialize position with default value (0.0, 0.0)
    // Initialize orientation with default value
    // This represents a vector pointing along the y-axis.
    std::shared_ptr<GPoint> currentPos =
        std::make_shared<GPoint>(GPoint(units::angle::degree_t(0.0),
                                        units::angle::degree_t(0.0)));
    std::shared_ptr<GPoint> targetPos =
        std::make_shared<GPoint>(GPoint(units::angle::degree_t(1.0),
                                        units::angle::degree_t(0.0)));

    mCurrentTargetVector = GLine(currentPos, targetPos);

    mCurrentHeading = units::angle::degree_t(0.0);
    // Initialize rotation status to false
    mIsRotating = false;
}

// Constructor with start and end points
GAlgebraicVector::GAlgebraicVector(const GPoint startPoint,
                                   const GPoint& endPoint)
{
    // Set position to startPoint
    std::shared_ptr<GPoint> currentPos =
        std::make_shared<GPoint>(startPoint);
    std::shared_ptr<GPoint> targetPos =
        std::make_shared<GPoint>(endPoint);

    mCurrentTargetVector = GLine(currentPos, targetPos);

    mCurrentHeading = mCurrentTargetVector.getHeading();

    // Initialize rotation status to false
    mIsRotating = false;
}

// Check if the vector is rotating
bool GAlgebraicVector::isRotating() const
{
    return mIsRotating;  // Return rotation status
}

// Get getAngleToTarget angle in degrees
units::angle::degree_t GAlgebraicVector::getAngleToTarget() const
{
    return mCurrentHeading - mCurrentTargetVector.getHeading();
}


// Set target point and maximum rate of turn
void GAlgebraicVector::setTargetAndMaxROT(const GPoint& target,
                                         units::angle::degree_t maxROTPerSec)
{
    // Set target point
    std::shared_ptr<GPoint> targetPos =
        std::make_shared<GPoint>(target);
    mCurrentTargetVector.setEndPoint(targetPos);

    // Set maximum rate of turn
    mMaxROTPerSec_ = maxROTPerSec;
}

GPoint GAlgebraicVector::getTarget()
{
    return *mCurrentTargetVector.endPoint();
}

// Set orientation based on end point
void GAlgebraicVector::setHeadingByEndPoint(const GPoint& endPoint)
{
    std::shared_ptr<GPoint> ePoint = std::make_shared<GPoint>(endPoint);
    mCurrentTargetVector.setEndPoint(ePoint);

    // Set orientation based on endPoint
    mCurrentHeading = mCurrentTargetVector.getHeading();
}

units::angle::degree_t GAlgebraicVector::
    getOrientationAngleWithRespectToNorth() const
{
    return mCurrentTargetVector.getHeading();
}

// Move by a specified distance
void GAlgebraicVector::moveByDistance(units::length::meter_t distance,
                                     units::time::second_t timeStep)
{
    // Perform rotation to the target by max ROT
    rotateToTargetByMaxROT(*mCurrentTargetVector.endPoint(), mMaxROTPerSec_, timeStep);

    std::shared_ptr<GPoint> newCurrentPos =
        std::make_shared<GPoint>(
        mCurrentTargetVector.startPoint()->
            pointAtDistanceAndHeading(distance,
                                      mCurrentHeading));
}

// Get current position
GPoint GAlgebraicVector::getCurrentPosition()
{
    return *mCurrentTargetVector.startPoint();  // Return current position
}

// Rotate to target point within maximum allowable rate of turn
void GAlgebraicVector::rotateToTargetByMaxROT(
    const GPoint& target,
    units::angle::degree_t maxROTPerSec,
    units::time::second_t deltaTime)
{
    // Calculate difference between current and target orientations
    auto angleDiff = getAngleToTarget();

    // Calculate allowable change in orientation
    auto maxOrientationChange = maxROTPerSec * deltaTime.value();

    // Check if difference is within allowable change
    if (std::abs(angleDiff.value()) < maxOrientationChange.value())
    {
        setHeadingByEndPoint(target);
        mIsRotating = false;  // No longer rotating
        return;
    }

    // Otherwise, rotate by maximum allowable amount
    mIsRotating = true;  // Still rotating
    if (angleDiff.value() > 0)
    {
        // if angle between current heading and target heading is +ve,
        // reduce it so we reach the target
        mCurrentHeading -= maxOrientationChange;
    }
    else
    {
        // if angle between current heading and target heading is -ve,
        // increase it so we reach the target
        mCurrentHeading += maxOrientationChange;
    }
}

// Calculate angle to another point
units::angle::degree_t GAlgebraicVector::angleTo(const GPoint& otherPoint) const
{
    GLine l = GLine(mCurrentTargetVector.startPoint(),
                    std::make_shared<GPoint>(otherPoint));
    return mCurrentTargetVector.angleWith(l);
}

AlgebraicVector::Environment GAlgebraicVector::getEnvironment() const
{
    return mStateEnv;
}

void GAlgebraicVector::setEnvironment(const AlgebraicVector::Environment env)
{
    mStateEnv = env;
}
