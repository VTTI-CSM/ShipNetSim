/**
 * @file dubinspathsmoother.h
 * @brief Dubins-style path smoothing for ship navigation.
 *
 * This file contains the DubinsPathSmoother class which transforms sharp
 * waypoint-to-waypoint paths into smooth arcs respecting the ship's minimum
 * turning radius. The smoothed paths are discretized into closely-spaced
 * waypoints for seamless integration with the existing navigation system.
 *
 * @author Ahmed Aredah
 * @date 2024
 */

#ifndef DUBINSPATHSMOOTHER_H
#define DUBINSPATHSMOOTHER_H

#include "../../third_party/units/units.h"
#include "../export.h"
#include "gline.h"
#include "gpoint.h"
#include "optimizedvisibilitygraph.h"
#include <QVector>
#include <memory>

namespace ShipNetSimCore
{

/**
 * @struct DubinsSmoothingConfig
 * @brief Configuration parameters for Dubins path smoothing.
 *
 * This struct holds all the parameters needed to configure the path
 * smoothing algorithm, including the turning radius, arc discretization
 * step size, and thresholds for when to apply smoothing.
 */
struct SHIPNETSIM_EXPORT DubinsSmoothingConfig
{
    /// The ship's minimum turning radius based on waterline length and rudder angle.
    units::length::meter_t turningRadius = units::length::meter_t(100.0);

    /// Distance between consecutive waypoints along the arc (meters).
    /// Smaller values produce smoother paths but more waypoints.
    units::length::meter_t arcStepSize = units::length::meter_t(5.0);

    /// Minimum turn angle to trigger smoothing (degrees).
    /// Turns smaller than this are left as sharp corners.
    units::angle::degree_t minTurnAngle = units::angle::degree_t(5.0);

    /// Whether to allow reducing the turning radius when there isn't
    /// enough space for the full radius.
    bool allowRadiusReduction = true;

    /// Minimum allowed turning radius when reduction is enabled (meters).
    /// This prevents overly tight turns that may be unrealistic.
    units::length::meter_t minRadius = units::length::meter_t(50.0);
};

/**
 * @class DubinsPathSmoother
 * @brief Transforms sharp waypoint paths into smooth arcs.
 *
 * The DubinsPathSmoother class provides functionality to smooth navigation
 * paths by replacing sharp corners with smooth arcs that respect the ship's
 * minimum turning radius. The algorithm:
 *
 * 1. For each corner waypoint, calculates the turn angle
 * 2. If the turn exceeds the threshold, computes tangent points
 * 3. Generates an arc between tangent points at the specified step size
 * 4. Replaces the corner with the arc waypoints
 *
 * The resulting path integrates seamlessly with the ship's navigation system
 * and prevents rate-of-turn limitation issues that occur with sharp turns.
 */
class SHIPNETSIM_EXPORT DubinsPathSmoother
{
public:
    /**
     * @brief Smooth an entire path by replacing sharp corners with arcs.
     *
     * This is the main entry point for path smoothing. It processes each
     * corner in the path and replaces sharp turns with smooth arcs.
     *
     * @param path The original path from pathfinding
     * @param config Smoothing configuration parameters
     * @return Smoothed path with arc waypoints
     */
    static ShortestPathResult smoothPath(
        const ShortestPathResult& path,
        const DubinsSmoothingConfig& config);

    /**
     * @brief Smooth a single corner by generating arc waypoints.
     *
     * Given three consecutive waypoints forming a corner, this function
     * calculates the smooth arc that replaces the corner while respecting
     * the turning radius constraint.
     *
     * @param prevPoint Point before the corner
     * @param cornerPoint The corner point to be smoothed
     * @param nextPoint Point after the corner
     * @param config Smoothing configuration
     * @return Vector of points forming the smoothed arc (includes tangent points)
     */
    static QVector<std::shared_ptr<GPoint>> smoothCorner(
        const std::shared_ptr<GPoint>& prevPoint,
        const std::shared_ptr<GPoint>& cornerPoint,
        const std::shared_ptr<GPoint>& nextPoint,
        const DubinsSmoothingConfig& config);

    /**
     * @brief Calculate the turn angle at a corner.
     *
     * Computes the angle change between the incoming segment (prev->corner)
     * and the outgoing segment (corner->next). Positive values indicate
     * left turns, negative values indicate right turns.
     *
     * @param prevPoint Point before the corner
     * @param cornerPoint The corner point
     * @param nextPoint Point after the corner
     * @return Turn angle in degrees [-180, 180]
     */
    static units::angle::degree_t calculateTurnAngle(
        const std::shared_ptr<GPoint>& prevPoint,
        const std::shared_ptr<GPoint>& cornerPoint,
        const std::shared_ptr<GPoint>& nextPoint);

private:
    /**
     * @brief Calculate the center point of the turning arc.
     *
     * The arc center is perpendicular to the corner's angle bisector,
     * at a distance equal to the turning radius.
     *
     * @param cornerPoint The corner being smoothed
     * @param incomingAzimuth Azimuth from prev to corner
     * @param outgoingAzimuth Azimuth from corner to next
     * @param radius Turning radius
     * @param turnLeft True if turning left, false if turning right
     * @return The center point of the arc
     */
    static GPoint calculateArcCenter(
        const GPoint& cornerPoint,
        units::angle::degree_t incomingAzimuth,
        units::angle::degree_t outgoingAzimuth,
        units::length::meter_t radius,
        bool turnLeft);

    /**
     * @brief Generate evenly-spaced points along an arc.
     *
     * Creates waypoints along the circular arc from the start angle to
     * the end angle at the specified step size.
     *
     * @param center Arc center point
     * @param radius Arc radius
     * @param startAngle Starting azimuth of the arc (degrees)
     * @param endAngle Ending azimuth of the arc (degrees)
     * @param stepSize Distance between consecutive points (meters)
     * @param clockwise Direction of arc traversal
     * @return Vector of points along the arc
     */
    static QVector<std::shared_ptr<GPoint>> generateArcPoints(
        const GPoint& center,
        units::length::meter_t radius,
        units::angle::degree_t startAngle,
        units::angle::degree_t endAngle,
        units::length::meter_t stepSize,
        bool clockwise);

    /**
     * @brief Calculate the tangent point where the arc meets a straight segment.
     *
     * The tangent point is where the smooth arc transitions to/from the
     * straight path segment. It lies at a distance of R*tan(delta/2) from
     * the corner along the segment.
     *
     * @param cornerPoint The corner point
     * @param otherPoint The other point of the segment (prev or next)
     * @param tangentDistance Distance from corner to tangent point
     * @return The tangent point
     */
    static GPoint calculateTangentPoint(
        const GPoint& cornerPoint,
        const GPoint& otherPoint,
        units::length::meter_t tangentDistance);

    /**
     * @brief Calculate the available distance from corner to adjacent point.
     *
     * Used to check if there's enough space for the full turning radius,
     * and to compute reduced radius when necessary.
     *
     * @param cornerPoint The corner point
     * @param adjacentPoint The adjacent point (prev or next)
     * @return Geodesic distance between the points
     */
    static units::length::meter_t calculateAvailableDistance(
        const GPoint& cornerPoint,
        const GPoint& adjacentPoint);

    /**
     * @brief Generate lines connecting consecutive path points.
     *
     * @param points Vector of path points
     * @return Vector of lines connecting consecutive points
     */
    static QVector<std::shared_ptr<GLine>> generateLinesFromPoints(
        const QVector<std::shared_ptr<GPoint>>& points);
};

}  // namespace ShipNetSimCore

#endif  // DUBINSPATHSMOOTHER_H
