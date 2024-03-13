/**
 * @file line.h
 *
 * @brief This file declares the Line class, which represents a geometric
 *        line segment in a two-dimensional space. The Line class inherits
 *        from the BaseGeometry class and encapsulates the properties and
 *        behaviors relevant to a line segment. It provides various methods
 *        to calculate geometric properties of the line and its relationships
 *        with other geometric objects.
 *
 *        The Line class uses the units library to handle various physical
 *        units, ensuring type-safe calculations.
 *
 * @author Ahmed Aredah
 * @date 10.12.2023
 */
#ifndef LINE_H
#define LINE_H

#include "basegeometry.h"
#include "point.h"
#include "../../third_party/units/units.h"
#include "algebraicvector.h"
#include <cmath>
#include <memory>

class GLine; // forward declaration

/**
 * @class Line
 *
 * @brief Represents a geometric line segment in a two-dimensional space.
 *
 *        The Line class encapsulates the properties and behaviors of a line
 *        segment. It contains start and end points, length, maximum speed,
 *        width, and depth properties. Various geometric and relational methods
 *        are provided to calculate properties and relationships with other
 *        geometric objects.
 */
class Line : public BaseGeometry
{
private:
    std::shared_ptr<Point> start; // Start point of the line.
    std::shared_ptr<Point> end;   // End point of the line.
    units::length::meter_t mLength; // Length of the line.
    units::length::meter_t mWidth; // Width of the line.
public:
    /**
     * @brief Enum for specifying start or end of the line.
     */
    enum class LineEnd {
        Start,
        End
    };

    /**
     * @brief Enum for orientation of points or lines.
     */
    enum Orientation {
        Collinear,
        Clockwise,
        CounterClockwise
    };

    /**
     * @brief Enum for location of a point to a line.
     */
    enum LocationToLine
    {
        left,
        right,
        onLine
    };

    /**
     * @brief Constructor to create a line with start and end points.
     *
     * @param start A shared pointer to the start point of the line.
     * @param end A shared pointer to the end point of the line.
     */
    Line(std::shared_ptr<Point> start,
         std::shared_ptr<Point> end);

    /**
     * @brief Destructor to clean up resources.
     */
    ~Line();

    // Accessor methods
    /**
     * @return A shared pointer to the start point of the line.
     */
    std::shared_ptr<Point> startPoint() const;

    /**
     * @return A shared pointer to the end point of the line.
     */
    std::shared_ptr<Point> endPoint() const;

    /**
     * @return The length of the line.
     */
    units::length::meter_t length() const;

    // Geometric and relational methods

    /**
     * @brief check orientation and collinearity of 3 points.
     * @param p The first point p
     * @param q The second point q
     * @param r The thrid point r
     * @return Orientation::Collinear if the three points are
     *          collinear with each other.
     *          Orientation::Clockwise if the point is clockwise
     *          to the line.
     *          Orientation::CounterClockwise if the point is
     *          counterclockwise with the line.
     */
    static Line::Orientation orientation(std::shared_ptr<Point> p,
                                         std::shared_ptr<Point> q,
                                         std::shared_ptr<Point> r);
    /**
     * @brief Check if two lines intersect.
     *
     * @param other The other line to check intersection with.
     * @return True if lines intersect, false otherwise.
     */
    bool intersects(const Line& other, bool ignoreEdgePoints = true) const;

    /**
     * @brief Calculate the angle with another line.
     *
     * @param other The other line to calculate angle with.
     * @return The angle between the two lines.
     */
    units::angle::radian_t angleWith(Line& other) const;

    /**
     * @brief Get a point on the line by distance from one end.
     *
     * @param distance The distance from the specified end.
     * @param from The specified end to measure the distance from.
     * @return The point on the line at the specified distance from the end.
     */
    Point getPointByDistance(units::length::meter_t distance,
                             LineEnd from) const;

    /**
     * @brief Get a point on the line by distance from another point.
     *
     * @param distance The distance from the specified point.
     * @param from The specified point to measure the distance from.
     * @return The point on the line at the specified distance from the point.
     */
    Point getPointByDistance(units::length::meter_t distance,
                             std::shared_ptr<Point> from) const;

    Point getNearestPoint(
        const std::shared_ptr<Point>& point) const;

    Point getProjectionFrom(const Point& point) const;
    /**
     * @brief Calculate the perpendicular distance from a point to the line.
     *
     * @param point The point to calculate the distance from.
     * @return The perpendicular distance from the point to the line.
     */
    units::length::meter_t getPerpendicularDistance(const Point& point) const;

    units::length::meter_t distanceToPoint(
        const std::shared_ptr<Point>& point) const;

    /**
     * @return The theoretical width of the line.
     */
    units::length::meter_t getTheoriticalWidth() const;

    // Mutator methods
    /**
     * @brief Set the theoretical width of the line.
     *
     * @param newWidth The new theoretical width of the line.
     */
    void setTheoriticalWidth(const units::length::meter_t newWidth);

    // Algebraic vector representation
    /**
     * @brief Convert the line to an algebraic vector.
     *
     * @param startPoint The point from which to create the vector.
     * @return The algebraic vector representation of the line.
     */
    AlgebraicVector toAlgebraicVector(
        const std::shared_ptr<Point> startPoint) const;

    // Location of a point relative to a line
    /**
     * @brief Get the location of a point relative to the line.
     *
     * @param point The point to determine the location of.
     * @return The location of the point relative to the line.
     */
    LocationToLine getlocationToLine(
        const std::shared_ptr<Point>& point) const;

    GLine reprojectTo(OGRSpatialReference* targetSR);

    // /**
    //  * @brief Check if the line falls within a bounding box
    //  * defined by two points.
    //  *
    //  * @param min_point the lower right point
    //  * @param max_point the top left point
    //  * @return true if the line falls within the bounding box,
    //  * false otherwise
    //  */
    // bool isInsideBoundingBox(const Point& min_point,
    //                          const Point& max_point) const;

    Point midpoint() const;

    // Operator overloads
    /**
     * @brief Equality operator to compare two lines.
     *
     * @param other The other line to compare with.
     * @return True if lines are equal, false otherwise.
     */
    bool operator==(const Line& other) const;

    // BaseGeometry method override
    /**
     * @brief Convert the line to a string representation.
     *
     * @return The string representation of the line.
     */
    QString toString() const override;
};

#endif // LINE_H
