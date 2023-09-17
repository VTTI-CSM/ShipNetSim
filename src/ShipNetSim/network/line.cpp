#include "line.h"
#include <sstream>
#include <cmath>

const double EPSILON = 1e-9;

Line::Line(std::shared_ptr<Point> start, std::shared_ptr<Point> end,
           units::velocity::meters_per_second_t maxSpeed) :
    start(start),
    end(end),
    mMaxSpeed(maxSpeed)
{
    mLength = start->distance(*end);
}

Line::Line(std::shared_ptr<Point> start,
           std::shared_ptr<Point> end) :
    start(start),
    end(end),
    mMaxSpeed(units::velocity::meters_per_second_t(std::nan("no value")))
{
    mLength = start->distance(*end);
}
Line::~Line()
{

}

units::length::meter_t Line::length() const
{
    return mLength;
}


std::shared_ptr<Point> Line::startPoint() const
{
    return start;
}

std::shared_ptr<Point> Line::endPoint() const
{
    return end;
}

// Utility function to check if q lies on the
// line segment defined by p and r
bool onSegment(std::shared_ptr<Point> p,
               std::shared_ptr<Point> q,
               std::shared_ptr<Point> r)
{
    return (q->x().value() <= std::max(p->x().value(), r->x().value()) &&
            q->x().value() >= std::min(p->x().value(), r->x().value()) &&
            q->y().value() <= std::max(p->y().value(), r->y().value()) &&
            q->y().value() >= std::min(p->y().value(), r->y().value()));
}


Line::Orientation orientation(std::shared_ptr<Point> p,
                              std::shared_ptr<Point> q,
                              std::shared_ptr<Point> r)
{
    // Use temporary variables to avoid multiple calls to .value()
    double px = p->x().value(), py = p->y().value();
    double qx = q->x().value(), qy = q->y().value();
    double rx = r->x().value(), ry = r->y().value();

    double val = (qy - py) * (rx - qx) - (qx - px) * (ry - qy);

    if (std::abs(val) < EPSILON) return Line::Collinear;
    return (val > 0) ? Line::Clockwise : Line::CounterClockwise;
}

bool Line::intersects(Line &other) const
{
    // Get points of both lines
    std::shared_ptr<Point> p1 = this->start;
    std::shared_ptr<Point> q1 = this->end;
    std::shared_ptr<Point> p2 = other.start;
    std::shared_ptr<Point> q2 = other.end;

    // Calculate the four orientations
    Line::Orientation o1 = orientation(p1, q1, p2);
    Line::Orientation o2 = orientation(p1, q1, q2);
    Line::Orientation o3 = orientation(p2, q2, p1);
    Line::Orientation o4 = orientation(p2, q2, q1);

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

units::angle::radian_t Line::angleWith(Line& other) const
{
    Point commonPoint;

    // Identify the common point
    if (*(this->startPoint()) == *(other.startPoint()) ||
        this->startPoint() == other.endPoint())
    {
        commonPoint = *(this->startPoint());
    }
    else if (this->endPoint() == other.startPoint() ||
               this->endPoint() == other.endPoint())
    {
        commonPoint = *(this->endPoint());
    }
    else
    {
        throw std::invalid_argument(
            "The lines do not share a common point.");
    }

    Point a, c;

    if (*(this->startPoint()) == commonPoint)
    {
        a = *(this->endPoint());
    }
    else
    {
        a = *(this->startPoint());
    }

    if (*(other.startPoint()) == commonPoint)
    {
        c = *(other.endPoint());
    }
    else
    {
        c = *(other.startPoint());
    }

    long double ax = a.x().value() - commonPoint.x().value();
    long double ay = a.y().value() - commonPoint.y().value();
    long double cx = c.x().value() - commonPoint.x().value();
    long double cy = c.y().value() - commonPoint.y().value();

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
    if (distance.value() < 0 || distance > mLength) {
        throw std::out_of_range("Distance is outside of the line segment.");
    }

    Point origin;
    Point destination;

    if (from == LineEnd::Start) {
        origin = *(startPoint());
        destination = *(endPoint());
    } else {  // LineEnd::End
        origin = *(endPoint());
        destination = *(startPoint());
    }

    // Calculate the direction vector (dx, dy)
    units::length::meter_t dx = destination.x() - origin.x();
    units::length::meter_t dy = destination.y() - origin.y();

    // Normalize the vector
    units::length::meter_t len = origin.distance(destination);

    // Handle the case when the line length is zero
    if (len.value() == 0) {
        return origin;  // Both start and end points are the same
    }

    long double unit_dx = (dx / len).value();
    long double unit_dy = (dy / len).value();

    // Scale the unit vector and add it to the origin point
    units::length::meter_t new_x = origin.x() + unit_dx * distance;
    units::length::meter_t new_y = origin.y() + unit_dy * distance;

    Point result(new_x, new_y);

    return result;
}

Point Line::getPointByDistance(units::length::meter_t distance,
                               std::shared_ptr<Point> from) const
{
    LineEnd le;
    if (*(from) == *(startPoint()))
    {
        le = LineEnd::Start;
    }
    else if (*(from) == *(endPoint()))
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

units::length::meter_t
Line::getPerpendicularDistance(const std::shared_ptr<Point>& point) const
{
    units::length::meter_t dirX = endPoint()->x() - startPoint()->x();
    units::length::meter_t dirY = endPoint()->y() - startPoint()->y();
    units::length::meter_t magnitude =
        units::math::sqrt(dirX * dirX + dirY * dirY);

    units::length::meter_t relativeX = point->x() - startPoint()->x();
    units::length::meter_t relativeY = point->y() - startPoint()->y();

    return units::math::abs(relativeX * dirY - relativeY * dirX) / magnitude;
}

bool Line::operator==(const Line& other) const
{
    return start == other.start && end == other.end;
}

std::string Line::toString()
{
    std::stringstream ss;
    ss << "Start Point: " << start->toString() <<
        " || End Point: " << end->toString();

    return ss.str();
}
