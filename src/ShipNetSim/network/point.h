#ifndef POINT_H
#define POINT_H

#include "basegeometry.h"
#include <string>
#include <functional>
#include <cmath>
#include "../../third_party/units/units.h"

class Point : public BaseGeometry {
public:
    Point();
    Point(units::length::meter_t xCoord, units::length::meter_t yCoord,
          std::string ID, unsigned int index);
    Point(units::length::meter_t xCoord, units::length::meter_t yCoord);
    ~Point();
    bool isValid();
    units::length::meter_t distance(const Point &endPoint) const;
    std::string toString() override;
    bool operator==(const Point &other) const;

    units::length::meter_t x() const;
    units::length::meter_t y() const;


private:
    units::length::meter_t mx;
    units::length::meter_t my;
    std::string userID;
    unsigned int index;
};

template <>
struct std::hash<Point> {
    std::size_t operator()(const Point& p) const;
};

#endif // POINT_H
