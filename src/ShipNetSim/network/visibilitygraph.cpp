#include "visibilitygraph.h"
#include "VisibilityGraph.h"
#include <boost/graph/dijkstra_shortest_paths.hpp>
#include <map>
#include "line.h"

VisibilityGraph::VisibilityGraph(Point *startNode,
                                 Point *endNode,
                                 Polygon *polygon)
    : startNode(startNode), endNode(endNode), polygon(polygon)
{
    // check if the start and end points are within the polygon boundaries
    if (!boost::geometry::within(*startNode, polygon->internal_polygon))
    {
        throw std::invalid_argument("The start node is not within "
                                    "the boundary of the polygon.");
    }

    if (!boost::geometry::within(*endNode, polygon->internal_polygon))
    {
        throw std::invalid_argument("The end node is not within "
                                    "the boundary of the polygon.");
    }
}

VisibilityGraph::VisibilityGraph(Polygon *polygon)
    : startNode(nullptr), endNode(nullptr), polygon(polygon) {}


void VisibilityGraph::setStartPoint(Point *startPoint)
{
    if (!startPoint->isValid())
    {
        throw std::invalid_argument("The node is not a valid node!");
    }

    startNode = startPoint;

    // check if the start point is within the polygon boundaries
    if (!boost::geometry::within(startNode, polygon->internal_polygon))
    {
        throw std::invalid_argument("The start node is not within "
                                    "the boundary of the polygon.");
    }
}

void VisibilityGraph::setEndPoint(Point *endPoint)
{
    if (!endPoint->isValid())
    {
        throw std::invalid_argument("The node is not a valid node!");
    }

    endNode = endPoint;

    // check if the end point is within the polygon boundaries
    if (!boost::geometry::within(endNode, polygon->internal_polygon))
    {
        throw std::invalid_argument("The end node is not within "
                                    "the boundary of the polygon.");
    }
}

Point* VisibilityGraph::startPoint()
{
    return startNode;
}

Point* VisibilityGraph::endPoint()
{
    return endNode;
}



void VisibilityGraph::removeVerticesAndEdges(Point* nodeToRemove)
{
    // Find the vertex descriptor corresponding to
    // the node you want to remove
    auto it = pointToVertex.find(*nodeToRemove);
    if (it == pointToVertex.end()) {
        return;
    }

    // check if the point is in the polygon structure
    // if yes, dont remove it
    if (polygon->pointIsInPolygon(*nodeToRemove))
    {
        return;
    }

    boost::graph_traits<BoostGraph>::vertex_descriptor vertexToRemove =
        it->second;

    // Remove all edges associated with this vertex
    boost::clear_vertex(vertexToRemove, boostGraph);

    // Remove the vertex from the graph
    boost::remove_vertex(vertexToRemove, boostGraph);

    // Remove the vertex mapping from pointToVertex
    pointToVertex.erase(it);
}


void VisibilityGraph::buildGraph(
    units::velocity::meters_per_second_t maxSpeed)
{
    if (! startNode || ! endNode)
    {
        throw std::exception("Both start point and "
                             "end point need to be defined"
                             "to construct the visibility graph!");
    }
    removeVerticesAndEdges(startNode);
    removeVerticesAndEdges(endNode);

//    boostGraph.clear();
//    pointToVertex.clear();

    // Add start and end nodes to the graph and to the mapping
    startVertex = add_vertex(boostGraph);
    endVertex = add_vertex(boostGraph);
    pointToVertex[*startNode] = startVertex;
    pointToVertex[*endNode] = endVertex;

    // Add vertices for all points in the polygon and populate the mapping
    for (const auto& point : polygon->internal_polygon.outer())
    {
        if (pointToVertex.find(point) == pointToVertex.end())
        {
            auto vertex = add_vertex(boostGraph);
            pointToVertex[point] = vertex;
        }
    }

    // Create edges based on visibility.
    for (const auto& srcPoint : pointToVertex)
    {
        for (const auto& destPoint : pointToVertex)
        {
            if (srcPoint.first == destPoint.first) continue; // Skip self-loops

            std::shared_ptr<Line> segment =
                std::make_shared<Line>(srcPoint.first,
                                       destPoint.first, maxSpeed);

            if (!segment->intersects(polygon->internal_polygon))
            {
                // Check if the edge already exists.
                std::pair<boost::graph_traits<BoostGraph>::edge_descriptor,
                          bool> edgeCheck =
                    boost::edge(srcPoint.second,
                                destPoint.second,
                                boostGraph);

                // Only add the edge if it doesn't already exist.
                if (!edgeCheck.second)
                {
                    EdgeProperty edge_properties(segment->length(), segment);
                    // Add an edge with this segment as a property.
                    add_edge(srcPoint.second, destPoint.second,
                             edge_properties, boostGraph);
                }
            }
        }
    }
}

std::vector<std::shared_ptr<Line>> VisibilityGraph::dijkstraShortestPath()
{
    std::vector<std::shared_ptr<Line>> result;

    std::vector<boost::graph_traits<BoostGraph>::vertex_descriptor>
        predecessors(num_vertices(boostGraph));

    auto pred_map_var = boost::make_iterator_property_map(
        predecessors.begin(),
        get(boost::vertex_index, boostGraph)
        );

    // Run Dijkstra's algorithm
    boost::dijkstra_shortest_paths(
        boostGraph, startVertex,
        predecessor_map(pred_map_var)
            .weight_map(get(boost::edge_weight, boostGraph))
        );

    // Create the result vector based on predecessors
    auto currentVertex = endVertex;
    while (currentVertex != startVertex)
    {
        auto predVertex = predecessors[currentVertex];
        auto edge_result = edge(predVertex, currentVertex, boostGraph);
        if (edge_result.second)
        { // check that the edge exists
            boost::property_map<BoostGraph,
                                boost::edge_name_t>::type edgeNameMap =
                get(boost::edge_name, boostGraph);
            result.push_back(edgeNameMap[edge_result.first]);
        }
        currentVertex = predVertex;
    }
    std::reverse(result.begin(), result.end());

    return result;
}
