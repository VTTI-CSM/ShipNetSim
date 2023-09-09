#ifndef LINE_H
#define LINE_H

#include "geometry.h"
#include "point.h"

class Line : public Geometry
{
private:
    boost::geometry::model::segment<Point> internal_segment;

public:
    Line(const Point& start, const Point& end);
    ~Line();
    Point startPoint() const;
    Point endPoint() const;
    long double length() const;
    bool intersects(const boost::geometry::model::polygon<Point>&
                        polygon) const;

    std::string toString() override;
};


#endif // LINE_H
