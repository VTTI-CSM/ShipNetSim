/**
 * @file polygon.h
 * @brief Geodetic Polygon Implementation for WGS84 Ellipsoid.
 *
 * This file contains the Polygon class which represents a two-dimensional
 * polygon on the WGS84 ellipsoid. The polygon supports both an outer boundary
 * and multiple inner holes, enabling accurate representation of complex
 * geographic regions like water bodies with islands.
 *
 * Key features:
 * - Accurate geodetic area and perimeter calculations using GeographicLib
 * - Support for outer boundary and multiple inner holes (rings)
 * - Point-in-polygon testing with hole awareness
 * - Line intersection detection
 * - Segment validation for pathfinding through water regions
 * - Antimeridian crossing support for global-scale polygons
 * - Boundary transformation (offset/buffer operations)
 * - GDAL/OGR integration for spatial operations
 *
 * @author Ahmed Aredah
 * @date 10.12.2023
 */

#ifndef POLYGON_H
#define POLYGON_H

#include "../../third_party/units/units.h"
#include "../export.h"
#include "basegeometry.h"
#include "gdal_priv.h"
#include "gline.h"
#include "gpoint.h"
#include "ogr_spatialref.h"
#include <QVector>
#include <memory>

namespace ShipNetSimCore
{

/**
 * @class Polygon
 * @brief Represents a geodetic polygon with outer boundary and inner holes.
 *
 * The Polygon class models a geographic region defined by an outer boundary
 * and zero or more inner holes. All geometric calculations use geodesic
 * mathematics via GeographicLib, ensuring accuracy for global-scale
 * applications including shipping simulation.
 *
 * The polygon is stored both as QVector of GPoint shared pointers (for
 * convenient access) and as an OGRPolygon (for GDAL spatial operations).
 *
 * @extends BaseGeometry
 */
class SHIPNETSIM_EXPORT Polygon : public BaseGeometry
{
    // =========================================================================
    // Member Variables
    // =========================================================================
private:
    /// Points defining the outer boundary (exterior ring)
    QVector<std::shared_ptr<GPoint>> mOutterBoundary;

    /// Points defining inner holes (interior rings)
    QVector<QVector<std::shared_ptr<GPoint>>> mInnerHoles;

    /// GDAL/OGR polygon representation for spatial operations
    OGRPolygon mPolygon;

    /// User-defined identifier for the polygon
    QString mUserID;

    /// Cache for antimeridian crossing status (-1=unchecked, 0=no, 1=yes)
    mutable int mCrossesAntimeridianCache = -1;

    // =========================================================================
    // Private Helper Methods
    // =========================================================================

    /**
     * @brief Offset a ring boundary by a given distance.
     *
     * Uses GDAL buffer operations to create an offset version of the ring.
     * The operation projects to a planar CRS, applies buffer, and re-projects
     * back to WGS84.
     *
     * @param ring The ring to offset
     * @param inward True to offset inward (shrink), false for outward (expand)
     * @param offset The distance to offset in meters
     * @return Unique pointer to the new offset ring
     * @throws std::runtime_error if transformation fails
     */
    std::unique_ptr<OGRLinearRing>
    offsetBoundary(const OGRLinearRing &ring, bool inward,
                   units::length::meter_t offset) const;

    /**
     * @brief Validate that a ring has sufficient valid points.
     *
     * Checks that the ring has at least 3 points and is not degenerate
     * (i.e., points are not collinear for triangles).
     *
     * @param ring The ring to validate
     * @param description Description for error messages
     * @throws std::runtime_error if ring is degenerate
     */
    static void validateRing(const OGRLinearRing &ring,
                             const QString       &description);

    /**
     * @brief Check if a segment passes through the interior of a hole.
     *
     * Samples points along the segment and tests if any fall inside the hole.
     *
     * @param segment The line segment to check
     * @param holeIndex Index of the hole to test against
     * @return true if segment passes through the hole interior
     */
    bool isSegmentPassingThroughHole(const std::shared_ptr<GLine> &segment,
                                     int holeIndex) const;

    /**
     * @brief Check if a segment crosses a hole boundary at non-vertex points.
     *
     * Detects invalid crossings where a segment intersects hole edges
     * at points that are not shared vertices.
     *
     * @param segment The line segment to check
     * @param holeIndex Index of the hole to test against
     * @return true if segment crosses hole boundary invalidly
     */
    bool isSegmentCrossingHoleBoundary(const std::shared_ptr<GLine> &segment,
                                       int holeIndex) const;

    /**
     * @brief Check if intersection between two segments is at a vertex.
     *
     * Determines if two segments meet at a shared endpoint rather than
     * crossing through each other.
     *
     * @param segment1 First line segment
     * @param segment2 Second line segment
     * @return true if intersection is at a shared vertex
     */
    bool isIntersectionAtVertex(const std::shared_ptr<GLine> &segment1,
                                const std::shared_ptr<GLine> &segment2) const;

