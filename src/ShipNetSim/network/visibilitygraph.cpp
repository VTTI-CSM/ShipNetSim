#include "visibilitygraph.h"
#include "line.h"
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <limits>
#include <algorithm>

VisibilityGraph::VisibilityGraph(const Point& startNode,
                                 const Point& endNode,
                                 const Polygon& polygon)
    : startNode(startNode), endNode(endNode), polygon(polygon)
{
    // check if the start and end points are within the polygon boundaries
    if (!boost::geometry::within(startNode, polygon.internal_polygon))
    {
        throw std::invalid_argument("The start node is not within "
                                    "the boundary of the polygon.");
    }

    if (!boost::geometry::within(endNode, polygon.internal_polygon))
    {
        throw std::invalid_argument("The end node is not within "
                                    "the boundary of the polygon.");
    }
}


void VisibilityGraph::buildGraph()
{
    // make sure the segments are empty before proceeding
    segments.clear();

    // Add line segments from startNode to all points of the polygon
    for (const auto& point : polygon.internal_polygon.outer()) {
        Line segment(startNode, point);
        if (!segment.intersects(polygon.internal_polygon)) {
            segments.push_back(segment);
        }
    }

    // Add line segments from all points of the polygon to endNode
    for (const auto& point : polygon.internal_polygon.outer()) {
        Line segment(point, endNode);
        if (!segment.intersects(polygon.internal_polygon)) {
            segments.push_back(segment);
        }
    }

    // Add line segments between all pairs of points on the
    // polygon's outer boundary
    const auto& points = polygon.internal_polygon.outer();
    for (size_t i = 0; i < points.size(); ++i) {
        for (size_t j = i + 1; j < points.size(); ++j) {
            Line segment(points[i], points[j]);
            if (!segment.intersects(polygon.internal_polygon)) {
                segments.push_back(segment);
            }
        }
    }
}

std::vector<Point> VisibilityGraph::dijkstraShortestPath()
{
    // Define a custom comparison function for the priority queue.
    // This is used to compare the pairs in the priority queue
    // based on their distances.
    auto comp = [](const std::pair<long double, Point>& a,
                   const std::pair<long double, Point>& b) {
        return a.first > b.first;
    };

    // Initialize a priority queue to store <distance, Point> pairs.
    // The queue is sorted based on the distances.
    std::priority_queue<std::pair<long double, Point>,
                        std::vector<std::pair<long double, Point>>,
                        decltype(comp)> pq(comp);

    // Create a hash map to store the shortest distance from the
    // startNode to each Point.
    std::unordered_map<Point, long double> distances;

    // Create a hash map to store the predecessor of each Point
    // on the path.
    std::unordered_map<Point, Point> predecessors;

    // Create a hash set to store the visited Points.
    std::unordered_set<Point> visited;


    // Push the startNode into the priority queue with a distance of 0.
    pq.push({0.0L, startNode});
    distances[startNode] = 0.0L;

    // Main loop of the Dijkstra's algorithm
    while (!pq.empty())
    {
        // Pop the Point with the smallest distance.
        long double currDist = pq.top().first;
        Point currPoint = pq.top().second;
        pq.pop();

        // If the Point has already been visited, skip it.
        if (visited.find(currPoint) != visited.end()) continue;
        visited.insert(currPoint);

        // If we reached the destination, break the loop.
        if (currPoint == endNode) break; // Reached the destination


        // Iterate through all connected segments to the current Point.
        for (const Line& segment : segments)
        {
            Point neighbor;

            // Determine the neighboring Point connected by the segment.
            if (segment.startPoint() == currPoint) {
                neighbor = segment.endPoint();
            } else if (segment.endPoint() == currPoint) {
                neighbor = segment.startPoint();
            } else {
                continue;
            }

            // If the neighbor has already been visited, skip it.
            if (visited.find(neighbor) != visited.end()) continue;

            // Calculate the new distance to the neighbor.
            auto endp = segment.endPoint();
            long double newDist = currDist +
                                  segment.startPoint().distance(endp);

            // If the new distance is shorter, update the
            // distance and predecessor.
            if (newDist < distances[neighbor] ||
                distances.find(neighbor) == distances.end()) {
                pq.push({newDist, neighbor});
                distances[neighbor] = newDist;
                predecessors[neighbor] = currPoint;
            }
        }
    }

    // Reconstruct the shortest path from endNode to startNode.
    std::vector<Point> path;
    // If there is no path from startNode to endNode, return an empty path.
    if (predecessors.find(endNode) == predecessors.end())
    {
        return path; // No path found
    }

    // Build the path by backtracking from the endNode to the startNode.
    for (Point at = endNode; at != startNode; at = predecessors[at]) {
        path.push_back(at);
    }
    path.push_back(startNode);

    // Reverse the path to go from startNode to endNode.
    std::reverse(path.begin(), path.end());

    return path;
}

std::vector<Point> VisibilityGraph::aStarShortestPath()
{
    // Define a custom comparison function for the priority queue.
    // This is used to compare the pairs in the priority queue
    // based on their distances.
    auto comp = [](const std::pair<long double, Point>& a, const std::pair<long double, Point>& b) {
        return a.first > b.first;
    };

    std::priority_queue<std::pair<long double, Point>,
                        std::vector<std::pair<long double, Point>>,
                        decltype(comp)> pq(comp);

    std::unordered_map<Point, long double> g_costs;
    std::unordered_map<Point, long double> f_costs;
    std::unordered_map<Point, Point> came_from;

    g_costs[startNode] = 0.0L;
    f_costs[startNode] = startNode.distance(endNode);
    pq.push({f_costs[startNode], startNode});

    while (!pq.empty()) {
        Point current = pq.top().second;
        pq.pop();

        if (current == endNode) {
            std::vector<Point> path;
            while (current != startNode) {
                path.push_back(current);
                current = came_from[current];
            }
            path.push_back(startNode);
            std::reverse(path.begin(), path.end());
            return path;
        }

        for (const Line& segment : segments)
        {
            Point neighbor;
            if (segment.startPoint() == current)
            {
                neighbor = segment.endPoint();
            }
            else if (segment.endPoint() == current)
            {
                neighbor = segment.startPoint();
            }
            else
            {
                continue;
            }

            long double tentative_g_cost =
                g_costs[current] + current.distance(neighbor);
            if (tentative_g_cost < g_costs[neighbor] ||
                g_costs.find(neighbor) == g_costs.end()) {
                came_from[neighbor] = current;
                g_costs[neighbor] = tentative_g_cost;
                f_costs[neighbor] =
                    g_costs[neighbor] + neighbor.distance(endNode);
                pq.push({f_costs[neighbor], neighbor});
            }
        }
    }

    return {};  // Empty path, if the end node is unreachable
}


