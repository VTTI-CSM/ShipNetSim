#ifndef VISIBILITYGRAPH_H
#define VISIBILITYGRAPH_H

#include "Point.h"
#include "Polygon.h"
#include "Line.h"
#include <vector>
#include <unordered_map>
#include <boost/graph/adjacency_list.hpp>
#include "../../third_party/units/units.h"
#include "VisibilityGraph.h"

typedef boost::property<
    boost::edge_weight_t,
    double,
    boost::property<
        boost::edge_name_t, std::shared_ptr<Line>
        >
    > EdgeProperty;
typedef boost::adjacency_list<boost::vecS, boost::vecS,
                              boost::undirectedS, boost::no_property,
                              EdgeProperty> BoostGraph;



class VisibilityGraph
{
private:
    Point* startNode;
    Point* endNode;
    Polygon* polygon;
    BoostGraph boostGraph;
    std::unordered_map<Point,
                       boost::graph_traits<BoostGraph>::vertex_descriptor>
        pointToVertex;
    boost::graph_traits<BoostGraph>::vertex_descriptor startVertex;
    boost::graph_traits<BoostGraph>::vertex_descriptor endVertex;

    void removeVerticesAndEdges(Point* nodeToRemove);

public:
    VisibilityGraph(Point *startNode, Point *endNode, Polygon *polygon);
    VisibilityGraph(Polygon *polygon);
    ~VisibilityGraph();

    void setStartPoint(Point *startPoint);
    void setEndPoint(Point *endPoint);
    Point* startPoint();
    Point* endPoint();

    void buildGraph(
        units::velocity::meters_per_second_t maxSpeed);

    std::vector<std::shared_ptr<Line> > dijkstraShortestPath();
};

#endif // VISIBILITYGRAPH_H
