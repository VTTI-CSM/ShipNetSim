/**
 * @file gpoint.cpp
 * @brief Implementation of the GPoint class.
 *
 * This file implements geodetic point operations using GeographicLib for
 * accurate calculations on the WGS84 ellipsoid. All geodetic calculations
 * use the WGS84 ellipsoid exclusively, which is the international standard
 * for maritime navigation, GPS, ECDIS, and AIS systems.
 *
 * @note This implementation enforces WGS84 for all geodetic operations.
 *       Input data in other coordinate reference systems should be
 *       converted to WGS84 before use.
 *
 * @author Ahmed Aredah
 * @date 10.12.2023
 */

#include "gpoint.h"
#include "point.h"
#include "polygon.h"
#include "qendian.h"
#include "../utils/gdal_compat.h"
#include "../utils/utils.h"
#include <GeographicLib/Geodesic.hpp>
#include <ogr_geometry.h>
#include <ogr_spatialref.h>

namespace ShipNetSimCore
{

// =============================================================================
// Anonymous Namespace - Internal Helper Functions
// =============================================================================

namespace
{

/**
 * @brief Validate that both spatial references are set and compatible.
 *
 * @param thisSR Spatial reference of the first point
 * @param otherSR Spatial reference of the second point
 * @throws std::runtime_error if either SR is null or they don't match
 */
void validateSpatialReferences(const OGRSpatialReference *thisSR,
                               const OGRSpatialReference *otherSR)
{
    if (thisSR == nullptr || otherSR == nullptr)
    {
        throw std::runtime_error(
            "Spatial reference not set for one or both points.");
    }

    if (!thisSR->IsSame(otherSR))
    {
        throw std::runtime_error("Mismatch geodetic datums!");
    }
}

/**
 * @brief Get the WGS84 Geodesic calculator (cached static instance).
 *
 * WGS84 is the international standard for maritime navigation, GPS,
 * ECDIS, and AIS systems. Using the cached static instance avoids
 * object construction overhead on every geodetic calculation.
 *
 * @return Reference to the cached WGS84 Geodesic calculator
 */
inline const GeographicLib::Geodesic& wgs84Geodesic()
{
    return GeographicLib::Geodesic::WGS84();
}

/**
 * @brief Normalize latitude to the range [-90, 90].
 *
 * If latitude exceeds the valid range, it "flips" back into range.
 * For example, 100 degrees becomes 80 degrees (180 - 100).
 *
 * @param lat Raw latitude value in degrees
 * @return Normalized latitude in [-90, 90]
 */
double normalizeLatitude(double lat)
{
    while (lat > 90.0 || lat < -90.0)
    {
        if (lat > 90.0)
        {
            lat = 180.0 - lat;
        }
        else if (lat < -90.0)
        {
            lat = -180.0 - lat;
        }
    }
    return lat;
}

/**
 * @brief Assign spatial reference to a point from a CRC or use default.
 *
 * @param point The OGRPoint to assign the SR to
 * @param crc The optional custom spatial reference
 *
 * @note For performance, when using the default WGS84 reference, we assign
 *       the shared static reference directly rather than cloning it.
 *       OGRSpatialReference::Clone() is extremely expensive (involves PROJ
 *       library operations), and during pathfinding millions of temporary
 *       GPoints may be created. The shared static reference has a longer
 *       lifetime than any GPoint, so this is safe.
 */
void assignSpatialReference(OGRPoint &point, const OGRSpatialReference &crc)
{
    if (!crc.IsEmpty())
    {
        // Custom spatial reference - must clone for ownership
        if (!crc.IsGeographic())
        {
            throw std::runtime_error("Spatial reference must be geodetic!");
        }
        OGRSpatialReference *sr = crc.Clone();
        point.assignSpatialReference(sr);
    }
    else
    {
        // Default WGS84 - increment ref count before passing to GDAL
        // GDAL's assignSpatialReference(OGRSpatialReference*) takes ownership,
        // but OGRSpatialReference uses reference counting. By calling Reference()
        // first, we ensure GDAL's eventual Release() won't destroy the object
        // since the shared_ptr still holds a reference.
        auto defaultSR = GPoint::getDefaultReprojectionReference();
        defaultSR->Reference();  // Increment ref count
        point.assignSpatialReference(defaultSR.get());
    }
}

}  // anonymous namespace

// =============================================================================
// Static Member Initialization
// =============================================================================

std::shared_ptr<OGRSpatialReference> GPoint::spatialRef = nullptr;

// =============================================================================
// Constructors
// =============================================================================

GPoint::GPoint()
    : mOGRPoint(0.0, 0.0)
    , mIsPort(false)
    , mDwellTime(units::time::second_t(0.0))
{
}

GPoint::GPoint(units::angle::degree_t lon, units::angle::degree_t lat,
               OGRSpatialReference crc)
    : mIsPort(false)
    , mDwellTime(units::time::second_t(0.0))
{
    setLatitude(lat);
    setLongitude(lon);
    assignSpatialReference(mOGRPoint, crc);
}

GPoint::GPoint(units::angle::degree_t lon, units::angle::degree_t lat,
               QString ID, OGRSpatialReference crc)
    : mUserID(ID)
    , mIsPort(false)
    , mDwellTime(units::time::second_t(0.0))
{
    setLatitude(lat);
    setLongitude(lon);
    assignSpatialReference(mOGRPoint, crc);
}

// =============================================================================
// Static Methods - Spatial Reference Management
// =============================================================================

std::shared_ptr<OGRSpatialReference>
GPoint::getDefaultReprojectionReference()
{
    if (!spatialRef)
    {
        spatialRef       = std::make_shared<OGRSpatialReference>();
        OGRErr err       = spatialRef->SetWellKnownGeogCS("WGS84");
        if (err != OGRERR_NONE)
        {
            throw std::runtime_error("Failed to set WGS84 spatial reference");
        }
    }
    return spatialRef;
}

void GPoint::setDefaultReprojectionReference(std::string wellknownCS)
{
    if (!spatialRef)
    {
        spatialRef = std::make_shared<OGRSpatialReference>();
    }

    // Validate the new reference before assigning
    auto tempRef = std::make_shared<OGRSpatialReference>();
    OGRErr err   = tempRef->SetWellKnownGeogCS(wellknownCS.c_str());

    if (err != OGRERR_NONE)
    {
        throw std::runtime_error(
            "Failed to interpret the provided spatial reference: "
            + wellknownCS);
    }

    if (!tempRef->IsGeographic())
    {
        throw std::runtime_error(
            "The provided spatial reference is not geodetic: " + wellknownCS);
    }

    spatialRef = tempRef;
}

// =============================================================================
// Accessors - Coordinate Getters
// =============================================================================

units::angle::degree_t GPoint::getLatitude() const
{
    return units::angle::degree_t(mOGRPoint.getY());
}

units::angle::degree_t GPoint::getLongitude() const
{
    return units::angle::degree_t(mOGRPoint.getX());
}

QString GPoint::getUserID() const
{
    return mUserID;
}

OGRPoint GPoint::getGDALPoint() const
{
    return mOGRPoint;
}

// =============================================================================
// Mutators - Coordinate Setters
// =============================================================================

void GPoint::setLatitude(units::angle::degree_t lat)
{
    mOGRPoint.setY(normalizeLatitude(lat.value()));
}

void GPoint::setLongitude(units::angle::degree_t lon)
{
    mOGRPoint.setX(AngleUtils::normalizeLongitude(lon.value()));
}

// =============================================================================
// Geodetic Calculations
// =============================================================================

units::length::meter_t GPoint::distance(const GPoint &other) const
{
    validateSpatialReferences(mOGRPoint.getSpatialReference(),
                              other.mOGRPoint.getSpatialReference());

    const auto& geod = wgs84Geodesic();

    double dist;
    geod.Inverse(getLatitude().value(), getLongitude().value(),
                 other.getLatitude().value(), other.getLongitude().value(),
                 dist);

    return units::length::meter_t(dist);
}

units::length::meter_t GPoint::fastDistance(const GPoint &other) const
{
    const auto& geod = wgs84Geodesic();

    double dist;
    geod.Inverse(getLatitude().value(), getLongitude().value(),
                 other.getLatitude().value(), other.getLongitude().value(),
                 dist);

    return units::length::meter_t(dist);
}

units::angle::degree_t GPoint::forwardAzimuth(const GPoint &other) const
{
    validateSpatialReferences(mOGRPoint.getSpatialReference(),
                              other.mOGRPoint.getSpatialReference());

    const auto& geod = wgs84Geodesic();

    double dist, azi1, azi2;
    geod.Inverse(getLatitude().value(), getLongitude().value(),
                 other.getLatitude().value(), other.getLongitude().value(),
                 dist, azi1, azi2);

    return units::angle::degree_t(azi1);
}

units::angle::degree_t GPoint::backwardAzimuth(const GPoint &other) const
{
    validateSpatialReferences(mOGRPoint.getSpatialReference(),
                              other.mOGRPoint.getSpatialReference());

    const auto& geod = wgs84Geodesic();

    double dist, azi1, azi2;
    geod.Inverse(other.getLatitude().value(), other.getLongitude().value(),
                 getLatitude().value(), getLongitude().value(),
                 dist, azi1, azi2);

    return units::angle::degree_t(azi2);
}

GPoint GPoint::pointAtDistanceAndHeading(units::length::meter_t  distance,
                                         units::angle::degree_t heading) const
{
    const auto& geod = wgs84Geodesic();

    double newLat, newLon;
    geod.Direct(getLatitude().value(), getLongitude().value(), heading.value(),
                distance.value(), newLat, newLon);

    return GPoint(units::angle::degree_t(newLon),
                  units::angle::degree_t(newLat));
}

GPoint GPoint::getMiddlePoint(const GPoint &endPoint) const
{
    validateSpatialReferences(mOGRPoint.getSpatialReference(),
                              endPoint.mOGRPoint.getSpatialReference());

    const auto& geod = wgs84Geodesic();

    // Compute distance and initial azimuth in a single call
    double dist, azi1, azi2;
    geod.Inverse(getLatitude().value(), getLongitude().value(),
                 endPoint.getLatitude().value(), endPoint.getLongitude().value(),
                 dist, azi1, azi2);

    // Compute midpoint by traveling half the distance
    double midLat, midLon;
    geod.Direct(getLatitude().value(), getLongitude().value(), azi1,
                dist / 2.0, midLat, midLon);

    return GPoint(units::angle::degree_t(midLon),
                  units::angle::degree_t(midLat));
}

// =============================================================================
// Coordinate Transformations
// =============================================================================

void GPoint::transformDatumTo(OGRSpatialReference *targetSR)
{
    if (!targetSR || !targetSR->IsGeographic())
    {
        throw std::runtime_error("Target spatial reference is not geodetic!");
    }

    const OGRSpatialReference *currentSR = mOGRPoint.getSpatialReference();
    if (currentSR && !currentSR->IsSame(targetSR))
    {
        OGRCoordinateTransformation *coordTransform =
            OGRCreateCoordinateTransformation(currentSR, targetSR);

        if (coordTransform)
        {
            double x = mOGRPoint.getX();
            double y = mOGRPoint.getY();

            if (coordTransform->Transform(1, &x, &y))
            {
                mOGRPoint.setX(x);
                mOGRPoint.setY(y);
                mOGRPoint.assignSpatialReference(targetSR);
            }

            DESTROY_COORD_TRANSFORM(coordTransform);
        }
    }
}

Point GPoint::projectTo(OGRSpatialReference *targetSR) const
{
    if (!targetSR || !targetSR->IsProjected())
    {
        throw std::runtime_error(
            "Target Spatial Reference is not valid or not a projected CRS.");
    }

    const OGRSpatialReference *currentSR = mOGRPoint.getSpatialReference();
    if (currentSR == nullptr)
    {
        throw std::runtime_error("Current Spatial Reference is not set.");
    }

    OGRCoordinateTransformation *coordTransform =
        OGRCreateCoordinateTransformation(currentSR, targetSR);

    if (!coordTransform)
    {
        throw std::runtime_error("Failed to create coordinate transformation.");
    }

    double x = mOGRPoint.getX();
    double y = mOGRPoint.getY();

    if (!coordTransform->Transform(1, &x, &y))
    {
        DESTROY_COORD_TRANSFORM(coordTransform);
        throw std::runtime_error("Failed to transform point coordinates.");
    }

    DESTROY_COORD_TRANSFORM(coordTransform);

    return Point(units::length::meter_t(x), units::length::meter_t(y),
                 mUserID, *targetSR);
}

// =============================================================================
// Port/Waypoint Operations
// =============================================================================

bool GPoint::isPort() const
{
    return mIsPort;
}

units::time::second_t GPoint::getDwellTime() const
{
    return mDwellTime;
}

void GPoint::MarkAsPort(units::time::second_t dwellTime)
{
    mIsPort    = true;
    mDwellTime = dwellTime;
}

void GPoint::MarkAsNonPort()
{
    mIsPort    = false;
    mDwellTime = units::time::second_t(0.0);
}

// =============================================================================
// Polygon Ownership
// =============================================================================

QVector<std::shared_ptr<Polygon>> GPoint::getOwningPolygons() const
{
    return mOwningPolygons;
}

void GPoint::addOwningPolygon(const std::shared_ptr<Polygon>& polygon)
{
    if (polygon)
    {
        mOwningPolygons.append(polygon);
    }
}

void GPoint::clearOwningPolygons()
{
    mOwningPolygons.clear();
}

// =============================================================================
// Visibility Cache
// =============================================================================

bool GPoint::hasVisibleNeighborsCache(const Polygon* polygon) const
{
    return mVisibleNeighborsCache.find(polygon) != mVisibleNeighborsCache.end();
}

QVector<std::shared_ptr<GPoint>>
GPoint::getVisibleNeighborsInPolygon(const Polygon* polygon) const
{
    auto it = mVisibleNeighborsCache.find(polygon);
    if (it != mVisibleNeighborsCache.end())
    {
        return it->second;
    }
    return {};
}

void GPoint::setVisibleNeighborsInPolygon(
    const Polygon* polygon,
    const QVector<std::shared_ptr<GPoint>>& neighbors) const
{
    mVisibleNeighborsCache[polygon] = neighbors;
}

void GPoint::clearVisibleNeighborsCache() const
{
    mVisibleNeighborsCache.clear();
}

// =============================================================================
// Serialization
// =============================================================================

void GPoint::serialize(std::ostream &out) const
{
    if (!out)
    {
        throw std::runtime_error("Output stream is not ready for writing.");
    }

    // Serialize longitude (x-coordinate)
    std::uint64_t xNet = qToBigEndian(
        std::bit_cast<std::uint64_t>(getLongitude().value()));
    out.write(reinterpret_cast<const char *>(&xNet), sizeof(xNet));

    // Serialize latitude (y-coordinate)
    std::uint64_t yNet = qToBigEndian(
        std::bit_cast<std::uint64_t>(getLatitude().value()));
    out.write(reinterpret_cast<const char *>(&yNet), sizeof(yNet));

    // Serialize user ID (length-prefixed string)
    std::string   userIDStr = mUserID.toStdString();
    std::uint64_t userIDLen =
        qToBigEndian(static_cast<std::uint64_t>(userIDStr.size()));
    out.write(reinterpret_cast<const char *>(&userIDLen), sizeof(userIDLen));
    out.write(userIDStr.c_str(), userIDStr.size());

    // Serialize port flag
    out.write(reinterpret_cast<const char *>(&mIsPort), sizeof(mIsPort));

    // Serialize dwell time
    std::uint64_t dwellTimeNet =
        qToBigEndian(std::bit_cast<std::uint64_t>(mDwellTime.value()));
    out.write(reinterpret_cast<const char *>(&dwellTimeNet),
              sizeof(dwellTimeNet));

    if (!out)
    {
        throw std::runtime_error("Failed to write point data to output stream.");
    }
}

void GPoint::deserialize(std::istream &in)
{
    if (!in)
    {
        throw std::runtime_error("Input stream is not ready for reading.");
    }

    // Deserialize longitude (x-coordinate)
    std::uint64_t xNet;
    in.read(reinterpret_cast<char *>(&xNet), sizeof(xNet));
    if (!in)
    {
        throw std::runtime_error("Failed to read x-coordinate from input stream.");
    }
    mOGRPoint.setX(qFromBigEndian(std::bit_cast<double>(xNet)));

    // Deserialize latitude (y-coordinate)
    std::uint64_t yNet;
    in.read(reinterpret_cast<char *>(&yNet), sizeof(yNet));
    if (!in)
    {
        throw std::runtime_error("Failed to read y-coordinate from input stream.");
    }
    mOGRPoint.setY(qFromBigEndian(std::bit_cast<double>(yNet)));

    // Deserialize user ID
    std::uint64_t userIDLen;
    in.read(reinterpret_cast<char *>(&userIDLen), sizeof(userIDLen));
    userIDLen = qFromBigEndian(userIDLen);
    std::string userIDStr(userIDLen, '\0');
    in.read(&userIDStr[0], userIDLen);
    if (!in)
    {
        throw std::runtime_error("Failed to read userID from input stream.");
    }
    mUserID = QString::fromStdString(userIDStr);

    // Deserialize port flag
    in.read(reinterpret_cast<char *>(&mIsPort), sizeof(mIsPort));
    if (!in)
    {
        throw std::runtime_error("Failed to read mIsPort flag from input stream.");
    }

    // Deserialize dwell time
    std::uint64_t dwellTimeNet;
    in.read(reinterpret_cast<char *>(&dwellTimeNet), sizeof(dwellTimeNet));
    if (!in)
    {
        throw std::runtime_error("Failed to read dwell time from input stream.");
    }
    mDwellTime = units::time::second_t(
        qFromBigEndian(std::bit_cast<double>(dwellTimeNet)));

    // Restore default spatial reference (critical for subsequent operations)
    // Use Reference() to increment ref count before GDAL takes "ownership"
    auto defaultSR = getDefaultReprojectionReference();
    if (defaultSR)
    {
        defaultSR->Reference();  // Increment ref count
        mOGRPoint.assignSpatialReference(defaultSR.get());
    }
}

// =============================================================================
// String Representation
// =============================================================================

QString GPoint::toString(const QString &format, int decimalPercision) const
{
    QString xStr =
        QString::number(getLongitude().value(), 'f', decimalPercision);
    QString yStr =
        QString::number(getLatitude().value(), 'f', decimalPercision);
    QString idStr = mUserID.isEmpty() ? "N/A" : mUserID;

    QString result = format;
    result.replace("%x", xStr, Qt::CaseInsensitive);
    result.replace("%y", yStr, Qt::CaseInsensitive);
    result.replace("%id", idStr, Qt::CaseInsensitive);

    return result;
}

// =============================================================================
// Operators
// =============================================================================

bool GPoint::operator==(const GPoint &other) const
{
    return getLatitude() == other.getLatitude()
           && getLongitude() == other.getLongitude();
}

GPoint GPoint::operator+(const GPoint &other) const
{
    GPoint result;
    result.setLatitude(getLatitude() + other.getLatitude());
    result.setLongitude(getLongitude() + other.getLongitude());
    return result;
}

GPoint GPoint::operator-(const GPoint &other) const
{
    GPoint result;
    result.setLatitude(getLatitude() - other.getLatitude());
    result.setLongitude(getLongitude() - other.getLongitude());
    return result;
}

std::ostream &operator<<(std::ostream &os, const GPoint &point)
{
    os << "Point(ID: " << point.mUserID.toStdString()
       << ", Lat: " << point.getLatitude().value()
       << ", Lon: " << point.getLongitude().value() << ")";
    return os;
}

// =============================================================================
// Hash and Equality Functors
// =============================================================================

std::size_t GPoint::Hash::operator()(const std::shared_ptr<GPoint> &p) const
{
    if (!p)
    {
        return 0;
    }

    auto lat = p->getLatitude().value();
    auto lon = p->getLongitude().value();

    auto h1 = std::hash<decltype(lat)>()(lat);
    auto h2 = std::hash<decltype(lon)>()(lon);
    // Boost-style hash combine to avoid clustering from simple XOR
    return h1 ^ (h2 * 0x9e3779b97f4a7c15ULL +
                 0x9e3779b97f4a7c15ULL + (h1 << 6) + (h1 >> 2));
}

bool GPoint::Equal::operator()(const std::shared_ptr<GPoint> &lhs,
                               const std::shared_ptr<GPoint> &rhs) const
{
    if (!lhs || !rhs)
    {
        return false;
    }
    return *lhs == *rhs;
}

}  // namespace ShipNetSimCore

// =============================================================================
// std::hash Specialization
// =============================================================================

std::size_t
std::hash<ShipNetSimCore::GPoint>::operator()(
    const ShipNetSimCore::GPoint &p) const
{
    auto lat = p.getLatitude().value();
    auto lon = p.getLongitude().value();

    auto h1 = std::hash<decltype(lat)>()(lat);
    auto h2 = std::hash<decltype(lon)>()(lon);
    // Boost-style hash combine to avoid clustering from simple XOR
    return h1 ^ (h2 * 0x9e3779b97f4a7c15ULL +
                 0x9e3779b97f4a7c15ULL + (h1 << 6) + (h1 >> 2));
}
