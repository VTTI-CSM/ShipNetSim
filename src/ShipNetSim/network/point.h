#ifndef POINT_H
#define POINT_H

#include "basegeometry.h"
#include <string>
#include <functional>
#include <cmath>
#include "../../third_party/units/units.h"

class Point : public BaseGeometry
{

public:
    Point();
    Point(units::length::meter_t xCoord, units::length::meter_t yCoord,
          QString ID, unsigned int index);
    Point(units::length::meter_t xCoord, units::length::meter_t yCoord);

    ~Point();
    [[nodiscard]] bool isValid();
    [[nodiscard]] units::length::meter_t distance(const Point &endPoint) const;
    [[nodiscard]] QString toString() override;
    [[nodiscard]] bool operator==(const Point &other) const;

    [[nodiscard]] units::length::meter_t x() const;
    [[nodiscard]] units::length::meter_t y() const;
    [[nodiscard]] bool isPort();
    [[nodiscard]] units::time::second_t getDwellTime();
    void MarkAsPort(units::time::second_t dwellTime);

    void setX(units::length::meter_t newX);
    void setY(units::length::meter_t newY);

private:
    units::length::meter_t mx;
    units::length::meter_t my;
    QString userID;
    unsigned int index;
    bool mIsPort;
    units::time::second_t mDwellTime;
};

template <>
struct std::hash<Point> {
    std::size_t operator()(const Point& p) const;
};

#endif // POINT_H
