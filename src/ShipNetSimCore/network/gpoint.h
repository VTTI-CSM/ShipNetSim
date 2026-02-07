/**
 * @file gpoint.h
 * @brief Geodetic Point Implementation for WGS84 Ellipsoid.
 *
 * This file contains the GPoint class which represents a geographic point
 * with latitude and longitude coordinates on the WGS84 ellipsoid. The class
 * provides accurate geodesic calculations using GeographicLib and supports
 * GDAL/OGR integration for spatial operations.
 *
 * Key features:
 * - Accurate geodesic distance and azimuth calculations
 * - Coordinate normalization (longitude wrap, latitude flip)
 * - Datum transformation and projection support
 * - Binary serialization for network transmission and storage
 * - Port/waypoint functionality for shipping simulation
 *
 * @author Ahmed Aredah
 * @date 10.12.2023
 */

#ifndef GPOINT_H
#define GPOINT_H

#include "../../third_party/units/units.h"
#include "../export.h"
#include "basegeometry.h"
#include <QString>
#include <QVector>
#include <memory>
#include <gdal.h>
#include <ogr_geometry.h>
#include <ogr_spatialref.h>

namespace ShipNetSimCore
{

class Point;    // Forward declaration for projected point type
class Polygon;  // Forward declaration for polygon ownership

/**
 * @class GPoint
 * @brief Represents a geographic point on the WGS84 ellipsoid.
 *
 * GPoint encapsulates a geographic location with latitude and longitude,
 * providing type-safe coordinate handling and geodesic calculations.
 * All geodetic operations use GeographicLib for accurate results on the
 * WGS84 ellipsoid.
 */
class SHIPNETSIM_EXPORT GPoint : public BaseGeometry
{
    // =========================================================================
    // Member Variables
    // =========================================================================
private:
    OGRPoint mOGRPoint;  ///< GDAL/OGR point representation
    QString  mUserID;    ///< Optional user-defined identifier
    bool     mIsPort;    ///< Flag indicating if this point is a port
    units::time::second_t mDwellTime;  ///< Dwell time at port (if applicable)

    /// Polygons that own this vertex (for polygon boundary vertices).
    /// Uses shared_ptr to avoid circular reference with Polygon.
    /// Empty for points that are not polygon vertices.
    QVector<std::shared_ptr<Polygon>> mOwningPolygons;

    /// Visibility cache: maps polygon → visible vertices within that polygon.
    /// Uses raw Polygon* as key (polygon lifetime managed by visibility graph).
    /// mutable because visibility computation is logically const.
    mutable std::unordered_map<const Polygon*,
                               QVector<std::shared_ptr<GPoint>>> mVisibleNeighborsCache;

    /// Default coordinate reference system (WGS84)
    static std::shared_ptr<OGRSpatialReference> spatialRef;

    // =========================================================================
    // Constructors
    // =========================================================================
public:
    /**
     * @brief Default constructor creating a point at origin (0, 0).
     *
     * Creates a point at the intersection of the Prime Meridian and Equator
     * with WGS84 spatial reference.
     */
    GPoint();

    /**
     * @brief Construct a GPoint with specified coordinates.
     *
     * @param lon Longitude in degrees [-180, 180]
     * @param lat Latitude in degrees [-90, 90]
     * @param crc Optional spatial reference (defaults to WGS84)
     * @throws std::runtime_error if spatial reference is not geographic
     */
    GPoint(units::angle::degree_t lon, units::angle::degree_t lat,
           OGRSpatialReference crc = OGRSpatialReference());

    /**
     * @brief Construct a GPoint with coordinates and an identifier.
     *
     * @param lon Longitude in degrees [-180, 180]
     * @param lat Latitude in degrees [-90, 90]
     * @param ID User-defined identifier for the point
     * @param crc Optional spatial reference (defaults to WGS84)
     * @throws std::runtime_error if spatial reference is not geographic
     */
    GPoint(units::angle::degree_t lon, units::angle::degree_t lat,
           QString ID, OGRSpatialReference crc = OGRSpatialReference());

