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
             QString ID, unsigned int index)
    : mx(xCoord), my(yCoord),
    mIsPort(false),
    mDwellTime(units::time::second_t(0.0)),
    userID(ID), index(index)
{
}

Point::Point(units::length::meter_t xCoord,
             units::length::meter_t yCoord)
    : mx(xCoord), my(yCoord),
    mIsPort(false),
    mDwellTime(units::time::second_t(0.0)),
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

QString Point::toString()
{
    QString str = QString("Point %1(%2, %3)")
                      .arg(userID)
                      .arg(mx.value())
                      .arg(my.value());
    return str;
}

bool Point::isPort()
{
    return mIsPort;
}

units::time::second_t Point::getDwellTime()
{
    return mDwellTime;
}

void Point::MarkAsPort(units::time::second_t dwellTime)
{
    mIsPort = true;
    mDwellTime = dwellTime;
}

void Point::setX(units::length::meter_t newX)
{
    mx = newX;
}
void Point::setY(units::length::meter_t newY)
{
    my = newY;
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
