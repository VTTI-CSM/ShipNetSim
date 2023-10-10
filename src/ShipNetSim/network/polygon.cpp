#include "Polygon.h"
#include "line.h"
#include <sstream>
#include <cmath>
#include <limits>
#include <QVector>

Polygon::Polygon() {}

Polygon::Polygon(const QVector<std::shared_ptr<Point>>& boundary,
                 const QVector<QVector<std::shared_ptr<Point>>>& holes)
    : outer_boundary(boundary), inner_holes(holes) {}

QVector<std::shared_ptr<Point>> Polygon::outer() const
{
    return outer_boundary;
}

QVector<QVector<std::shared_ptr<Point>>> Polygon::inners() const
{
    return inner_holes;
}

void Polygon::setMaxAllowedSpeed(
    const units::velocity::meters_per_second_t newMaxSpeed)
{
    mMaxSpeed = newMaxSpeed;
}

units::velocity::meters_per_second_t Polygon::getMaxAllowedSpeed() const
{
    return mMaxSpeed;
}

bool Polygon::pointIsInPolygon(const Point& pointToCheck) const
{
    // Initialize count of intersections to 0
    int count = 0;

    // Loop through each edge in the polygon
    for (size_t i = 0, j = outer_boundary.size() - 1;
         i < outer_boundary.size(); j = i++)
    {
        auto xi = outer_boundary[i]->x().value(),
            yi = outer_boundary[i]->y().value();
        auto xj = outer_boundary[j]->x().value(),
            yj = outer_boundary[j]->y().value();

        // Check if the pointToCheck lies on an edge, if so, return true
        if ((yi > pointToCheck.y().value()) !=
                (yj > pointToCheck.y().value()) &&
            pointToCheck.x().value() <
                (xj - xi) * (pointToCheck.y().value() - yi) /
                        (yj - yi) + xi)
        {
            count++;
        }
    }

    // If the count of intersections is odd,
    // return true (inside). Otherwise, return false (outside).
    return count % 2 != 0;
}

bool Polygon::PointIsPolygonStructure(const Point& pointToCheck) const
{
    auto it = std::find_if(outer_boundary.begin(),
                           outer_boundary.end(),
                           [pointToCheck](const std::shared_ptr<Point>& p)
                           {
                               return *p == pointToCheck;
                           });
    if (it != outer_boundary.end())
    {
        return true;
    }

    for (const auto & poly: inner_holes)
    {
        auto it = std::find_if(poly.begin(),
                               poly.end(),
                               [pointToCheck](const std::shared_ptr<Point>& p)
                               {
                                   return *p == pointToCheck;
                               });
        if (it != poly.end())
        {
            return true;
        }
    }

    return false;
}

bool Polygon::PointIsPolygonStructure(
    const std::shared_ptr<Point>& pointToCheck) const
{
    auto it = std::find_if(outer_boundary.begin(),
                           outer_boundary.end(),
                           [&pointToCheck]
                           (const std::shared_ptr<Point>& p)
                           {
                               return *p == *pointToCheck ||
                                      p == pointToCheck;
                           });

    if (it != outer_boundary.end())
    {
        return true;
    }

    for (const auto& poly : inner_holes) {
        auto it = std::find_if(poly.begin(),
                               poly.end(),
                               [&pointToCheck]
                               (const std::shared_ptr<Point>& p)
                               {
                                   return *p == *pointToCheck ||
                                          p == pointToCheck;
                               });

        if (it != poly.end()) {
            return true;
        }
    }

    return false;
}

units::area::square_meter_t Polygon::area() const
{
    units::area::square_meter_t result =
        units::area::square_meter_t(0.0);

    size_t n = outer_boundary.size();

    // Calculate the area for the outer boundary using the shoelace formula
    for (size_t i = 0; i < n; ++i)
    {
        units::length::meter_t x1 = outer_boundary[i]->x();
        units::length::meter_t y1 = outer_boundary[i]->y();
        units::length::meter_t x2 = outer_boundary[(i + 1) % n]->x();
        units::length::meter_t y2 = outer_boundary[(i + 1) % n]->y();

        result += (x1 * y2) - (x2 * y1);
    }

    // Subtract the area of the inner holes using the shoelace formula
    for (const auto& hole : inner_holes)
    {
        size_t m = hole.size();
        for (size_t i = 0; i < m; ++i)
        {
            units::length::meter_t x1 = hole[i]->x();
            units::length::meter_t y1 = hole[i]->y();
            units::length::meter_t x2 = hole[(i + 1) % m]->x();
            units::length::meter_t y2 = hole[(i + 1) % m]->y();

            result -= (x1 * y2) - (x2 * y1);
        }
    }

    return units::math::abs(result) / ((double)2.0);
}


units::length::meter_t Polygon::perimeter() const {
    units::length::meter_t p = units::length::meter_t(0.0);

    // Calculate the perimeter of the outer_boundary only
    for (size_t i = 0; i < outer_boundary.size(); ++i)
    {
        // wrap around to the first point
        size_t j = (i + 1) % outer_boundary.size();
        auto line = Line(outer_boundary[i], outer_boundary[j]);
        p += line.length();
    }

    return p;
}

