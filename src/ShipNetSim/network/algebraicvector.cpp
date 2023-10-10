#include "algebraicvector.h"
#include <cmath>
#include <QDebug>

AlgebraicVector::AlgebraicVector()
{
    // Initialize with a default position
    mPosition_ = Point(units::length::meter_t(0.0),
                       units::length::meter_t(0.0));
    // Initialize with a default orientation (along the x-axis)
    // Already normalized.
    mOrientation.append(units::length::meter_t(1));
    mOrientation.append(units::length::meter_t(0));
    mIsRotating = false;  // Initialize mIsRotating to false
}

AlgebraicVector::AlgebraicVector(const Point startPoint, const Point& endPoint)
{
    mPosition_ = startPoint;
    setOrientationByEndPoint(endPoint);
    mIsRotating = false;  // Initialize mIsRotating to false
}

bool AlgebraicVector::isRotating() const { return mIsRotating; }

//AlgebraicVector::AlgebraicVector(const Point startPoint)
//{
//    mPosition_ = startPoint;
//    // Initialize with a default orientation (along the x-axis)
//    // Already normalized.
//    mOrientation.append(units::length::meter_t(1));
//    mOrientation.append(units::length::meter_t(0));
//}

units::angle::degree_t AlgebraicVector::orientation() const
{
    double x = mOrientation[0].value();
    double y = mOrientation[1].value();
    return units::angle::degree_t(std::atan2(y, x) * 180.0 / M_PI);
}

//void AlgebraicVector::changeHeading(units::angle::degree_t deltaThetaDegrees)
//{
//    double deltaThetaRadians = deltaThetaDegrees.value() * M_PI / 180.0;

//    units::length::meter_t newX =
//        mOrientation[0] * std::cos(deltaThetaRadians) -
//        mOrientation[1] * std::sin(deltaThetaRadians);
//    units::length::meter_t newY =
//        mOrientation[0] * std::sin(deltaThetaRadians) +
//        mOrientation[1] * std::cos(deltaThetaRadians);

//    // Normalize the new orientation
//    double magnitude =
//        std::sqrt(newX.value() * newX.value() +
//                  newY.value() * newY.value());
//    mOrientation[0] = newX / magnitude;
//    mOrientation[1] = newY / magnitude;
//}

void AlgebraicVector::setTargetAndMaxROT(const Point& target,
                                         units::angle::degree_t maxROTPerSec)
{
    mTargetPoint_ = target;
    mMaxROTPerSec_ = maxROTPerSec;
}

void AlgebraicVector::setOrientationByEndPoint(const Point& endPoint)
{
    double dx = endPoint.x().value() - mPosition_.x().value();
    double dy = endPoint.y().value() - mPosition_.y().value();

    // Normalize
    double magnitude = std::sqrt(dx * dx + dy * dy);

    if (magnitude == 0) {
        // Handle zero magnitude vector as per your application's requirements.
        return;
    }

    mOrientation[0] = units::length::meter_t(dx / magnitude);
    mOrientation[1] = units::length::meter_t(dy / magnitude);
}


void AlgebraicVector::moveByDistance(units::length::meter_t distance,
                                     units::time::second_t timeStep)
{
    // Perform rotation to the target by max ROT
    rotateToTargetByMaxROT(mTargetPoint_, mMaxROTPerSec_, timeStep);

    // Perform translation
    double x = mOrientation[0].value();
    double y = mOrientation[1].value();

    // The orientation is already normalized, so we can directly use it.
    mPosition_.setX(mPosition_.x() + x * distance);
    mPosition_.setY(mPosition_.y() + y * distance);
}

Point AlgebraicVector::getCurrentPosition()
{
    return mPosition_;
}

void AlgebraicVector::
    rotateToTargetByMaxROT(const Point& target,
                           units::angle::degree_t maxROTPerSec,
                           units::time::second_t deltaTime)
{
    // Get the target orientation
    double dx = target.x().value() - mPosition_.x().value();
    double dy = target.y().value() - mPosition_.y().value();
    auto targetOrientation =
        units::angle::degree_t(std::atan2(dy, dx) * 180.0 / M_PI);

    // Find the difference between the current and target orientation
    auto currentOrientation = orientation();
    auto diff = targetOrientation - currentOrientation;

    // Keep the difference between -180 and 180 degrees
    while (diff.value() < -180) diff += units::angle::degree_t(360);
    while (diff.value() > 180) diff -= units::angle::degree_t(360);

    // Calculate the orientation change allowed by max ROT
    auto orientationChange = maxROTPerSec * deltaTime.value();

    // If the difference in orientation is less
    // than the allowable change, set to the target orientation
    if (std::abs(diff.value()) < orientationChange.value())
    {
        setOrientationByEndPoint(target);
        // Not rotating as the target orientation is achieved
        mIsRotating = false;
        return;
    }

    // Otherwise, rotate by the maximum allowable amount
    // in the direction of the target orientation
    // --> Still rotating as the target orientation is not yet achieved
     mIsRotating = true;
    if (diff.value() > 0)
    {
        currentOrientation += orientationChange;
    }
    else
    {
        currentOrientation -= orientationChange;
    }

    // Set the new orientation
    double radian = currentOrientation.value() * M_PI / 180.0;
    mOrientation[0] = units::length::meter_t(std::cos(radian));
    mOrientation[1] = units::length::meter_t(std::sin(radian));
}

units::angle::degree_t AlgebraicVector::angleTo(const Point& otherPoint) const
{
    // Calculate the direction vector from the
    // current position to the other point
    double dx = otherPoint.x().value() - mPosition_.x().value();
    double dy = otherPoint.y().value() - mPosition_.y().value();

    // Handle case when otherPoint is the same as mPosition_
    double magnitude = std::sqrt(dx * dx + dy * dy);
    if (magnitude == 0) return units::angle::degree_t(0);

    // Calculate the angles between the direction
    // vector and the current orientation vector
    double targetAngle = std::atan2(dy, dx) * 180.0 / M_PI;
    double currentAngle = std::atan2(mOrientation[1].value(),
                                     mOrientation[0].value()) * 180.0 / M_PI;

    double angleDifference = targetAngle - currentAngle;

    // Normalize angle to be between -180 and 180 degrees
    while (angleDifference > 180.0) angleDifference -= 360.0;
    while (angleDifference < -180.0) angleDifference += 360.0;

    return units::angle::degree_t(angleDifference);
}


