/**
 * @file gline.h
 * @brief Geodetic Line Segment Implementation.
 *
 * This file contains the GLine class which represents a geodetic line segment
 * on the WGS84 ellipsoid. Unlike planar geometry, geodetic lines follow the
 * curvature of the Earth and use geodesic calculations for accurate distance
 * and orientation computations.
 *
 * Key features:
 * - Accurate geodesic distance calculations using GeographicLib
 * - Spherical geometry for orientation and point-to-line operations
 * - Works correctly everywhere on Earth, including polar regions
 * - GDAL/OGR integration for spatial operations
 *
 * @author Ahmed Aredah
 * @date 10.12.2023
 */

#ifndef GLINE_H
#define GLINE_H

#include "../export.h"
#include "basegeometry.h"
#include "gpoint.h"
#include "line.h"

namespace ShipNetSimCore
{

class GAlgebraicVector;  // Forward declaration

/**
 * @class GLine
 * @brief Represents a geodetic line segment between two geographic points.
 *
 * GLine models a shortest-path (geodesic) line segment on the Earth's surface.
 * All geometric calculations use geodesic mathematics rather than planar
 * approximations, ensuring accuracy for global-scale applications.
 */
class SHIPNETSIM_EXPORT GLine : public BaseGeometry
{
    // =========================================================================
    // Member Variables
    // =========================================================================
private:
    std::shared_ptr<GPoint> start;   ///< Start point of the line segment
    std::shared_ptr<GPoint> end;     ///< End point of the line segment
    OGRLineString line;              ///< GDAL internal line representation

    units::length::meter_t mLength;           ///< Geodesic length (meters)
    units::angle::degree_t mForwardAzimuth;   ///< Azimuth from start to end
    units::angle::degree_t mBackwardAzimuth;  ///< Azimuth from end to start
    units::length::meter_t mWidth;            ///< Theoretical width (meters)

    /// Tolerance for point proximity comparisons (meters)
    static constexpr double TOLERANCE = 0.1;

    // =========================================================================
    // Constructors and Destructor
    // =========================================================================
public:
    /**
     * @brief Default constructor creating a zero-length line at origin.
     */
    GLine();

    /**
     * @brief Construct a geodetic line between two points.
     *
     * @param start Shared pointer to the start point
     * @param end Shared pointer to the end point
     * @throws std::runtime_error if spatial references don't match
     */
    GLine(std::shared_ptr<GPoint> start, std::shared_ptr<GPoint> end);

    /**
     * @brief Destructor.
     */
    ~GLine();

    // =========================================================================
    // Accessors - Basic Properties
    // =========================================================================

    /**
     * @brief Get the GDAL OGRLineString representation.
     * @return Copy of the internal OGRLineString
     */
    OGRLineString getGDALLine() const;

    /**
     * @brief Get the start point.
     * @return Shared pointer to the start point
     */
    std::shared_ptr<GPoint> startPoint() const;

    /**
     * @brief Get the end point.
     * @return Shared pointer to the end point
     */
    std::shared_ptr<GPoint> endPoint() const;

    /**
     * @brief Get the geodesic length of the line segment.
     * @return Length in meters
     */
    units::length::meter_t length() const;

    /**
     * @brief Get the forward azimuth (bearing from start to end).
     * @return Azimuth in degrees (0-360, clockwise from north)
     */
    units::angle::degree_t forwardAzimuth() const;

    /**
     * @brief Get the backward azimuth (bearing from end to start).
     * @return Azimuth in degrees (0-360, clockwise from north)
     */
    units::angle::degree_t backwardAzimuth() const;

    /**
     * @brief Get the theoretical width of the line.
     * @return Width in meters
     */
    units::length::meter_t getTheoriticalWidth() const;

    // =========================================================================
    // Mutators
    // =========================================================================

    /**
     * @brief Set the start point and recalculate line properties.
     * @param sPoint New start point
     */
    void setStartPoint(std::shared_ptr<GPoint> sPoint);

    /**
     * @brief Set the end point and recalculate line properties.
     * @param ePoint New end point
     */
    void setEndPoint(std::shared_ptr<GPoint> ePoint);

    /**
     * @brief Set the theoretical width of the line.
     * @param newWidth New width in meters
     */
    void setTheoriticalWidth(const units::length::meter_t newWidth);

    // =========================================================================
    // Geometric Operations - Points on Line
    // =========================================================================

    /**
     * @brief Get a point on the line at a specified distance from an endpoint.
     *
     * @param distance Distance from the specified endpoint (meters)
     * @param from Which endpoint to measure from (Start or End)
     * @return Point at the specified distance along the geodesic
     * @throws std::out_of_range if distance exceeds line length
     */
    GPoint getPointByDistance(units::length::meter_t distance,
                              Line::LineEnd from) const;

    /**
     * @brief Get a point on the line at a specified distance from a reference point.
     *
     * @param distance Distance from the reference point (meters)
     * @param from Reference point (must be start or end of line)
     * @return Point at the specified distance along the geodesic
     * @throws std::out_of_range if distance exceeds line length
     * @throws std::invalid_argument if reference point is not an endpoint
     */
    GPoint getPointByDistance(units::length::meter_t distance,
                              std::shared_ptr<GPoint> from) const;

