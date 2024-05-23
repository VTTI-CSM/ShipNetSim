// quad.cpp
#include "quadtree.h"
#include <algorithm>
#include <limits>
#include <QtEndian>

namespace ShipNetSimCore
{
Quadtree::Node::Node(Quadtree* tree, Node* parent, int quadrant)
    : host(tree), quadrant(quadrant), isLeaf(true), parent(parent),
    min_point(std::make_shared<GPoint>(
        units::angle::degree_t(-180.0),
        units::angle::degree_t(-90.0))),
    max_point(std::make_shared<GPoint>(
        units::angle::degree_t(180.0),
        units::angle::degree_t(90.0)))
{
    std::fill(std::begin(children), std::end(children), nullptr);
}


Quadtree::Node::~Node()
{
    // Delete child nodes, which are the resources owned by this node
    for (Node*& child : children)
    {
        delete child;    // Delete the child node
        child = nullptr; // Set the pointer to nullptr after deletion
    }
    parent = nullptr;
    host = nullptr;
}

bool Quadtree::Node::isPointWithinNode(
    const std::shared_ptr<GPoint>& point) const
{
    // Check longitude bounds
    bool withinLongitude =
        point->getLongitude() >= min_point->getLongitude() &&
        point->getLongitude() <= max_point->getLongitude();

    // Check latitude bounds
    bool withinLatitude =
        point->getLatitude() >= min_point->getLatitude() &&
        point->getLatitude() <= max_point->getLatitude();

    return withinLongitude && withinLatitude;
}

units::length::meter_t Quadtree::Node::distanceFromPointToBoundingBox(
    const std::shared_ptr<GPoint>& point) const {
    // Initialize minDistance with a large value.
    units::length::meter_t minDistance(std::numeric_limits<double>::max());

    // Get the bounding box corners.
    units::angle::degree_t minLon = min_point->getLongitude();
    units::angle::degree_t maxLon = max_point->getLongitude();
    units::angle::degree_t minLat = min_point->getLatitude();
    units::angle::degree_t maxLat = max_point->getLatitude();

    // If the point is outside the bounding box in both dimensions,
    // check corner distances.
    std::vector<std::shared_ptr<GPoint>> corners = {
        std::make_shared<GPoint>(minLon, minLat),
        std::make_shared<GPoint>(minLon, maxLat),
        std::make_shared<GPoint>(maxLon, minLat),
        std::make_shared<GPoint>(maxLon, maxLat)
    };

    for (const auto& corner : corners) {
        minDistance = std::min(minDistance, point->distance(*corner));
    }

    return minDistance;
}


void Quadtree::Node::subdivide(Quadtree* tree)
{
    if (!isLeaf) return;

    if (line_segments.size() == 0) { isLeaf = true; return; }

    createChildren(tree); // create 4 children

    QVector<std::shared_ptr<GLine>> segmentsToKeep;

    for (auto& segment : line_segments) {
        // Check if the segment crosses the antimeridian and should be split
        if (isSegmentCrossingAntimeridian(segment)) {
            auto splitSegments = splitSegmentAtAntimeridian(segment);
            for (auto& splitSegment : splitSegments) {
                // Determine which child(ren) the split segment
                // belongs to and add it there
                auto r = distributeSegmentToChildren(splitSegment);
                if (r != nullptr)
                {
                    segmentsToKeep.push_back(r);
                }
            }
        } else {
            // For segments not crossing the boundary,
            // reassign to the correct child
            auto r = distributeSegmentToChildren(segment);
            if (r != nullptr)
            {
                segmentsToKeep.push_back(r);
            }
        }
    }

    // Clear the parent node's segments as they are now reassigned
    line_segments.clear();
    line_segments = segmentsToKeep; // Retain only undistributed segments
    isLeaf = false; // This node is no longer a leaf

    // Recursively subdivide children if they exceed
    // the maximum number of segments.
    for (int i = 0; i < 4; ++i) {
        if (children[i]->line_segments.size() > host->MAX_SEGMENTS_PER_NODE) {
            children[i]->subdivide(tree);
        }
    }
}

std::shared_ptr<GLine> Quadtree::Node::distributeSegmentToChildren(
    const std::shared_ptr<GLine>& segment) {
    bool distributed = false;
    for (int i = 0; i < 4; ++i) {
        if (children[i]->doesLineSegmentIntersectNode(segment)) {
            children[i]->line_segments.push_back(segment);
            distributed = true;
            // Do not break; segment may intersect multiple children
            // due to wrap-around
        }
    }
    if (!distributed) {
        // handle the case where the segment does not fit cleanly into any child
        // Keep it at the current node
        return segment;
    }
    return nullptr;
}

void Quadtree::Node::createChildren(Quadtree* tree)
{
    // Calculate the center point of the current node
    units::angle::degree_t centerLon =
        (min_point->getLongitude() + max_point->getLongitude()) / 2.0;
    units::angle::degree_t centerLat =
        (min_point->getLatitude() + max_point->getLatitude()) / 2.0;

    // Create each child node and set its boundaries
    for (int i = 0; i < 4; ++i) {
        children[i] = new Node(tree, this, i);
        children[i]->isLeaf = true;

        // Calculate the boundaries for each child
        units::angle::degree_t minLon =
            (i % 2 == 0) ? min_point->getLongitude() : centerLon;
        units::angle::degree_t maxLon =
            (i % 2 == 0) ? centerLon : max_point->getLongitude();
        units::angle::degree_t minLat =
            (i < 2) ? centerLat : min_point->getLatitude();
        units::angle::degree_t maxLat =
            (i < 2) ? max_point->getLatitude() : centerLat;

        children[i]->min_point =
            std::make_shared<GPoint>(minLon, minLat);
        children[i]->max_point =
            std::make_shared<GPoint>(maxLon, maxLat);
    }
}

// bool Quadtree::getSegmentCrossesMinLongitudeLine(
//     const std::shared_ptr<GLine>& segment) const
// {
//     // Get the x-coordinate values of the root's minimum and maximum points
//     double minLon = root.min_point->getLongitude().value();
//     double maxLon = root.max_point->getLongitude().value();

//     // Extract the x-coordinate values of the start and end points
//     // of the segment
//     double startLon = segment->startPoint()->getLongitude().value();
//     double endLon = segment->endPoint()->getLongitude().value();

//     // Check if the segment crosses from one edge of the map to the other
//     // A segment crosses the minimum longitude line if one end is
//     // near the minimum and the other end is near the maximum
//     bool crossesFromMinToMax = (startLon <= minLon && endLon >= maxLon);
//     bool crossesFromMaxToMin = (startLon >= maxLon && endLon <= minLon);

//     return crossesFromMinToMax || crossesFromMaxToMin;
// }

// QVector<std::shared_ptr<GLine>> Quadtree::splitSegmentAtMinLongitudeLine(
//     const std::shared_ptr<GLine>& segment)
// {
//     QVector<std::shared_ptr<GLine>> splitSegments;

//     // Extract the x-coordinate values of the root's minimum
//     // and maximum points
//     double minLongitude = root.min_point->getLongitude().value();
//     double maxLongitude = root.max_point->getLongitude().value();

//     // Extract the coordinates of the segment's start and end points
//     double startLon = segment->startPoint()->getLongitude().value();
//     double startLat = segment->startPoint()->getLatitude().value();
//     double endLon = segment->endPoint()->getLongitude().value();
//     double endLat = segment->endPoint()->getLongitude().value();

//     // Check if the segment needs to be split
//     if (!getSegmentCrossesMinLongitudeLine(segment)) {
//         // If the segment doesn't cross the minimum longitude line,
//         // return it as is
//         splitSegments.push_back(segment);
//         return splitSegments;
//     }

//     // Calculate the Y-coordinate at the point of crossing the
//     // minimum longitude line
//     // Linear interpolation is used to find the intersection point
//     // on the Y-axis
//     double ratio = (minLongitude - startLon) / (endLon - startLon);
//     double crossLat = startLat + ratio * (endLat - startLat);

//     // Create new points at the crossing
//     std::shared_ptr<GPoint> crossingPointMin =
//         std::make_shared<GPoint>(units::angle::degree_t(minLongitude),
//                                  units::angle::degree_t(crossLat));
//     std::shared_ptr<GPoint> crossingPointMax =
//         std::make_shared<GPoint>(units::angle::degree_t(maxLongitude),
//                                  units::angle::degree_t(crossLat));

//     // Split the segment into two at the crossing points
//     if (startLon < endLon)
//     {
//         // Segment crosses from left to right
//         splitSegments.push_back(
//             std::make_shared<GLine>(segment->startPoint(),
//                                    crossingPointMin));
//         splitSegments.push_back(
//             std::make_shared<GLine>(crossingPointMax,
//                                    segment->endPoint()));
//     }
//     else
//     {
//         // Segment crosses from right to left
//         splitSegments.push_back(
//             std::make_shared<GLine>(segment->startPoint(),
//                                    crossingPointMax));
//         splitSegments.push_back(
//             std::make_shared<GLine>(crossingPointMin,
//                                    segment->endPoint()));
//     }

//     return splitSegments;
// }




Quadtree::Quadtree(const QVector<std::shared_ptr<Polygon>>& polygons) :
    root(this)
{

    auto initializeBoundary =
        [this](const std::shared_ptr<GPoint> point) -> void
    {
        root.min_point->setLongitude(
            units::math::min(root.min_point->getLongitude(),
                             point->getLongitude()));
        root.min_point->setLatitude(
            units::math::min(root.min_point->getLatitude(),
                             point->getLatitude()));
        root.max_point->setLongitude(
            units::math::max(root.max_point->getLongitude(),
                             point->getLongitude()));
        root.max_point->setLatitude(
            units::math::max(root.max_point->getLatitude(),
                             point->getLatitude()));
    };

    // Initialize the root node's boundary based on polygons
    for (const std::shared_ptr<Polygon>& polygon : polygons)
    {
        for (const std::shared_ptr<GPoint>& point : polygon->outer())
        {
            initializeBoundary(point);
        }

        // Include inner holes in boundary initialization
        for (const QVector<std::shared_ptr<GPoint>>& hole : polygon->inners())
        {
            for (const std::shared_ptr<GPoint>& point : hole)
            {
                initializeBoundary(point);
            }
        }
    }

    // Process each line segment of the polygon
    auto processLineSegment =
        [this](
            const std::shared_ptr<GPoint>& start,
            const std::shared_ptr<GPoint>& end)
    {
        auto segment = std::make_shared<GLine>(start, end);

        root.line_segments.push_back(segment);

    };

    // Process each polygon
    for (const std::shared_ptr<Polygon>& polygon : polygons)
    {
        // Insert segments from outer boundary
        const auto& outer = polygon->outer();
        for (qsizetype i = 0; i < outer.size() - 1; ++i)
        {
            processLineSegment(outer[i], outer[i + 1]);
        }

        // Insert segments from inner holes
        for (const auto& hole : polygon->inners())
        {
            for (qsizetype i = 0; i < hole.size() - 1; ++i)
            {
                processLineSegment(hole[i], hole[i + 1]);
            }
        }
    }

    // Subdivide the root node if necessary
    if (root.line_segments.size() > MAX_SEGMENTS_PER_NODE)
    {
        root.subdivide(this);
    }
}

QVector<Quadtree::Node*> Quadtree::findNodesIntersectingLineSegment(
    const std::shared_ptr<GLine>& segment) const
{
    QVector<Quadtree::Node*> intersecting_nodes;
    if (isSegmentCrossingAntimeridian(segment)) {
        auto splitSegments = splitSegmentAtAntimeridian(segment);
        for (auto& splitSegment: splitSegments)
        {
            findIntersectingNodesHelper(splitSegment, root, intersecting_nodes);
        }
    }
    else
    {
        findIntersectingNodesHelper(segment, root, intersecting_nodes);
    }
    return intersecting_nodes;
}


void Quadtree::findIntersectingNodesHelper(
    const std::shared_ptr<GLine>& segment,
    const Node& node,
    QVector<Quadtree::Node*>& intersecting_nodes) const
{
    if (!node.doesLineSegmentIntersectNode(segment))
    {
        return; // No intersection with this node
    }

    if (node.isLeaf)
    {
        // Cast to non-const to match return type
        intersecting_nodes.push_back(const_cast<Node*>(&node));
    }
    else
    {
        for (const Node* child : node.children)
        {
            if (child)
            {
                findIntersectingNodesHelper(segment,
                                            *child,
                                            intersecting_nodes);
            }
        }
    }

}

bool Quadtree::Node::doesLineSegmentIntersectNode(
    const std::shared_ptr<GLine>& segment) const
{
    // Step 1: Standard intersection check.
    // perform a standard bounding box intersection check.
    // This is straightforward and involves checking if either endpoint
    // of the segment is within the node's bounding box or if any edge
    // of the bounding box intersects the line segment.
    if (standardIntersectionCheck(segment))
    {
        return true;
    }

    // Step 2 & 3: Wrap-around Handling
    // step 2: Detect if a line segment potentially wraps around the globe.
    //          This involves comparing the longitudes of the line segment's
    //          endpoints against the global boundaries. Wrap-around occurs
    //          if one endpoint is near the maximum longitude while the other
    //          is near the minimum longitude, considering the map's extent.
    // step 3: For segments that wrap around, adjust their longitude values
    //          for intersection checks. The idea is to "unwrap" the segment
    //          for the purpose of intersection calculations. You might need
    //          to split the segment into two: one part extending to the
    //          right boundary of the map and another from the left boundary,
    //          effectively treating it as two segments for intersection
    //          checks.
    if (isSegmentCrossingAntimeridian(segment)) {
        // Adjust the segment for wrap-around and
        // perform the intersection check
        auto adjustedSegments = splitSegmentAtAntimeridian(segment);
        for (const auto& adjSegment : adjustedSegments) {
            if (standardIntersectionCheck(adjSegment)) {
                return true;
            }
        }
    }

    return false;
}

std::vector<std::shared_ptr<GLine>> Quadtree::splitSegmentAtAntimeridian(
    const std::shared_ptr<GLine>& segment)
{
    std::vector<std::shared_ptr<GLine>> adjustedSegments;

    double startLon = segment->startPoint()->getLongitude().value();
    double endLon = segment->endPoint()->getLongitude().value();

    // Normalize longitudes to [0, 360) range to simplify calculations
    startLon = fmod((startLon + 360), 360);
    endLon = fmod((endLon + 360), 360);

    // Determine if crossing occurs from east to west or west to east
    bool crossesFromEastToWest = startLon > 180 && endLon < 180;
    bool crossesFromWestToEast = startLon < 180 && endLon > 180;

    if (!crossesFromEastToWest && !crossesFromWestToEast) {
        // No wrap-around, return the original segment
        adjustedSegments.push_back(segment);
        return adjustedSegments;
    }

    // Calculate intersection latitude using linear interpolation
    double startLat = segment->startPoint()->getLatitude().value();
    double endLat = segment->endPoint()->getLatitude().value();
    double ratio = std::abs(startLon - 180.0) / std::abs(endLon - startLon);
    units::angle::degree_t intersectionLat =
        units::angle::degree_t(startLat + ratio * (endLat - startLat));

    // Create the intersection point at the boundary
    std::shared_ptr<GPoint> intersectionPoint =
        std::make_shared<GPoint>(
            crossesFromEastToWest ?
                units::angle::degree_t(180.0) : units::angle::degree_t(-180.0),
            intersectionLat);

    // Create two new segments from the original segment
    if (crossesFromEastToWest) {
        adjustedSegments.push_back(
            std::make_shared<GLine>(segment->startPoint(),
                                    intersectionPoint));
        adjustedSegments.push_back(
            std::make_shared<GLine>(
                std::make_shared<GPoint>(
                    units::angle::degree_t(-180.0), intersectionLat),
                segment->endPoint()));
    } else { // crossesFromWestToEast
        adjustedSegments.push_back(
            std::make_shared<GLine>(
                segment->startPoint(),
                std::make_shared<GPoint>(
                    units::angle::degree_t(180.0), intersectionLat)));
        adjustedSegments.push_back(
            std::make_shared<GLine>(intersectionPoint, segment->endPoint()));
    }

    return adjustedSegments;
}

bool Quadtree::Node::standardIntersectionCheck(
    const std::shared_ptr<GLine>& segment) const
{
    // Check if either of the line segment's endpoints
    // is inside the node's bounding box
    if (min_point->getLongitude() <= segment->startPoint()->getLongitude() &&
        segment->startPoint()->getLongitude() <= max_point->getLongitude() &&
        min_point->getLatitude() <= segment->startPoint()->getLatitude() &&
        segment->startPoint()->getLatitude() <= max_point->getLatitude())
    {
        return true;
    }

    if (min_point->getLongitude() <= segment->endPoint()->getLongitude() &&
        segment->endPoint()->getLongitude() <= max_point->getLongitude() &&
        min_point->getLatitude() <= segment->endPoint()->getLatitude() &&
        segment->endPoint()->getLatitude() <= max_point->getLatitude())
    {
        return true;
    }

    // Check for intersection with each of
    // the four edges of the node
    GLine topEdge(
        min_point,
        std::make_shared<GPoint>(max_point->getLongitude(),
                                 min_point->getLatitude()));
    GLine bottomEdge(
        std::make_shared<GPoint>(min_point->getLongitude(),
                                 max_point->getLatitude()),
        max_point);
    GLine leftEdge(
        min_point,
        std::make_shared<GPoint>(min_point->getLongitude(),
                                 max_point->getLatitude()));
    GLine rightEdge(
        std::make_shared<GPoint>(max_point->getLongitude(),
                                 min_point->getLatitude()),
        max_point);

    if (segment->intersects(topEdge) ||
        segment->intersects(bottomEdge) ||
        segment->intersects(leftEdge) ||
        segment->intersects(rightEdge))
    {
        return true;
    }

    return false;
}

bool Quadtree::isSegmentCrossingAntimeridian(
    const std::shared_ptr<GLine>& segment)
{
    // Assuming GPoint::getLongitude() returns longitude in degrees
    double startLon = segment->startPoint()->getLongitude().value();
    double endLon = segment->endPoint()->getLongitude().value();

    // Normalize longitudes to handle wrap-around at the -180/180 degree mark
    // if the start is -170, the result is 10
    startLon = fmod((startLon + 180.0), 360.0); // get remainder
    endLon = fmod((endLon + 180.0), 360.0); // get remainder of division
    if (startLon < 0) startLon += 360.0;
    if (endLon < 0) endLon += 360.0;

    // Check if the segment crosses the boundary
    bool crosses = false;
    if ((startLon > endLon && (startLon - endLon) > 180.0) ||
        (endLon > startLon && (endLon - startLon) > 180.0)) {
        crosses = true;
    }

    return crosses;
}


QVector<std::shared_ptr<GLine>> Quadtree::getAllSegmentsInNode(
    const Node* node) const
{
    if (!node || node->isLeaf)
    {
        return node ? node->line_segments :
                   QVector<std::shared_ptr<GLine>>();
    }

    QVector<std::shared_ptr<GLine>> segments;
    for (const Node* child : node->children)
    {
        QVector<std::shared_ptr<GLine>> childSegments =
            getAllSegmentsInNode(child);

        segments += childSegments; // append all the children to segments
    }
    return segments;
}

QVector<Quadtree::Node*> Quadtree::getAdjacentNodes(
    const Node* node) const
{
    QVector<Node*> adjacentNodes;
    if (node == nullptr || node->parent == nullptr)
    {
        return adjacentNodes; // No adjacent nodes for root or null node
    }

    // Lambda to add a node's children if it's not a leaf
    auto addChildren = [&adjacentNodes](const Node* n)
    {
        if (n != nullptr && !n->isLeaf)
        {
            for (Node* child : n->children)
            {
                if (child != nullptr)
                {
                    adjacentNodes.push_back(child);
                }
            }
        }
    };

    // Get parent's siblings that share an edge with the node
    Node* parent = node->parent;
    int quadrant = node->quadrant;
    switch (quadrant) {
    case 0:  // top-left
        addChildren(parent->children[1]); // top-right of parent
        addChildren(parent->children[2]); // bottom-left of parent
        break;
    case 1:  // top-right
        addChildren(parent->children[0]); // top-left of parent
        addChildren(parent->children[3]); // bottom-right of parent
        break;
    case 2:  // bottom-left
        addChildren(parent->children[0]); // top-left of parent
        addChildren(parent->children[3]); // bottom-right of parent
        break;
    case 3:  // bottom-right
        addChildren(parent->children[1]); // top-right of parent
        addChildren(parent->children[2]); // bottom-left of parent
        break;
    }

    return adjacentNodes;
}

bool Quadtree::isNodeAtLeftEdge(const Node* node) const
{
    // Define the left edge X coordinate of your map
    const double leftEdgeLon = root.min_point->getLongitude().value();

    // Check if the node's bounding box aligns with or
    // is close to the left edge
    double nodeMinLon = node->min_point->getLongitude().value();

    // Using a small tolerance value for precision issues
    return std::abs(nodeMinLon - leftEdgeLon) <= tolerance;
}

bool Quadtree::isNodeAtRightEdge(const Node* node) const
{
    // Define the right edge X coordinate of your map
    const double rightEdgeLon = root.max_point->getLongitude().value();

    // Check if the node's bounding box aligns with or
    // is close to the right edge
    double nodeMaxLon = node->max_point->getLongitude().value();

    // Using a small tolerance value for precision issues
    return std::abs(nodeMaxLon - rightEdgeLon) <= tolerance;
}

QVector<Quadtree::Node*> Quadtree::findNodesOnRightEdge() const
{
    QVector<Node*> rightEdgeNodes;
    std::function<void(const Node*)> findRightEdgeNodes =
        [&](const Node* currentNode)
    {
        if (currentNode == nullptr) return;

        if (isNodeAtRightEdge(currentNode))
        {
            rightEdgeNodes.push_back(const_cast<Node*>(currentNode));
        }

        if (!currentNode->isLeaf)
        {
            for (const Node* child : currentNode->children)
            {
                findRightEdgeNodes(child);
            }
        }
    };

    findRightEdgeNodes(&root);
    return rightEdgeNodes;
}

QVector<Quadtree::Node*> Quadtree::findNodesOnLeftEdge() const
{
    QVector<Node*> leftEdgeNodes;
    std::function<void(const Node*)> findLeftEdgeNodes =
        [&](const Node* currentNode)
    {
        if (currentNode == nullptr) return;

        if (isNodeAtLeftEdge(currentNode))
        {
            leftEdgeNodes.push_back(const_cast<Node*>(currentNode));
        }

        if (!currentNode->isLeaf)
        {
            for (const Node* child : currentNode->children)
            {
                findLeftEdgeNodes(child);
            }
        }
    };

    findLeftEdgeNodes(&root);
    return leftEdgeNodes;
}



std::shared_ptr<GLine> Quadtree::findLineSegment(
    const std::shared_ptr<GPoint>& point1,
    const std::shared_ptr<GPoint>& point2) const
{
    std::shared_ptr<GLine> result = nullptr;
    findLineSegmentHelper(&root, point1, point2, result);
    return result;
}

void Quadtree::findLineSegmentHelper(
    const Node* node,
    const std::shared_ptr<GPoint>& point1,
    const std::shared_ptr<GPoint>& point2,
    std::shared_ptr<GLine>& result) const
{
    if (!node || result)
    {
        return;
    }

    for (const std::shared_ptr<GLine>& line : node->line_segments) {
        if ((*line->startPoint() == *point1 &&
             *line->endPoint() == *point2) ||
            (*line->startPoint() == *point2 &&
             *line->endPoint() == *point1))
        {
            result = line;
            return;
        }
    }

    // recursion to find the line
    if (!node->isLeaf) {
        for (const Node* child : node->children) {
            findLineSegmentHelper(child, point1, point2, result);
        }
    }
}

void Quadtree::insertLineSegment(const std::shared_ptr<GLine>& segment)
{
    // Start with the root node
    if (isSegmentCrossingAntimeridian(segment))
    {
        auto splitSegments = splitSegmentAtAntimeridian(segment);
        for (auto& splitSegment : splitSegments)
        {
            insertLineSegmentHelper(splitSegment, &root);
        }
    }
    else
    {
        insertLineSegmentHelper(segment, &root);
    }
}


void Quadtree::insertLineSegmentHelper(const std::shared_ptr<GLine>& segment,
                                       Node* node)
{
    if (!node) return;

    // Check if the segment intersects the node's bounding box
    if (!(*node).doesLineSegmentIntersectNode(segment))
    {
        return; // Segment does not intersect this node
    }

    if (node->isLeaf)
    {
        // If the node is a leaf and can still accommodate more segments
        if (node->line_segments.size() < MAX_SEGMENTS_PER_NODE)
        {
            node->line_segments.push_back(segment);
        }
        else
        {
            // Subdivide the node if it has reached its capacity
            node->subdivide(this);
            // Insert the segment into the appropriate child nodes
            for (int i = 0; i < 4; ++i)
            {
                insertLineSegmentHelper(segment, node->children[i]);
            }
        }
    }
    else
    {
        // If the node is not a leaf, insert the segment
        // into the appropriate child nodes
        for (int i = 0; i < 4; ++i)
        {
            insertLineSegmentHelper(segment, node->children[i]);
        }
    }
}


bool Quadtree::deleteLineSegment(const std::shared_ptr<GLine>& segment)
{
    return deleteLineSegmentHelper(segment, &root);
}

bool Quadtree::deleteLineSegmentHelper(const std::shared_ptr<GLine>& segment,
                                       Node* node)
{
    if (!node) return false;

    // Check if the segment intersects the node's bounding box
    if (!(*node).doesLineSegmentIntersectNode(segment))
    {
        return false; // Segment does not intersect this node
    }

    if (node->isLeaf)
    {
        // Look for the segment in the current node's line segments
        auto it = std::find(node->line_segments.begin(),
                            node->line_segments.end(),
                            segment);
        if (it != node->line_segments.end())
        {
            // If found, erase it
            node->line_segments.erase(it);
            return true;
        }
    } else {
        // If the node is not a leaf, check its children
        for (int i = 0; i < 4; ++i)
        {
            if (deleteLineSegmentHelper(segment, node->children[i]))
            {
                return true;
            }
        }
    }
    return false;
}

int Quadtree::getMaxDepth() const
{
    return getMaxDepthHelper(&root, 0);
}

int Quadtree::getMaxDepthHelper(const Node* node, int currentDepth) const
{
    if (!node || node->isLeaf)
    {
        return currentDepth;
    }

    int maxDepth = currentDepth;
    for (const Node* child : node->children)
    {
        if (child)
        {
            maxDepth = std::max(maxDepth,
                                getMaxDepthHelper(child, currentDepth + 1));
        }
    }
    return maxDepth;
}

QVector<std::shared_ptr<GLine>> Quadtree::rangeQuery(const QRectF& range) const
{
    QVector<std::shared_ptr<GLine>> foundSegments;

    // Initial range query
    rangeQueryHelper(range, &root, foundSegments);

    return foundSegments;
}

void Quadtree::rangeQueryHelper(
    const QRectF& range,
    const Node* node,
    QVector<std::shared_ptr<GLine>>& foundSegments) const
{
    if (!node) return;
    std::shared_ptr<OGRSpatialReference> SR =
        Point::getDefaultProjectionReference();
    Point minP = node->min_point->projectTo(SR.get());
    Point maxP = node->max_point->projectTo(SR.get());


    // Check if the current node's bounding box intersects the query range
    QRectF nodeBoundingBox(QPointF(minP.x().value(),
                                   minP.y().value()),
                           QPointF(maxP.x().value(),
                                   maxP.y().value()));

    if (!range.intersects(nodeBoundingBox))
    {
        SR->Release();
        return; // The range does not intersect this node
    }

    if (node->isLeaf)
    {
        // Check each line segment in the leaf node
        for (const std::shared_ptr<GLine>& segment : node->line_segments)
        {

            Line sgmt = segment->projectTo(SR.get());
            if (segmentIntersectsRange(sgmt, range))
            {
                foundSegments.push_back(segment);
            }
        }
    }
    else
    {
        // If the node is not a leaf, recurse into its children
        for (const Node* child : node->children)
        {
            rangeQueryHelper(range, child, foundSegments);
        }
    }
    SR->Release();
}


bool Quadtree::segmentIntersectsRange(const Line& segment,
                                      const QRectF& range) const
{
    // Extract the x and y coordinates of the segment's start and end points
    double startX = segment.startPoint()->x().value();
    double startY = segment.startPoint()->y().value();
    double endX = segment.endPoint()->x().value();
    double endY = segment.endPoint()->y().value();

    // Check if either of the segment's endpoints is inside the range
    if (range.contains(QPointF(startX, startY)) ||
        range.contains(QPointF(endX, endY)))
    {
        return true;
    }

    // Check intersection with each edge of the range
    // Create line segments for each edge of the range
    Line topEdge = Line(
        std::make_shared<Point>(units::length::meter_t(range.left()),
                                units::length::meter_t(range.top())),
        std::make_shared<Point>(units::length::meter_t(range.right()),
                                units::length::meter_t(range.top())));

    Line bottomEdge = Line(
        std::make_shared<Point>(units::length::meter_t(range.left()),
                                units::length::meter_t(range.bottom())),
        std::make_shared<Point>(units::length::meter_t(range.right()),
                                units::length::meter_t(range.bottom())));

    Line leftEdge = Line(
        std::make_shared<Point>(units::length::meter_t(range.left()),
                                units::length::meter_t(range.top())),
        std::make_shared<Point>(units::length::meter_t(range.left()),
                                units::length::meter_t(range.bottom())));

    Line rightEdge = Line(
        std::make_shared<Point>(units::length::meter_t(range.right()),
                                units::length::meter_t(range.top())),
        std::make_shared<Point>(units::length::meter_t(range.right()),
                                units::length::meter_t(range.bottom())));

    // Check if the segment intersects any of the edges of the range
    return segment.intersects(topEdge) || segment.intersects(bottomEdge) ||
           segment.intersects(leftEdge) || segment.intersects(rightEdge);
}

std::shared_ptr<GLine> Quadtree::findNearestNeighbor(
    const std::shared_ptr<GPoint>& point) const
{
    std::shared_ptr<GLine> nearestSegment = nullptr;
    units::length::meter_t minDistance =
        units::length::meter_t(std::numeric_limits<double>::max());
    findNearestNeighborHelper(point, &root, nearestSegment, minDistance);

    return nearestSegment;
}

void Quadtree::findNearestNeighborHelper(
    const std::shared_ptr<GPoint>& point,
    const Node* node,
    std::shared_ptr<GLine>& nearestSegment,
    units::length::meter_t &minDistance) const
{
    if (!node) return;

    // Calculate the distance from the point to the node's bounding box
    units::length::meter_t distanceToNode =
        distanceFromPointToNode(point, node);
    if (distanceToNode > minDistance)
    {
        return; // Node is too far to contain the nearest neighbor
    }

    if (node->isLeaf) {
        // Check each line segment in the leaf node
        for (const auto& segment : node->line_segments)
        {
            units::length::meter_t distance =
                segment->distanceToPoint(point);
            if (distance < minDistance) {
                minDistance = distance;
                nearestSegment = segment;
            }
        }
    } else {
        // If the node is not a leaf, recurse into its children
        for (const Node* child : node->children) {
            findNearestNeighborHelper(point, child,
                                      nearestSegment, minDistance);
        }
    }
}

units::length::meter_t Quadtree::distanceFromPointToNode(
    const std::shared_ptr<GPoint>& point,
    const Node* node) const
{
    if (!node)
    {
        return units::length::meter_t(std::numeric_limits<double>::max());
    }

    // Initialize minimum distance to a large value
    units::length::meter_t minDistance =
        units::length::meter_t(std::numeric_limits<double>::max());

    // Create points for each corner of the node
    std::shared_ptr<GPoint> corners[4] = {
        node->min_point, // Bottom-left
        std::make_shared<GPoint>(node->min_point->getLongitude(),
                                 node->max_point->getLatitude()), // Top-left
        node->max_point, // Top-right
        std::make_shared<GPoint>(node->max_point->getLongitude(),
                                 node->min_point->getLatitude()) // Bottom-right
    };

    // Check distance from the point to each corner
    for (int i = 0; i < 4; ++i) {
        units::length::meter_t distance = point->distance(*corners[i]);
        minDistance = std::min(minDistance, distance);
    }

    // Additionally, check if the point is directly north, south,
    // east, or west of the node and compare those distances
    // to the minDistance
    std::shared_ptr<GPoint> edges[4] = {
        std::make_shared<GPoint>(point->getLongitude(),
                                 node->min_point->getLatitude()), // Directly south
        std::make_shared<GPoint>(point->getLongitude(),
                                 node->max_point->getLatitude()), // Directly north
        std::make_shared<GPoint>(node->min_point->getLongitude(),
                                 point->getLatitude()), // Directly west
        std::make_shared<GPoint>(node->max_point->getLongitude(),
                                 point->getLatitude())  // Directly east
    };

    for (int i = 0; i < 4; ++i) {
        units::length::meter_t distance = point->distance(*edges[i]);
        if (distance.value() < minDistance.value()) {
            minDistance = distance;
        }
    }

    return minDistance;
}

std::shared_ptr<GPoint> Quadtree::findNearestNeighborPoint(
    const std::shared_ptr<GPoint>& point) const
{
    std::shared_ptr<GPoint> nearestPoint = nullptr;
    units::length::meter_t minDistance =
        units::length::meter_t(std::numeric_limits<double>::max());
    findNearestNeighborPointHelper(point, &root, nearestPoint, minDistance);
    return nearestPoint;
}

void Quadtree::findNearestNeighborPointHelper(
    const std::shared_ptr<GPoint>& point,
    const Node* node,
    std::shared_ptr<GPoint>& nearestPoint,
    units::length::meter_t& minDistance) const
{
    if (!node) return;

    // Calculate distance to node's bounding box to determine
    // if the node is worth exploring.
    units::length::meter_t bboxDistance =
        node->distanceFromPointToBoundingBox(point);

    // If the bounding box distance is already greater than
    // the current minimum distance, and the point is not within
    // the node, there's no need to explore this node further.
    if (bboxDistance >= minDistance && !node->isPointWithinNode(point)) {
        return;
    }

    // At this point, either the point is within the node,
    // or the node's bounding box is closer than the current minimum
    // distance, indicating potential for finding a
    // nearer neighbor within this node or its descendants.
    if (node->isLeaf) {
        // For leaf nodes, check distance against all points in the node.
        for (const auto& segment : node->line_segments) {
            checkAndUpdateMinDistance(
                point, segment->startPoint(), nearestPoint, minDistance);
            checkAndUpdateMinDistance(
                point, segment->endPoint(), nearestPoint, minDistance);
        }
    } else {
        // If not a leaf, recurse into each child node, prioritizing
        // those that might contain the point or are closer to the
        // point based on the bounding box distance.
        for (const auto& child : node->children) {
            if (child != nullptr) {
                findNearestNeighborPointHelper(
                    point, child, nearestPoint, minDistance);
            }
        }
    }
}

void Quadtree::checkAndUpdateMinDistance(
    const std::shared_ptr<GPoint>& targetPoint,
    const std::shared_ptr<GPoint>& point,
    std::shared_ptr<GPoint>& nearestPoint,
    units::length::meter_t& minDistance) const
{
    units::length::meter_t distance = targetPoint->distance(*point);
    if (distance < minDistance)
    {
        minDistance = distance;
        nearestPoint = point;
    }
}

void Quadtree::clearTree()
{
    // Recursively clear the tree starting from the root
    clearTreeHelper(&root);

    // Reset the root node's properties instead of assigning a new node
    root.host = this;
    root.quadrant = -1;
    root.isLeaf = true; // Assuming the root becomes a leaf after clearing
    root.line_segments.clear();
    root.min_point = std::make_shared<GPoint>(); // Reset to default
    root.max_point = std::make_shared<GPoint>(); // Reset to default
    root.min_point->setLongitude(
        units::angle::degree_t(std::numeric_limits<double>::max()));
    root.min_point->setLatitude(
        units::angle::degree_t(std::numeric_limits<double>::max()));

    root.max_point->setLongitude(
        units::angle::degree_t(std::numeric_limits<double>::lowest()));
    root.max_point->setLatitude(
        units::angle::degree_t(std::numeric_limits<double>::lowest()));

    // Initialize children pointers to nullptr
    std::fill(std::begin(root.children), std::end(root.children), nullptr);
}

void Quadtree::clearTreeHelper(Node* node)
{
    if (!node) return;

    // Recursively delete child nodes
    for (int i = 0; i < 4; ++i)
    {
        if (node->children[i])
        {
            clearTreeHelper(node->children[i]);
            delete node->children[i];
            node->children[i] = nullptr;
        }
    }

    // Clear the data in the current node
    node->line_segments.clear();
}


void Quadtree::serialize(std::ostream& out) const
{
    serializeNode(out, &root);
}

void Quadtree::serializeNode(std::ostream& out, const Node* node) const
{
    if (!out)
    {
        throw std::runtime_error("Output stream is not ready for writing.");
    }

    // Serialize nullity of the node
    bool isNull = (node == nullptr);
    out.write(reinterpret_cast<const char*>(&isNull), sizeof(isNull));
    if (isNull) {
        return; // If the node is null, there's nothing more to serialize
    }

    // Serialize the min and max points using GPoint's serialization method
    node->min_point->serialize(out);
    node->max_point->serialize(out);

    // Serialize line segments
    std::uint64_t numSegments =
        static_cast<std::uint64_t>(node->line_segments.size());
    out.write(reinterpret_cast<const char*>(&numSegments),
              sizeof(numSegments));

    for (const auto& segment : node->line_segments)
    {
        segment->startPoint()->serialize(out);
        segment->endPoint()->serialize(out);
    }

    // Serialize leaf status
    out.write(reinterpret_cast<const char*>(&node->isLeaf),
              sizeof(node->isLeaf));

    // Serialize child nodes recursively
    for (const auto& child : node->children)
    {
        serializeNode(out, child);
    }

    // Check for write failures
    if (!out) {
        throw std::runtime_error("Failed to write node "
                                 "data to output stream.");
    }
}


void Quadtree::deserialize(std::istream& in) {
    try
    {
        // Clears the current tree
        clearTree();

        // Directly deserialize into the root node
        deserializeNode(in, &root);
    }
    catch (const std::runtime_error& e)
    {
        // Cleanup in case of an exception
        clearTree(); // Clear any partially constructed tree
        throw e; // Rethrow the exception after cleanup
    }
}

void Quadtree::deserializeNode(std::istream& in, Node* node)
{
    if (!in)
    {
        throw std::runtime_error("Input stream is in a bad state.");
    }

    bool isNull;
    in.read(reinterpret_cast<char*>(&isNull), sizeof(isNull));
    if (isNull)
    {
        node = nullptr;
        return;
    }

    node = new Node(this);

    // Deserialize the min and max points using GPoint's deserialize method
    node->min_point = std::make_shared<GPoint>();
    node->min_point->deserialize(in);

    node->max_point = std::make_shared<GPoint>();
    node->max_point->deserialize(in);

    // Deserialize line segments
    std::uint64_t numSegments;
    in.read(reinterpret_cast<char*>(&numSegments), sizeof(numSegments));
    if (!in)
    {
        throw std::runtime_error("Failed to read number of line segments.");
    }

    for (std::uint64_t i = 0; i < numSegments; ++i)
    {
        auto startPoint = std::make_shared<GPoint>();
        startPoint->deserialize(in);

        auto endPoint = std::make_shared<GPoint>();
        endPoint->deserialize(in);

        node->line_segments.push_back(
            std::make_shared<GLine>(startPoint, endPoint));
    }

    // Deserialize leaf status
    bool isLeaf;
    in.read(reinterpret_cast<char*>(&isLeaf), sizeof(isLeaf));
    node->isLeaf = isLeaf;

    // Deserialize child nodes recursively
    for (int i = 0; i < 4; ++i)
    {
        deserializeNode(in, node->children[i]);
        if (!in)
        {
            throw std::runtime_error("Failed to deserialize child node.");
        }
    }
}


units::angle::degree_t Quadtree::getMapWidth() const
{
    return (root.max_point->getLongitude() - root.min_point->getLongitude());
}

units::angle::degree_t Quadtree::getMapHeight() const
{
    return (root.max_point->getLatitude() - root.min_point->getLatitude());
}

bool Quadtree::isNearBoundary(
    const std::shared_ptr<GPoint>& point) const
{
    // Check if the point is near the left or right boundary
    return (std::abs(point->getLongitude().value() -
                     root.min_point->getLongitude().value()) < tolerance) ||
           (std::abs(point->getLongitude().value() -
                     root.max_point->getLongitude().value()) < tolerance);
}

GPoint Quadtree::getMapMinPoint() const
{
    return *(root.min_point);
}

GPoint Quadtree::getMapMaxPoint() const
{
    return *(root.max_point);
}


};
