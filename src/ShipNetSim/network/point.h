/**
 * @file point.h
 *
 * @brief This file contains the declaration of the Point class,
 * which represents a point in a two-dimensional space.
 *
 * The Point class is derived from the BaseGeometry class. It includes
 * properties such as coordinates, unique ID, index, and dwell time if the
 * point is a port. It provides methods to calculate the distance between
 * points, check the validity of the point, and get a string representation
 * of the point. It also provides operator overloads for equality.
 *
 * @author Ahmed Aredah
 * @date 10.12.2023
 */
#ifndef POINT_H
#define POINT_H

#include <gdal.h>
#include <ogr_geometry.h>
#include <ogr_spatialref.h>
#include "basegeometry.h"
#include <functional>
#include <cmath>
#include "../../third_party/units/units.h"


class GPoint; // Forward declaration

/**
 * @class Point
 *
 * @brief Represents a point in a two-dimensional space.
 *
 * This class stores the x and y coordinates of the point, a unique ID,
 * and an index. If the point is a port, it also stores the dwell time.
 * It provides methods to calculate the distance to another point,
 * check the validity of the point, and get a string representation of
 * the point. It also includes operator overloads for equality.
 */
class Point : public BaseGeometry
{

public:
    /**
     * @brief Default constructor.
     *
     * Initializes the point with default values.
     */
    Point();

    /**
     * @brief Parameterized constructor.
     *
     * Initializes the point with the given x and y coordinates,
     * unique ID, and index.
     *
     * @param xCoord The x-coordinate of the point.
     * @param yCoord The y-coordinate of the point.
     * @param ID The unique ID of the point.
     */
    Point(units::length::meter_t xCoord, units::length::meter_t yCoord,
          QString ID, OGRSpatialReference crc = OGRSpatialReference());

    OGRPoint getGDALPoint() const;

    Point pointAtDistanceAndHeading(units::length::meter_t distance,
                                    units::angle::degree_t heading) const;

    void transformDatumTo(OGRSpatialReference* targetSR);
    GPoint reprojectTo(OGRSpatialReference* targetSR);

    static std::shared_ptr<OGRSpatialReference> getDefaultProjectionReference();

    static void setDefaultProjectionReference(std::string wellknownCS);

    /**
     * @brief Parameterized constructor.
     *
     * Initializes the point with the given x and y coordinates.
     *
     * @param xCoord The x-coordinate of the point.
     * @param yCoord The y-coordinate of the point.
     */
    Point(units::length::meter_t xCoord, units::length::meter_t yCoord);

    /**
     * @brief Destructor.
     *
     * Destroys the point object.
     */
    ~Point();

    /**
     * @brief Check if the point is valid.
     *
     * @return True if valid, false otherwise.
     */
    [[nodiscard]] bool isValid();

    /**
     * @brief Calculate distance to another point.
     *
     * @param endPoint The other point.
     * @return Distance to the other point.
     */
    [[nodiscard]] units::length::meter_t
    distance(const Point &endPoint,
             units::length::meter_t mapWidth =
             units::length::meter_t(std::nan("No Value"))) const;

    /**
     * @brief Get a string representation of the point.
     *
     * @return A string representing the point.
     */
    [[nodiscard]] QString toString() const override;

    /**
     * @brief Equality operator.
     *
     * Checks if this point is equal to another point.
     *
     * @param other The other point.
     * @return True if equal, false otherwise.
     */
    [[nodiscard]] bool operator==(const Point &other) const;

    [[nodiscard]] Point operator*(const double scale) const;
    [[nodiscard]] Point operator+(const Point& other) const;
    [[nodiscard]] Point operator-(const Point& other) const;

    /**
     * @brief Check if two points are exactly equal.
     * this includes all parameters.
     * @param other The other point.
     * @return True if exactly equal, false otherwise.
     */
    [[nodiscard]] bool isExactlyEqual(const Point& other) const;

    /**
     * @brief Get the x-coordinate of the point.
     *
     * @return The x-coordinate.
     */
    [[nodiscard]] units::length::meter_t x() const;

    /**
     * @brief Get the y-coordinate of the point.
     *
     * @return The y-coordinate.
     */
    [[nodiscard]] units::length::meter_t y() const;

    /**
     * @brief Check if the point is a port.
     *
     * @return True if the point is a port, false otherwise.
     */
    [[nodiscard]] bool isPort();

    /**
     * @brief Get the dwell time of the port.
     *
     * @return The dwell time if the point is a port.
     */
    [[nodiscard]] units::time::second_t getDwellTime();

    /**
     * @brief Mark the point as a port with the given dwell time.
     *
     * @param dwellTime The dwell time of the port.
     */
    void MarkAsPort(units::time::second_t dwellTime);

    /**
     * @brief Set the x-coordinate of the point.
     *
     * @param newX The new x-coordinate.
     */
    void setX(units::length::meter_t newX);

    /**
     * @brief Set the y-coordinate of the point.
     *
     * @param newY The new y-coordinate.
     */
    void setY(units::length::meter_t newY);

    /**
     * @brief get the middle point between the current point and an end point.
     * @param endPoint The end point of the segment.
     * @return the middle point between the current point and the end point.
     */
    [[nodiscard]] Point getMiddlePoint(const Point& endPoint);

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

    // Nested custom hash function for Point
    struct Hash {
        std::size_t operator()(
            const std::shared_ptr<Point>& p) const;
    };

    // Nested custom equality function for Point
    struct Equal {
        bool operator()(const std::shared_ptr<Point>& lhs,
                        const std::shared_ptr<Point>& rhs) const;
    };

    /**
     * @brief Stream insertion operator for Point.
     *
     * Outputs a string representation of the Point to an output stream.
     *
     * @param os The output stream.
     * @param point The Point object to be outputted.
     * @return Reference to the output stream.
     */
    friend std::ostream& operator<<(std::ostream& os, const Point& point);


private:
    OGRPoint mOGRPoint; ///< GDAL Point definition
    QString mUserID; ///< The unique ID of the point.
    bool mIsPort; ///< Flag indicating if the point is a port.
    units::time::second_t
        mDwellTime; ///< The dwell time if the point is a port.

     /// The default Coordinate system
    static std::shared_ptr<OGRSpatialReference> spatialRef;
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
struct std::hash<Point> {
    std::size_t operator()(const Point& p) const;
};

#endif // POINT_H
