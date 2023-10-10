#ifndef LINE_H
#define LINE_H

#include "basegeometry.h"
#include "point.h"
#include "../../third_party/units/units.h"
#include "algebraicvector.h"
#include <cmath>
#include <memory>

class Line : public BaseGeometry
{
private:
    std::shared_ptr<Point> start;
    std::shared_ptr<Point> end;
    units::length::meter_t mLength;
    units::velocity::meters_per_second_t mMaxSpeed;
    units::length::meter_t mWidth;
    units::length::meter_t mDepth;

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

    enum LocationToLine
    {
        left,
        right,
        onLine
    };

    Line(std::shared_ptr<Point> start, std::shared_ptr<Point> end,
         units::velocity::meters_per_second_t maxSpeed);
    Line(std::shared_ptr<Point> start,
         std::shared_ptr<Point> end);
    ~Line();
    std::shared_ptr<Point> startPoint() const;
    std::shared_ptr<Point> endPoint() const;
    units::length::meter_t length() const;
    units::velocity::meters_per_second_t getMaxSpeed() const;
    units::length::meter_t depth() const;
    bool intersects(Line& other) const;
    units::angle::radian_t angleWith(Line& other) const;
    Point getPointByDistance(units::length::meter_t distance,
                             LineEnd from) const;
    Point getPointByDistance(units::length::meter_t distance,
                             std::shared_ptr<Point> from) const;
    units::length::meter_t getPerpendicularDistance(const Point& point) const;
    units::length::meter_t getTheoriticalWidth() const;
    void setTheoriticalWidth(const units::length::meter_t newWidth);
    AlgebraicVector toAlgebraicVector(
        const std::shared_ptr<Point> startPoint) const;
    LocationToLine getlocationToLine(
        const std::shared_ptr<Point>& point) const;
    bool operator==(const Line& other) const;
    QString toString() override;
};

#endif // LINE_H
