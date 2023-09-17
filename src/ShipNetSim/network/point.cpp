#include "point.h"
#include <cmath>
#include <sstream>

Point::Point()
    : mx(std::nan("No Value")),
    my(std::nan("No Value")),
    userID(""),
    index(0) {}

Point::Point(units::length::meter_t xCoord,
             units::length::meter_t yCoord,
             std::string ID, unsigned int index)
    : mx(xCoord), my(yCoord), userID(ID), index(index)
{
}

Point::Point(units::length::meter_t xCoord,
      units::length::meter_t yCoord)
    : mx(xCoord), my(yCoord),
    userID("temporary point"),
    index(std::numeric_limits<unsigned int>::min())
{

}

Point::~Point() {
    // Destructor logic
}

units::length::meter_t Point::x() const
{
    return mx;
}

units::length::meter_t Point::y() const
{
    return my;
}


bool Point::isValid()
{
    return !(std::isnan(mx.value()) || std::isnan(my.value()));
}

units::length::meter_t Point::distance(const Point &endPoint) const
{
    units::length::meter_t dx = mx - endPoint.x();
    units::length::meter_t dy = my - endPoint.y();
    return units::math::sqrt(units::math::pow<2>(dx) +
                             units::math::pow<2>(dy));
}

std::string Point::toString()
{
    std::ostringstream oss;
    oss << "Point " << userID << "(" << mx.value() <<
        ", " << my.value() << ")";
    return oss.str();
}

bool Point::operator==(const Point &other) const
{
    return mx == other.x() && my == other.y();
}

// Hash function
std::size_t std::hash<Point>::operator()(const Point &p) const
{
    return std::hash<long double>()(p.x().value()) ^
           std::hash<long double>()(p.y().value());
}