    // =========================================================================
    // Static Methods - Spatial Reference Management
    // =========================================================================

    /**
     * @brief Get the default spatial reference (WGS84).
     *
     * Returns the shared default spatial reference used for new GPoint
     * instances when no explicit reference is provided.
     *
     * @return Shared pointer to the default WGS84 spatial reference
     */
    static std::shared_ptr<OGRSpatialReference>
    getDefaultReprojectionReference();

    /**
     * @brief Set a custom default spatial reference.
     *
     * @param wellknownCS Well-known coordinate system name (e.g., "WGS84")
     * @throws std::runtime_error if the CS is invalid or not geographic
     */
    static void setDefaultReprojectionReference(std::string wellknownCS);

    // =========================================================================
    // Accessors - Coordinate Getters
    // =========================================================================

    /**
     * @brief Get the latitude of the point.
     * @return Latitude in degrees [-90, 90]
     */
    [[nodiscard]] units::angle::degree_t getLatitude() const;

    /**
     * @brief Get the longitude of the point.
     * @return Longitude in degrees [-180, 180]
     */
    [[nodiscard]] units::angle::degree_t getLongitude() const;

    /**
     * @brief Get the user-defined identifier.
     * @return The user ID, or empty string if not set
     */
    [[nodiscard]] QString getUserID() const;

    /**
     * @brief Get the GDAL/OGR point representation.
     * @return Copy of the internal OGRPoint
     */
    OGRPoint getGDALPoint() const;

    // =========================================================================
    // Mutators - Coordinate Setters
    // =========================================================================

    /**
     * @brief Set the latitude with automatic normalization.
     *
     * Values outside [-90, 90] are normalized by "flipping" the latitude.
     * For example, 100 degrees becomes 80 degrees.
     *
     * @param lat New latitude value in degrees
     */
    void setLatitude(units::angle::degree_t lat);

    /**
     * @brief Set the longitude with automatic normalization.
     *
     * Values outside [-180, 180] are wrapped to the valid range.
     * For example, 270 degrees becomes -90 degrees.
     *
     * @param lon New longitude value in degrees
     */
    void setLongitude(units::angle::degree_t lon);

    // =========================================================================
    // Geodetic Calculations
    // =========================================================================

    /**
     * @brief Calculate geodesic distance to another point.
     *
     * Uses GeographicLib for accurate geodesic distance on the WGS84
     * ellipsoid. Both points must have the same spatial reference.
     *
     * @param other The target point
     * @return Geodesic distance in meters
     * @throws std::runtime_error if spatial references are missing or mismatched
     */
    [[nodiscard]] units::length::meter_t distance(const GPoint &other) const;

    /**
     * @brief Calculate geodesic distance without spatial reference validation.
     *
     * Identical to distance() but skips validateSpatialReferences() for
     * performance in hot paths where all points are known to use WGS84.
     *
     * @param other The target point
     * @return Geodesic distance in meters
     */
    [[nodiscard]] units::length::meter_t
    fastDistance(const GPoint &other) const;

    /**
     * @brief Calculate forward azimuth to another point.
     *
     * Returns the initial bearing (azimuth) from this point toward the
     * target point, measured clockwise from true north.
     *
     * @param other The target point
     * @return Forward azimuth in degrees [0, 360) or [-180, 180]
     * @throws std::runtime_error if spatial references are missing or mismatched
     */
    [[nodiscard]] units::angle::degree_t
    forwardAzimuth(const GPoint &other) const;

    /**
     * @brief Calculate backward azimuth from another point to this one.
     *
     * Returns the final bearing at the target point when traveling from
     * the other point to this point.
     *
     * @param other The source point
     * @return Backward azimuth in degrees
     * @throws std::runtime_error if spatial references are missing or mismatched
     */
    [[nodiscard]] units::angle::degree_t
    backwardAzimuth(const GPoint &other) const;

