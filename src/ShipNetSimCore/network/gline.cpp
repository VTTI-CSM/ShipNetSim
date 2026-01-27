/**
 * @file gline.cpp
 * @brief Implementation of the GLine geodetic line segment class.
 *
 * This implementation uses pure geodesic calculations for all geometric
 * operations, ensuring accuracy everywhere on Earth including polar regions.
 * Key algorithms:
 * - GeographicLib for geodesic distance and position calculations
 * - Spherical cross product for orientation determination
 * - Golden section search for minimum distance optimization
 *
 * @author Ahmed Aredah
 * @date 10.12.2023
 */

#include "gline.h"
#include "galgebraicvector.h"

#include <GeographicLib/Geodesic.hpp>
#include <GeographicLib/GeodesicLine.hpp>

#include <cmath>
#include <limits>
#include <utility>

namespace ShipNetSimCore
{

// =============================================================================
// Anonymous Namespace - Internal Helper Functions
// =============================================================================

namespace
{

/**
 * @brief Convert degrees to radians.
 */
constexpr double DEG_TO_RAD = M_PI / 180.0;

/**
 * @brief Tolerance for collinearity detection in spherical cross product.
 */
constexpr double COLLINEARITY_TOLERANCE = 1e-12;

/**
 * @brief Tolerance for golden section search convergence (meters).
 */
constexpr double SEARCH_TOLERANCE_METERS = 1.0;

/**
 * @brief Minimum segment length to avoid degenerate cases (meters).
 */
constexpr double MIN_SEGMENT_LENGTH = 1e-10;

/**
 * @brief Calculate the spherical cross product for three points.
 *
 * Converts geographic coordinates to 3D Cartesian coordinates on a unit
 * sphere, then computes the signed area of the spherical triangle.
 * The sign indicates orientation:
 * - Positive: counter-clockwise (point R is left of line P→Q)
 * - Negative: clockwise (point R is right of line P→Q)
 * - Zero: collinear
 *
 * @param lat1, lon1 First point (P) coordinates in degrees
 * @param lat2, lon2 Second point (Q) coordinates in degrees
 * @param lat3, lon3 Third point (R) coordinates in degrees
 * @return Signed spherical area (positive = CCW, negative = CW)
 */
double computeSphericalCrossProduct(
    double lat1, double lon1,
    double lat2, double lon2,
    double lat3, double lon3)
{
    // Convert to radians
    const double phi1 = lat1 * DEG_TO_RAD;
    const double lambda1 = lon1 * DEG_TO_RAD;
    const double phi2 = lat2 * DEG_TO_RAD;
    const double lambda2 = lon2 * DEG_TO_RAD;
    const double phi3 = lat3 * DEG_TO_RAD;
    const double lambda3 = lon3 * DEG_TO_RAD;

    // Convert to 3D Cartesian coordinates on unit sphere
    const double x1 = std::cos(phi1) * std::cos(lambda1);
    const double y1 = std::cos(phi1) * std::sin(lambda1);
    const double z1 = std::sin(phi1);

    const double x2 = std::cos(phi2) * std::cos(lambda2);
    const double y2 = std::cos(phi2) * std::sin(lambda2);
    const double z2 = std::sin(phi2);

    const double x3 = std::cos(phi3) * std::cos(lambda3);
    const double y3 = std::cos(phi3) * std::sin(lambda3);
    const double z3 = std::sin(phi3);

    // Vectors from P to Q and P to R
    const double vx1 = x2 - x1;
    const double vy1 = y2 - y1;
    const double vz1 = z2 - z1;

    const double vx2 = x3 - x1;
    const double vy2 = y3 - y1;
    const double vz2 = z3 - z1;

    // Cross product gives normal vector to the plane
    const double nx = vy1 * vz2 - vz1 * vy2;
    const double ny = vz1 * vx2 - vx1 * vz2;
    const double nz = vx1 * vy2 - vy1 * vx2;

    // Dot product with P gives signed area
    return nx * x1 + ny * y1 + nz * z1;
}

/**
 * @brief Find the nearest point on a geodesic segment to a target point.
 *
 * Uses golden section search to efficiently find the minimum distance
 * point along the geodesic arc. This algorithm:
 * 1. Creates a geodesic line from segment start using pre-computed azimuth
 * 2. Searches along the arc using golden section optimization
 * 3. Compares with endpoints to ensure global minimum
 *
 * Works correctly for all latitudes including polar regions.
 *
 * @param segStart Start point of the geodesic segment
 * @param segEnd End point of the geodesic segment
 * @param targetPoint Point to find distance from
 * @param segmentLength Pre-computed length of segment (optimization)
 * @param forwardAzimuth Pre-computed forward azimuth from start to end (optimization)
 * @return Pair of (nearest point on segment, distance to target)
 */
std::pair<GPoint, units::length::meter_t> findNearestPointOnGeodesicSegment(
    const GPoint& segStart,
    const GPoint& segEnd,
    const GPoint& targetPoint,
    units::length::meter_t segmentLength,
    units::angle::degree_t forwardAzimuth)
{
    const GeographicLib::Geodesic& geod = GeographicLib::Geodesic::WGS84();

    // Extract coordinates
    const double lat1 = segStart.getLatitude().value();
    const double lon1 = segStart.getLongitude().value();
    const double latP = targetPoint.getLatitude().value();
    const double lonP = targetPoint.getLongitude().value();

    const double totalDist = segmentLength.value();

    // Handle degenerate case: zero-length segment
    if (totalDist < MIN_SEGMENT_LENGTH)
    {
        const units::length::meter_t dist = targetPoint.distance(segStart);
        return std::make_pair(segStart, dist);
    }

    // Create geodesic line from start toward end using pre-computed azimuth
    // This avoids a redundant geod.Inverse() call since caller already has the azimuth
    GeographicLib::GeodesicLine geodLine = geod.Line(lat1, lon1, forwardAzimuth.value());

    // Lambda to compute distance from a point along the geodesic to target
    auto distanceAt = [&](double s) -> double {
        double lat, lon;
        geodLine.Position(s, lat, lon);
        double dist;
        geod.Inverse(lat, lon, latP, lonP, dist);
        return dist;
    };

    // Golden section search for minimum distance
    const double phi = (1.0 + std::sqrt(5.0)) / 2.0;
    const double resphi = 2.0 - phi;

    double a = 0.0;
    double b = totalDist;
    double x1 = a + resphi * (b - a);
    double x2 = b - resphi * (b - a);
    double f1 = distanceAt(x1);
    double f2 = distanceAt(x2);

    // Iterate until convergence
    while (std::abs(b - a) > SEARCH_TOLERANCE_METERS)
    {
        if (f1 < f2)
        {
            b = x2;
            x2 = x1;
            f2 = f1;
            x1 = a + resphi * (b - a);
            f1 = distanceAt(x1);
        }
        else
        {
            a = x1;
            x1 = x2;
            f1 = f2;
            x2 = b - resphi * (b - a);
            f2 = distanceAt(x2);
        }
    }

    // Best position from search
    const double searchBestS = (a + b) / 2.0;

    // Compare with endpoints to find global minimum
    const double distStart = distanceAt(0.0);
    const double distEnd = distanceAt(totalDist);
    const double distSearch = distanceAt(searchBestS);

    double finalS;
    double finalDist;

    if (distStart <= distSearch && distStart <= distEnd)
    {
        finalS = 0.0;
        finalDist = distStart;
    }
    else if (distEnd <= distSearch && distEnd <= distStart)
    {
        finalS = totalDist;
        finalDist = distEnd;
    }
    else
    {
        finalS = searchBestS;
        finalDist = distSearch;
    }

    // Compute coordinates of the nearest point
    double nearestLat, nearestLon;
    geodLine.Position(finalS, nearestLat, nearestLon);

    GPoint nearestPoint{
        units::angle::degree_t(nearestLon),
        units::angle::degree_t(nearestLat)
    };

    return std::make_pair(nearestPoint, units::length::meter_t(finalDist));
}

/**
 * @brief Recalculate all derived properties of a line from its endpoints.
 *
 * Updates length, forward azimuth, and backward azimuth using optimized
 * geodesic calculations. This implementation uses only 2 GeographicLib
 * Inverse calls instead of the naive 3 calls, providing ~33% reduction
 * in geodesic computation overhead.
 *
 * @param start Start point
 * @param end End point
 * @param[out] length Geodesic length
 * @param[out] forwardAzimuth Azimuth from start to end
 * @param[out] backwardAzimuth Azimuth at start when traveling from end to start
 */
void recalculateLineProperties(
    const GPoint& start,
    const GPoint& end,
    units::length::meter_t& length,
    units::angle::degree_t& forwardAzimuth,
    units::angle::degree_t& backwardAzimuth)
{
    const GeographicLib::Geodesic& geod = GeographicLib::Geodesic::WGS84();

    const double lat1 = start.getLatitude().value();
    const double lon1 = start.getLongitude().value();
    const double lat2 = end.getLatitude().value();
    const double lon2 = end.getLongitude().value();

    // Call 1: Get distance AND forward azimuth in a single Inverse call
    // This replaces two separate calls (distance + forwardAzimuth)
    double dist, fwdAzi, tmpAzi;
    geod.Inverse(lat1, lon1, lat2, lon2, dist, fwdAzi, tmpAzi);

    // Call 2: Get backward azimuth (azimuth at start when coming from end)
    // We need the azimuth at the second point (start) of the reversed geodesic
    double tmp, bwdAzi;
    geod.Inverse(lat2, lon2, lat1, lon1, tmp, tmp, bwdAzi);

    length = units::length::meter_t(dist);
    forwardAzimuth = units::angle::degree_t(fwdAzi);
    backwardAzimuth = units::angle::degree_t(bwdAzi);
}

}  // anonymous namespace

// =============================================================================
// Constructors and Destructor
// =============================================================================

GLine::GLine()
    : mLength(units::length::meter_t(0.0))
    , mForwardAzimuth(units::angle::degree_t(0.0))
    , mBackwardAzimuth(units::angle::degree_t(0.0))
    , mWidth(units::length::meter_t(0.0))
{
    // Create default points at origin
    start = std::make_shared<GPoint>(
        units::angle::degree_t(0.0),
        units::angle::degree_t(0.0));
    end = std::make_shared<GPoint>(
        units::angle::degree_t(0.0),
        units::angle::degree_t(0.0));

    // Initialize GDAL line
    OGRPoint sp = start->getGDALPoint();
    OGRPoint ep = end->getGDALPoint();
    line.addPoint(&sp);
    line.addPoint(&ep);
}

GLine::GLine(std::shared_ptr<GPoint> startPoint, std::shared_ptr<GPoint> endPoint)
    : start(startPoint)
    , end(endPoint)
    , mWidth(units::length::meter_t(0.0))
{
    // Validate spatial reference compatibility
    if (!start->getGDALPoint().getSpatialReference()->IsSame(
            end->getGDALPoint().getSpatialReference()))
    {
        throw std::runtime_error(
            "Mismatch spatial reference for the two points!");
    }

    // Initialize GDAL line
    OGRPoint sp = start->getGDALPoint();
    OGRPoint ep = end->getGDALPoint();
    line.addPoint(&sp);
    line.addPoint(&ep);

    // Calculate geodesic properties
    recalculateLineProperties(*start, *end, mLength, mForwardAzimuth, mBackwardAzimuth);
}

GLine::~GLine()
{
    // Smart pointers handle cleanup automatically
}

// =============================================================================
// Accessors - Basic Properties
// =============================================================================

OGRLineString GLine::getGDALLine() const
{
    return line;
}

std::shared_ptr<GPoint> GLine::startPoint() const
{
    return start;
}

std::shared_ptr<GPoint> GLine::endPoint() const
{
    return end;
}

units::length::meter_t GLine::length() const
{
    return mLength;
}

units::angle::degree_t GLine::forwardAzimuth() const
{
    return mForwardAzimuth;
}

units::angle::degree_t GLine::backwardAzimuth() const
{
    return mBackwardAzimuth;
}

units::length::meter_t GLine::getTheoriticalWidth() const
{
    return mWidth;
}

// =============================================================================
// Mutators
// =============================================================================

void GLine::setStartPoint(std::shared_ptr<GPoint> sPoint)
{
    start = sPoint;

    OGRPoint sp = start->getGDALPoint();
    line.setPoint(0, &sp);

    recalculateLineProperties(*start, *end, mLength, mForwardAzimuth, mBackwardAzimuth);
}

void GLine::setEndPoint(std::shared_ptr<GPoint> ePoint)
{
    end = ePoint;

    OGRPoint ep = end->getGDALPoint();
    line.setPoint(1, &ep);

    recalculateLineProperties(*start, *end, mLength, mForwardAzimuth, mBackwardAzimuth);
}

void GLine::setTheoriticalWidth(const units::length::meter_t newWidth)
{
    mWidth = newWidth;
}

// =============================================================================
// Geometric Operations - Points on Line
// =============================================================================

GPoint GLine::getPointByDistance(units::length::meter_t distance,
                                 Line::LineEnd from) const
{
    if (distance.value() < 0 || distance > mLength)
    {
        throw std::out_of_range("Distance is out of range of the line length.");
    }

    if (from == Line::LineEnd::Start)
    {
        return start->pointAtDistanceAndHeading(distance, mForwardAzimuth);
    }
    else
    {
        return end->pointAtDistanceAndHeading(distance, mBackwardAzimuth);
    }
}

GPoint GLine::getPointByDistance(units::length::meter_t distance,
                                 std::shared_ptr<GPoint> from) const
{
    if (distance.value() < 0 || distance > mLength)
    {
        throw std::out_of_range("Distance is out of range of the line length.");
    }

    if (*from == *start)
    {
        return start->pointAtDistanceAndHeading(distance, mForwardAzimuth);
    }
    else if (*from == *end)
    {
        return end->pointAtDistanceAndHeading(distance, mBackwardAzimuth);
    }
    else
    {
        throw std::invalid_argument("The specified point is not on the line.");
    }
}

GPoint GLine::midpoint() const
{
    return start->getMiddlePoint(*end);
}

// =============================================================================
// Geometric Operations - Distance Calculations
// =============================================================================

units::length::meter_t
GLine::distanceToPoint(const std::shared_ptr<GPoint> &point) const
{
    auto [nearestPoint, distance] = findNearestPointOnGeodesicSegment(
        *start, *end, *point, mLength, mForwardAzimuth);
    return distance;
}

units::length::meter_t GLine::getPerpendicularDistance(const GPoint &point) const
{
    auto [nearestPoint, distance] = findNearestPointOnGeodesicSegment(
        *start, *end, point, mLength, mForwardAzimuth);
    return distance;
}

// =============================================================================
// Geometric Operations - Orientation and Position
// =============================================================================

Line::Orientation GLine::orientation(std::shared_ptr<GPoint> p,
                                     std::shared_ptr<GPoint> q,
                                     std::shared_ptr<GPoint> r)
{
    const double cross = computeSphericalCrossProduct(
        p->getLatitude().value(), p->getLongitude().value(),
        q->getLatitude().value(), q->getLongitude().value(),
        r->getLatitude().value(), r->getLongitude().value());

    if (std::abs(cross) < COLLINEARITY_TOLERANCE)
    {
        return Line::Collinear;
    }

    // Positive cross = counter-clockwise when viewed from outside sphere
    return (cross > 0) ? Line::CounterClockwise : Line::Clockwise;
}

Line::LocationToLine
GLine::getlocationToLine(const std::shared_ptr<GPoint> &point) const
{
    const double cross = computeSphericalCrossProduct(
        start->getLatitude().value(), start->getLongitude().value(),
        end->getLatitude().value(), end->getLongitude().value(),
        point->getLatitude().value(), point->getLongitude().value());

    if (std::abs(cross) < COLLINEARITY_TOLERANCE)
    {
        return Line::LocationToLine::onLine;
    }

    return (cross > 0) ? Line::LocationToLine::left : Line::LocationToLine::right;
}

// =============================================================================
// Geometric Operations - Line Relationships
// =============================================================================

bool GLine::intersects(GLine &other, bool ignoreEdgePoints) const
{
    if (ignoreEdgePoints)
    {
        auto pointsAreClose = [this](const GPoint &p1, const GPoint &p2) -> bool {
            return p1.distance(p2).value() <= TOLERANCE;
        };

        if (pointsAreClose(*start, *other.start) ||
            pointsAreClose(*start, *other.end) ||
            pointsAreClose(*end, *other.start) ||
            pointsAreClose(*end, *other.end))
        {
            return false;
        }
    }

    OGRLineString otherLine = other.getGDALLine();
    return getGDALLine().Intersects(&otherLine);
}

units::angle::radian_t GLine::smallestAngleWith(GLine &other) const
{
    // Find the common endpoint
    std::shared_ptr<GPoint> commonPoint;

    if (*startPoint() == *other.startPoint() || *startPoint() == *other.endPoint())
    {
        commonPoint = startPoint();
    }
    else if (*endPoint() == *other.startPoint() || *endPoint() == *other.endPoint())
    {
        commonPoint = endPoint();
    }
    else
    {
        throw std::invalid_argument("The lines do not share a common point.");
    }

    // Find the non-common endpoints
    std::shared_ptr<GPoint> thisOther =
        (*startPoint() == *commonPoint) ? endPoint() : startPoint();

    std::shared_ptr<GPoint> otherOther =
        (*other.startPoint() == *commonPoint) ? other.endPoint() : other.startPoint();

    // Compute azimuths from common point to each non-common point
    const double azi1 = commonPoint->forwardAzimuth(*thisOther).value();
    const double azi2 = commonPoint->forwardAzimuth(*otherOther).value();

    // Calculate and normalize angle difference
    double angle = std::fmod(azi2 - azi1 + 360.0, 360.0);
    if (angle > 180.0)
    {
        angle = 360.0 - angle;
    }

    return units::angle::degree_t(angle).convert<units::angle::radian>();
}

// =============================================================================
// Transformations
// =============================================================================

GLine GLine::reverse() const
{
    return GLine(end, start);
}

Line GLine::projectTo(OGRSpatialReference *targetSR) const
{
    if (!targetSR || !targetSR->IsProjected())
    {
        throw std::runtime_error(
            "Target Spatial Reference is not valid or not a projected CRS.");
    }

    auto projectedStart = std::make_shared<Point>(start->projectTo(targetSR));
    auto projectedEnd = std::make_shared<Point>(end->projectTo(targetSR));

    return Line(projectedStart, projectedEnd);
}

GAlgebraicVector GLine::toAlgebraicVector(
    const std::shared_ptr<GPoint> startPoint) const
{
    GPoint vectorStart, vectorEnd;

    if (*startPoint == *start)
    {
        vectorStart = *start;
        vectorEnd = *end;
    }
    else
    {
        vectorStart = *end;
        vectorEnd = *start;
    }

    return GAlgebraicVector(vectorStart, vectorEnd);
}

// =============================================================================
// Operators
// =============================================================================

bool GLine::operator==(const GLine &other) const
{
    return *start == *(other.start) && *end == *(other.end);
}

// =============================================================================
// String Representation
// =============================================================================

QString GLine::toString(const QString &format, int decimalPercision) const
{
    const QString startStr = start
        ? start->toString("(%x, %y)", decimalPercision)
        : "N/A";
    const QString endStr = end
        ? end->toString("(%x, %y)", decimalPercision)
        : "N/A";

    QString result = format;
    result.replace("%start", startStr, Qt::CaseInsensitive);
    result.replace("%end", endStr, Qt::CaseInsensitive);

    return result;
}

// =============================================================================
// Hash and Equality Functors
// =============================================================================

std::size_t GLine::Hash::operator()(const std::shared_ptr<GLine> &line) const
{
    if (!line || !line->startPoint() || !line->endPoint())
    {
        return 0;
    }

    const double startLon = line->startPoint()->getLongitude().value();
    const double startLat = line->startPoint()->getLatitude().value();
    const double endLon = line->endPoint()->getLongitude().value();
    const double endLat = line->endPoint()->getLatitude().value();

    // Hash each point
    const std::size_t hash1 =
        std::hash<double>()(startLon) ^ (std::hash<double>()(startLat) << 1);
    const std::size_t hash2 =
        std::hash<double>()(endLon) ^ (std::hash<double>()(endLat) << 1);

    // XOR is commutative, making this direction-independent
    return hash1 ^ hash2;
}

bool GLine::Equal::operator()(const std::shared_ptr<GLine> &lhs,
                              const std::shared_ptr<GLine> &rhs) const
{
    if (!lhs || !rhs)
    {
        return lhs == rhs;
    }

    if (!lhs->startPoint() || !lhs->endPoint() ||
        !rhs->startPoint() || !rhs->endPoint())
    {
        return false;
    }

    // Direction-independent comparison
    const bool forwardMatch =
        (*lhs->startPoint() == *rhs->startPoint()) &&
        (*lhs->endPoint() == *rhs->endPoint());

    const bool reverseMatch =
        (*lhs->startPoint() == *rhs->endPoint()) &&
        (*lhs->endPoint() == *rhs->startPoint());

    return forwardMatch || reverseMatch;
}

}  // namespace ShipNetSimCore
