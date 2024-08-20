#include "visibilitygraph.h"
#include <boost/graph/dijkstra_shortest_paths.hpp>
#include <map>
#include "line.h"
#include <queue>

//VisibilityGraph::VisibilityGraph(std::shared_ptr<Point> startNode,
//                                 std::shared_ptr<Point> endNode,
//                                 std::shared_ptr<Polygon> polygon)
//    : startNode(startNode), endNode(endNode), polygon(polygon)
//{
//    // check if the start and end points are within
//    // the polygon boundaries
//    if (! polygon->pointIsInPolygon(*startNode))
//    {
//        throw std::invalid_argument("The start point is not within "
//                                    "the boundary of the polygon.");
//    }

//    if (! polygon->pointIsInPolygon(*endNode))
//    {
//        throw std::invalid_argument("The end point is not within "
//                                    "the boundary of the polygon.");
//    }
//}

VisibilityGraph::VisibilityGraph(QVector<std::shared_ptr<Point> > points,
                                 std::shared_ptr<Polygon> polygon)
    : mustTraversePoints(points), polygon(polygon)
{
    if (mustTraversePoints.size() < 2)
    {
        throw std::invalid_argument("The must traverse points vector"
                                    "cannot have less than two points!");
    }
}

VisibilityGraph::VisibilityGraph(std::shared_ptr<Polygon> polygon)
    : polygon(polygon) {}

VisibilityGraph::~VisibilityGraph() {}


void VisibilityGraph::setTraversePoints(QVector<std::shared_ptr<Point>> points)
{
    mustTraversePoints = points;

    if (mustTraversePoints.size() < 2)
    {
        throw std::invalid_argument("The must traverse points vector"
                                    "cannot have less than two points!");
    }
}


std::shared_ptr<Point> VisibilityGraph::startPoint()
{
    return mustTraversePoints.front();
}

std::shared_ptr<Point> VisibilityGraph::endPoint()
{
    return mustTraversePoints.back();
}