    /**
     * @brief Calculate a point at given distance and heading from this point.
     *
     * Uses geodesic forward calculation to find a destination point.
     *
     * @param distance Distance to travel in meters
     * @param heading Initial heading in degrees (clockwise from north)
     * @return New GPoint at the calculated location
     */
    GPoint pointAtDistanceAndHeading(units::length::meter_t  distance,
                                     units::angle::degree_t heading) const;

    /**
     * @brief Get the geodesic midpoint between this point and another.
     *
     * Calculates the point exactly halfway along the geodesic path
     * between the two points.
     *
     * @param endPoint The other endpoint
     * @return GPoint at the geodesic midpoint
     * @throws std::runtime_error if spatial references are missing or mismatched
     */
    GPoint getMiddlePoint(const GPoint &endPoint) const;

    // =========================================================================
    // Coordinate Transformations
    // =========================================================================

    /**
     * @brief Transform the datum (spatial reference) of this point.
     *
     * Converts the point's coordinates to a different geodetic datum.
     * The target must be a geographic (not projected) coordinate system.
     *
     * @param targetSR Target spatial reference (must be geographic)
     * @throws std::runtime_error if targetSR is null or not geographic
     */
    void transformDatumTo(OGRSpatialReference *targetSR);

    /**
     * @brief Project this point to a projected coordinate system.
     *
     * Creates a new Point object in a projected coordinate system
     * (e.g., UTM, State Plane).
     *
     * @param targetSR Target projected spatial reference
     * @return Projected Point in the target coordinate system
     * @throws std::runtime_error if targetSR is invalid or not projected
     */
    Point projectTo(OGRSpatialReference *targetSR) const;

    // =========================================================================
    // Port/Waypoint Operations
    // =========================================================================

    /**
     * @brief Check if this point represents a port.
     * @return true if the point is marked as a port
     */
    [[nodiscard]] bool isPort() const;

    /**
     * @brief Get the dwell time at this port.
     * @return Dwell time in seconds (0 if not a port)
     */
    [[nodiscard]] units::time::second_t getDwellTime() const;

    /**
     * @brief Mark this point as a port with specified dwell time.
     * @param dwellTime Time to dwell at the port
     */
    void MarkAsPort(units::time::second_t dwellTime);

    /**
     * @brief Clear the port status of this point.
     */
    void MarkAsNonPort();

    // =========================================================================
    // Polygon Ownership (for visibility graph optimization)
    // =========================================================================

    /**
     * @brief Get the polygons that own this vertex.
     *
     * For polygon boundary vertices, returns the polygon(s) this vertex
     * belongs to. Returns empty vector for non-vertex points.
     * Used for O(1) visibility lookups instead of O(P×V) containment checks.
     *
     * @return Vector of owning polygons (may be empty)
     */
    [[nodiscard]] QVector<std::shared_ptr<Polygon>> getOwningPolygons() const;

    /**
     * @brief Add an owning polygon to this vertex.
     *
     * Called during polygon/visibility graph construction to register
     * that this vertex belongs to the specified polygon's boundary.
     *
     * @param polygon The polygon that owns this vertex
     */
    void addOwningPolygon(const std::shared_ptr<Polygon>& polygon);

    /**
     * @brief Clear all owning polygon references.
     *
     * Called when polygons are cleared or replaced in the visibility graph.
     */
    void clearOwningPolygons();

    // =========================================================================
    // Visibility Cache (for pathfinding optimization)
    // =========================================================================

    /**
     * @brief Check if visibility to neighbors in a polygon is cached.
     * @param polygon The polygon to check
     * @return true if cache exists for this polygon
     */
    [[nodiscard]] bool hasVisibleNeighborsCache(const Polygon* polygon) const;

