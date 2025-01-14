/**
 * Class GPoint - Represents a geographical point with latitude and longitude,
 *                and optional spatial reference for geodetic calculations.
 *
 * The class encapsulates operations such as distance calculation,
 * coordinate transformation, and serialization. It utilizes GDAL for
 * spatial operations and adheres to units library for type-safe computations.
 *
 *
 * Usage:
 * - To create a GPoint, use one of the constructors with the desired
 *   latitude, longitude, and optionally, a spatial reference and a
 *   unique ID.
 * - To transform the point's datum, provide a target spatial reference
 *   to transformDatumTo.
 *   Ensure the target spatial reference is managed externally.
 * - Serialization and deserialization methods allow for persistent
 *   storage or network transmission of GPoint instances in a binary format.
 */

#ifndef GPOINT_H
#define GPOINT_H

#include "../export.h"
#include <gdal.h>
#include <ogr_geometry.h>
#include <ogr_spatialref.h>
#include <QString>
#include "../../third_party/units/units.h"
#include "basegeometry.h"

namespace ShipNetSimCore
{
class Point; // Forward declaration

class SHIPNETSIM_EXPORT GPoint : public BaseGeometry
{
public:
    // Default constructor: initializes a point at the origin
    // (0, 0) with default spatial reference.
    GPoint();

    /**
     * Constructs a GPoint with specified longitude and latitude.
     * Optionally, a custom spatial reference can be provided; if omitted,
     * a default (WGS84) is used.
     * @param lon Longitude in degrees.
     * @param lat Latitude in degrees.
     * @param crc Optional spatial reference (defaults to WGS84 if
     *            not provided).
     */
    GPoint(units::angle::degree_t lon, units::angle::degree_t lat,
           OGRSpatialReference crc = OGRSpatialReference());

    /**
     * Constructs a GPoint with specified longitude, latitude, and
     *  a unique ID.
     * Optionally, a custom spatial reference can be provided; if
     * omitted, a default (WGS84) is used.
     * @param lon Longitude in degrees.
     * @param lat Latitude in degrees.
     * @param ID A unique identifier for the point.
     * @param crc Optional spatial reference (defaults to WGS84
     *            if not provided).
     */
    GPoint(units::angle::degree_t lon, units::angle::degree_t lat,
           QString ID, OGRSpatialReference crc = OGRSpatialReference());

    /**
     * Calculates the geodesic distance to another GPoint.
     * Both points must have spatial references set and they must be the same.
     * @param other The other GPoint to calculate the distance to.
     * @return Distance in meters as units::length::meter_t.
     */
    [[nodiscard]] units::length::meter_t distance(const GPoint& other) const;

    /**
     * Calculates the geodesic azimuth from this point to another GPoint.
     * Both points must have spatial references set and they must be the same.
     * @param other The other GPoint to calculate the azimuth to.
     * @return Azimuth in degrees as units::angle::degree_t.
     */
    [[nodiscard]] units::angle::degree_t
    forwardAzimuth(const GPoint& other) const;

    /**
     * Calculates the geodesic azimuth from the another GPoint to this point.
     * Both points must have spatial references set and they must be the same.
     * @param other The other GPoint to calculate the azimuth from.
     * @return Azimuth in degrees as units::angle::degree_t.
     */
    [[nodiscard]] units::angle::degree_t
    backwardAzimuth(const GPoint& other) const;

    // Returns the GDAL OGRPoint representation of this GPoint.
    OGRPoint getGDALPoint() const;

    /**
     * Transforms the datum (spatial reference) of this point to the
     * target spatial reference.
     * The target spatial reference must be geographic (not projected).
     * @param targetSR Pointer to the target OGRSpatialReference.
     *                 The caller is responsible for managing the
     *                 lifecycle of the spatial reference.
     */
    void transformDatumTo(OGRSpatialReference* targetSR);

    /**
     * Projects this geographic point to a new spatial reference,
     * typically a projected coordinate system.
     * @param targetSR Pointer to the target OGRSpatialReference for
     *                 projection.
     *                 The caller is responsible for managing the
     *                 lifecycle of the spatial reference.
     * @return A new Point instance in the projected coordinate system.
     */
    Point projectTo(OGRSpatialReference* targetSR) const;

    // Getters for latitude and longitude.

    /**
     * @brief Gets the latitude of the point.
     * @return The latitude of the point in degrees.
     */
    [[nodiscard]] units::angle::degree_t getLatitude() const;

    /**
     * @brief Gets the longitude of the point.
     * @return The longitude of the point in degrees.
     */
    [[nodiscard]] units::angle::degree_t getLongitude() const;

    /**
     * @brief Sets the latitude of the point.
     * @param lat new latitude value.
     */
    void setLatitude(units::angle::degree_t lat);

    /**
     * @brief Sets the longitude of the point.
     * @param lon new longitude value.
     */
    void setLongitude(units::angle::degree_t lon);

    /**
     * @brief A static function to get the default reprojection reference.
     * @details GPoint takes ownership of the pointer.
     *          The class manages the lifecycle
     *          of these objects, ensuring they remain valid as long as
     *          the GPoint instances use them.
     * @return A shared pointer for the default reprojection reference.
     */
    static std::shared_ptr<OGRSpatialReference>
    getDefaultReprojectionReference();

    /**
     * @brief A static function to set the default reprojection reference.
     * @param wellknownCS the well known geodetic coordinate system
     *                    as a CS reference of the point.
     */
    static void setDefaultReprojectionReference(std::string wellknownCS);

