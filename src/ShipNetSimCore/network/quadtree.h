/**
 * @file quad.h
 * @brief Quadtree Data Structure Implementation.
 *
 * This file contains the declaration of the Quadtree class and its associated
 * Node structure. The Quadtree class is designed for efficient spatial
 * partitioning and querying of two-dimensional geometric data, particularly
 * line segments. It is mainly used in visibility graph creation, that is
 * used in ship path definition.
 *
 * The Quadtree decomposes space into four quadrants at each level, allowing
 * for efficient spatial querying and management of geometric data.
 *
 * @author Ahmed Aredah
 * @date 12.12.2023
 */

// quad.h
#ifndef QUADTREE_H
#define QUADTREE_H

#include <QRectF>
#include "gpoint.h"
#include "gline.h"
#include "polygon.h"

namespace ShipNetSimCore
{
/**
 * @class Quadtree
 * @brief Represents a quadtree data structure for efficient
 * spatial partitioning.
 *
 * The Quadtree class is designed to efficiently store
 * and query line segments in a two-dimensional space.
 * It is particularly useful for applications that
 * require fast lookup of spatial data, such as collision
 * detection or rendering.
 */
class Quadtree {

private:
    /**
     * @struct Node
     * @brief Represents a node in the Quadtree.
     *
     * Each node can either be a leaf node or have four children.
     * Leaf nodes contain line segments.
     * Non-leaf nodes contain pointers to their children.
     */
    struct Node
    {

        Quadtree* host;
        /// Quadrant of the node: 0=top-left, 1=top-right,
        /// 2=bottom-left, 3=bottom-right.
        int quadrant;
        /// Flag to indicate if the node is a leaf.
        bool isLeaf;
        /// GLine segments contained in the node.
        QVector<std::shared_ptr<GLine>> line_segments;
        /// Pointers to child nodes.
        Node* children[4];
        /// Pointer to the parent node.
        Node* parent;
        /// Minimum point defining the node's bounding box.
        std::shared_ptr<GPoint> min_point;
        /// Maximum point defining the node's bounding box.
        std::shared_ptr<GPoint> max_point;


        Node(Quadtree* tree, Node* parent = nullptr, int quadrant = -1);
        ~Node();

        // Disable copy constructor and copy assignment operator
        Node(const Node& other) = delete;
        Node& operator=(const Node& other) = delete;

        // Disable move constructor and move assignment operator
        Node(Node&& other) noexcept = delete;
        Node& operator=(Node&& other) noexcept = delete;

        // Node(const Node& other);  // Copy constructor
        // Node(Node&& other) noexcept;  // Move constructor
        // Node& operator=(const Node& other);  // Copy assignment operator
        // Node& operator=(Node&& other) noexcept;  // Move assignment operator

        /// Maximum point defining the node's bounding box.
        void subdivide(Quadtree *tree);

        /// Distribute segments to child nodes
        std::shared_ptr<GLine> distributeSegmentToChildren(
            const std::shared_ptr<GLine>& segment);

        /// Creates child nodes during subdivision.
        void createChildren(Quadtree *tree);

        bool doesLineSegmentIntersectNode(
            const std::shared_ptr<GLine>& segment) const;

        bool standardIntersectionCheck(
            const std::shared_ptr<GLine>& segment) const;

        bool isPointWithinNode(const std::shared_ptr<GPoint>& point) const;

        units::length::meter_t distanceFromPointToBoundingBox(
            const std::shared_ptr<GPoint>& point) const;
    };

    /// Maximum number of line segments a node can
    /// hold before subdividing.
    /// leave it to 1 so the visibility graph can work
    const qsizetype MAX_SEGMENTS_PER_NODE = 100;

    /// Root node of the quadtree
    Node root;

    const double tolerance = 0.1; // Small tolerance for double precision


    std::shared_ptr<GPoint> wrapPoint(
        const std::shared_ptr<GPoint>& point) const;

    void findIntersectingNodesHelper(
        const std::shared_ptr<GLine>& segment,
        const Node& node,
        QVector<Node*>& intersecting_nodes) const;

    void findLineSegmentHelper(const Node* node,
                               const std::shared_ptr<GPoint>& point1,
                               const std::shared_ptr<GPoint>& point2,
                               std::shared_ptr<GLine>& result) const;

    void insertLineSegmentHelper(
        const std::shared_ptr<GLine>& segment,
        Node* node);

    bool deleteLineSegmentHelper(
        const std::shared_ptr<GLine>& segment,
        Node* node);

    int getMaxDepthHelper(const Node* node, int currentDepth) const;

