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
