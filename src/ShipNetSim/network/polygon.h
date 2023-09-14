#ifndef POLYGON_H
#define POLYGON_H

#include "basegeometry.h"
#include "Point.h"
#include <vector>
#include <boost/geometry.hpp>
#include "../../third_party/units/units.h"

class Polygon : BaseGeometry
{
private:
    units::velocity::meters_per_second_t mMaxSpeed =
        units::velocity::meters_per_second_t(200.0);

public:
    boost::geometry::model::polygon<Point> internal_polygon;

    Polygon();
    Polygon(const std::vector<Point>& boundary,
            const std::vector<std::vector<Point>>& holes = {});
    void setMaxAllowedSpeed(
        const units::velocity::meters_per_second_t newMaxSpeed);
    units::velocity::meters_per_second_t getMaxAllowedSpeed();
    bool pointIsInPolygon(const Point& pointToCheck);

    long double area() const;
    long double perimeter() const;

    std::string toString() override;
};

#endif // POLYGON_H
