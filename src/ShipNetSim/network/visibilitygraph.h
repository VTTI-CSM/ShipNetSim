#ifndef VISIBILITYGRAPH_H
#define VISIBILITYGRAPH_H

#include "Point.h"
#include "Polygon.h"
#include "Line.h"
#include <vector>

class VisibilityGraph
{
private:
    Point startNode;
    Point endNode;
    Polygon polygon;
    std::vector<Line> segments;
public:
    VisibilityGraph(const Point& startNode,
                    const Point& endNode,
                    const Polygon& polygon);

    VisibilityGraph(const Polygon& polygon);
    void setStartPoint(const Point& startPoint);
    void setEndPoint(const Point& endPoint);
    Point startPoint() const;
    Point endPoint() const;

    void buildGraph();
    std::vector<Point> dijkstraShortestPath();
    std::vector<Point> aStarShortestPath();
};

#endif // VISIBILITYGRAPH_H
