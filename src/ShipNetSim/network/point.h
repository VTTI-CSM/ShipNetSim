#ifndef POINT_H
#define POINT_H

#include "basegeometry.h"
#include <boost/geometry.hpp>
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/geometries/register/point.hpp>
#include <functional>

class Point : public BaseGeometry
{
public:
    Point();
    Point (long double xCoord,
          long double yCoord,
          std::string ID,
          int index);
    ~Point();
    bool isValid();
    long double distance(Point &endPoint);
    std::string toString() override;
    bool operator==(const Point& other) const;
//    bool operator>(const Point& other) const;
//    bool operator<(const Point& other) const;

    long double x;
    long double y;

private:
    std::string userID;
    int index;

};

BOOST_GEOMETRY_REGISTER_POINT_2D(Point, long double,
                                 boost::geometry::cs::cartesian,
                                 x,y)


template <>
struct std::hash<Point> {
    std::size_t operator()(const Point& p) const;
};

//struct PointEqual {
//    bool operator()(const Point& lhs, const Point& rhs) const;
//};

#endif // POINT_H
