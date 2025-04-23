/**
 * @file algebraicvector.cpp
 *
 * @brief Implementation of the AlgebraicVector class.
 *
 * This file contains the implementation of the AlgebraicVector class,
 * which is used to represent a vector in a 2D space with additional
 * functionalities such as rotation and translation.
 *
 * @author Ahmed Aredah
 * @date 10.12.2023
 */

#include "algebraicvector.h" // Include AlgebraicVector class definition
#include <QDebug>            // Include debugging utilities
#include <cmath>             // Include mathematical functions

namespace ShipNetSimCore
{
// Default constructor
AlgebraicVector::AlgebraicVector()
{
    // Initialize position with default value (0.0, 0.0)
    mPosition_ = Point(units::length::meter_t(0.0),
                       units::length::meter_t(0.0));
    // Initialize orientation with default value (1, 0)
    // This represents a vector pointing along the x-axis.
    mOrientation.append(units::length::meter_t(1));
    mOrientation.append(units::length::meter_t(0));
    // Initialize rotation status to false
    mIsRotating = false;
}

// Constructor with start and end points
AlgebraicVector::AlgebraicVector(const Point  startPoint,
                                 const Point &endPoint)
{
    // Set position to startPoint
    mPosition_ = Point(startPoint.x(), startPoint.y());
    // Set orientation based on endPoint
    setOrientationByEndPoint(endPoint);
    // Initialize rotation status to false
    mIsRotating = false;
}

// Check if the vector is rotating
bool AlgebraicVector::isRotating() const
{
    return mIsRotating; // Return rotation status
}

// Get getOrientationWithRespectToTarget angle in degrees
units::angle::degree_t
AlgebraicVector::getOrientationWithRespectToTarget() const
{
    // Calculate orientation angle based on orientation vector
    double x = mOrientation[0].value();
    double y = mOrientation[1].value();
    return units::angle::degree_t(std::atan2(y, x) * 180.0 / M_PI);
}

QVector<units::length::meter_t>
AlgebraicVector::getOrientationVector() const
{
    return mOrientation;
}

// Set target point and maximum rate of turn
void AlgebraicVector::setTargetAndMaxROT(
    const Point &target, units::angle::degree_t maxROTPerSec)
{
    // Set target point
    mTargetPoint_ = target;
    // Set maximum rate of turn
    mMaxROTPerSec_ = maxROTPerSec;
}

Point AlgebraicVector::getTarget()
{
    return mTargetPoint_;
}

// Set orientation based on end point
void AlgebraicVector::setOrientationByEndPoint(const Point &endPoint)
{
    // Calculate direction vector components
    double dx = endPoint.x().value() - mPosition_.x().value();
    double dy = endPoint.y().value() - mPosition_.y().value();

    // Calculate magnitude of direction vector
    double magnitude = std::sqrt(dx * dx + dy * dy);

    // Handle zero magnitude vector as per application requirements
    if (magnitude == 0)
        return;

    if (mOrientation.size() < 2)
    {
        mOrientation.resize(2);
    }
    // Normalize direction vector
    mOrientation[0] = units::length::meter_t(dx / magnitude);
    mOrientation[1] = units::length::meter_t(dy / magnitude);
}

units::angle::degree_t
AlgebraicVector::getOrientationAngleWithRespectToNorth() const
{
    if (mOrientation.size() < 2)
    {
        // Handle error or uninitialized state as appropriate
        return units::angle::degree_t(0.0);
    }

    // atan2 returns the angle between the x-axis and the vector (dx,
    // dy), but we need the angle with respect to the y-axis, so we
    // swap dx and dy. Also, atan2 expects dy first, then dx, to
    // compute the angle from the x-axis. To get the angle from the
    // north, we essentially calculate atan2(dx, dy) but then we need
    // to convert radians to degrees and adjust the angle so that
    // north is 0/360 degrees and it increases clockwise.
    double dy = mOrientation[0].value(); // x (swaping)
    double dx = mOrientation[1].value(); // y
    double radians =
        std::atan2(dx, dy); // Swap dx and dy for y-axis orientation

    // Convert radians to degrees
    double degrees = radians * (180.0 / M_PI);

    // Adjust degrees to make north 0/360 degrees
    // Since atan2 returns [-180, 180], add 360 to negative angles to
    // normalize
    if (degrees < 0)
    {
        degrees += 360;
    }

    return units::angle::degree_t(degrees);
}

// Move by a specified distance
void AlgebraicVector::moveByDistance(units::length::meter_t distance,
                                     units::time::second_t  timeStep)
{
    // Perform rotation to the target by max ROT
    rotateToTargetByMaxROT(mTargetPoint_, mMaxROTPerSec_, timeStep);

    // Perform translation
    double x = mOrientation[0].value();
    double y = mOrientation[1].value();

    // The orientation is already normalized, so we can directly use
    // it.
    mPosition_.setX(mPosition_.x() + x * distance);
    mPosition_.setY(mPosition_.y() + y * distance);
}

// Get current position
Point AlgebraicVector::getCurrentPosition()
{
    return mPosition_; // Return current position
}

// Rotate to target point within maximum allowable rate of turn
void AlgebraicVector::rotateToTargetByMaxROT(
    const Point &target, units::angle::degree_t maxROTPerSec,
    units::time::second_t deltaTime)
{
    // Calculate target orientation angle
    double dx = target.x().value() - mPosition_.x().value();
    double dy = target.y().value() - mPosition_.y().value();
    auto   targetOrientation =
        units::angle::degree_t(std::atan2(dy, dx) * 180.0 / M_PI);

    // Calculate difference between current and target orientations
    auto currentOrientation = getOrientationWithRespectToTarget();
    auto diff               = targetOrientation - currentOrientation;

    // Normalize difference to be within [-180, 180] degrees
    while (diff.value() < -180)
        diff += units::angle::degree_t(360);
    while (diff.value() > 180)
        diff -= units::angle::degree_t(360);

    // Calculate allowable change in orientation
    auto orientationChange = maxROTPerSec * deltaTime.value();

    // Check if difference is within allowable change
    if (std::abs(diff.value()) < orientationChange.value())
    {
        setOrientationByEndPoint(target);
        mIsRotating = false; // No longer rotating
        return;
    }

    // Otherwise, rotate by maximum allowable amount
    mIsRotating = true; // Still rotating
    if (diff.value() > 0)
    {
        currentOrientation += orientationChange;
    }
    else
    {
        currentOrientation -= orientationChange;
    }

    // Update orientation vector
    double radian   = currentOrientation.value() * M_PI / 180.0;
    mOrientation[0] = units::length::meter_t(std::cos(radian));
    mOrientation[1] = units::length::meter_t(std::sin(radian));
}

// Calculate angle to another point
units::angle::degree_t
AlgebraicVector::angleTo(const Point &otherPoint) const
{
    // Calculate direction vector components
    double dx = otherPoint.x().value() - mPosition_.x().value();
    double dy = otherPoint.y().value() - mPosition_.y().value();

    // Handle case where otherPoint is same as current position
    double magnitude = std::sqrt(dx * dx + dy * dy);
    if (magnitude == 0)
        return units::angle::degree_t(0);

    // Calculate angles between direction vector and current
    // orientation
    double targetAngle = std::atan2(dy, dx) * 180.0 / M_PI;
    double currentAngle =
        std::atan2(mOrientation[1].value(), mOrientation[0].value())
        * 180.0 / M_PI;

    double angleDifference = targetAngle - currentAngle;

    // Normalize angle to be within [-180, 180] degrees
    while (angleDifference > 180.0)
        angleDifference -= 360.0;
    while (angleDifference < -180.0)
        angleDifference += 360.0;

    return units::angle::degree_t(angleDifference);
}

AlgebraicVector::Environment AlgebraicVector::getEnvironment() const
{
    return mStateEnv;
}

void AlgebraicVector::setEnvironment(const Environment env)
{
    mStateEnv = env;
}
}; // namespace ShipNetSimCore
