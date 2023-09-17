#include "visibilitygraph.h"
#include "VisibilityGraph.h"
#include <boost/graph/dijkstra_shortest_paths.hpp>
#include <map>
#include "line.h"
#include <queue>

VisibilityGraph::VisibilityGraph(std::shared_ptr<Point> startNode,
                                 std::shared_ptr<Point> endNode,
                                 std::shared_ptr<Polygon> polygon)
    : startNode(startNode), endNode(endNode), polygon(polygon)
{
    // check if the start and end points are within
    // the polygon boundaries
    if (! polygon->pointIsInPolygon(*startNode))
    {
        throw std::invalid_argument("The start point is not within "
                                    "the boundary of the polygon.");
    }

    if (! polygon->pointIsInPolygon(*endNode))
    {
        throw std::invalid_argument("The end point is not within "
                                    "the boundary of the polygon.");
    }
}

VisibilityGraph::VisibilityGraph(std::shared_ptr<Polygon> polygon)
    : polygon(polygon) {}

VisibilityGraph::~VisibilityGraph() {}


void VisibilityGraph::setStartPoint(std::shared_ptr<Point> startPoint)
{
    if (!startPoint->isValid())
    {
        throw std::invalid_argument("The point is not a valid point!");
    }
    // check if the start point is within the polygon boundaries
    if (! polygon->pointIsInPolygon(*startNode))
    {
        throw std::invalid_argument("The start point is not within "
                                    "the boundary of the polygon.");
    }

    startNode = startPoint;

}

void VisibilityGraph::setEndPoint(std::shared_ptr<Point> endPoint)
{
    if (!endPoint)
    {
        throw std::invalid_argument("The point is not a valid point!");
    }
    if (!endPoint->isValid())
    {
        throw std::invalid_argument("The point is not a valid point!");
    }

    endNode = endPoint;

    // check if the end point is within the polygon boundaries
    if (! polygon->pointIsInPolygon(*endNode))
    {
        throw std::invalid_argument("The end point is not within "
                                    "the boundary of the polygon.");
    }
}

std::shared_ptr<Point> VisibilityGraph::startPoint()
{
    return startNode;
}

std::shared_ptr<Point> VisibilityGraph::endPoint()
{
    return endNode;
}



void VisibilityGraph::
    removeVerticesAndEdges(std::shared_ptr<Point> nodeToRemove)
{
    // Check if the point exists in the graph
    if (graph.find(nodeToRemove) == graph.end())
    {
        return;
    }

    // Check if the point is in the polygon structure
    if (polygon->PointIsPolygonStructure(nodeToRemove))
    {
        return;
    }

    // Remove all edges that include this vertex from the graph
    for (auto& kv : graph) {
        kv.second.erase(
            std::remove_if(
                kv.second.begin(),
                kv.second.end(),
                [nodeToRemove]
                (const std::pair<std::shared_ptr<Line>,
                                 units::length::meter_t>& lineAndLength)
                {
                    return lineAndLength.first->startPoint() == nodeToRemove ||
                           lineAndLength.first->endPoint() == nodeToRemove;
                }),
            kv.second.end());
    }

    // Remove the vertex from the graph
    graph.erase(nodeToRemove);
}



void VisibilityGraph::buildGraph(
    units::velocity::meters_per_second_t maxSpeed)
{
    if (!startNode || !endNode)
    {
        throw std::runtime_error("Both start point and "
                                 "end point need to be "
                                 "defined to construct "
                                 "the visibility graph!");
    }

    // Clear the existing graph
    removeVerticesAndEdges(startNode);
    removeVerticesAndEdges(endNode);

    // Empty vector to hold all points
    std::vector<std::shared_ptr<Point>> allPoints = {};

    // Add points from the outer boundary of the polygon
    const auto& outerPoints = polygon->outer();
    allPoints.insert(allPoints.end(),
                     outerPoints.begin(),
                     outerPoints.end());

    // Add points from the inner holes of the polygon
    const auto& holes = polygon->inners();
    for (const auto& hole : holes)
    {
        allPoints.insert(allPoints.end(),
                         hole.begin(),
                         hole.end());
    }

    // Add the start and end points to the end; they have lower priority
    allPoints.push_back(startNode);
    allPoints.push_back(endNode);

    // Create lines between every pair of points
    for (const auto& pointA : allPoints)
    {
        std::vector<std::pair<std::shared_ptr<Line>,
                              units::length::meter_t>> visibleLines;

        for (const auto& pointB : allPoints)
        {
            if (pointA == pointB) continue;

            // Find if a similar line already exists
            auto it =
                std::find_if(
                    visibleLines.begin(), visibleLines.end(),
                    [pointA, pointB](const auto& linePair)
                {
                        return (linePair.first->startPoint() == pointA &&
                                linePair.first->endPoint() == pointB) ||
                               (linePair.first->startPoint() == pointB &&
                                linePair.first->endPoint() == pointA);
                    });

            std::shared_ptr<Line> line;

            if (it != visibleLines.end())
            {
                // Use the existing line
                line = it->first;
            } else {
                // Create a new line
                line = std::make_shared<Line>(pointA, pointB, maxSpeed);
            }

            if (!polygon->intersects(line))
            {
                // Line is "visible"; add to the list of visible
                // lines for pointA
                visibleLines.emplace_back(line, line->length());

                // Add the line to pointB's visibility list as well,
                // to make it undirected
                graph[pointB].emplace_back(line, line->length());
            }
        }

        // Append the new visible lines to existing ones for pointA
        graph[pointA].insert(graph[pointA].end(),
                             visibleLines.begin(),
                             visibleLines.end());
    }
}

ShortestPathResult VisibilityGraph::dijkstraShortestPath()
{
    using Pair = std::pair<units::length::meter_t,
                           std::shared_ptr<Point>>;

    auto comp = [](const Pair& a, const Pair& b)
    {
        return a.first > b.first;
    };
    std::priority_queue<Pair,
                        std::vector<Pair>,
                        decltype(comp)> pq(comp);
    std::unordered_map<std::shared_ptr<Point>,
                       units::length::meter_t> dist;
    std::unordered_map<std::shared_ptr<Point>,
                       std::shared_ptr<Line>> prevLine;

    pq.emplace(0, startNode);
    dist[startNode] = units::length::meter_t(0.0);

    while (!pq.empty())
    {
        auto [currentDist, currentPoint] = pq.top();
        pq.pop();

        if (currentPoint == endNode)
        {
            break;
        }

        for (auto& [nextLine, lineLength] : graph[currentPoint])
        {
            auto nextPoint =
                (nextLine->startPoint() == currentPoint) ?
                                 nextLine->endPoint() :
                                 nextLine->startPoint();

            units::length::meter_t newDist =
                currentDist + lineLength;

            if (newDist < dist[nextPoint] ||
                dist.find(nextPoint) == dist.end())
            {
                pq.emplace(newDist, nextPoint);
                dist[nextPoint] = newDist;
                prevLine[nextPoint] = nextLine;
            }
        }
    }

    ShortestPathResult result;
    if (dist.find(endNode) == dist.end())
    {
        return result;  // Empty result signifies no path exists
    }

    std::shared_ptr<Point> currentPoint = endNode;
    while (currentPoint != startNode)
    {
        auto line = prevLine[currentPoint];
        result.lines.insert(result.lines.begin(), line);
        result.points.insert(result.points.begin(), currentPoint);

        currentPoint = (line->startPoint() == currentPoint) ?
                           line->endPoint() : line->startPoint();
    }
    result.points.insert(result.points.begin(), startNode);

    return result;
}