    /**
     * @brief Point-in-polygon test for a specific hole using raw coordinates.
     *
     * Optimized version that performs ray casting directly on OGR ring
     * coordinates without creating intermediate GPoint objects. This is
     * the core implementation used by all point-in-hole tests.
     *
     * @param lon Longitude of the point to test
     * @param lat Latitude of the point to test
     * @param holeIndex Index of the hole to test against
     * @return true if point is inside the hole
     */
    bool isPointInHoleByCoords(double lon, double lat, int holeIndex) const;

    /**
     * @brief Point-in-polygon test for a specific hole.
     *
     * Convenience overload that extracts coordinates from GPoint.
     * Delegates to isPointInHoleByCoords for the actual test.
     *
     * @param point The point to test
     * @param holeIndex Index of the hole to test against
     * @return true if point is inside the hole
     */
    bool isPointInHole(const std::shared_ptr<GPoint> &point,
                       int                            holeIndex) const;

    /**
     * @brief Check if this polygon crosses the antimeridian (+-180 degrees).
     *
     * Analyzes edge directions to determine if the polygon wraps around
     * the antimeridian. Uses vertex ordering convention to distinguish
     * between antimeridian-crossing and prime-meridian-spanning polygons.
     *
     * @return true if the polygon crosses the antimeridian
     */
    bool crossesAntimeridian() const;

    // =========================================================================
    // Constructors
    // =========================================================================
public:
    /**
     * @brief Default constructor creating an empty polygon.
     */
    Polygon();

    /**
     * @brief Construct a polygon with boundary and optional holes.
     *
     * @param boundary Points defining the outer boundary (will be closed)
     * @param holes Optional vector of inner hole boundaries
     * @param ID Optional user-defined identifier
     */
    Polygon(const QVector<std::shared_ptr<GPoint>>          &boundary,
            const QVector<QVector<std::shared_ptr<GPoint>>> &holes = {},
            const QString                                    ID    = "");

    // =========================================================================
    // Boundary Accessors and Mutators
    // =========================================================================

    /**
     * @brief Set the outer boundary points.
     *
     * Updates the exterior ring while preserving any existing interior rings.
     *
     * @param newOuter Vector of points defining the new outer boundary
     */
    void setOuterPoints(const QVector<std::shared_ptr<GPoint>> &newOuter);

    /**
     * @brief Get the outer boundary points.
     * @return QVector of shared pointers to the outer boundary points
     */
    QVector<std::shared_ptr<GPoint>> outer() const;

    /**
     * @brief Set the inner hole boundaries.
     *
     * Replaces all existing holes with the new set while preserving
     * the exterior ring.
     *
     * @param holes Vector of vectors, each defining a hole boundary
     */
    void setInnerHolesPoints(
        const QVector<QVector<std::shared_ptr<GPoint>>> holes);

    /**
     * @brief Get the inner hole boundaries.
     * @return QVector of QVectors, each containing points of one hole
     */
    QVector<QVector<std::shared_ptr<GPoint>>> inners() const;

    // =========================================================================
    // Point Containment Tests
    // =========================================================================

    /**
     * @brief Check if a point is within the exterior ring.
     *
     * Returns true if the point is on the boundary or inside the
     * exterior ring, ignoring any holes.
     *
     * @param pointToCheck The point to test
     * @return true if point is within or on the exterior ring
     */
    bool isPointWithinExteriorRing(const GPoint &pointToCheck) const;

    /**
     * @brief Check if a point is within any interior ring (hole).
     *
     * Returns true if the point is on the boundary or inside any
     * of the holes.
     *
     * @param pointToCheck The point to test
     * @return true if point is within or on any hole
     */
    bool isPointWithinInteriorRings(const GPoint &pointToCheck) const;

    /**
     * @brief Find which interior ring (hole) contains a point.
     *
     * Returns the index of the hole containing the point, or -1 if
     * the point is not in any hole.
     *
     * @param pointToCheck The point to test
     * @return Index of containing hole (0-based), or -1 if not in any hole
     */
    int findContainingHoleIndex(const GPoint &pointToCheck) const;

    /**
     * @brief Check if a point is within the polygon (excluding holes).
     *
     * Returns true if the point is inside the exterior ring but not
     * inside any holes. This is the main point-in-polygon test for
     * determining if a point is in the valid polygon area.
     *
     * Handles antimeridian-crossing polygons automatically.
     *
     * @param pointToCheck The point to test
     * @return true if point is in the valid polygon area
     */
    bool isPointWithinPolygon(const GPoint &pointToCheck) const;

