/**
 * @file point.cpp
 * @brief Implementation of the Point class and its utilities.
 *
 * This file contains implementations for the Point class and
 * associated utilities. It includes constructors, destructors,
 * getter functions, setter functions, comparison operators, and
 * additional functionality like calculating distance between points
 * and marking a point as a port.
 *
 * Author: Ahmed Aredah
 * Date: 10.12.2023
 */

#include "point.h"  // Include the header file for the Point class.
#include <cmath>  // Include the cmath library for mathematical functions.
#include <sstream>  // Include for string stream operations.


// Default constructor, initializes members with NaN and default values.
Point::Point()
    : mx(std::nan("No Value")),  // Initialize mx with NaN.
    my(std::nan("No Value")),  // Initialize my with NaN.
    userID(""),  // Initialize userID with an empty string.
    index(0),  // Initialize index with 0.
    mIsPort(false),  // Initialize mIsPort with false.
    mDwellTime(units::time::second_t(0.0))  // Initialize mDwellTime with 0.
{}

// Parameterized constructor with user ID and index specified.
Point::Point(units::length::meter_t xCoord,
             units::length::meter_t yCoord,
             QString ID, unsigned int index)
    : mx(xCoord),  // Initialize mx with xCoord.
    my(yCoord),  // Initialize my with yCoord.
    mIsPort(false),  // Initialize mIsPort with false.
    mDwellTime(units::time::second_t(0.0)),  // Initialize mDwellTime with 0.
    userID(ID),  // Initialize userID with ID.
    index(index)  // Initialize index with index.
{}

// Parameterized constructor without user ID and index.
Point::Point(units::length::meter_t xCoord,
             units::length::meter_t yCoord)
    : mx(xCoord),  // Initialize mx with xCoord.
    my(yCoord),  // Initialize my with yCoord.
    mIsPort(false),  // Initialize mIsPort with false.
    mDwellTime(units::time::second_t(0.0)),  // Initialize mDwellTime with 0.
    userID("temporary point"),  // Initialize userID with a default value.
    // Initialize index with the minimum possible value for unsigned int.
    index(std::numeric_limits<unsigned int>::min())
{}

// Destructor for the Point class.
Point::~Point() {
    // Destructor logic
}

// Getter function for x-coordinate.
units::length::meter_t Point::x() const
{
    return mx;  // Return mx.
}

// Getter function for y-coordinate.
units::length::meter_t Point::y() const
{
    return my;  // Return my.
}


// Function to check if the point is valid (i.e., coordinates are not NaN).
bool Point::isValid()
{
    // Return true if neither mx nor my is NaN.
    return !(std::isnan(mx.value()) || std::isnan(my.value()));
}

// Function to calculate the distance between the current point and endPoint.
units::length::meter_t Point::distance(const Point &endPoint) const
{
    // Calculate the difference in x-coordinates.
    units::length::meter_t dx = mx - endPoint.x();
    // Calculate the difference in y-coordinates.
    units::length::meter_t dy = my - endPoint.y();
    // Calculate and return the Euclidean distance.
    return units::math::sqrt(units::math::pow<2>(dx) +
                             units::math::pow<2>(dy));
}

// Function to convert the point to a string representation.
QString Point::toString()
{
    // Format the string as "Point userID(x, y)".
    QString str = QString("Point %1(%2, %3)")
                      .arg(userID)  // Insert userID.
                      .arg(mx.value())  // Insert x-coordinate.
                      .arg(my.value());  // Insert y-coordinate.
    return str;  // Return the formatted string.
}

// Function to check if the point is a port.
bool Point::isPort()
{
    return mIsPort;  // Return the value of mIsPort.
}

// Getter function for dwell time.
units::time::second_t Point::getDwellTime()
{
    return mDwellTime;  // Return mDwellTime.
}

// Function to mark the point as a port and set its dwell time.
void Point::MarkAsPort(units::time::second_t dwellTime)
{
    mIsPort = true;  // Set mIsPort to true.
    mDwellTime = dwellTime;  // Set mDwellTime to dwellTime.
}

// Setter function for x-coordinate.
void Point::setX(units::length::meter_t newX)
{
    mx = newX;  // Set mx to newX.
}

// Setter function for y-coordinate.
void Point::setY(units::length::meter_t newY)
{
    my = newY;  // Set my to newY.
}

// Overloaded equality operator to compare two points.
bool Point::operator==(const Point &other) const
{
    // Return true if both x and y coordinates are the same.
    return mx == other.x() && my == other.y();
}

// Hash function for the Point class.
std::size_t std::hash<Point>::operator()(const Point &p) const
{
    // Calculate the hash value using the x and y coordinates.
    return std::hash<long double>()(p.x().value()) ^
           std::hash<long double>()(p.y().value());
}
