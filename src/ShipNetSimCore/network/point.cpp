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
#include "gpoint.h" // Include the header file for the Geodetic point class
#include <cmath>  // Include the cmath library for mathematical functions.
// #include <sstream>  // Include for string stream operations.
#include <QtEndian>  // Include Qt's endianness conversion functions
#include <GeographicLib/Geocentric.hpp>

namespace ShipNetSimCore
{
std::shared_ptr<OGRSpatialReference> Point::spatialRef = nullptr;

// Default constructor, initializes members with NaN and default values.
Point::Point()
    : mOGRPoint(std::nan("No Value"), std::nan("No Value")), // Initialize NaN.
    mUserID(""),  // Initialize userID with an empty string.
    mIsPort(false),  // Initialize mIsPort with false.
    mDwellTime(units::time::second_t(0.0))  // Initialize mDwellTime with 0.
{}

// Parameterized constructor with user ID and index specified.
Point::Point(units::length::meter_t xCoord,
             units::length::meter_t yCoord,
             QString ID,
             OGRSpatialReference crc)
    : mOGRPoint(xCoord.value(), yCoord.value()),  // Initialize the Coord.
    mUserID(ID),  // Initialize mIsPort with false.
    mIsPort(false),  // Initialize userID with ID.
    mDwellTime(units::time::second_t(0.0))  // Initialize index with index.
{
    OGRSpatialReference* SR = nullptr;
    if (!crc.IsEmpty()) {
        if (!crc.IsProjected())
        {
            qFatal("Spatial reference must be projected!");
        }
        // Clone the passed spatial reference and assign
        // it to spatialRef pointer
        SR = crc.Clone();
    } else {
        // If no spatial reference is passed, create a
        // new spatial reference for world_behrmann projection
        SR = getDefaultProjectionReference()->Clone();
    }

    // Assign the spatial reference to mOGRPoint.
    // mOGRPoint does not take ownership of the spatialRef.
    mOGRPoint.assignSpatialReference(SR);
}

std::shared_ptr<OGRSpatialReference> Point::getDefaultProjectionReference()
{
    // Initialize spatialRef with WGS84 if it hasn't been initialized yet
    if (!spatialRef) {
        Point::spatialRef = std::make_shared<OGRSpatialReference>();
        const char *pszWKT = "PROJCS[\"World_Behrmann\","
                             "GEOGCS[\"WGS 84\","
                             "DATUM[\"WGS_1984\","
                             "SPHEROID[\"WGS 84\",6378137,298.257223563,"
                             "AUTHORITY[\"EPSG\",\"7030\"]],"
                             "AUTHORITY[\"EPSG\",\"6326\"]],"
                             "PRIMEM[\"Greenwich\",0],"
                             "UNIT[\"Degree\",0.0174532925199433]],"
                             "PROJECTION[\"Cylindrical_Equal_Area\"],"
                             "PARAMETER[\"standard_parallel_1\",30],"
                             "PARAMETER[\"central_meridian\",0],"
                             "PARAMETER[\"false_easting\",0],"
                             "PARAMETER[\"false_northing\",0],"
                             "UNIT[\"metre\",1,"
                             "AUTHORITY[\"EPSG\",\"9001\"]],"
                             "AXIS[\"Easting\",EAST],"
                             "AXIS[\"Northing\",NORTH],"
                             "AUTHORITY[\"ESRI\",\"54017\"]]";

        OGRErr err = Point::spatialRef->importFromWkt(pszWKT);
        if (err != OGRERR_NONE) {
            qFatal("Failed to set World Behrmann spatial reference");
        }
    }
    return Point::spatialRef;
}

void Point::setDefaultProjectionReference(std::string wellknownCS)
{
    if (!Point::spatialRef) {
        Point::spatialRef = std::make_shared<OGRSpatialReference>();
    }

    // Temporary spatial reference for validation
    std::shared_ptr<OGRSpatialReference> tempRef =
        std::make_shared<OGRSpatialReference>();
    OGRErr err = tempRef->SetWellKnownGeogCS(wellknownCS.c_str());
    if (err != OGRERR_NONE) {
        // Exit the function on failure
        qFatal("Failed to interpret the provided spatial reference: %s",
               qPrintable(QString::fromStdString(wellknownCS)));
    }

    // Check if the spatial reference is geodetic
    if (!tempRef->IsProjected()) {
        // Exit the function if not projected
        qFatal("The provided spatial reference is not projected: %s",
               qPrintable(QString::fromStdString(wellknownCS)));
    }

    // If validation passed, assign the validated spatial
    // reference to the class's static member
    Point::spatialRef = tempRef;

}

OGRPoint Point::getGDALPoint() const { return mOGRPoint; }

void Point::transformDatumTo(OGRSpatialReference *targetSR)
{
    if (targetSR && targetSR->IsProjected())
    {
        const OGRSpatialReference* currentSR = mOGRPoint.getSpatialReference();
        if (currentSR && !currentSR->IsSame(targetSR)) {
            OGRCoordinateTransformation* coordTransform =
                OGRCreateCoordinateTransformation(currentSR, targetSR);
            if (coordTransform) {
                double x = mOGRPoint.getX(), y = mOGRPoint.getY();
                if (coordTransform->Transform(1, &x, &y)) {
                    // Update the internal OGRPoint coordinates
                    mOGRPoint.setX(x);
                    mOGRPoint.setY(y);

                    // Update point's spatial reference to the target
                    mOGRPoint.assignSpatialReference(targetSR);
                }
                OCTDestroyCoordinateTransformation(coordTransform);
            }
        }
    }
    else
    {
        qFatal("Target spatial reference is not projected!");
    }
}

GPoint Point::reprojectTo(OGRSpatialReference *targetSR)
{
    // Ensure the target Spatial Reference is valid and is a projected CRS
    if (!targetSR || !targetSR->IsGeographic()) {
        qFatal("Target Spatial Reference "
               "is not valid or not a geographic CRS.");
    }

    const OGRSpatialReference* currentSR = mOGRPoint.getSpatialReference();
    if (currentSR == nullptr) {
        qFatal("Current Spatial Reference is not set.");
    }

    // Create a coordinate transformation from the current
    // geographic CRS to the target projected CRS
    OGRCoordinateTransformation* coordTransform =
        OGRCreateCoordinateTransformation(currentSR, targetSR);
    if (!coordTransform) {
        qFatal("Failed to create coordinate transformation.");
    }

    double x = mOGRPoint.getX();
    double y = mOGRPoint.getY();

    // Transform the point's coordinates from geographic to projected CRS
    if (!coordTransform->Transform(1, &x, &y)) {
        OCTDestroyCoordinateTransformation(coordTransform);
        qFatal("Failed to transform point coordinates.");
    }

    OCTDestroyCoordinateTransformation(coordTransform);

    // Create and return the transformed point.
    return GPoint(units::angle::degree_t(x),
                 units::angle::degree_t(y),
                 mUserID, *targetSR);
}

// Parameterized constructor without user ID and index.
Point::Point(units::length::meter_t xCoord,
             units::length::meter_t yCoord)
    : mOGRPoint(xCoord.value(), yCoord.value()),  // Initialize the Coord.
    mUserID("temporary point"),  // Initialize mIsPort with false.
    mIsPort(false),  // Initialize userID with a default value.
    // Initialize index with the minimum possible value for unsigned int.
    mDwellTime(units::time::second_t(0.0))
{
    OGRSpatialReference* SR = getDefaultProjectionReference()->Clone();

    // Assign the spatial reference to mOGRPoint.
    // mOGRPoint does not take ownership of the spatialRef.
    mOGRPoint.assignSpatialReference(SR);
}

// Destructor for the Point class.
Point::~Point() {
    // Destructor logic
}

// Getter function for x-coordinate.
units::length::meter_t Point::x() const
{
    return units::length::meter_t(mOGRPoint.getX());  // Return mx.
}

// Getter function for y-coordinate.
units::length::meter_t Point::y() const
{
    return units::length::meter_t(mOGRPoint.getY());  // Return my.
}


// Function to check if the point is valid (i.e., coordinates are not NaN).
bool Point::isValid()
{
    // Return true if neither mx nor my is NaN.
    return !(std::isnan(mOGRPoint.getX()) || std::isnan(mOGRPoint.getY()));
}

// Function to calculate the distance between the current point and endPoint.
units::length::meter_t Point::distance(const Point &endPoint,
                                       units::length::meter_t mapWidth) const
{
    // Calculate the difference in x-coordinates.
    units::length::meter_t dx = x() - endPoint.x();
    // Calculate the difference in y-coordinates.
    units::length::meter_t dy = y() - endPoint.y();

    // Check if crossing the map boundary is shorter
    if (!std::isnan(mapWidth.value()) && dx > mapWidth / 2)
    {
        dx = mapWidth - dx;
    }

    // Calculate and return the Euclidean distance.
    return units::math::sqrt(units::math::pow<2>(dx) +
                             units::math::pow<2>(dy));
}

// Function to convert the point to a string representation.
QString Point::toString(const QString &format, int decimalPercision) const
{
    QString str;
    QString xStr =
        QString::number(mOGRPoint.getX(), 'f', decimalPercision); // Convert x-coordinate to string in decimal format
    QString yStr =
        QString::number(mOGRPoint.getY(), 'f', decimalPercision); // Convert y-coordinate to string in decimal format
    QString idStr = mUserID.isEmpty() ? "N/A" : mUserID;

    QString result = format;

    // Replace placeholders (case-insensitive)
    result.replace("%x", xStr, Qt::CaseInsensitive);
    result.replace("%y", yStr, Qt::CaseInsensitive);
    result.replace("%id", idStr, Qt::CaseInsensitive);

    return str;  // Return the formatted string.
}

Point Point::pointAtDistanceAndHeading(units::length::meter_t distance,
                                         units::angle::degree_t heading) const
{
    // Convert heading to radians
    double headingRadians = heading.value() * (M_PI / 180.0);

    // Calculate new x and y using trigonometry
    // Heading is measured clockwise from the north
    double newX = x().value() + distance.value() * cos(headingRadians);
    double newY = y().value() + distance.value() * sin(headingRadians);

    // Create a new GPoint with the calculated x, y.
    // Assuming the spatial reference (SR) of the new point is the same
    // as the current point.
    const OGRSpatialReference* currentSR = mOGRPoint.getSpatialReference();

    return Point(units::length::meter_t(newX),
                 units::length::meter_t(newY),
                 "NewPoint", *currentSR);
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
    mOGRPoint.setX(newX.value());  // Set mx to newX.
}

// Setter function for y-coordinate.
void Point::setY(units::length::meter_t newY)
{
    mOGRPoint.setY(newY.value());  // Set my to newY.
}

Point Point::getMiddlePoint(const Point& endPoint)
{
    // Calculate the midpoint coordinates.
    double midX = (x().value() + endPoint.x().value()) / 2.0;
    double midY = (y().value() + endPoint.y().value()) / 2.0;

    // Return the midpoint as a Point object.
    return Point(units::length::meter_t(midX), units::length::meter_t(midY));
}
// // check if the point is within a bounding box
// bool Point::isPointInsideBoundingBox(const Point& min_point,
//                                      const Point& max_point) const
// {
//     return x() >= min_point.x() && x() <= max_point.x() &&
//            y() >= min_point.y() && y() <= max_point.y();
// }

Point Point::operator*(const double scale) const
{
    return Point(x() * scale, y() * scale);
}

Point Point::operator-(const Point& other) const
{
    return Point(x() - other.x(), y() - other.y());
}

Point Point::operator+(const Point& other) const
{
    return Point(x() + other.x(), y() + other.y());
}

// Overloaded equality operator to compare two points.
bool Point::operator==(const Point &other) const
{
    // Return true if both x and y coordinates are the same.
    return x() == other.x() && y() == other.y();
}

bool Point::isExactlyEqual(const Point& other) const
{
    return this==&other && mUserID == other.mUserID &&
           mIsPort == other.mIsPort &&
           mDwellTime == other.mDwellTime;
}

void Point::serialize(std::ostream& out) const
{
    if (!out) {
        throw std::runtime_error("Output stream is not ready for writing.");
    }

    // Serialize x-coordinate
    std::uint64_t xNet =
        qToBigEndian(std::bit_cast<std::uint64_t>(x().value()));
    out.write(reinterpret_cast<const char*>(&xNet), sizeof(xNet));

    // Serialize y-coordinate
    std::uint64_t yNet =
        qToBigEndian(std::bit_cast<std::uint64_t>(y().value()));
    out.write(reinterpret_cast<const char*>(&yNet), sizeof(yNet));

    // Serialize mUserID as string length followed by string data
    std::string userIDStr = mUserID.toStdString();
    std::uint64_t userIDLen =
        qToBigEndian(static_cast<std::uint64_t>(userIDStr.size()));
    out.write(reinterpret_cast<const char*>(&userIDLen),
              sizeof(userIDLen));
    out.write(userIDStr.c_str(), userIDStr.size());

    // Serialize mIsPort
    out.write(reinterpret_cast<const char*>(&mIsPort),
              sizeof(mIsPort));

    // Serialize mDwellTime
    std::uint64_t dwellTimeNet =
        qToBigEndian(std::bit_cast<std::uint64_t>(mDwellTime.value()));
    out.write(reinterpret_cast<const char*>(&dwellTimeNet),
              sizeof(dwellTimeNet));

    // Check for write failures
    if (!out) {
        throw std::runtime_error("Failed to write point"
                                 " data to output stream.");
    }
}

void Point::deserialize(std::istream& in)
{
    if (!in)
    {
        throw std::runtime_error("Input stream is not ready for reading.");
    }

    // Deserialize x-coordinate
    std::uint64_t xNet;
    in.read(reinterpret_cast<char*>(&xNet), sizeof(xNet));
    if (!in)
    {
        throw std::runtime_error("Failed to read x-coordinate"
                                 " from input stream.");
    }
    mOGRPoint.setX(qFromBigEndian(std::bit_cast<double>(xNet)));

    // Deserialize y-coordinate
    std::uint64_t yNet;
    in.read(reinterpret_cast<char*>(&yNet), sizeof(yNet));
    if (!in) {
        throw std::runtime_error("Failed to read y-coordinate "
                                 "from input stream.");
    }
    mOGRPoint.setY(qFromBigEndian(std::bit_cast<double>(yNet)));

    // Deserialize userID
    std::uint64_t userIDLen;
    in.read(reinterpret_cast<char*>(&userIDLen), sizeof(userIDLen));
    userIDLen = qFromBigEndian(userIDLen);
    std::string userIDStr(userIDLen, '\0');
    in.read(&userIDStr[0], userIDLen);
    if (!in)
    {
        throw std::runtime_error("Failed to read userID "
                                 "from input stream.");
    }
    mUserID = QString::fromStdString(userIDStr);

    // Deserialize mIsPort
    in.read(reinterpret_cast<char*>(&mIsPort), sizeof(mIsPort));
    if (!in)
    {
        throw std::runtime_error("Failed to read mIsPort "
                                 "flag from input stream.");
    }

    // Deserialize mDwellTime
    std::uint64_t dwellTimeNet;
    in.read(reinterpret_cast<char*>(&dwellTimeNet),
            sizeof(dwellTimeNet));
    if (!in) {
        throw std::runtime_error("Failed to read dwell "
                                 "time from input stream.");
    }
    mDwellTime = units::time::second_t(
        qFromBigEndian(std::bit_cast<double>(dwellTimeNet)));
}

// Implementation of the nested Hash structure
std::size_t Point::Hash::operator()(const std::shared_ptr<Point>& p) const
{
    if (!p) return 0; // Handle null pointers

    auto x_value = p->x().value();
    auto y_value = p->y().value();

    return std::hash<decltype(x_value)>()(x_value) ^
           std::hash<decltype(y_value)>()(y_value);
}

// Implementation of the nested Equal structure
bool Point::Equal::operator()(const std::shared_ptr<Point>& lhs,
                              const std::shared_ptr<Point>& rhs) const
{
    if (!lhs || !rhs) return false; // Handle null pointers
    return *lhs == *rhs;
}

std::ostream& operator<<(std::ostream& os, const Point& point)
{
    return os << point.toString().toStdString();
}
};

// Hash function for the Point class.
std::size_t
std::hash<ShipNetSimCore::Point>::operator()(const ShipNetSimCore::Point &p)
    const
{
    // Calculate the hash value using the x and y coordinates.
    return std::hash<long double>()(p.x().value()) ^
           std::hash<long double>()(p.y().value());
}
