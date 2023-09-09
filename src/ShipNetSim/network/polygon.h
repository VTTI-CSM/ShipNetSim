#ifndef POLYGON_H
#define POLYGON_H

#include "geometry.h"
#include "Point.h"
#include <vector>
#include <boost/geometry.hpp>

class Polygon : Geometry
{
public:
    boost::geometry::model::polygon<Point> internal_polygon;

    Polygon();
    Polygon(const std::vector<Point>& boundary,
            const std::vector<std::vector<Point>>& holes = {});

    long double area() const;
    long double perimeter() const;

    std::string toString() override;
};

#endif // POLYGON_H
