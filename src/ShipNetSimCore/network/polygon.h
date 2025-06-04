/**
 * @file polygon.h
 *
 * @brief Definition of the Polygon class.
 *
 * This file contains the definition of the Polygon class,
 * which is used to represent a two-dimensional polygon.
 *
 * @author Ahmed Aredah
 * @date 10.12.2023
 */
#ifndef POLYGON_H
#define POLYGON_H

#include "../../third_party/units/units.h"
#include "basegeometry.h"
#include "gdal_priv.h"
#include "gline.h"
#include "gpoint.h"
#include "ogr_spatialref.h"
#include <QVector>
#include <memory> // for std::shared_ptr

namespace ShipNetSimCore
{
/**
 * @class Polygon
 *
 * @brief Represents a two-dimensional polygon.
 *
 * The Polygon class represents a polygon defined by a series of
 * points. The polygon can have a single outer boundary and multiple
 * inner holes. Each boundary or hole is represented as a QVector of
 * std::shared_ptr to GPoint objects. It provides methods to calculate
 * area, perimeter, and check if a point is inside the polygon.
 *
 * @extends BaseGeometry
 */
class Polygon : public BaseGeometry
{
private:
    QVector<std::shared_ptr<GPoint>>          mOutterBoundary;
    QVector<QVector<std::shared_ptr<GPoint>>> mInnerHoles;

    OGRPolygon mPolygon;

    QString mUserID;

    /**
     * @brief Offset the boundary by a given distance.
     *
     * @param boundary The boundary to offset.
     * @param inward True to offset inward, false for outward.
     * @param offset The distance to offset.
     * @return The offset boundary.
     */
    std::unique_ptr<OGRLinearRing>
    offsetBoundary(const OGRLinearRing &ring, bool inward,
                   units::length::meter_t offset) const;

    // check if the polygon has correct points (degenerate/colocated
    // points)
    static void validateRing(const OGRLinearRing &ring,
                             const QString       &description);

    /**
     * @brief Checks if a segment passes through the interior of a
     * specific hole
     * @param segment The line segment to check
     * @param holeIndex The index of the hole to check against
     * @return true if segment passes through the hole interior
     */
    bool
    isSegmentPassingThroughHole(const std::shared_ptr<GLine> &segment,
                                int holeIndex) const;

    /**
     * @brief Checks if a segment crosses hole boundary at non-vertex
     * points
     * @param segment The line segment to check
     * @param holeIndex The index of the hole to check against
     * @return true if segment crosses hole boundary invalidly
     */
    bool isSegmentCrossingHoleBoundary(
        const std::shared_ptr<GLine> &segment, int holeIndex) const;

    /**
     * @brief Checks if intersection between two segments occurs at a
     * vertex
     * @param segment1 First line segment
     * @param segment2 Second line segment
     * @return true if intersection is at a shared vertex
     */
    bool isIntersectionAtVertex(
        const std::shared_ptr<GLine> &segment1,
        const std::shared_ptr<GLine> &segment2) const;

    /**
     * @brief Point-in-polygon test for a specific hole
     * @param point The point to test
     * @param holeIndex The index of the hole to test against
     * @return true if point is inside the hole
     */
    bool isPointInHole(const std::shared_ptr<GPoint> &point,
                       int holeIndex) const;

    /**
     * @brief Helper function for point-in-polygon test using hole
     * vertices
     * @param point The point to test
     * @param hole The hole vertices
     * @return true if point is inside the hole
     */
    bool isPointInHoleVertices(
        const std::shared_ptr<GPoint>          &point,
        const QVector<std::shared_ptr<GPoint>> &hole) const;

public:
    /**
     * @brief Default constructor.
     *
     * Initializes an empty polygon.
     */
    Polygon();

    /**
     * @brief Parameterized constructor.
     *
     * Initializes the polygon with the given outer boundary and inner
     * holes.
     *
     * @param boundary The outer boundary of the polygon.
     * @param holes The inner holes of the polygon.
     */
    Polygon(
        const QVector<std::shared_ptr<GPoint>>          &boundary,
        const QVector<QVector<std::shared_ptr<GPoint>>> &holes = {},
        const QString                                    ID    = "");

    /**
     * @brief set a new vector that defines the outer points of the
     * polygon
     * @param newOuter a vector of the points that defines the polygon
     */
    void
    setOuterPoints(const QVector<std::shared_ptr<GPoint>> &newOuter);
    /**
     * @brief Get the outer boundary of the polygon.
     *
     * @return A QVector of std::shared_ptr to GPoint objects
     * representing the outer boundary.
     */
    QVector<std::shared_ptr<GPoint>> outer() const;

    /**
     * @brief set a new vector that defines the outer points of the
     * polygon
     * @param holes a vector of vectors of the points that defines
     * the holes in the polygons.
     */
    void setInnerHolesPoints(
        const QVector<QVector<std::shared_ptr<GPoint>>> holes);
    /**
     * @brief Get the inner holes of the polygon.
     *
     * @return A QVector of QVector of std::shared_ptr to GPoint
     * objects representing the inner holes.
     */
    QVector<QVector<std::shared_ptr<GPoint>>> inners() const;

