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

#include "basegeometry.h"
#include <string>
#include <functional>
#include <cmath>
#include "../../third_party/units/units.h"

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
     * @param index The index of the point.
     */
    Point(units::length::meter_t xCoord, units::length::meter_t yCoord,
          QString ID, unsigned int index);

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
    distance(const Point &endPoint) const;

    /**
     * @brief Get a string representation of the point.
     *
     * @return A string representing the point.
     */
    [[nodiscard]] QString toString() override;

    /**
     * @brief Equality operator.
     *
     * Checks if this point is equal to another point.
     *
     * @param other The other point.
     * @return True if equal, false otherwise.
     */
    [[nodiscard]] bool operator==(const Point &other) const;

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

private:
    units::length::meter_t mx; ///< The x-coordinate of the point.
    units::length::meter_t my; ///< The y-coordinate of the point.
    QString userID; ///< The unique ID of the point.
    unsigned int index; ///< The index of the point.
    bool mIsPort; ///< Flag indicating if the point is a port.
    units::time::second_t
        mDwellTime; ///< The dwell time if the point is a port.
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