bool Polygon::intersects(const std::shared_ptr<Line> line)
{
    // Check the outer boundary
    for (size_t i = 0; i < outer_boundary.size(); ++i)
    {
        size_t j = (i + 1) % outer_boundary.size(); // Next point
        Line edge = Line(outer_boundary[i],
                         outer_boundary[j]);

        if (line->intersects(edge))
        {
            return true;
        }
    }

    // Check the inner holes
    for (const auto& hole : inner_holes)
    {
        for (size_t i = 0; i < hole.size(); ++i)
        {
            size_t j = (i + 1) % hole.size(); // Next point
            Line edge = Line(hole[i], hole[j]);

            if (line->intersects(edge))
            {
                return true;
            }
        }
    }

    return false;
}

QVector<std::shared_ptr<Point>> Polygon::offsetBoundary(
    const QVector<std::shared_ptr<Point>>& boundary,
    bool inward,
    units::length::meter_t offset
    ) const {
    QVector<std::shared_ptr<Point>> new_boundary;
    for (size_t i = 0; i < boundary.size(); ++i) {
        auto& p1 = boundary[i];
        auto& p2 = boundary[(i + 1) % boundary.size()];
        auto& p3 = boundary[(i + 2) % boundary.size()];

        // Calculate unit vector from p1 to p2
        auto dx1 = p2->x().value() - p1->x().value();
        auto dy1 = p2->y().value() - p1->y().value();
        auto length1 = std::sqrt(dx1 * dx1 + dy1 * dy1);
        auto ux1 = dx1 / length1;
        auto uy1 = dy1 / length1;

        // Calculate unit vector from p2 to p3
        auto dx2 = p3->x().value() - p2->x().value();
        auto dy2 = p3->y().value() - p2->y().value();
        auto length2 = std::sqrt(dx2 * dx2 + dy2 * dy2);
        auto ux2 = dx2 / length2;
        auto uy2 = dy2 / length2;

        // Calculate average unit vector
        auto ax = (ux1 + ux2) / 2.0;
        auto ay = (uy1 + uy2) / 2.0;

        // Normalize the average vector
        auto alength = std::sqrt(ax * ax + ay * ay);
        ax /= alength;
        ay /= alength;

        // Reverse direction for outward offset (used for holes)
        if (!inward) {
            ax = -ax;
            ay = -ay;
        }

        // Offset the point p2
        auto new_x = p2->x() + offset * ax;
        auto new_y = p2->y() + offset * ay;

        // Store the new point
        new_boundary.push_back(std::make_shared<Point>(new_x, new_y));
    }
    return new_boundary;
}



units::length::meter_t Polygon::getMaxClearWidth(const Line& line) const
{
    units::length::meter_t leftClearWidth =
        std::numeric_limits<units::length::meter_t>::max();
    units::length::meter_t rightClearWidth =
        std::numeric_limits<units::length::meter_t>::max();

    auto calculateClearWidths =
        [&leftClearWidth, &rightClearWidth, &line]
        (const QVector<std::shared_ptr<Point>>& vertices)
    {
        for (size_t i = 0; i < vertices.size(); ++i)
            {
            std::shared_ptr<Point> vertexA =
                vertices[i];
            std::shared_ptr<Point> vertexB =
                vertices[(i + 1) % vertices.size()];

            Line edge(vertexA, vertexB);

            units::length::meter_t dist1 =
                edge.getPerpendicularDistance(*(line.startPoint()));
            units::length::meter_t dist2 =
                edge.getPerpendicularDistance(*(line.endPoint()));

            Line::LocationToLine isLeft1 = line.getlocationToLine(vertexA);
            Line::LocationToLine isLeft2 = line.getlocationToLine(vertexB);

            if (isLeft1 == Line::LocationToLine::left)
            {
                leftClearWidth = std::min(leftClearWidth, dist1);
            }
            else if (isLeft1 == Line::LocationToLine::right)
            {
                rightClearWidth = std::min(rightClearWidth, dist1);
            }

            if (isLeft2 == Line::LocationToLine::left)
            {
                leftClearWidth = std::min(leftClearWidth, dist2);
            }
            else if (isLeft2 == Line::LocationToLine::right)
            {
                rightClearWidth = std::min(rightClearWidth, dist2);
            }

        }
    };

    // Invoke the lambda function
    calculateClearWidths(outer_boundary);

    for (auto& hole: inner_holes)
    {
        calculateClearWidths(hole);
    }

    return rightClearWidth + leftClearWidth;
}



QString Polygon::toString()
{
    QString str =
        QString("Polygon Perimeter: %1 || Area: %2 || "
                "# of Points: %3 || # of holes")
            .arg(perimeter().value())
            .arg(area().value())
            .arg(outer_boundary.size())
            .arg(inner_holes.size());

    return str;
}