    /**
     * @brief Check if a point is within the polygon's exterior ring.
     *
     * @param pointToCheck The point to check.
     * @return True if the point is within the polygon's exterior
     * ring, false otherwise.
     */
    bool isPointWithinExteriorRing(const GPoint &pointToCheck) const;

    /**
     * @brief Check if a point is within the polygon's interior rings.
     *
     * @param pointToCheck The point to check.
     * @return True if the point is within the polygon's interior
     * rings, false otherwise.
     */
    bool isPointWithinInteriorRings(const GPoint &pointToCheck) const;

    /**
     * @brief check if a point is within the polygon but not its
     * holes.
     *
     * @details The function checks if the point is within the
     * polygon. If the point is within the holes, it returns false. If
     * the point is outside the holes but inside the polygon, it
     * returns true.
     *
     * @param pointToCheck The point to check.
     * @return True if the point is within the polygon's interior
     * rings, false otherwise.
     */
    bool isPointWithinPolygon(const GPoint &pointToCheck) const;

    /**
     * @brief Check if a line intersects the polygon.
     *
     * @param line The line to check.
     * @return True if the line intersects the polygon, false
     * otherwise.
     */
    bool intersects(const std::shared_ptr<GLine> line);

    /**
     * @brief Offset the outer polygon boundary by a given distance.
     *
     * @param inward True to offset inward, false for outward.
     * @param offset The distance to offset.
     */
    void transformOuterBoundary(bool                   inward,
                                units::length::meter_t offset);

    /**
     * @brief Offset all the inner holes polygon boundaries by a given
     * distance.
     *
     * @param inward True to offset inward, false for outward.
     * @param offset The distance to offset.
     */
    void transformInnerHolesBoundaries(bool                   inward,
                                       units::length::meter_t offset);

    /**
     * @brief Get the maximum clear width of a line inside the
     * polygon.
     *
     * @param line The line to check.
     * @return The maximum clear width of the line inside the polygon.
     */
    units::length::meter_t getMaxClearWidth(const GLine &line) const;

    /**
     * @brief Get the area of the polygon.
     *
     * @return The area of the polygon.
     */
    units::area::square_meter_t area() const;

    /**
     * @brief Get the perimeter of the polygon.
     *
     * @return The perimeter of the polygon.
     */
    units::length::meter_t perimeter() const;

    /**
     * @brief Converts the Polygon object to a formatted string
     * representation.
     *
     * This function dynamically formats the output string based on
     * the user-provided format string. Placeholders in the format
     * string are replaced as follows:
     * - `%perimeter`: Replaced with the perimeter value of the
     * polygon.
     * - `%area`: Replaced with the area value of the polygon.
     *
     * The replacement is case-insensitive, allowing placeholders such
     * as `%Perimeter` or `%AREA`.
     * If no format is provided, the default format `"Polygon
     * Perimeter: %perimeter || Area: %area"` is used.
     *
     * @param format A QString specifying the desired format of the
     * output. It must include placeholders (`%perimeter`, `%area`)
     *               for the perimeter and area values, respectively.
     *               If not provided, the default format
     *               `"Polygon Perimeter: %perimeter || Area: %area"`
     * is used.
     *
     * @return A QString containing the formatted output with
     * placeholders replaced. If a placeholder is missing, it will not
     * be replaced.
     *
     * @example
     * Polygon polygon;
     * polygon.setPerimeter(units::length::meter_t(123.456));
     * polygon.setArea(units::area::square_meter_t(789.123));
     *
     * qDebug() << polygon.toString();                  // Default:
     * "Polygon Perimeter: 123.456 || Area: 789.123" qDebug() <<
     * polygon.toString("P: %perimeter, A: %area"); // Custom: "P:
     * 123.456, A: 789.123"
     */
    QString toString(const QString &format =
                         "Polygon Perimeter: %perimeter || "
                         "Area: %area",
                     int decimalPercision = 5) const override;

    /**
     * @brief Check if the polygon ringsContain the point either in
     * their boundary or inner holes structure.
     * @param point The point to check
     * @return True if the point is found, false otherwise.
     */
    bool ringsContain(std::shared_ptr<GPoint> point) const;

    /**
     * @brief Checks if a line segment is valid within this water
     * polygon
     * @param segment The line segment to validate
     * @return true if the segment doesn't create invalid diagonals
     * through holes
     */
    bool
    isValidWaterSegment(const std::shared_ptr<GLine> &segment) const;

    /**
     * @brief Checks if a segment crosses through any holes in this
     * polygon
     * @param segment The line segment to check
     * @return true if the segment crosses polygon holes
     */
    bool
    segmentCrossesHoles(const std::shared_ptr<GLine> &segment) const;

    /**
     * @brief Checks if a segment creates an invalid diagonal through
     * polygon holes
     * @param segment The line segment to check
     * @return true if the segment is an invalid diagonal through a
     * hole
     */
    bool isSegmentDiagonalThroughHole(
        const std::shared_ptr<GLine> &segment) const;
};
}; // namespace ShipNetSimCore
#endif // POLYGON_H
