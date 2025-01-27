#ifndef GLINE_H
#define GLINE_H

#include "../export.h"
#include "gpoint.h"
#include "line.h"
#include "basegeometry.h"

namespace ShipNetSimCore
{
class GAlgebraicVector;  // forward declaration

class SHIPNETSIM_EXPORT GLine : public BaseGeometry
{

private:
    std::shared_ptr<GPoint> start; // Start point of the line.
    std::shared_ptr<GPoint> end;   // End point of the line.
    OGRLineString line; // gdal internal line
    units::length::meter_t mLength; // geodetic length between the two points
    units::angle::degree_t mForwardAzimuth; // azimuth from start to end
    units::angle::degree_t mBackwardAzimuth; // azimuth from end to start
    units::length::meter_t mWidth; // Width of the line.

    static constexpr double TOLERANCE = 0.1;
public:

    /**
     * @brief Default constructor
     */
    GLine();

    /**
     * @brief Constructor to create a geodetic line with start
     * and end geodetic points.
     *
     * @param start A shared pointer to the start point of the line.
     * @param end A shared pointer to the end point of the line.
     */
    GLine(std::shared_ptr<GPoint> start,
         std::shared_ptr<GPoint> end);

    /**
     * @brief Destructor to clean up resources.
     */
    ~GLine();


    // Accessor methods

    /**
     * @brief gets the internal OGRLineString of the gdal library
     * @return The OGRLineString
     */
    OGRLineString getGDALLine() const;

    /**
     * @return A shared pointer to the start point of the line.
     */
    std::shared_ptr<GPoint> startPoint() const;

    /**
     * @return A shared pointer to the end point of the line.
     */
    std::shared_ptr<GPoint> endPoint() const;

    void setStartPoint(std::shared_ptr<GPoint> sPoint);

    void setEndPoint(std::shared_ptr<GPoint> ePoint);

    /**
     * @return The length of the line.
     */
    units::length::meter_t length() const;

    /**
     * @return the forward azimuth from the start to end points.
     */
    units::angle::degree_t forwardAzimuth() const;

    /**
     * @return the forward azimuth from the end to start points.
     */
    units::angle::degree_t backwardAzimuth() const;

    Line projectTo(OGRSpatialReference* targetSR) const;

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
    static Line::Orientation orientation(std::shared_ptr<GPoint> p,
                                         std::shared_ptr<GPoint> q,
                                         std::shared_ptr<GPoint> r);

    /**
     * @brief Check if two lines intersect.
     *
     * @param other The other line to check intersection with.
     * @return True if lines intersect, false otherwise.
     */
    bool intersects(GLine& other, bool ignoreEdgePoints = true) const;

    /**
     * @brief Calculate the angle with another line.
     *
     * @param other The other line to calculate angle with.
     * @return The angle between the two lines.
     */
    units::angle::radian_t smallestAngleWith(GLine& other) const;

    /**
     * @brief Get a point on the line by distance from one end.
     *
     * @param distance The distance from the specified end.
     * @param from The specified end to measure the distance from.
     * @return The point on the line at the specified distance from the end.
     */
    GPoint getPointByDistance(units::length::meter_t distance,
                              Line::LineEnd from) const;

    /**
     * @brief Get a point on the line by distance from another point.
     *
     * @param distance The distance from the specified point.
     * @param from The specified point to measure the distance from.
     * @return The point on the line at the specified distance from the point.
     */
    GPoint getPointByDistance(units::length::meter_t distance,
                              std::shared_ptr<GPoint> from) const;

    /**
     * @brief Calculate the perpendicular distance from a point to the line.
     *
     * @param point The point to calculate the distance from.
     * @return The perpendicular distance from the point to the line.
     */
    units::length::meter_t getPerpendicularDistance(const GPoint& point) const;


    units::length::meter_t distanceToPoint(
        const std::shared_ptr<GPoint>& point) const;

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
    GAlgebraicVector toAlgebraicVector(
        const std::shared_ptr<GPoint> startPoint) const;

    // Location of a point relative to a line
    /**
     * @brief Get the location of a point relative to the line.
     *
     * @param point The point to determine the location of.
     * @return The location of the point relative to the line.
     */
    Line::LocationToLine getlocationToLine(
        const std::shared_ptr<GPoint>& point) const;

    /**
     * @brief Get the mid point on the line
     * @return the mid geodetic point
     */
    GPoint midpoint() const;

    /**
     * @brief Create a reversed copy of this line (start and end points swapped)
     * @return A new GLine instance with reversed endpoints
     */
    GLine reverse() const;

    // Operator overloads
    /**
     * @brief Equality operator to compare two lines.
     *
     * @param other The other line to compare with.
     * @return True if lines are equal, false otherwise.
     */
    bool operator==(const GLine &other) const;

    // BaseGeometry method override
    /**
     * @brief Converts the GLine object to a formatted string representation.
     *
     * This function dynamically formats the output string based on the
     * user-provided format string. Placeholders in the format string are
     * replaced as follows:
     * - `%start`: Replaced with the string representation of the start point.
     * - `%end`: Replaced with the string representation of the end point.
     *
     * The replacement is case-insensitive, allowing placeholders such as
     * `%START` or `%End`.
     * If no format is provided, the default format `"Start Point: %start
     * || End Point: %end"` is used.
     *
     * @param format A QString specifying the desired format of the output.
     *               It must include placeholders (`%start`, `%end`) for
     *               the start and end points, respectively. If not provided,
     *               the default format `"Start Point: %start ||
     *               End Point: %end"` is used.
     *
     * @return A QString containing the formatted output with placeholders
     *          replaced.
     *         If a placeholder is missing, it will not be replaced.
     *
     * @example
     * GLine line;
     * line.setStart(new GPoint(10.123, 20.456, "StartID"));
     * line.setEnd(new GPoint(30.789, 40.987, "EndID"));
     *
     * qDebug() << line.toString();                         // Default: "Start Point: (10.123, 20.456) || End Point: (30.789, 40.987)"
     * qDebug() << line.toString("%start -> %end");         // Custom: "(10.123, 20.456) -> (30.789, 40.987)"
     */
    QString toString(const QString &format =
                     "Start Point: %start || End Point: %end",
                     int decimalPercision = 5) const override;


};
};
#endif // GLINE_H