void VisibilityGraph::
    removeVerticesAndEdges(std::shared_ptr<Point> nodeToRemove)
{
    // If the point is not in the graph, skip
    if (graph.find(nodeToRemove) == graph.end())
    {
        return;
    }

    // If the point is in the polygon points, skip
    if (polygon->contains(nodeToRemove))
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



void VisibilityGraph::buildGraph()
{
    if (mustTraversePoints.empty())
    {
        throw std::runtime_error("mustTraversePoints vector need to be "
                                 "defined to construct "
                                 "the visibility graph!");
    }

    // Clear the existing graph
    for (auto& point: mustTraversePoints)
    {
        removeVerticesAndEdges(point);
    }

    // Empty vector to hold all points
    QVector<std::shared_ptr<Point>> allPoints;

    // Function to add a point if it's not already in the vector
    auto addIfUnique = [&allPoints](const std::shared_ptr<Point>& newPoint)
    {
        for (const auto& point : allPoints)
        {
            if (*point == *newPoint)
            {
                return; // Point is already in the vector, don't add it again
            }
        }
        // If we got here, the point is not in the vector, so we add it
        allPoints.append(newPoint);
    };

    // Add points from the outer boundary of the polygon
    for (const auto& point : polygon->outer())
    {
        addIfUnique(point);
    }

    // Add points from the inner holes of the polygon
    for (const auto& hole : polygon->inners())
    {
        for (const auto& point : hole) {
            addIfUnique(point);
        }
    }

    // Add points from the must traverse points
    for (const auto& point : mustTraversePoints)
    {
        addIfUnique(point);
    }


    // Create lines between every pair of points
    for (const auto& pointA : allPoints)
    {
        QVector<std::pair<std::shared_ptr<Line>,
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
                line = std::make_shared<Line>(pointA,
                                              pointB);
                auto wdth = polygon->getMaxClearWidth(*line);
                line->setTheoriticalWidth(wdth);
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
        graph[pointA] += visibleLines;
    }
}

ShortestPathResult VisibilityGraph::dijkstraShortestPath()
{
    ShortestPathResult result;

    // Check if we have enough points to form a path
    if (mustTraversePoints.size() < 2) {
        // If there's only one point or none, return empty
        // result or result with only one point
        if (!mustTraversePoints.isEmpty()) {
            result.points.append(mustTraversePoints.first());
        }
        return result;
    }

    // We start with the first point
    result.points.append(mustTraversePoints.first());

    for (qsizetype i = 0; i < mustTraversePoints.size() - 1; ++i)
    {
        auto startPoint = mustTraversePoints[i];
        auto endPoint = mustTraversePoints[i + 1];

        // Compute the shortest path between startPoint and endPoint
        ShortestPathResult dijResult =
            dijkstraShortestPath(startPoint, endPoint);

        // We've already added the startPoint to the path,
        // so we start with the second point
        for (qsizetype j = 1; j < dijResult.points.size(); ++j)
        {
            result.points.append(dijResult.points[j]);
        }

        // Append the lines connecting these points to the result's lines
        // Since lines are connecting the points, the number of lines
        // should always be one less than the number of points
        for (auto& line : dijResult.lines)
        {
            result.lines.append(line);
        }
    }

    return result;
}


ShortestPathResult VisibilityGraph::dijkstraShortestPath(
    std::shared_ptr<Point> startPoint,
    std::shared_ptr<Point> endPoint)
{
    using Pair = std::pair<units::length::meter_t,
                           std::shared_ptr<Point>>;

    auto comp = [](const Pair& a, const Pair& b)
    {
        return a.first > b.first;
    };
    std::priority_queue<Pair,
                        QVector<Pair>,
                        decltype(comp)> pq(comp);
    std::unordered_map<std::shared_ptr<Point>,
                       units::length::meter_t> dist;
    std::unordered_map<std::shared_ptr<Point>,
                       std::shared_ptr<Line>> prevLine;

    pq.emplace(0, startPoint);
    dist[startPoint] = units::length::meter_t(0.0);

    while (!pq.empty())
    {
        auto [currentDist, currentPoint] = pq.top();
        pq.pop();

        if (currentPoint == endPoint)
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
    if (dist.find(endPoint) == dist.end())
    {
        return result;  // Empty result signifies no path exists
    }

    std::shared_ptr<Point> currentPoint = endPoint;
    while (currentPoint != startPoint)
    {
        auto line = prevLine[currentPoint];
        result.lines.prepend(line);
        result.points.prepend(currentPoint);

        currentPoint = (line->startPoint() == currentPoint) ?
                           line->endPoint() : line->startPoint();
    }
    result.points.prepend(startPoint);

    return result;
}

QVector<std::shared_ptr<Point>>
VisibilityGraph::getPointsFromLines(
    const QVector<std::shared_ptr<Line>>& pathLines)
{
    if (pathLines.empty()) {
        return {};
    }

    QVector<std::shared_ptr<Point>> points;

    // Start with the provided startPoint
    points.push_back(startPoint());

    for (const auto& line : pathLines) {
        const auto lastPoint = points.back();

        if (*(line->startPoint()) == *lastPoint)
        {
            points.push_back(line->endPoint());
        }
        else if (*(line->endPoint()) == *lastPoint)
        {
            points.push_back(line->startPoint());
        }
        else
        {
            throw std::runtime_error("Non-contiguous lines in path.");
        }
    }

    return points;
}

QVector<std::shared_ptr<Line>>
VisibilityGraph::getLinesFromPoints(
    const QVector<std::shared_ptr<Point>>& pathPoints)
{
    QVector<std::shared_ptr<Line>> lines;

    for (qsizetype i = 0; i < pathPoints.size() - 1; ++i) {
        auto& pointA = pathPoints[i];
        auto& pointB = pathPoints[i + 1];

        // Check if pointA is in the graph
        auto it = graph.find(pointA);
        if (it == graph.end())
        {
            // Point A not found in graph, so add it
            graph[pointA] =
                QVector<std::pair<std::shared_ptr<Line>,
                                  units::length::meter_t>>();
        }

        // Try to find the line corresponding to
        // pointA and pointB in the graph
        bool lineFound = false;
        for (const auto& [line, _] : graph[pointA]) {
            if ((*(line->startPoint()) == *pointA &&
                 *(line->endPoint()) == *pointB) ||
                (*(line->startPoint()) == *pointB &&
                                                                                         *(line->endPoint()) == *pointA))
            {
                lines.push_back(line);
                lineFound = true;
                break;
            }
        }

        if (!lineFound) {
            // Create new line, add to the vector, and add to the graph
            auto newLine =
                std::make_shared<Line>(pointA,
                                       pointB);
            lines.push_back(newLine);

            // Add the new line to the graph for both pointA and pointB
            graph[pointA].emplace_back(newLine, newLine->length());
            graph[pointB].emplace_back(newLine, newLine->length());
        }
    }

    return lines;
}

