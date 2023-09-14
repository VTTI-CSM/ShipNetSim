#ifndef LINE_H
#define LINE_H

#include "basegeometry.h"
#include "point.h"
#include "../../third_party/units/units.h"

class Line : public BaseGeometry
{
private:
    boost::geometry::model::segment<Point> internal_segment;
    long double mLength;
    long double mMaxSpeed;

public:
    enum class LineEnd {
        Start,
        End
    };
    Line(const Point start, const Point end,
         units::velocity::meters_per_second_t maxSpeed);
    ~Line();
    Point startPoint() const;
    Point endPoint() const;
    long double length() const;
    bool intersects(
        const boost::geometry::model::polygon<Point>& polygon) const;
    long double angleWith(const Line& other) const;
    Point getPointByDistance(long double distance, LineEnd from) const;
    Point getPointByDistance(long double distance, Point &from) const;
    std::string toString() override;
};


#endif // LINE_H