    /**
     * Calculate a new point given a distance and heading from this point.
     *
     * @param distance Distance from the current point in meters.
     * @param heading Heading in degrees, measured clockwise from north.
     * @return A new GPoint representing the location after moving the given
     *          distance along the given heading.
     */
    GPoint pointAtDistanceAndHeading(units::length::meter_t distance,
                                     units::angle::degree_t heading) const;

    /**
     * @brief Check if the point is a port.
     *
     * @return True if the point is a port, false otherwise.
     */
    [[nodiscard]] bool isPort() const;

    /**
     * @brief Get the dwell time of the port.
     *
     * @return The dwell time if the point is a port.
     */
    [[nodiscard]] units::time::second_t getDwellTime() const;

    /**
     * @brief Mark the point as not a port
     */
    void MarkAsNonPort();

    /**
     * @brief Mark the point as a port with the given dwell time.
     *
     * @param dwellTime The dwell time of the port.
     */
    void MarkAsPort(units::time::second_t dwellTime);

    /**
     * @brief get the middle point between the current point and an end point.
     * @param endPoint The end point of the segment.
     * @return the middle point between the current point and the end point.
     */
    GPoint getMiddlePoint(const GPoint& endPoint) const;

    /**
     * @brief Converts the GPoint object to a formatted string representation.
     *
     * This function dynamically formats the output string based on the
     * user-provided format string. Placeholders in the format string are
     * replaced as follows:
     * - `%x`: Replaced with the longitude value.
     * - `%y`: Replaced with the latitude value.
     * - `%id`: Replaced with the user ID (or "N/A" if no ID is set).
     *
     * The replacement is case-insensitive, allowing placeholders such
     * as `%X`, `%Y`, or `%ID`.
     *
     * @param format A QString specifying the desired format of the output.
     *               It must include placeholders (`%x`, `%y`, `%id`) for
     *               the longitude, latitude, and user ID, respectively.
     *
     * @return A QString containing the formatted output with
     *          placeholders replaced.
     *         If a placeholder is missing, it will not be replaced.
     *
     * @example
     * GPoint point;
     * point.setLongitude(units::angle::degree_t(10.123));
     * point.setLatitude(units::angle::degree_t(20.456));
     * point.setUserID("Ship123");
     *
     * QString format = "ID: %id at (%x, %y)";
     * qDebug() << point.toString(format);
     * // Output: "ID: Ship123 at (10.123, 20.456)"
     */
    [[nodiscard]] QString toString(const QString &format= "(%x, %y)",
                                   int decimalPercision = 5) const override;

    /**
     * @brief Equality operator.
     *
     * Checks if this point is equal to another point.
     *
     * @param other The other point.
     * @return True if equal, false otherwise.
     */
    [[nodiscard]] bool operator==(const GPoint &other) const;

    GPoint operator+(const GPoint& other) const;

    GPoint operator-(const GPoint& other) const;

    // Custom hash function for GPoint
    struct Hash {
        std::size_t operator()(const std::shared_ptr<GPoint>& p) const;
    };

    // Custom equality function for GPoint
    struct Equal {
        bool operator()(const std::shared_ptr<GPoint>& lhs,
                        const std::shared_ptr<GPoint>& rhs) const;
    };

    /**
     * @brief Serialize the point to a binary format and write it
     *          to an output stream.
     *
     * This function serializes the point class data to a binary format.
     * The serialized data is written to the provided outptut stream.
     * The serialization format is designed for efficient storage and
     * is not hyman-readable. This function is typically used to save
     * the state of the point to a file or transmit it over a network.
     *
     * @param out The output stream to which the point is serialized.
     *              This stream should be opened in binary mode to
     *              correctly handle the binary data.
     *
     * Usage Example:
     *      std::ofstream outFile("point.bin", std::ios::binary);
     *      if (outFile.is_open()) {
     *          point.serialize(outFile);
     *          outFile.close();
     *      }
     */
    void serialize(std::ostream& out) const;

    /**
     * @brief Deserialize the point from a binary formate read from
     *          an input stream.
     *
     * This function reads binary data from the provided input stream
     * and reconstructs the point. The binary format should match the
     * format produced by the serialize function.
     * This function is typically used to load a point from a file
     * or received it from a network transmission. The existing content
     * of the Point is clearned before deserialization. If the binary
     * data is corrupted or improperly formatted,
     * The behavior of this function is undefined, and the resulting
     * Point may be incomplete or invalid.
     *
     * @param in The input stream from which the Point is deserialized.
     *              This stream should be opened in binary mode to
     *              coorectly handle the binary data.
     *
     * Usage Example:
     *     std::ifstream inFile("point.bin", std::ios::binary);
     *     if (inFile.is_open()) {
     *         point.deserialize(inFile);
     *         inFile.close();
     *     }
     */
    void deserialize(std::istream& in);

    // Friend declaration for stream insertion operator
    friend std::ostream& operator<<(std::ostream& os, const GPoint& point);
private:
    OGRPoint mOGRPoint; ///< GDAL Point definition
    QString mUserID; ///< The unique ID of the point.
    bool mIsPort; ///< Flag indicating if the point is a port.
    units::time::second_t
        mDwellTime; ///< The dwell time if the point is a port.

    /// The default Coordinate system
    static std::shared_ptr<OGRSpatialReference> spatialRef;
};

};

/**
 * @struct std::hash<Point>
 *
 * @brief Specialization of std::hash for Point class.
 *
 * This struct defines a hash function for objects of type Point,
 * enabling them to be used as keys in std::unordered_map or other
 * hash-based containers.
 */
template <>
struct std::hash<ShipNetSimCore::GPoint> {
    std::size_t operator()(const ShipNetSimCore::GPoint& p) const;
};
#endif // GPOINT_H
