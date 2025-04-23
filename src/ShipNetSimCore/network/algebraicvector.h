/**
 * @file algebraicvector.h
 * @brief The file contains the declaration of the AlgebraicVector
 * class which is used to represent a vector in a 2D space.
 *
 * The AlgebraicVector class provides functionalities for rotating and
 * moving the vector to a target position, and calculating various
 * properties such as orientation, angle to another point, and
 * whether it is rotating.
 *
 * @author Ahmed Aredah
 * @date 10.12.2023
 */
#ifndef ALGEBRAICVECTOR_H
#define ALGEBRAICVECTOR_H

#include "../../third_party/units/units.h"
#include "point.h"
#include <QVector>

namespace ShipNetSimCore
{
class AlgebraicVector
{
public:
    struct Environment
    {
        units::temperature::celsius_t temperature =
            units::temperature::celsius_t(0.0);
        units::concentration::pptd_t salinity =
            units::concentration::pptd_t(0.0);
        units::length::meter_t waveHeight =
            units::length::meter_t(0.0);
        units::frequency::hertz_t waveFrequency =
            units::frequency::hertz_t(0.0);
        units::angular_velocity::radians_per_second_t
            waveAngularFrequency =
                units::angular_velocity::radians_per_second_t(0.0);
        units::length::meter_t waveLength =
            units::length::meter_t(0.0);
        units::velocity::meters_per_second_t windSpeed_Northward =
            units::velocity::meters_per_second_t(0.0);
        units::velocity::meters_per_second_t windSpeed_Eastward =
            units::velocity::meters_per_second_t(0.0);
        units::length::meter_t waterDepth =
            units::length::meter_t(0.0);

        units::angle::degree_t
        getEncounterAngle(units::angle::degree_t shipHeadingAzimuth)
        {
            auto waveDirectionAz =
                calculateAzimuth(windSpeed_Northward.value(),
                                 windSpeed_Eastward.value());

            return calculateEncounterAngle(shipHeadingAzimuth,
                                           waveDirectionAz);
        }

        bool checkEnvironmentValidity() const
        {
            return !(std::isnan(temperature.value())
                     || std::isnan(salinity.value())
                     || std::isnan(waveHeight.value())
                     || std::isnan(waveFrequency.value())
                     || std::isnan(waveAngularFrequency.value())
                     || std::isnan(waveLength.value())
                     || std::isnan(windSpeed_Northward.value())
                     || std::isnan(windSpeed_Eastward.value())
                     || std::isnan(waterDepth.value()));
        }

    private:
        // Function to calculate the wave encounter angle
        units::angle::degree_t calculateEncounterAngle(
            units::angle::degree_t shipHeadingAzimuth,
            units::angle::degree_t waveDirectionAzimuth) const
        {
            // Calculate the absolute difference
            units::angle::degree_t mu = units::math::abs(
                waveDirectionAzimuth - shipHeadingAzimuth);

            // Normalize to 0-360 degrees
            mu = units::angle::degree_t(
                fmod(mu.value() + 360.0, 360.0));

            // Adjust to 0-180 degrees
            if (mu.value() > 180.0)
            {
                mu = units::angle::degree_t(360.0) - mu;
            }

            return mu;
        }

        // Function to calculate the azimuth angle of any wave/wind
        units::angle::degree_t calculateAzimuth(double Northward,
                                                double Eastward) const
        {
            // Convert from radians to degrees
            units::angle::degree_t waveAzimuth =
                units::angle::radian_t(atan2(Eastward, Northward))
                    .convert<units::angle::degrees>();

            // Ensure the angle is in the range [0, 360)
            if (waveAzimuth.value() < 0.0)
            {
                waveAzimuth += units::angle::degree_t(360.0);
            }

            return waveAzimuth;
        }
    };

    /**
     * @brief Default constructor.
     *
     * Constructs a default AlgebraicVector with initial position
     * at origin.
     */
    AlgebraicVector();

    /**
     * @brief Constructs an AlgebraicVector with given start and end
     * points.
     *
     * @param startPoint The start point of the vector.
     * @param endPoint The end point of the vector.
     */
    AlgebraicVector(const Point startPoint, const Point &endPoint);

    /**
     * @brief Sets the target position and maximum rate of turn (ROT).
     *
     * @param target The target position to reach.
     * @param maxROTPerSec The maximum rate of turn in degrees per
     * second.
     */
    void setTargetAndMaxROT(const Point           &target,
                            units::angle::degree_t maxROTPerSec);

    /**
     * @brief Gets the target position.
     *
     * @return target The target position to reach.
     */
    Point getTarget();

    /**
     * @brief Moves the vector by a given distance in a given time
     * step.
     *
     * @param distance The distance to move.
     * @param timeStep The time step of the movement.
     */
    void moveByDistance(units::length::meter_t distance,
                        units::time::second_t  timeStep);

    units::angle::degree_t
    getOrientationAngleWithRespectToNorth() const;

    /**
     * @brief Gets the getOrientationWithRespectToTarget of the vector
     * in degrees.
     *
     * @return The getOrientationWithRespectToTarget of the vector in
     * degrees.
     */
    units::angle::degree_t getOrientationWithRespectToTarget() const;

    /**
     * @brief Gets the orientation of the vector as a vector.
     * @return The orientation vector <x,y>.
     */
    QVector<units::length::meter_t> getOrientationVector() const;

    /**
     * @brief Gets the current position of the vector.
     *
     * @return The current position of the vector.
     */
    Point getCurrentPosition();

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
    units::angle::degree_t angleTo(const Point &otherPoint) const;

    Environment getEnvironment() const;

    void setEnvironment(const Environment env);

private:
    Point mTargetPoint_; ///< The target position to reach.
    units::angle::degree_t mMaxROTPerSec_; ///< Maximum rate of turn
                                           ///< in degrees per second.
    Point mPosition_; ///< The current position of the vector.
    QVector<units::length::meter_t>
        mOrientation; ///< Orientation of the vector toward a target
                      ///< point.
    bool mIsRotating; ///< Indicates whether the vector is rotating.
    Environment mStateEnv;

    /**
     * @brief Sets the orientation of the vector by the end point.
     *
     * @param endPoint The end point of the vector.
     */
    void setOrientationByEndPoint(const Point &endPoint);

    /**
     * @brief Rotates the vector to the target position by the maximum
     * rate of turn in a given time step.
     *
     * @param target The target position to rotate to.
     * @param maxROTPerSec The maximum rate of turn in degrees per
     * second.
     * @param timeStep The time step of the rotation.
     */
    void rotateToTargetByMaxROT(const Point           &target,
                                units::angle::degree_t maxROTPerSec,
                                units::time::second_t  timeStep);
};
}; // namespace ShipNetSimCore

#endif // ALGEBRAICVECTOR_H
