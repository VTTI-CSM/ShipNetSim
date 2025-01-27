/** @file line.cpp
 * @brief Implementation of Line class.
 *
 * File contains definitions and implementations of Line class which
 * represents a line segment defined by two points. The class provides
 * utility functions to work with line segments.
 *
 * @author Ahmed Aredah
 * @date 10.12.2023
 */

#include "line.h"  // Include line header file.
#include "gpoint.h"
#include "gline.h"
#include <sstream>  // Include stringstream class for string manipulation.
#include <cmath>    // Include cmath for mathematical operations.

namespace ShipNetSimCore
{
// Constructor with start and end points.
Line::Line(std::shared_ptr<Point> start,
           std::shared_ptr<Point> end) :
    start(start),  // Initialize start point.
    end(end)      // Initialize end point.
{
    if (! start->getGDALPoint().getSpatialReference()->IsSame(
            end->getGDALPoint().getSpatialReference()))
    {
        throw std::runtime_error("Mismatch spatial reference for the two points!");
    }
    mLength = start->distance(*end);  // Calculate line segment length.
}

// Destructor.
Line::~Line()
{
    // No dynamic memory to free.
}

// Get length of the line segment.
units::length::meter_t Line::length() const
{
    return mLength;  // Return length of the line segment.
}

// Get starting point of the line segment.
std::shared_ptr<Point> Line::startPoint() const
{
    return start;  // Return start point.
}

// Get ending point of the line segment.
std::shared_ptr<Point> Line::endPoint() const
{
    return end;  // Return end point.
}


// Utility function to check if q lies on the
// line segment defined by p and r
bool onSegment(std::shared_ptr<Point> p,
               std::shared_ptr<Point> q,
               std::shared_ptr<Point> r)
{
    // Check if q is within the bounding box defined by p and r.
    return (q->x().value() <= std::max(p->x().value(), r->x().value()) &&
            q->x().value() >= std::min(p->x().value(), r->x().value()) &&
            q->y().value() <= std::max(p->y().value(), r->y().value()) &&
            q->y().value() >= std::min(p->y().value(), r->y().value()));
}

// Determine orientation of three points.
Line::Orientation Line::orientation(std::shared_ptr<Point> p,
                                     std::shared_ptr<Point> q,
                                     std::shared_ptr<Point> r)
{
    // Use temporary variables to avoid multiple calls to .value()
    double px = p->x().value(), py = p->y().value();
    double qx = q->x().value(), qy = q->y().value();
    double rx = r->x().value(), ry = r->y().value();

    // Calculate determinant of orientation matrix.
    double val = (qy - py) * (rx - qx) - (qx - px) * (ry - qy);

    // double check collinearity.
    // This is more accurate than just the val < eps
    double area = p->x().value() * (q->y().value() - r->y().value()) +
                  q->x().value() * (r->y().value() - p->y().value()) +
                  r->x().value() * (p->y().value() - q->y().value());
    if (std::abs(area) < std::numeric_limits<double>::epsilon())
    {
        return Line::Collinear;
    }
    // Check if points are collinear, clockwise, or counterclockwise.
    // if (std::abs(val) < std::numeric_limits<double>::epsilon())
    // {
    //     return Line::Collinear;
    // }
    return (val > 0) ? Line::Clockwise : Line::CounterClockwise;
}

// Check if two line segments intersect.
bool Line::intersects(const Line &other, bool ignoreEdgePoints) const
{
    // Get start and end points of both line segments.
    std::shared_ptr<Point> p1 = this->start;
    std::shared_ptr<Point> q1 = this->end;
    std::shared_ptr<Point> p2 = other.start;
    std::shared_ptr<Point> q2 = other.end;

    if (ignoreEdgePoints &&
        (*p1 == *p2 || *p1 == *q2 || *q1 == *p2 || *q1 == *q2))
    {
        return false; // Ignore intersection as it's at the endpoint
    }

    // Calculate orientations for the line segment pairs.
    Line::Orientation o1 = Line::orientation(p1, q1, p2);
    Line::Orientation o2 = Line::orientation(p1, q1, q2);
    Line::Orientation o3 = Line::orientation(p2, q2, p1);
    Line::Orientation o4 = Line::orientation(p2, q2, q1);

    // General case: The line segments intersect
    // if the orientations are different
    if (o1 != o2 && o3 != o4) return true;

    // Special cases
    if (o1 == Line::Collinear &&
        onSegment(p1, p2, q1)) return true; // p2 is on the segment p1-q1
    if (o2 == Line::Collinear &&
        onSegment(p1, q2, q1)) return true; // q2 is on the segment p1-q1
    if (o3 == Line::Collinear &&
        onSegment(p2, p1, q2)) return true; // p1 is on the segment p2-q2
    if (o4 == Line::Collinear &&
        onSegment(p2, q1, q2)) return true; // q1 is on the segment p2-q2

    return false; // The line segments do not intersect
}

// Calculate angle between two line segments.
units::angle::radian_t Line::angleWith(Line& other) const
{
    // Identify common point between line segments.
    std::shared_ptr<Point> commonPoint;

    // Identify the common point
    if (*(this->startPoint()) == *(other.startPoint()) ||
        *(this->startPoint()) == *(other.endPoint()))
    {
        commonPoint = this->startPoint();
    }
    else if (*(this->endPoint()) == *(other.startPoint()) ||
               *(this->endPoint()) == *(other.endPoint()))
    {
        commonPoint = this->endPoint();
    }
    else
    {
        // Line segments do not share a common point.
        // TODO: Solve this to not throw error in the middle of a simulation.
        throw std::invalid_argument(
            "The lines do not share a common point.");
    }

    // Identify non-common points for line segments.
    std::shared_ptr<Point> a, c;

    // Get non-common points for both line segments.
    if (*(this->startPoint()) == *commonPoint)
    {
        a = this->endPoint();
    }
    else
    {
        a = this->startPoint();
    }

    if (*(other.startPoint()) == *commonPoint)
    {
        c = other.endPoint();
    }
    else
    {
        c = other.startPoint();
    }

    long double ax = a->x().value() - commonPoint->x().value();
    long double ay = a->y().value() - commonPoint->y().value();
    long double cx = c->x().value() - commonPoint->x().value();
    long double cy = c->y().value() - commonPoint->y().value();

    long double dotProduct = ax * cx + ay * cy;
    long double magA = std::sqrt(ax * ax + ay * ay);
    long double magC = std::sqrt(cx * cx + cy * cy);

    if (magA == 0.0L || magC == 0.0L)
    {
        throw std::invalid_argument(
            "Invalid line segments. "
            "Magnitude of vectors cannot be zero.");
    }

    long double angle = std::acos(dotProduct / (magA * magC));

    return units::angle::radian_t (angle); // The angle in radians
}

Point Line::getPointByDistance(units::length::meter_t distance,
                               LineEnd from) const
{
    if (distance.value() < 0 || distance > mLength)
    {
        throw std::out_of_range("Distance is "
                                "outside of the line segment.");
    }

    std::shared_ptr<Point> origin;
    std::shared_ptr<Point> destination;

    if (from == LineEnd::Start)
    {
        origin = startPoint();
        destination = endPoint();
    }
    else
    {  // LineEnd::End
        origin = endPoint();
        destination = startPoint();
    }

    // Calculate the direction vector (dx, dy)
    units::length::meter_t dx = destination->x() - origin->x();
    units::length::meter_t dy = destination->y() - origin->y();

    // Normalize the vector
    units::length::meter_t len = origin->distance(*destination);

    // Handle the case when the line length is zero
    if (len.value() == 0)
    {
        return *origin;  // Both start and end points are the same
    }

    long double unit_dx = (dx / len).value();
    long double unit_dy = (dy / len).value();

    // Scale the unit vector and add it to the origin point
    units::length::meter_t new_x = origin->x() + unit_dx * distance;
    units::length::meter_t new_y = origin->y() + unit_dy * distance;

    Point result(new_x, new_y);

    return result;
}

// Function to get a point on the line at a certain distance
// from another point 'from'.
Point Line::getPointByDistance(
    units::length::meter_t distance,  // Distance from the 'from' point.
    std::shared_ptr<Point> from) const  // Starting point for measurement.
{
    // Enum to represent which end of line 'from' point is closer to.
    LineEnd le;
    // If 'from' is at the starting point of the line.
    if (*(from) == *(startPoint()))
    {
        le = LineEnd::Start;  // 'from' is at the start of the line.
    }
    // If 'from' is at the ending point of the line.
    else if (*(from) == *(endPoint()))
    {
        le = LineEnd::End;  // 'from' is at the end of the line.
    }
    // If 'from' is neither at the start nor at the end of the line.
    else
    {
        // Throw an out_of_range exception.
        throw std::out_of_range("Point is neither the start "
                                "nor the end point.");
    }
    // Call the overloaded getPointByDistance function with
    // distance and LineEnd.
    return getPointByDistance(distance, le);
}

Point Line::getProjectionFrom(const Point& point) const
{
    Point ap = point - *start;
    Point ab = *end - *start;

    double ap_dot_ab = ap.x().value() * ab.x().value() +
                       ap.y().value() * ab.y().value();
    double ab_dot_ab = ab.x().value() * ab.x().value() +
                       ab.y().value() * ab.y().value();

    // Calculate the magnitude of the projection of ap onto ab
    double magnitude = ap_dot_ab / ab_dot_ab;

    // Scale ab by the magnitude of the projection
    // and add to a to get the projection point
    Point projection = *start + (ab * magnitude);

    return projection;
}

// Function to calculate the perpendicular distance from a point to the line.
units::length::meter_t Line::getPerpendicularDistance(
    const Point& point) const  // Point from which distance is to be measured.
{
    // find the projection of point on the line
    Point projection = getProjectionFrom(point);

    // calculate the distance between the two points
    return point.distance(projection);
}

Point Line::getNearestPoint(
    const std::shared_ptr<Point>& point) const
{
    double x0 = point->x().value();
    double y0 = point->y().value();
    double x1 = startPoint()->x().value();
    double y1 = startPoint()->y().value();
    double x2 = endPoint()->x().value();
    double y2 = endPoint()->y().value();

    // Calculate the line segment vector
    double dx = x2 - x1;
    double dy = y2 - y1;

    // Calculate the t that minimizes the distance
    // P(t) = P1 + t (P2 - P1)
    // t ranges from 0 to 1
    // t = (P0 - P1) * (P2 - P1) / ||P2 - P1||^2
    double t = ((x0 - x1) * dx + (y0 - y1) * dy) / (dx * dx + dy * dy);

    // Point on the segment nearest to the external point
    double nearestX, nearestY;

    if (t < 0.0) {
        // Nearest point is the start point
        nearestX = x1;
        nearestY = y1;
    } else if (t > 1.0) {
        // Nearest point is the end point
        nearestX = x2;
        nearestY = y2;
    } else {
        // Nearest point is within the segment
        nearestX = x1 + t * dx;
        nearestY = y1 + t * dy;
    }

    return Point(units::length::meter_t(nearestX),
                 units::length::meter_t(nearestY));
}

units::length::meter_t Line::distanceToPoint(
    const std::shared_ptr<Point>& point) const
{

    Point nearestPoint = getNearestPoint(point);

    // Calculate the distance from the point to the
    // nearest point on the segment
    return point->distance(nearestPoint);
}


// Function to get the theoretical width of the line.
units::length::meter_t Line::getTheoriticalWidth() const
{
    return mWidth;  // Return the value of mWidth.
}

// Function to check the location of a point relative to the line.
Line::LocationToLine Line::getlocationToLine(
    const std::shared_ptr<Point>& point) const  // Point to be checked.
{
    // Calculate relative x and y coordinates of the
    // point from the starting point of the line.
    units::length::meter_t relX = point->x() - start->x();
    units::length::meter_t relY = point->y() - start->y();
    // Calculate the x and y coordinates of the line's direction vector.
    units::length::meter_t dirX = end->x() - start->x();
    units::length::meter_t dirY = end->y() - start->y();

    // Calculate the cross product of the direction vector
    // and relative position vector.
    auto result = dirX.value() * relY.value() - dirY.value() * relX.value();
    // Check if the point is to the left of the line.
    if (result > 0.0)
    {
        return LocationToLine::left;
    }
    // Check if the point is to the right of the line.
    else if (result < 0.0)
    {
        return LocationToLine::right;
    }
    // If neither, then the point is on the line.
    else
    {
        return LocationToLine::onLine;
    }
}

GLine Line::reprojectTo(OGRSpatialReference* targetSR)
{
    if (! targetSR || ! targetSR->IsGeographic())
    {
        throw std::runtime_error("Target Spatial Reference "
                                 "is not valid or not a geographic CRS.");
    }
    std::shared_ptr<GPoint> ps =
        std::make_shared<GPoint>(start->reprojectTo(targetSR));
    std::shared_ptr<GPoint> pe =
        std::make_shared<GPoint>(end->reprojectTo(targetSR));

    return GLine(ps, pe);
}


// Function to set the theoretical width of the line.
void Line::setTheoriticalWidth(const units::length::meter_t newWidth)
{
    mWidth = newWidth;  // Set the value of mWidth to newWidth.
}

// Function to convert the line to an algebraic vector.
AlgebraicVector Line::toAlgebraicVector(
    const std::shared_ptr<Point> startPoint) const
{
    Point begin, finish;  // Declare start and end points of the vector.
    // Check if startPoint is the same as the starting point of the line.
    if (*startPoint == *start)
    {
        begin = *start;  // Set begin to start.
        finish = *end;   // Set finish to end.
    }
    // If startPoint is not the starting point of the line.
    else
    {
        begin = *end;     // Set begin to end.
        finish = *start;  // Set finish to start.
    }
    // Create an algebraic vector from begin to finish and return it.
    AlgebraicVector result(begin, finish);
    return result;
}

Point Line::midpoint() const
{
    const Point endPoint = *end.get();
    return start->getMiddlePoint(endPoint);
}

// bool Line::isInsideBoundingBox(const Point& min_point,
//                                const Point& max_point) const
// {
//     return startPoint()->isPointInsideBoundingBox(min_point, max_point) ||
//            endPoint()->isPointInsideBoundingBox(min_point, max_point);
// }

// Overloaded equality operator to compare two lines.
bool Line::operator==(const Line& other) const
{
    // Return true if the starting and ending points
    // of both lines are the same.
    return *start == *(other.start) && *end == *(other.end);
}

// Function to convert the line to a string representation.
QString Line::toString(const QString &format, int decimalPercision) const
{
    // Get the string representations of the start and end points
    QString startStr =
        start ? start->toString("(%x, %y)", decimalPercision) : "N/A";
    QString endStr =
        end ? end->toString("(%x, %y)", decimalPercision) : "N/A";

    // Create the formatted string
    QString result = format;

    // Replace placeholders (case-insensitive)
    result.replace("%start", startStr, Qt::CaseInsensitive);
    result.replace("%end", endStr, Qt::CaseInsensitive);

    return result; // Return the formatted string
}
};
