#include "gpoint.h"
#include "point.h"
#include "qendian.h"
#include <GeographicLib/Geodesic.hpp>
#include <ogr_geometry.h>
#include <ogr_spatialref.h>

namespace ShipNetSimCore
{
std::shared_ptr<OGRSpatialReference> GPoint::spatialRef = nullptr;

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
    // Initialize mOGRPoint with longitude and latitude
    setLatitude(lat);
    setLongitude(lon);

    OGRSpatialReference *SR = nullptr;
    if (!crc.IsEmpty())
    {
        if (!crc.IsGeographic())
        {
            throw std::runtime_error(
                "Spatial reference must be geodetic!");
        }
        // Clone the passed spatial reference and assign
        // it to spatialRef pointer
        SR = crc.Clone();
    }
    else
    {
        // If no spatial reference is passed, create a
        // new spatial reference for WGS84
        SR = getDefaultReprojectionReference()->Clone();
    }

    // Assign the spatial reference to mOGRPoint.
    // mOGRPoint does not take ownership of the spatialRef.
    mOGRPoint.assignSpatialReference(SR);
}

// Constructor with optional CRS, defaults to an empty spatial
// reference
GPoint::GPoint(units::angle::degree_t lon, units::angle::degree_t lat,
               QString ID, OGRSpatialReference crc)
    : mUserID(ID)
    , mIsPort(false)
    , mDwellTime(units::time::second_t(0.0))
{
    // Initialize mOGRPoint with longitude and latitude
    setLatitude(lat);
    setLongitude(lon);

    OGRSpatialReference *SR = nullptr;
    if (!crc.IsEmpty())
    {
        if (!crc.IsGeographic())
        {
            throw std::runtime_error(
                "Spatial reference must be geodetic!");
        }
        // Clone the passed spatial reference and assign
        // it to spatialRef pointer
        SR = crc.Clone();
    }
    else
    {
        // If no spatial reference is passed, create a
        // new spatial reference for WGS84
        SR = getDefaultReprojectionReference()->Clone();
    }

    // Assign the spatial reference to mOGRPoint.
    // mOGRPoint does not take ownership of the spatialRef.
    mOGRPoint.assignSpatialReference(SR);
}

std::shared_ptr<OGRSpatialReference>
GPoint::getDefaultReprojectionReference()
{
    // Initialize spatialRef with WGS84 if it hasn't been initialized
    // yet
    if (!GPoint::spatialRef)
    {
        GPoint::spatialRef = std::make_shared<OGRSpatialReference>();
        OGRErr err         = spatialRef->SetWellKnownGeogCS("WGS84");
        if (err != OGRERR_NONE)
        {
            throw std::runtime_error(
                "Failed to set WGS84 spatial reference");
        }
    }
    return GPoint::spatialRef;
}

void GPoint::setDefaultReprojectionReference(std::string wellknownCS)
{
    if (!spatialRef)
    {
        GPoint::spatialRef = std::make_shared<OGRSpatialReference>();
    }

    // Temporary spatial reference for validation
    std::shared_ptr<OGRSpatialReference> tempRef =
        std::make_shared<OGRSpatialReference>();
    OGRErr err = tempRef->SetWellKnownGeogCS(wellknownCS.c_str());
    if (err != OGRERR_NONE)
    {
        // Exit the function on failure
        throw std::runtime_error(
            "Failed to interpret the provided spatial reference: "
            + wellknownCS);
    }

    // Check if the spatial reference is geodetic
    if (!tempRef->IsGeographic())
    {
        // Exit the function if not geodetic
        throw std::runtime_error(
            "The provided spatial reference is not geodetic: "
            + wellknownCS);
    }

    // If validation passed, assign the validated spatial
    // reference to the class's static member
    GPoint::spatialRef = tempRef;
}

OGRPoint GPoint::getGDALPoint() const
{
    return mOGRPoint;
}