    void rangeQueryHelper(
        const QRectF& range,
        const Node* node,
        QVector<std::shared_ptr<GLine>>& foundSegments) const;

    void findNearestNeighborHelper(const std::shared_ptr<GPoint>& point,
                                   const Node* node,
                                   std::shared_ptr<GLine>& nearestSegment,
                                   units::length::meter_t& minDistance) const;

    void findNearestNeighborPointHelper(
        const std::shared_ptr<GPoint>& point,
        const Node* node,
        std::shared_ptr<GPoint>& nearestPoint,
        units::length::meter_t& minDistance) const;

    void checkAndUpdateMinDistance(
        const std::shared_ptr<GPoint>& targetPoint,
        const std::shared_ptr<GPoint>& point,
        std::shared_ptr<GPoint>& nearestPoint,
        units::length::meter_t& minDistance) const;

    bool isNodeAtLeftEdge(const Node* node) const;
    bool isNodeAtRightEdge(const Node* node) const;
    QVector<Quadtree::Node*> findNodesOnRightEdge() const;
    QVector<Quadtree::Node*> findNodesOnLeftEdge() const;


    /**
     * @brief Calculates the minimum distance from a point
     * to a node's bounding box.
     * @param point The point from which to measure.
     * @param node The node whose bounding box is used for measurement.
     * @return The minimum distance between the point and the node's
     * bounding box.
     */
    units::length::meter_t distanceFromPointToNode(
        const std::shared_ptr<GPoint>& point,
        const Node* node) const;

    /**
     * @brief Checks if a line segment intersects with a given rectangular range.
     * @param segment The line segment to check.
     * @param range The rectangular range.
     * @return True if the segment intersects the range, false otherwise.
     */
    bool segmentIntersectsRange(const Line &segment,
                                const QRectF& range) const;

    /**
     * @brief Helper function to recursively delete nodes.
     * @param node The current node to delete.
     */
    void clearTreeHelper(Node* node);

    // check if a segment crosses the antimeridian line (min x of the map)
    static bool isSegmentCrossingAntimeridian(
        const std::shared_ptr<GLine>& segment);

    static std::vector<std::shared_ptr<GLine>> splitSegmentAtAntimeridian(
        const std::shared_ptr<GLine>& segment);


    void serializeNode(std::ostream& out, const Node* node) const;

    void deserializeNode(std::istream& in, Node* node);
public:

    /**
     * @brief Constructor for Quadtree.
     * @brief Default constructor for quadtree.
     */
    Quadtree();

    /**
     * @brief Constructor for Quadtree.
     * @param polygons A collection of polygons to initialize the quadtree.
     */
    Quadtree(const QVector<std::shared_ptr<Polygon>>& polygons);

    /**
     * @brief Finds nodes in the quadtree that intersect
     * a given line segment.
     * @details This function finds all nodes in the quadtree that
     * intersect a line segment. It uses the bounding box of the node
     * to do the check and not all the segments inside which makes it
     * very fast.
     * @param segment The line segment for which intersecting
     * nodes are to be found.
     * @return A vector of pointers to nodes that
     * intersect the given line segment.
     */
    QVector<Node*> findNodesIntersectingLineSegment(
        const std::shared_ptr<GLine>& segment) const;

    /**
     * @brief Retrieves all line segments within a given node.
     * @param node The node from which to retrieve line segments.
     * @return A vector of shared pointers to line segments contained
     * in the node.
     */
    QVector<std::shared_ptr<GLine>> getAllSegmentsInNode(
        const Node* node) const;

    /**
     * @brief Retrieves adjacent nodes to a given node.
     * @param node The node for which adjacent nodes are to be found.
     * @return A vector of pointers to adjacent nodes.
     */
    QVector<Node*> getAdjacentNodes(const Node* node) const;

    /**
     * @brief Searches for a line segment connecting two
     * given points within the quadtree.
     * @param point1 The first endpoint of the line segment.
     * @param point2 The second endpoint of the line segment.
     * @return A shared pointer to the found line segment,
     * or nullptr if not found.
     */
    std::shared_ptr<GLine> findLineSegment(
        const std::shared_ptr<GPoint>& point1,
        const std::shared_ptr<GPoint>& point2) const;

    /**
     * @brief Inserts a new line segment into the quadtree.
     * @param segment The line segment to be inserted.
     */
    void insertLineSegment(const std::shared_ptr<GLine>& segment);

