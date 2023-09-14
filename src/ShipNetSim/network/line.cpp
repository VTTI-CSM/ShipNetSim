#include "line.h"
#include <sstream>

Line::Line(const Point start, const Point end,
           units::velocity::meters_per_second_t freeFlowSpeed) :
    internal_segment(start, end),
    mMaxSpeed(freeFlowSpeed)
{
    mLength = boost::geometry::length(internal_segment);
}

Line::~Line()
{

}

long double Line::length() const
{
    return mLength;
}


Point Line::startPoint() const
{
    return internal_segment.first;
}

Point Line::endPoint() const
{
    return internal_segment.second;
}

bool Line::intersects(const boost::geometry::model::polygon<Point>&
                          polygon) const
{
    return boost::geometry::intersects(internal_segment, polygon);
}

long double Line::angleWith(const Line& other) const
{
    Point commonPoint;

    // Identify the common point
    if (this->startPoint() == other.startPoint() ||
        this->startPoint() == other.endPoint())
    {
        commonPoint = this->startPoint();
    }
    else if (this->endPoint() == other.startPoint() ||
               this->endPoint() == other.endPoint())
    {
        commonPoint = this->endPoint();
    }
    else
    {
        throw std::invalid_argument(
            "The lines do not share a common point.");
    }

    Point a, c;

    if (this->startPoint() == commonPoint)
    {
        a = this->endPoint();
    }
    else
    {
        a = this->startPoint();
    }

    if (other.startPoint() == commonPoint)
    {
        c = other.endPoint();
    }
    else
    {
        c = other.startPoint();
    }

    long double ax = a.x - commonPoint.x;
    long double ay = a.y - commonPoint.y;
    long double cx = c.x - commonPoint.x;
    long double cy = c.y - commonPoint.y;

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

    return angle; // The angle in radians
}

Point Line::getPointByDistance(long double distance, LineEnd from) const
{
    if (distance < 0 || distance > mLength) {
        throw std::out_of_range("Distance is outside of the line segment.");
    }

    Point origin;
    Point destination;

    if (from == LineEnd::Start) {
        origin = startPoint();
        destination = endPoint();
    } else {  // LineEnd::End
        origin = endPoint();
        destination = startPoint();
    }

    // Calculate the direction vector (dx, dy)
    long double dx = destination.x - origin.x;
    long double dy = destination.y - origin.y;

    // Normalize the vector
    long double len = boost::geometry::distance(origin, destination);

    // Handle the case when the line length is zero
    if (len == 0) {
        return origin;  // Both start and end points are the same
    }

    long double unit_dx = dx / len;
    long double unit_dy = dy / len;

    // Scale the unit vector and add it to the origin point
    long double new_x = origin.x + unit_dx * distance;
    long double new_y = origin.y + unit_dy * distance;

    Point result;
    boost::geometry::assign_values(result, new_x, new_y);

    return result;
}

Point Line::getPointByDistance(long double distance, Point &from) const
{
    LineEnd le;
    if (from == startPoint())
    {
        le = LineEnd::Start;
    }
    else if (from == endPoint())
    {
        le = LineEnd::End;
    }
    else
    {
        throw std::out_of_range("Point is neight "
                                "the start nor the end point.");
    }
    return getPointByDistance(distance, le);
}

std::string Line::toString()
{
    std::stringstream ss;
    ss << "Start Point: " << internal_segment.first.toString() <<
        " || End Point: " << internal_segment.second.toString();

    return ss.str();
}
