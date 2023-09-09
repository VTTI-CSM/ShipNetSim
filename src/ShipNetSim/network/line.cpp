#include "line.h"
#include <sstream>

Line::Line(const Point& start, const Point& end) :
    internal_segment(start, end) {}

Line::~Line()
{

}

long double Line::length() const
{
    return boost::geometry::length(internal_segment);
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

std::string Line::toString()
{
    std::stringstream ss;
    ss << "Start Point: " << internal_segment.first.toString() <<
        " || End Point: " << internal_segment.second.toString();

    return ss.str();
}
