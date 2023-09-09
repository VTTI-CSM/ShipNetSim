#include "point.h"
#include <boost/geometry.hpp>
#include <cmath>
#include <sstream>
#include <functional>

Point::Point() {}

Point::Point(long double xCoord, long double yCoord, std::string ID, int index)
    : x(xCoord), y(yCoord), userID(ID), index(index) {
}

Point::~Point() {
    // Destructor logic
}

long double Point::distance(Point &endPoint)
{
    return boost::geometry::distance(*this, endPoint);
}

std::string Point::toString() {
    std::ostringstream oss;
    oss << "Point(" << x << ", " << y << "), UserID: "
        << userID << ", Index: " << index;
    return oss.str();
}

bool Point::operator==(const Point& other) const
{
    return x == other.x && y == other.y;
}

//bool Point::operator>(const Point& other) const
//{
//    if (x != other.x) {
//        return x > other.x;
//    }
//    return y > other.y;
//}

//bool Point::operator<(const Point& other) const
//{
//    if (x != other.x)
//    {
//        return x < other.x;
//    }
//    return y < other.y;
//}


// Hash function
std::size_t std::hash<Point>::operator()(const Point &p) const
{
    return std::hash<long double>()(p.x) ^ std::hash<long double>()(p.y);
}


//// Equality function
//bool PointEqual::operator()(const Point& lhs, const Point& rhs) const
//{
//    return lhs.x == rhs.x && lhs.y == rhs.y;
//}