    /**
     * @brief Check if a point lies on any ring boundary.
     *
     * Returns true if the point is on the exterior ring boundary
     * or any hole boundary.
     *
     * @param point The point to check
     * @return true if point is on any ring boundary
     */
    bool ringsContain(std::shared_ptr<GPoint> point) const;

    // =========================================================================
    // Line and Segment Operations
    // =========================================================================

    /**
     * @brief Check if a line intersects the polygon boundary.
     *
     * Returns true if the line crosses the polygon boundary at a
     * non-endpoint location. Touching at line endpoints only does
     * not count as intersection.
     *
     * @param line The line to test
     * @return true if line intersects (crosses) the polygon
     */
    bool intersects(const std::shared_ptr<GLine> line);

    /**
     * @brief Check if a segment is valid for water navigation.
     *
     * For water polygons (where holes represent land/islands), this
     * checks that a segment doesn't create invalid paths through holes.
     *
     * @param segment The segment to validate
     * @return true if segment doesn't pass through holes
     */
    bool isValidWaterSegment(const std::shared_ptr<GLine> &segment) const;

    /**
     * @brief Check if a segment crosses through any holes.
     *
     * @param segment The segment to check
     * @return true if segment crosses any hole
     */
    bool segmentCrossesHoles(const std::shared_ptr<GLine> &segment) const;

    /**
     * @brief Check if a segment creates an invalid diagonal through a hole.
     *
     * @param segment The segment to check
     * @return true if segment is an invalid diagonal through any hole
     */
    bool
    isSegmentDiagonalThroughHole(const std::shared_ptr<GLine> &segment) const;

    // =========================================================================
    // Geometric Calculations
    // =========================================================================

    /**
     * @brief Calculate the geodetic area of the polygon.
     *
     * Uses GeographicLib for accurate area calculation on the WGS84
     * ellipsoid. The area of holes is subtracted from the total.
     *
     * @return Area in square meters
     */
    units::area::square_meter_t area() const;

    /**
     * @brief Calculate the geodetic perimeter of the exterior ring.
     *
     * Uses GeographicLib for accurate perimeter calculation on the
     * WGS84 ellipsoid. Only calculates the exterior ring perimeter.
     *
     * @return Perimeter in meters
     */
    units::length::meter_t perimeter() const;

    /**
     * @brief Get the maximum clear width for a line inside the polygon.
     *
     * Calculates the sum of the minimum distances from the line to
     * the polygon boundaries on both sides.
     *
     * @param line The reference line
     * @return Total clear width (left + right) in meters
     */
    units::length::meter_t getMaxClearWidth(const GLine &line) const;

    // =========================================================================
    // Bounding Box Operations
    // =========================================================================

    /**
     * @brief Get the bounding box (envelope) of the polygon.
     *
     * @param[out] minLon Minimum longitude
     * @param[out] maxLon Maximum longitude
     * @param[out] minLat Minimum latitude
     * @param[out] maxLat Maximum latitude
     */
    void getEnvelope(double &minLon, double &maxLon, double &minLat,
                     double &maxLat) const;

    /**
     * @brief Check if a segment's bounding box intersects the polygon's.
     *
     * Fast preliminary check before detailed intersection testing.
     *
     * @param segment The segment to check
     * @return true if bounding boxes overlap
     */
    bool segmentBoundsIntersect(const std::shared_ptr<GLine> &segment) const;

    // =========================================================================
    // Boundary Transformations
    // =========================================================================

    /**
     * @brief Offset the outer boundary by a given distance.
     *
     * Expands (outward) or shrinks (inward) the exterior ring.
     *
     * @param inward True to shrink, false to expand
     * @param offset Distance to offset in meters
     */
    void transformOuterBoundary(bool inward, units::length::meter_t offset);

    /**
     * @brief Offset all inner hole boundaries by a given distance.
     *
     * Expands or shrinks all holes simultaneously.
     *
     * @param inward True to shrink holes, false to expand them
     * @param offset Distance to offset in meters
     */
    void transformInnerHolesBoundaries(bool                   inward,
                                       units::length::meter_t offset);

    // =========================================================================
    // String Representation
    // =========================================================================

    /**
     * @brief Convert the polygon to a formatted string.
     *
     * Supported placeholders (case-insensitive):
     * - %perimeter: Perimeter of exterior ring in meters
     * - %area: Area of polygon (excluding holes) in square meters
     *
     * @param format Format string with placeholders
     * @param decimalPercision Number of decimal places for values
     * @return Formatted string representation
     */
    QString toString(const QString &format =
                         "Polygon Perimeter: %perimeter || Area: %area",
                     int decimalPercision = 5) const override;
};

}  // namespace ShipNetSimCore

#endif  // POLYGON_H