    /**
     * @brief Get the midpoint of the line segment.
     * @return Point at the geodesic midpoint
     */
    GPoint midpoint() const;

    // =========================================================================
    // Geometric Operations - Distance Calculations
    // =========================================================================

    /**
     * @brief Calculate the minimum distance from a point to this line segment.
     *
     * Uses pure geodesic calculations via golden section search along the
     * geodesic arc. Works correctly everywhere on Earth, including polar
     * regions.
     *
     * @param point The point to measure distance from
     * @return Minimum geodesic distance in meters
     */
    units::length::meter_t
    distanceToPoint(const std::shared_ptr<GPoint> &point) const;

    /**
     * @brief Calculate the perpendicular distance from a point to this line.
     *
     * Equivalent to distanceToPoint() - finds the minimum distance from
     * the point to any point on the geodesic segment.
     *
     * @param point The point to measure distance from
     * @return Minimum geodesic distance in meters
     */
    units::length::meter_t getPerpendicularDistance(const GPoint &point) const;

    // =========================================================================
    // Geometric Operations - Orientation and Position
    // =========================================================================

    /**
     * @brief Determine orientation of three points using spherical geometry.
     *
     * Uses 3D spherical cross product for accurate results everywhere on
     * Earth, including polar regions. No projection is used.
     *
     * @param p First point
     * @param q Second point
     * @param r Third point
     * @return Collinear, Clockwise, or CounterClockwise
     */
    static Line::Orientation orientation(std::shared_ptr<GPoint> p,
                                         std::shared_ptr<GPoint> q,
                                         std::shared_ptr<GPoint> r);

    /**
     * @brief Determine which side of the line a point lies on.
     *
     * Uses spherical geometry for accurate global results.
     *
     * @param point Point to test
     * @return left, right, or onLine
     */
    Line::LocationToLine
    getlocationToLine(const std::shared_ptr<GPoint> &point) const;

    // =========================================================================
    // Geometric Operations - Line Relationships
    // =========================================================================

    /**
     * @brief Check if this line intersects with another line.
     *
     * Uses GDAL's OGRGeometry intersection test.
     *
     * @param other The other line to test intersection with
     * @param ignoreEdgePoints If true, shared endpoints don't count as intersection
     * @return true if lines intersect
     */
    bool intersects(GLine &other, bool ignoreEdgePoints = true) const;

    /**
     * @brief Calculate the smallest angle between this line and another.
     *
     * Lines must share a common endpoint.
     *
     * @param other The other line segment
     * @return Angle in radians [0, PI]
     * @throws std::invalid_argument if lines don't share a common point
     */
    units::angle::radian_t smallestAngleWith(GLine &other) const;

    // =========================================================================
    // Transformations
    // =========================================================================

    /**
     * @brief Create a reversed copy of this line (endpoints swapped).
     * @return New GLine with start and end points swapped
     */
    GLine reverse() const;

    /**
     * @brief Project this geodetic line to a projected coordinate system.
     *
     * @param targetSR Target projected spatial reference
     * @return Projected Line object
     * @throws std::runtime_error if targetSR is invalid or not projected
     */
    Line projectTo(OGRSpatialReference *targetSR) const;

    /**
     * @brief Convert to an algebraic vector representation.
     *
     * @param startPoint Point to use as vector origin
     * @return GAlgebraicVector from startPoint toward the other endpoint
     */
    GAlgebraicVector
    toAlgebraicVector(const std::shared_ptr<GPoint> startPoint) const;

    // =========================================================================
    // Operators
    // =========================================================================

    /**
     * @brief Equality comparison (direction-sensitive).
     *
     * Two lines are equal if they have the same start and end points
     * in the same order.
     *
     * @param other Line to compare with
     * @return true if lines are identical
     */
    bool operator==(const GLine &other) const;

    // =========================================================================
    // String Representation
    // =========================================================================

    /**
     * @brief Convert the line to a formatted string.
     *
     * Supported placeholders:
     * - %start: Replaced with start point coordinates
     * - %end: Replaced with end point coordinates
     *
     * @param format Format string with placeholders (case-insensitive)
     * @param decimalPercision Number of decimal places for coordinates
     * @return Formatted string representation
     */
    QString toString(
        const QString &format = "Start Point: %start || End Point: %end",
        int decimalPercision = 5) const override;

    // =========================================================================
    // Hash and Equality Functors (for use in containers)
    // =========================================================================

    /**
     * @brief Direction-independent hash functor for GLine.
     *
     * Produces the same hash value regardless of line direction
     * (line a→b has the same hash as line b→a). Useful for containers
     * where line direction is irrelevant.
     */
    struct Hash
    {
        std::size_t
        operator()(const std::shared_ptr<GLine> &line) const;
    };

    /**
     * @brief Direction-independent equality functor for GLine.
     *
     * Two lines are equal if they connect the same points, regardless
     * of direction. Use with Hash for direction-independent containers.
     */
    struct Equal
    {
        bool operator()(const std::shared_ptr<GLine> &lhs,
                        const std::shared_ptr<GLine> &rhs) const;
    };
};

}  // namespace ShipNetSimCore

#endif  // GLINE_H
