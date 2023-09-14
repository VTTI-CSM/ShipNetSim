#include "polygon.h"

Polygon::Polygon() {}

Polygon::Polygon(const std::vector<Point>& boundary,
                 const std::vector<std::vector<Point>>& holes) {
    boost::geometry::assign_points(internal_polygon, boundary);

    for(const auto& hole : holes) {
        internal_polygon.inners().emplace_back();
        boost::geometry::assign_points(internal_polygon.inners().back(), hole);
    }
}
void Polygon::setMaxAllowedSpeed(
    const units::velocity::meters_per_second_t newMaxSpeed)
{
    mMaxSpeed = newMaxSpeed;
}
units::velocity::meters_per_second_t Polygon::getMaxAllowedSpeed()
{
    return mMaxSpeed;
}

bool Polygon::pointIsInPolygon(const Point &pointToCheck)
{
    // Check vertices in the outer ring
    for (const auto& vertex : internal_polygon.outer())
    {
        if (vertex == pointToCheck)
        {
            return true;
        }
    }

    // Check vertices in the inner rings (if any)
    for (const auto& inner_ring : internal_polygon.inners())
    {
        for (const auto& vertex : inner_ring)
        {
            if (vertex == pointToCheck)
            {
                return true;
            }
        }
    }

    return false;
}

long double Polygon::area() const {
    return boost::geometry::area(internal_polygon);
}

long double Polygon::perimeter() const {
    return boost::geometry::perimeter(internal_polygon);
}

std::string Polygon::toString()
{
    std::stringstream ss;
    ss << "Polygon Perimeter: " << perimeter() << " Area: " << area() <<
        "# pointers " << internal_polygon.outer().size() <<
        "# holes: " << internal_polygon.inners().size();
    return ss.str();
}
