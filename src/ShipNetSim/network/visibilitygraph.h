#ifndef VISIBILITYGRAPH_H
#define VISIBILITYGRAPH_H

#include "Point.h"
#include "Polygon.h"
#include "Line.h"
#include <vector>
#include <unordered_map>
#include "../../third_party/units/units.h"
#include "VisibilityGraph.h"

struct ShortestPathResult {
    std::vector<std::shared_ptr<Line>> lines;
    std::vector<std::shared_ptr<Point>> points;
};

class VisibilityGraph
{
private:
    std::shared_ptr<Point> startNode;
    std::shared_ptr<Point> endNode;
    std::shared_ptr<Polygon> polygon;
    std::unordered_map<std::shared_ptr<Point>,
                       std::vector<std::pair<std::shared_ptr<Line>,
                                             units::length::meter_t>>> graph;

    void removeVerticesAndEdges(std::shared_ptr<Point> nodeToRemove);


public:



    VisibilityGraph(std::shared_ptr<Point> startNode,
                    std::shared_ptr<Point> endNode,
                    std::shared_ptr<Polygon> polygon);
    VisibilityGraph(std::shared_ptr<Polygon> polygon);
    ~VisibilityGraph();

    void setStartPoint(std::shared_ptr<Point> startPoint);
    void setEndPoint(std::shared_ptr<Point> endPoint);
    std::shared_ptr<Point> startPoint();
    std::shared_ptr<Point> endPoint();

    void buildGraph(
        units::velocity::meters_per_second_t maxSpeed);

    ShortestPathResult dijkstraShortestPath();
};

#endif // VISIBILITYGRAPH_H
