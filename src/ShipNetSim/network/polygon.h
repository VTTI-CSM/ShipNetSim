#ifndef POLYGON_H
#define POLYGON_H

#include "basegeometry.h"
#include "Point.h"
#include <vector>
#include <memory>
#include "../../third_party/units/units.h"
#include "line.h"

class Polygon : public BaseGeometry
{
private:
    units::velocity::meters_per_second_t mMaxSpeed =
        units::velocity::meters_per_second_t(200.0);

    std::vector<std::shared_ptr<Point>> outer_boundary;

    std::vector<std::vector<std::shared_ptr<Point>>> inner_holes;

    std::vector<std::shared_ptr<Point>> offsetBoundary(
        const std::vector<std::shared_ptr<Point>>& boundary,
        bool inward,
        units::length::meter_t offset
        ) const;
public:
    Polygon();
    Polygon(const std::vector<std::shared_ptr<Point>>& boundary,
            const std::vector<std::vector<std::shared_ptr<Point>>>& holes = {});
    std::vector<std::shared_ptr<Point>> outer() const;
    std::vector<std::vector<std::shared_ptr<Point>>> inners() const;
    void setMaxAllowedSpeed(
        const units::velocity::meters_per_second_t newMaxSpeed);
    units::velocity::meters_per_second_t getMaxAllowedSpeed() const;
    bool pointIsInPolygon(const Point& pointToCheck) const;
    bool PointIsPolygonStructure(const Point& pointToCheck) const;
    bool PointIsPolygonStructure(
        const std::shared_ptr<Point>& pointToCheck) const;
    bool intersects(std::shared_ptr<Line> line);
    units::length::meter_t getMaxClearWidth(const Line &line) const;
    units::area::square_meter_t area() const;
    units::length::meter_t perimeter() const;

    std::string toString() override;
};

#endif // POLYGON_H
