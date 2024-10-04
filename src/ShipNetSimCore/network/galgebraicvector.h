#ifndef GALGEBRAICVECTOR_H
#define GALGEBRAICVECTOR_H

#include <QVector>
#include "gline.h"
#include "gpoint.h"
#include "algebraicvector.h"
#include "../../third_party/units/units.h"

namespace ShipNetSimCore
{
class GAlgebraicVector
{
public:
    /**
     * @brief Default constructor.
     *
     * Constructs a default geodetic AlgebraicVector with initial position
     * at origin.
     */
    GAlgebraicVector();

    /**
     * @brief Constructs a geodetic AlgebraicVector with given
     * start and end points.
     *
     * @param startPoint The start point of the vector.
     * @param endPoint The end point of the vector.
     */
    GAlgebraicVector(const GPoint startPoint, const GPoint& endPoint);

    /**
     * @brief Sets the target position and maximum rate of turn (ROT).
     *
     * @param target The target position to reach.
     * @param maxROTPerSec The maximum rate of turn in degrees per second.
     */
    void setTargetAndMaxROT(const GPoint& target,
                            units::angle::degree_t maxROTPerSec);

    /**
     * @brief Gets the target position.
     *
     * @return target The target position to reach.
     */
    GPoint getTarget();

    /**
     * @brief Moves the vector by a given distance in a given time step.
     *
     * @param distance The distance to move.
     * @param timeStep The time step of the movement.
     */
    void moveByDistance(units::length::meter_t distance,
                        units::time::second_t timeStep);

    units::angle::degree_t getVectorAzimuth() const;

    /**
     * @brief Gets the orientation of the vector in degrees.
     *
     * @return The orientation of the vector in degrees.
     */
    units::angle::degree_t getAngleToTarget() const;

    /**
     * @brief Gets the current position of the vector.
     *
     * @return The current position of the vector.
     */
    GPoint getCurrentPosition() const;

    /**
     * @brief Sets the current position of the vector.
     * @param newPosition the new position as a result of cyber attack.
     */
    void setCurrentPosition(GPoint newPosition);

    /**
     * @brief Checks whether the vector is rotating.
     *
     * @return True if the vector is rotating, false otherwise.
     */
    bool isRotating() const;

    /**
     * @brief Calculates the angle to another point from the vector's
     * current position.
     *
     * @param otherPoint The point to calculate the angle to.
     * @return The angle to the other point in degrees.
     */
    units::angle::degree_t angleTo(const GPoint& otherPoint) const;

    AlgebraicVector::Environment getEnvironment() const;

    void setEnvironment(const AlgebraicVector::Environment env);

    void setGPSUpdateState(bool isUpdating);

    void restoreLatestCorrectPosition();

private:
    bool mIsUpdating = true;  ///< Is the GPS updating.
    GLine mCurrentHeadingVector_backup; ///< this is a backup vector for attacks.
    GLine mCurrentHeadingVector; ///< The vector that holds the direction of movement.
    units::angle::degree_t
        mMaxROTPerSec_;  ///< Maximum rate of turn in degrees per second.

    units::angle::degree_t mCurrentCourse; ///< Azimuth of the vector
    bool mIsRotating;  ///< Indicates whether the vector is rotating.
    AlgebraicVector::Environment mStateEnv;

    /**
     * @brief Sets the orientation of the vector by the end point.
     *
     * @param endPoint The end point of the vector.
     */
    void setHeadingByEndPoint(const GPoint& endPoint);

    /**
     * @brief Rotates the vector to the target position by the maximum
     * rate of turn in a given time step.
     *
     * @param target The target position to rotate to.
     * @param maxROTPerSec The maximum rate of turn in degrees per second.
     * @param timeStep The time step of the rotation.
     */
    void rotateToTargetByMaxROT(const GPoint& target,
                                units::angle::degree_t maxROTPerSec,
                                units::time::second_t timeStep);
};
};
#endif // GALGEBRAICVECTOR_H