    /**
     * @brief Deletes a line segment from the quadtree.
     * @param segment The line segment to be deleted.
     * @return True if the segment was found and deleted, false otherwise.
     */
    bool deleteLineSegment(const std::shared_ptr<GLine>& segment);

    /**
     * @brief Returns the maximum depth of the quadtree.
     * @return The maximum depth of the tree.
     */
    int getMaxDepth() const;

    /**
     * @brief Performs a range query on the quadtree.
     * @param range The rectangular area for the query.
     * @return A vector of shared pointers to line segments within the range.
     */
    QVector<std::shared_ptr<GLine>> rangeQuery(const QRectF& range) const;

    /**
     * @brief Finds the nearest line segment to a given point.
     * @param point The reference point for the search.
     * @return A shared pointer to the nearest line segment, or nullptr
     *  if none is found.
     */
    std::shared_ptr<GLine> findNearestNeighbor(
        const std::shared_ptr<GPoint>& point) const;

    /**
     * @brief Finds the nearest neighbor point to a given point.
     * @param point The reference point for the search.
     * @return A shared pointer to the nearest point, or nullptr
     * if none is found.
     */
    std::shared_ptr<GPoint> findNearestNeighborPoint(
        const std::shared_ptr<GPoint>& point) const;

    // check if a segment crosses the min X Coordinate GLine
    // bool getSegmentCrossesMinLongitudeLine(
    //     const std::shared_ptr<GLine> &segment) const;

    // /**
    //  * @brief split Segment At Min Longitude GLine (MLL)
    //  * @param segment the segment to be split
    //  * @return a vector of segments that are split at the MLL
    //  */
    // QVector<std::shared_ptr<GLine>> splitSegmentAtMinLongitudeLine(
    //     const std::shared_ptr<GLine>& segment);

    /**
     * @brief Clears the entire Quadtree, deleting all nodes.
     */
    void clearTree();

    /**
     * @brief Serializes the Quadtree to a binary format and writes it
     *          to an output stream.
     *
     * This function traverses the Quadtree and serializes each node's data,
     * including its bounding box and any contained line segments, to a
     * binary format. The serialized data is written to the provided output
     * stream. The serialization format is designed for efficient storage
     * and is not human-readable. This function is typically used
     * to save the state of the Quadtree to a file or transmit it over
     * a network.
     *
     * @param out The output stream to which the Quadtree is serialized.
     *          This stream should be opened in binary mode to correctly
     *          handle the binary data.
     *
     * Usage Example:
     *     std::ofstream outFile("quadtree.bin", std::ios::binary);
     *     if (outFile.is_open()) {
     *         quadtree.serialize(outFile);
     *         outFile.close();
     *     }
     */
    void serialize(std::ostream& out) const;

    /**
     * @brief Deserializes a Quadtree from a binary format read
     *          from an input stream.
     *
     * This function reads binary data from the provided input stream
     * and reconstructs the Quadtree structure. The binary format
     * should match the format produced by the serialize function.
     * This function is typically used to load a Quadtree from a file
     * or receive it from a network transmission. The existing content
     * of the Quadtree is cleared before deserialization. If the
     * binary data is corrupted or improperly formatted,
     * the behavior of this function is undefined, and the resulting
     * Quadtree may be incomplete or invalid.
     *
     * @param in The input stream from which the Quadtree is deserialized.
     *          This stream should be opened in binary mode to correctly
     *          handle the binary data.
     *
     * Usage Example:
     *     std::ifstream inFile("quadtree.bin", std::ios::binary);
     *     if (inFile.is_open()) {
     *         quadtree.deserialize(inFile);
     *         inFile.close();
     *     }
     */
    void deserialize(std::istream& in);

    /**
     * @brief Gets the map width.
     *
     * This function returns the difference between
     * the max and min points along the x axis.
     *
     * @return the width of the map in degrees
     */
    units::angle::degree_t getMapWidth() const;

    /**
     * @brief Gets the map height.
     *
     * This function returns the difference between
     * the max and min points along the y axis.
     *
     * @return the height of the map in degrees
     */
    units::angle::degree_t getMapHeight() const;

    /**
     * @brief Determines if a point is close to the map edge.
     * @param point a pointer to the subject point.
     * @return true if the point is on the edge of the map.
     */
    bool isNearBoundary(const std::shared_ptr<GPoint>& point) const;

    /**
     * @brief get the map min point.
     * @return a point with the lower left corner of the map.
     */
    GPoint getMapMinPoint() const;

    /**
     * @brief get the map max point.
     * @return a point with the top right corner of the map.
     */
    GPoint getMapMaxPoint() const;


};
};
#endif // QUADTREE_H
