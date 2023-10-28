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

#include "basegeometry.h"
#include "Point.h"
#include <QVector>
#include <memory> // for std::shared_ptr
#include "../../third_party/units/units.h"
#include "line.h"

/**
 * @class Polygon
 *
 * @brief Represents a two-dimensional polygon.
 *
 * The Polygon class represents a polygon defined by a series of points.
 * The polygon can have a single outer boundary and multiple inner holes.
 * Each boundary or hole is represented as a QVector of std::shared_ptr
 * to Point objects. It provides methods to calculate area, perimeter,
 * and check if a point is inside the polygon.
 *
 * @extends BaseGeometry
 */
class Polygon : public BaseGeometry
{
private:
    units::velocity::meters_per_second_t mMaxSpeed =
        units::velocity::meters_per_second_t(200.0); ///< Max allowed speed.

    QVector<std::shared_ptr<Point>>
        outer_boundary; ///< Outer boundary points.

    QVector<QVector<std::shared_ptr<Point>>>
        inner_holes; ///< Inner hole points.

    /**
     * @brief Offset the boundary by a given distance.
     *
     * @param boundary The boundary to offset.
     * @param inward True to offset inward, false for outward.
     * @param offset The distance to offset.
     * @return The offset boundary.
     */
    QVector<std::shared_ptr<Point>> offsetBoundary(
        const QVector<std::shared_ptr<Point>>& boundary,
        bool inward,
        units::length::meter_t offset
        ) const;
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
     * Initializes the polygon with the given outer boundary and inner holes.
     *
     * @param boundary The outer boundary of the polygon.
     * @param holes The inner holes of the polygon.
     */
    Polygon(const QVector<std::shared_ptr<Point>>& boundary,
            const QVector<QVector<std::shared_ptr<Point>>>& holes = {});

    /**
     * @brief Get the outer boundary of the polygon.
     *
     * @return A QVector of std::shared_ptr to Point objects representing the
     * outer boundary.
     */
    QVector<std::shared_ptr<Point>> outer() const;

    /**
     * @brief Get the inner holes of the polygon.
     *
     * @return A QVector of QVector of std::shared_ptr to Point objects
     * representing the inner holes.
     */
    QVector<QVector<std::shared_ptr<Point>>> inners() const;

    /**
     * @brief Set the maximum allowed speed inside the polygon.
     *
     * @param newMaxSpeed The new maximum allowed speed.
     */
    void setMaxAllowedSpeed(
        const units::velocity::meters_per_second_t newMaxSpeed);

    /**
     * @brief Get the maximum allowed speed inside the polygon.
     *
     * @return The maximum allowed speed.
     */
    units::velocity::meters_per_second_t getMaxAllowedSpeed() const;

    /**
     * @brief Check if a point is inside the polygon.
     *
     * @param pointToCheck The point to check.
     * @return True if the point is inside the polygon, false otherwise.
     */
    bool pointIsInPolygon(const Point& pointToCheck) const;

    /**
     * @brief Check if a point is part of the polygon structure.
     *
     * @param pointToCheck The point to check.
     * @return True if the point is part of the polygon structure,
     * false otherwise.
     */
    bool PointIsPolygonStructure(const Point& pointToCheck) const;

    /**
     * @brief Check if a shared_ptr point is part of the polygon structure.
     *
     * @param pointToCheck The shared_ptr point to check.
     * @return True if the point is part of the polygon structure,
     * false otherwise.
     */
    bool PointIsPolygonStructure(
        const std::shared_ptr<Point>& pointToCheck) const;

    /**
     * @brief Check if a line intersects the polygon.
     *
     * @param line The line to check.
     * @return True if the line intersects the polygon, false otherwise.
     */
    bool intersects(const std::shared_ptr<Line> line);

    /**
     * @brief Get the maximum clear width of a line inside the polygon.
     *
     * @param line The line to check.
     * @return The maximum clear width of the line inside the polygon.
     */
    units::length::meter_t getMaxClearWidth(const Line &line) const;

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
     * @brief Get a string representation of the polygon.
     *
     * @return A string representing the polygon.
     */
    QString toString() override;
};

#endif // POLYGON_H
