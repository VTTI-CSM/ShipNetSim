/**
 * @file dubinspathsmoother.cpp
 * @brief Implementation of Dubins-style path smoothing for ship navigation.
 *
 * @author Ahmed Aredah
 * @date 2024
 */

#include "dubinspathsmoother.h"
#include "../utils/utils.h"
#include <cmath>
#include <QDebug>

namespace ShipNetSimCore
{

ShortestPathResult DubinsPathSmoother::smoothPath(
    const ShortestPathResult& path,
    const DubinsSmoothingConfig& config)
{
    // Need at least 3 points to have a corner to smooth
    if (path.points.size() < 3)
    {
        return path;
    }

    QVector<std::shared_ptr<GPoint>> smoothedPoints;

    // Always keep the first point
    smoothedPoints.append(path.points.first());

    // Process each corner (points 1 through n-2)
    for (qsizetype i = 1; i < path.points.size() - 1; ++i)
    {
        const auto& prevPoint = path.points[i - 1];
        const auto& cornerPoint = path.points[i];
        const auto& nextPoint = path.points[i + 1];

        // Skip smoothing for port waypoints - ships must stop there
        if (cornerPoint->isPort())
        {
            smoothedPoints.append(cornerPoint);
            continue;
        }

        // Calculate turn angle
        units::angle::degree_t turnAngle =
            calculateTurnAngle(prevPoint, cornerPoint, nextPoint);

        // Skip smoothing for very small turns
        if (units::math::abs(turnAngle) < config.minTurnAngle)
        {
            smoothedPoints.append(cornerPoint);
            continue;
        }

        // Try to smooth the corner
        QVector<std::shared_ptr<GPoint>> arcPoints =
            smoothCorner(prevPoint, cornerPoint, nextPoint, config);

        if (arcPoints.isEmpty())
        {
            // Smoothing failed, keep original corner
            smoothedPoints.append(cornerPoint);
        }
        else
        {
            // Add all arc points (they include the tangent points)
            for (const auto& arcPoint : arcPoints)
            {
                smoothedPoints.append(arcPoint);
            }
        }
    }

    // Always keep the last point
    smoothedPoints.append(path.points.last());

    // Generate lines from the smoothed points
    QVector<std::shared_ptr<GLine>> smoothedLines =
        generateLinesFromPoints(smoothedPoints);

    return ShortestPathResult{smoothedLines, smoothedPoints};
}

QVector<std::shared_ptr<GPoint>> DubinsPathSmoother::smoothCorner(
    const std::shared_ptr<GPoint>& prevPoint,
    const std::shared_ptr<GPoint>& cornerPoint,
    const std::shared_ptr<GPoint>& nextPoint,
    const DubinsSmoothingConfig& config)
{
    QVector<std::shared_ptr<GPoint>> result;

    // Calculate the turn angle
    units::angle::degree_t turnAngle =
        calculateTurnAngle(prevPoint, cornerPoint, nextPoint);

    double turnAngleDeg = turnAngle.value();
    double absTurnAngle = std::abs(turnAngleDeg);

    // Skip very small angles
    if (absTurnAngle < config.minTurnAngle.value())
    {
        return result;  // Empty result means keep original corner
    }

    // Determine turn direction: positive = left turn, negative = right turn
    bool turnLeft = turnAngleDeg > 0;

    // Calculate incoming and outgoing azimuths
    units::angle::degree_t incomingAzimuth =
        prevPoint->forwardAzimuth(*cornerPoint);
    units::angle::degree_t outgoingAzimuth =
        cornerPoint->forwardAzimuth(*nextPoint);

    // Calculate tangent distance: d = R * tan(|delta|/2)
    double halfTurnRad = (absTurnAngle / 2.0) * M_PI / 180.0;
    double tanHalfTurn = std::tan(halfTurnRad);

    // Prevent division issues for very sharp turns (near 180 degrees)
    if (tanHalfTurn > 100.0)
    {
        qDebug() << "Turn angle too sharp for smooth arc, keeping corner";
        return result;
    }

    units::length::meter_t tangentDistance =
        config.turningRadius * tanHalfTurn;

    // Check available distances
    units::length::meter_t distToPrev =
        calculateAvailableDistance(*cornerPoint, *prevPoint);
    units::length::meter_t distToNext =
        calculateAvailableDistance(*cornerPoint, *nextPoint);

    // Determine actual radius to use
    units::length::meter_t actualRadius = config.turningRadius;

    // Check if we have enough space
    if (tangentDistance > distToPrev || tangentDistance > distToNext)
    {
        if (!config.allowRadiusReduction)
        {
            // Not enough space and can't reduce radius
            qDebug() << "Insufficient space for arc, keeping corner";
            return result;
        }

        // Calculate maximum possible radius given available space
        units::length::meter_t minAvailDist =
            units::math::min(distToPrev, distToNext);

        // Solve for R: d = R * tan(|delta|/2) => R = d / tan(|delta|/2)
        // Use 90% of available distance to leave some margin
        actualRadius = (minAvailDist * 0.9) / tanHalfTurn;

        if (actualRadius < config.minRadius)
        {
            qDebug() << "Required radius" << actualRadius.value()
                     << "is below minimum" << config.minRadius.value()
                     << ", keeping corner";
            return result;
        }

        // Recalculate tangent distance with actual radius
        tangentDistance = actualRadius * tanHalfTurn;
    }

    // Calculate tangent points
    GPoint tangent1 = calculateTangentPoint(
        *cornerPoint, *prevPoint, tangentDistance);
    GPoint tangent2 = calculateTangentPoint(
        *cornerPoint, *nextPoint, tangentDistance);

    // Calculate arc center
    GPoint arcCenter = calculateArcCenter(
        *cornerPoint, incomingAzimuth, outgoingAzimuth,
        actualRadius, turnLeft);

    // Calculate start and end angles for the arc
    // The angles are measured from the center to the tangent points
    units::angle::degree_t startAngle = arcCenter.forwardAzimuth(tangent1);
    units::angle::degree_t endAngle = arcCenter.forwardAzimuth(tangent2);

    // Generate arc points
    // For a left turn (positive delta), we go counterclockwise (clockwise=false)
    // For a right turn (negative delta), we go clockwise (clockwise=true)
    bool clockwise = !turnLeft;

    QVector<std::shared_ptr<GPoint>> arcPoints = generateArcPoints(
        arcCenter, actualRadius, startAngle, endAngle,
        config.arcStepSize, clockwise);

    // Return the arc points (includes tangent points at start/end)
    return arcPoints;
}

units::angle::degree_t DubinsPathSmoother::calculateTurnAngle(
    const std::shared_ptr<GPoint>& prevPoint,
    const std::shared_ptr<GPoint>& cornerPoint,
    const std::shared_ptr<GPoint>& nextPoint)
{
    // Calculate incoming azimuth (direction from prev to corner)
    units::angle::degree_t inAzimuth =
        prevPoint->forwardAzimuth(*cornerPoint);

    // Calculate outgoing azimuth (direction from corner to next)
    units::angle::degree_t outAzimuth =
        cornerPoint->forwardAzimuth(*nextPoint);

    // Turn angle is the change in heading
    double delta = outAzimuth.value() - inAzimuth.value();

    // Normalize to [-180, 180]
    delta = AngleUtils::normalizeAngleDifference(delta);

    return units::angle::degree_t(delta);
}

GPoint DubinsPathSmoother::calculateArcCenter(
    const GPoint& cornerPoint,
    units::angle::degree_t incomingAzimuth,
    units::angle::degree_t outgoingAzimuth,
    units::length::meter_t radius,
    bool turnLeft)
{
    // Calculate the bisector angle (average of incoming and outgoing)
    double inDeg = incomingAzimuth.value();
    double outDeg = outgoingAzimuth.value();

    // Handle wrap-around for averaging
    double diff = AngleUtils::normalizeAngleDifference(outDeg - inDeg);
    double bisectorAngle = inDeg + diff / 2.0;

    // The arc center is perpendicular to the bisector
    // For a left turn, center is to the left (bisector - 90)
    // For a right turn, center is to the right (bisector + 90)
    double centerAngle;
    if (turnLeft)
    {
        centerAngle = bisectorAngle - 90.0;
    }
    else
    {
        centerAngle = bisectorAngle + 90.0;
    }

    // Normalize the angle
    centerAngle = AngleUtils::normalizeLongitude360(centerAngle);

    // Calculate the distance from corner to arc center
    // For an arc tangent to both segments, this is R / cos(delta/2)
    double halfDelta = std::abs(diff) / 2.0 * M_PI / 180.0;
    double cosHalfDelta = std::cos(halfDelta);

    // Prevent division by zero for 180-degree turns
    if (cosHalfDelta < 0.01)
    {
        cosHalfDelta = 0.01;
    }

    units::length::meter_t centerDist = radius / cosHalfDelta;

    // Calculate the center point
    return cornerPoint.pointAtDistanceAndHeading(
        centerDist, units::angle::degree_t(centerAngle));
}

QVector<std::shared_ptr<GPoint>> DubinsPathSmoother::generateArcPoints(
    const GPoint& center,
    units::length::meter_t radius,
    units::angle::degree_t startAngle,
    units::angle::degree_t endAngle,
    units::length::meter_t stepSize,
    bool clockwise)
{
    QVector<std::shared_ptr<GPoint>> points;

    double startDeg = startAngle.value();
    double endDeg = endAngle.value();

    // Calculate the arc sweep angle
    // Normalize both angles to [0, 360) first to avoid antimeridian boundary issues
    double normStart = AngleUtils::normalizeLongitude360(startDeg);
    double normEnd = AngleUtils::normalizeLongitude360(endDeg);

    double sweep;
    if (clockwise)
    {
        sweep = normStart - normEnd;
        if (sweep < 0) sweep += 360.0;
    }
    else
    {
        sweep = normEnd - normStart;
        if (sweep < 0) sweep += 360.0;
    }

    // Handle near-zero sweep that could be 360 due to floating-point precision
    if (sweep > 359.0 && std::abs(normStart - normEnd) < 1.0)
    {
        sweep = std::abs(normStart - normEnd);
    }

    // Calculate arc length
    double arcLength = radius.value() * sweep * M_PI / 180.0;

    // Calculate number of points
    int numPoints = std::max(3, static_cast<int>(arcLength / stepSize.value()));

    // Generate points along the arc
    for (int i = 0; i <= numPoints; ++i)
    {
        double fraction = static_cast<double>(i) / numPoints;
        double angle;

        if (clockwise)
        {
            angle = startDeg - fraction * sweep;
        }
        else
        {
            angle = startDeg + fraction * sweep;
        }

        // Normalize angle to [0, 360)
        angle = AngleUtils::normalizeLongitude360(angle);

        // Calculate point at this angle from center
        GPoint arcPoint = center.pointAtDistanceAndHeading(
            radius, units::angle::degree_t(angle));

        points.append(std::make_shared<GPoint>(arcPoint));
    }

    return points;
}

GPoint DubinsPathSmoother::calculateTangentPoint(
    const GPoint& cornerPoint,
    const GPoint& otherPoint,
    units::length::meter_t tangentDistance)
{
    // Calculate azimuth from corner toward the other point
    units::angle::degree_t azimuth = cornerPoint.forwardAzimuth(otherPoint);

    // The tangent point is at tangentDistance along this direction
    return cornerPoint.pointAtDistanceAndHeading(tangentDistance, azimuth);
}

units::length::meter_t DubinsPathSmoother::calculateAvailableDistance(
    const GPoint& cornerPoint,
    const GPoint& adjacentPoint)
{
    return cornerPoint.distance(adjacentPoint);
}

QVector<std::shared_ptr<GLine>> DubinsPathSmoother::generateLinesFromPoints(
    const QVector<std::shared_ptr<GPoint>>& points)
{
    QVector<std::shared_ptr<GLine>> lines;

    for (qsizetype i = 0; i < points.size() - 1; ++i)
    {
        auto line = std::make_shared<GLine>(points[i], points[i + 1]);
        lines.append(line);
    }

    return lines;
}

}  // namespace ShipNetSimCore