// Project this point to a given projected CRS
Point GPoint::projectTo(OGRSpatialReference *targetSR) const
{
    // Ensure the target Spatial Reference is valid and is a projected
    // CRS
    if (!targetSR || !targetSR->IsProjected())
    {
        throw std::runtime_error(
            "Target Spatial Reference "
            "is not valid or not a projected CRS.");
    }

    const OGRSpatialReference *currentSR =
        mOGRPoint.getSpatialReference();
    if (currentSR == nullptr)
    {
        throw std::runtime_error(
            "Current Spatial Reference is not set.");
    }

    // Create a coordinate transformation from the current
    // geographic CRS to the target projected CRS
    OGRCoordinateTransformation *coordTransform =
        OGRCreateCoordinateTransformation(currentSR, targetSR);
    if (!coordTransform)
    {
        throw std::runtime_error(
            "Failed to create coordinate transformation.");
    }

    double x = mOGRPoint.getX();
    double y = mOGRPoint.getY();

    // Transform the point's coordinates from geographic to projected
    // CRS
    if (!coordTransform->Transform(1, &x, &y))
    {
        OCTDestroyCoordinateTransformation(coordTransform);
        throw std::runtime_error(
            "Failed to transform point coordinates.");
    }

    OCTDestroyCoordinateTransformation(coordTransform);

    // Create and return the transformed point.
    return Point(units::length::meter_t(x), units::length::meter_t(y),
                 mUserID, *targetSR);
}

QString GPoint::getUserID() const
{
    return mUserID;
}

GPoint GPoint::pointAtDistanceAndHeading(
    units::length::meter_t distance,
    units::angle::degree_t heading) const
{
    // Ensure the spatial reference is set for both points
    const OGRSpatialReference *thisSR =
        mOGRPoint.getSpatialReference();

    double semiMajorAxis = thisSR->GetSemiMajor();
    double flattening    = 1.0 / thisSR->GetInvFlattening();

    // Create a GeographicLib::Geodesic object with the ellipsoid
    // parameters
    const GeographicLib::Geodesic geod(semiMajorAxis, flattening);

    double newLat, newLon;
    geod.Direct(this->getLatitude().value(),
                this->getLongitude().value(), heading.value(),
                distance.value(), newLat, newLon);

    // Create a new GPoint with the calculated latitude and longitude.
    // Assuming the spatial reference (SR) of the new point is the
    // same as the current point.
    const OGRSpatialReference *currentSR =
        mOGRPoint.getSpatialReference();
    OGRSpatialReference *newSR = nullptr;
    if (currentSR != nullptr)
    {
        newSR = currentSR->Clone();
    }

    return GPoint(units::angle::degree_t(newLon),
                  units::angle::degree_t(newLat), "NewPoint", *newSR);
}

