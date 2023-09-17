#ifndef LINE_H
#define LINE_H

#include "basegeometry.h"
#include "point.h"
#include "../../third_party/units/units.h"
#include <cmath>
#include <memory>

class Line : public BaseGeometry
{
private:
    std::shared_ptr<Point> start;
    std::shared_ptr<Point> end;
    units::length::meter_t mLength;
    units::velocity::meters_per_second_t mMaxSpeed;

public:
    enum class LineEnd {
        Start,
        End
    };
    // Enum for orientation
    enum Orientation {
        Collinear,
        Clockwise,
        CounterClockwise
    };

    Line(std::shared_ptr<Point> start, std::shared_ptr<Point> end,
         units::velocity::meters_per_second_t maxSpeed);
    Line(std::shared_ptr<Point> start, std::shared_ptr<Point> end);
    ~Line();
    std::shared_ptr<Point> startPoint() const;
    std::shared_ptr<Point> endPoint() const;
    units::length::meter_t length() const;
    bool intersects(Line& other) const;
    units::angle::radian_t angleWith(Line& other) const;
    Point getPointByDistance(units::length::meter_t distance,
                             LineEnd from) const;
    Point getPointByDistance(units::length::meter_t distance,
                             std::shared_ptr<Point> from) const;
    units::length::meter_t getPerpendicularDistance(
        const std::shared_ptr<Point>& point) const;
    bool operator==(const Line& other) const;
    std::string toString() override;
};

#endif // LINE_H