    /**
     * @brief Get cached visible neighbors within a polygon.
     * @param polygon The polygon containing this vertex
     * @return Cached visible neighbors, or empty if not cached
     */
    [[nodiscard]] QVector<std::shared_ptr<GPoint>>
    getVisibleNeighborsInPolygon(const Polygon* polygon) const;

    /**
     * @brief Cache visible neighbors within a polygon.
     * @param polygon The polygon containing this vertex
     * @param neighbors The visible neighbors to cache
     */
    void setVisibleNeighborsInPolygon(
        const Polygon* polygon,
        const QVector<std::shared_ptr<GPoint>>& neighbors) const;

    /**
     * @brief Clear all visibility caches.
     * Called when polygons change.
     */
    void clearVisibleNeighborsCache() const;

    // =========================================================================
    // Serialization
    // =========================================================================

    /**
     * @brief Serialize this point to a binary stream.
     *
     * Writes all point data in portable binary format using big-endian
     * byte order. The serialized data includes coordinates, user ID,
     * port status, and dwell time.
     *
     * @param out Output stream (must be opened in binary mode)
     * @throws std::runtime_error if stream is not ready for writing
     */
    void serialize(std::ostream &out) const;

    /**
     * @brief Deserialize a point from a binary stream.
     *
     * Reads binary data and reconstructs the point. The stream must
     * contain data written by serialize().
     *
     * @param in Input stream (must be opened in binary mode)
     * @throws std::runtime_error if stream is not ready or data is corrupted
     */
    void deserialize(std::istream &in);

    // =========================================================================
    // String Representation
    // =========================================================================

    /**
     * @brief Convert the point to a formatted string.
     *
     * Supported placeholders (case-insensitive):
     * - %x: Longitude value
     * - %y: Latitude value
     * - %id: User identifier (or "N/A" if not set)
     *
     * @param format Format string with placeholders
     * @param decimalPercision Number of decimal places for coordinates
     * @return Formatted string representation
     */
    [[nodiscard]] QString
    toString(const QString &format = "(%x, %y)",
             int decimalPercision  = 5) const override;

    // =========================================================================
    // Operators
    // =========================================================================

    /**
     * @brief Equality comparison based on coordinates.
     * @param other Point to compare with
     * @return true if coordinates are exactly equal
     */
    [[nodiscard]] bool operator==(const GPoint &other) const;

    /**
     * @brief Component-wise addition of coordinates.
     * @param other Point to add
     * @return New point with summed coordinates
     */
    GPoint operator+(const GPoint &other) const;

    /**
     * @brief Component-wise subtraction of coordinates.
     * @param other Point to subtract
     * @return New point with differenced coordinates
     */
    GPoint operator-(const GPoint &other) const;

    /**
     * @brief Stream output operator for debugging.
     */
    friend std::ostream &operator<<(std::ostream &os, const GPoint &point);

    // =========================================================================
    // Hash and Equality Functors
    // =========================================================================

    /**
     * @brief Hash functor for use in hash-based containers.
     *
     * Computes a hash based on latitude and longitude values.
     */
    struct Hash
    {
        std::size_t operator()(const std::shared_ptr<GPoint> &p) const;
    };

    /**
     * @brief Equality functor for use in hash-based containers.
     *
     * Compares points for coordinate equality through shared_ptr.
     */
    struct Equal
    {
        bool operator()(const std::shared_ptr<GPoint> &lhs,
                        const std::shared_ptr<GPoint> &rhs) const;
    };
};

}  // namespace ShipNetSimCore

/**
 * @brief std::hash specialization for GPoint.
 *
 * Enables GPoint to be used directly as key in std::unordered_map
 * and std::unordered_set.
 */
template <>
struct std::hash<ShipNetSimCore::GPoint>
{
    std::size_t operator()(const ShipNetSimCore::GPoint &p) const;
};

#endif  // GPOINT_H