void GPoint::transformDatumTo(OGRSpatialReference *targetSR)
{
    if (targetSR && targetSR->IsGeographic())
    {
        const OGRSpatialReference *currentSR =
            mOGRPoint.getSpatialReference();
        if (currentSR && !currentSR->IsSame(targetSR))
        {
            OGRCoordinateTransformation *coordTransform =
                OGRCreateCoordinateTransformation(currentSR,
                                                  targetSR);
            if (coordTransform)
            {
                double x = mOGRPoint.getX(), y = mOGRPoint.getY();
                if (coordTransform->Transform(1, &x, &y))
                {
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
        throw std::runtime_error(
            "Target spatial reference is not geodetic!");
    }
}

units::length::meter_t GPoint::distance(const GPoint &other) const
{
    // Ensure the spatial reference is set for both points
    const OGRSpatialReference *thisSR =
        mOGRPoint.getSpatialReference();
    const OGRSpatialReference *otherSR =
        other.mOGRPoint.getSpatialReference();

    if (thisSR == nullptr || otherSR == nullptr)
    {
        throw std::runtime_error("Spatial reference not "
                                 "set for one or both points.");
    }

    if (!thisSR->IsSame(otherSR))
    {
        throw std::runtime_error("Mismatch geodetic datums!");
    }

    double semiMajorAxis = thisSR->GetSemiMajor();
    double flattening    = 1.0 / thisSR->GetInvFlattening();

    // Create a GeographicLib::Geodesic object with the ellipsoid
    // parameters
    const GeographicLib::Geodesic geod(semiMajorAxis, flattening);

    double t = 0.0, distance = 0.0;
    // Calculate the distance
    geod.GenInverse(
        this->getLatitude().value(), this->getLongitude().value(),
        other.getLatitude().value(), other.getLongitude().value(),
        GeographicLib::Geodesic::mask::DISTANCE, distance, t, t, t, t,
        t, t);

    return units::length::meter_t(distance);
}

units::angle::degree_t
GPoint::forwardAzimuth(const GPoint &other) const
{
    // Ensure the spatial reference is set for both points
    const OGRSpatialReference *thisSR =
        mOGRPoint.getSpatialReference();
    const OGRSpatialReference *otherSR =
        other.mOGRPoint.getSpatialReference();

    if (thisSR == nullptr || otherSR == nullptr)
    {
        throw std::runtime_error("Spatial reference not "
                                 "set for one or both points.");
    }

    if (!thisSR->IsSame(otherSR))
    {
        throw std::runtime_error("Mismatch geodetic datums!");
    }

    double semiMajorAxis = thisSR->GetSemiMajor();
    double flattening    = 1.0 / thisSR->GetInvFlattening();

    // Create a GeographicLib::Geodesic object with the ellipsoid
    // parameters
    const GeographicLib::Geodesic geod(semiMajorAxis, flattening);

    double t = 0.0, azimuth = 0.0;

    // Calculate the azimuth
    geod.GenInverse(
        this->getLatitude().value(), this->getLongitude().value(),
        other.getLatitude().value(), other.getLongitude().value(),
        GeographicLib::Geodesic::mask::AZIMUTH, t, azimuth, t, t, t,
        t, t);

    return units::angle::degree_t(azimuth);
}

units::angle::degree_t
GPoint::backwardAzimuth(const GPoint &other) const
{
    // Ensure the spatial reference is set for both points
    const OGRSpatialReference *thisSR =
        mOGRPoint.getSpatialReference();
    const OGRSpatialReference *otherSR =
        other.mOGRPoint.getSpatialReference();

    if (thisSR == nullptr || otherSR == nullptr)
    {
        throw std::runtime_error("Spatial reference not "
                                 "set for one or both points.");
    }

    if (!thisSR->IsSame(otherSR))
    {
        throw std::runtime_error("Mismatch geodetic datums!");
    }

    double semiMajorAxis = thisSR->GetSemiMajor();
    double flattening    = 1.0 / thisSR->GetInvFlattening();

    // Create a GeographicLib::Geodesic object with the ellipsoid
    // parameters
    const GeographicLib::Geodesic geod(semiMajorAxis, flattening);

    double t = 0.0, azimuth = 0.0;
    // Calculate the azimuth
    geod.GenInverse(
        other.getLatitude().value(), other.getLongitude().value(),
        this->getLatitude().value(), this->getLongitude().value(),
        GeographicLib::Geodesic::mask::AZIMUTH, t, t, azimuth, t, t,
        t, t);

    return units::angle::degree_t(azimuth);
}

units::angle::degree_t GPoint::getLatitude() const
{
    return units::angle::degree_t(mOGRPoint.getY());
}

units::angle::degree_t GPoint::getLongitude() const
{
    return units::angle::degree_t(mOGRPoint.getX());
}

bool GPoint::isPort() const
{
    return mIsPort;
}

units::time::second_t GPoint::getDwellTime() const
{
    return mDwellTime;
}

void GPoint::setLatitude(units::angle::degree_t lat)
{
    double normalizedLat = lat.value();

    // Normalize latitude to the range [-90, 90]
    // If latitude goes beyond 90 or -90, it flips direction.
    while (normalizedLat > 90.0 || normalizedLat < -90.0)
    {
        if (normalizedLat > 90.0)
        {
            normalizedLat = 180.0 - normalizedLat;
        }
        else if (normalizedLat < -90.0)
        {
            normalizedLat = -180.0 - normalizedLat;
        }
    }

    mOGRPoint.setY(
        normalizedLat); // Update the internal OGRPoint as well
}

void GPoint::setLongitude(units::angle::degree_t lon)
{
    double normalizedLon = lon.value();

    // Normalize longitude to the range [-180, 180]
    while (normalizedLon > 180.0 || normalizedLon < -180.0)
    {
        if (normalizedLon > 180.0)
        {
            normalizedLon -= 360.0;
        }
        else if (normalizedLon < -180.0)
        {
            normalizedLon += 360.0;
        }
    }

    mOGRPoint.setX(
        normalizedLon); // Update the internal OGRPoint as well
}

void GPoint::MarkAsNonPort()
{
    mIsPort    = false;
    mDwellTime = units::time::second_t(0.0);
}

void GPoint::MarkAsPort(units::time::second_t dwellTime)
{
    mIsPort    = true;
    mDwellTime = dwellTime;
}

// Implementation of getMiddlePoint
GPoint GPoint::getMiddlePoint(const GPoint &endPoint) const
{
    // Ensure the spatial reference is set for both points
    const OGRSpatialReference *thisSR =
        mOGRPoint.getSpatialReference();
    const OGRSpatialReference *otherSR =
        endPoint.mOGRPoint.getSpatialReference();

    if (thisSR == nullptr || otherSR == nullptr)
    {
        throw std::runtime_error(
            "Spatial reference not set for one or both points.");
    }

    if (!thisSR->IsSame(otherSR))
    {
        throw std::runtime_error("Mismatch geodetic datums!");
    }

    // Get ellipsoid parameters from the spatial reference
    double semiMajorAxis = thisSR->GetSemiMajor();
    double flattening    = 1.0 / thisSR->GetInvFlattening();

    // Create a GeographicLib::Geodesic object with the ellipsoid
    // parameters
    const GeographicLib::Geodesic geod(semiMajorAxis, flattening);

    // Compute the geodesic distance and initial azimuth from this to
    // endPoint
    double distance = 0.0, azi1 = 0.0, t = 0.0;
    geod.GenInverse(this->getLatitude().value(),
                    this->getLongitude().value(),
                    endPoint.getLatitude().value(),
                    endPoint.getLongitude().value(),
                    GeographicLib::Geodesic::mask::DISTANCE
                        | GeographicLib::Geodesic::mask::AZIMUTH,
                    distance, azi1, t, t, t, t, t);

    // Compute the midpoint by traveling half the distance along the
    // geodesic
    double midLat = 0.0, midLon = 0.0;
    geod.Direct(this->getLatitude().value(),
                this->getLongitude().value(), azi1, distance / 2.0,
                midLat, midLon);

    // Create a new GPoint with the calculated midpoint coordinates
    OGRSpatialReference *newSR = thisSR->Clone();
    return GPoint(units::angle::degree_t(midLon),
                  units::angle::degree_t(midLat), "Midpoint", *newSR);
}

// Function to convert the point to a string representation.
QString GPoint::toString(const QString &format,
                         int            decimalPercision) const
{
    QString xStr = QString::number(getLongitude().value(), 'f',
                                   decimalPercision);
    QString yStr =
        QString::number(getLatitude().value(), 'f', decimalPercision);
    QString idStr = mUserID.isEmpty() ? "N/A" : mUserID;

    QString result = format;

    // Replace placeholders (case-insensitive)
    result.replace("%x", xStr, Qt::CaseInsensitive);
    result.replace("%y", yStr, Qt::CaseInsensitive);
    result.replace("%id", idStr, Qt::CaseInsensitive);

    return result;
}

// Implementation of the nested Hash structure
std::size_t
GPoint::Hash::operator()(const std::shared_ptr<GPoint> &p) const
{
    if (!p)
        return 0; // Handle null pointers

    auto x_value = p->getLatitude().value();
    auto y_value = p->getLongitude().value();

    return std::hash<decltype(x_value)>()(x_value)
           ^ std::hash<decltype(y_value)>()(y_value);
}

bool GPoint::operator==(const GPoint &other) const
{
    // Return true if both x and y coordinates are the same.
    return getLatitude() == other.getLatitude()
           && getLongitude() == other.getLongitude();
}

GPoint GPoint::operator+(const GPoint &other) const
{
    GPoint result;
    result.setLatitude(this->getLatitude() + other.getLatitude());
    result.setLongitude(this->getLongitude() + other.getLongitude());
    return result;
}

// Overload the - operator for subtracting two GPoint instances
GPoint GPoint::operator-(const GPoint &other) const
{
    GPoint result;
    result.setLatitude(this->getLatitude() - other.getLatitude());
    result.setLongitude(this->getLongitude() - other.getLongitude());
    return result;
}

// Implementation of the nested Equal structure
bool GPoint::Equal::operator()(
    const std::shared_ptr<GPoint> &lhs,
    const std::shared_ptr<GPoint> &rhs) const
{
    if (!lhs || !rhs)
        return false; // Handle null pointers
    return *lhs == *rhs;
}

void GPoint::serialize(std::ostream &out) const
{
    if (!out)
    {
        throw std::runtime_error(
            "Output stream is not ready for writing.");
    }

    // Serialize x-coordinate
    std::uint64_t xNet = qToBigEndian(
        std::bit_cast<std::uint64_t>(getLongitude().value()));
    out.write(reinterpret_cast<const char *>(&xNet), sizeof(xNet));

    // Serialize y-coordinate
    std::uint64_t yNet = qToBigEndian(
        std::bit_cast<std::uint64_t>(getLatitude().value()));
    out.write(reinterpret_cast<const char *>(&yNet), sizeof(yNet));

    // Serialize mUserID as string length followed by string data
    std::string   userIDStr = mUserID.toStdString();
    std::uint64_t userIDLen =
        qToBigEndian(static_cast<std::uint64_t>(userIDStr.size()));
    out.write(reinterpret_cast<const char *>(&userIDLen),
              sizeof(userIDLen));
    out.write(userIDStr.c_str(), userIDStr.size());

    // Serialize mIsPort
    out.write(reinterpret_cast<const char *>(&mIsPort),
              sizeof(mIsPort));

    // Serialize mDwellTime
    std::uint64_t dwellTimeNet = qToBigEndian(
        std::bit_cast<std::uint64_t>(mDwellTime.value()));
    out.write(reinterpret_cast<const char *>(&dwellTimeNet),
              sizeof(dwellTimeNet));

    // Check for write failures
    if (!out)
    {
        throw std::runtime_error("Failed to write point"
                                 " data to output stream.");
    }
}

void GPoint::deserialize(std::istream &in)
{
    if (!in)
    {
        throw std::runtime_error(
            "Input stream is not ready for reading.");
    }

    // Deserialize x-coordinate
    std::uint64_t xNet;
    in.read(reinterpret_cast<char *>(&xNet), sizeof(xNet));
    if (!in)
    {
        throw std::runtime_error("Failed to read x-coordinate"
                                 " from input stream.");
    }
    mOGRPoint.setX(qFromBigEndian(std::bit_cast<double>(xNet)));

    // Deserialize y-coordinate
    std::uint64_t yNet;
    in.read(reinterpret_cast<char *>(&yNet), sizeof(yNet));
    if (!in)
    {
        throw std::runtime_error("Failed to read y-coordinate "
                                 "from input stream.");
    }
    mOGRPoint.setY(qFromBigEndian(std::bit_cast<double>(yNet)));

    // Deserialize userID
    std::uint64_t userIDLen;
    in.read(reinterpret_cast<char *>(&userIDLen), sizeof(userIDLen));
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
    in.read(reinterpret_cast<char *>(&mIsPort), sizeof(mIsPort));
    if (!in)
    {
        throw std::runtime_error("Failed to read mIsPort "
                                 "flag from input stream.");
    }

    // Deserialize mDwellTime
    std::uint64_t dwellTimeNet;
    in.read(reinterpret_cast<char *>(&dwellTimeNet),
            sizeof(dwellTimeNet));
    if (!in)
    {
        throw std::runtime_error("Failed to read dwell "
                                 "time from input stream.");
    }
    mDwellTime = units::time::second_t(
        qFromBigEndian(std::bit_cast<double>(dwellTimeNet)));
}

// Definition of the stream insertion operator
std::ostream &operator<<(std::ostream &os, const GPoint &point)
{
    os << "Point(ID: " << point.mUserID.toStdString()
       << ", Lat: " << point.getLatitude().value()
       << ", Lon: " << point.getLongitude().value() << ")";
    return os;
}
} // namespace ShipNetSimCore
