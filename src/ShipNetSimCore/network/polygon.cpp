/**
 * @file polygon.cpp
 * @brief Implementation of the Polygon class.
 *
 * This file contains the implementation for the Polygon class, providing
 * geodetic polygon operations including area/perimeter calculations,
 * point-in-polygon testing, line intersection detection, and boundary
 * transformations. All geodetic operations use the WGS84 ellipsoid
 * exclusively, which is the international standard for maritime navigation.
 *
 * @note This implementation enforces WGS84 for all geodetic operations.
 *
 * @author Ahmed Aredah
 * @date 10.12.2023
 */

#include "polygon.h"
#include "../utils/gdal_compat.h"
#include "../utils/utils.h"
#include "qdebug.h"
#include <GeographicLib/Geodesic.hpp>
#include <GeographicLib/PolygonArea.hpp>
#include <QVector>
#include <cmath>
#include <limits>
#include <sstream>

namespace ShipNetSimCore
{

// =============================================================================
// Anonymous Namespace - Internal Helper Functions
// =============================================================================

namespace
{

/**
 * @brief Get the WGS84 Geodesic calculator (cached static instance).
 *
 * WGS84 is the international standard for maritime navigation.
 * Using the cached static instance avoids object construction overhead.
 *
 * @return Reference to the cached WGS84 Geodesic calculator
 */
inline const GeographicLib::Geodesic& wgs84Geodesic()
{
    return GeographicLib::Geodesic::WGS84();
}

/**
 * @brief Create a GPoint from OGR ring coordinates at given index.
 *
 * @param ring The OGR ring to extract coordinates from
 * @param index Point index in the ring
 * @return Shared pointer to new GPoint (using default WGS84)
 */
std::shared_ptr<GPoint> pointFromRing(const OGRLinearRing *ring, int index)
{
    return std::make_shared<GPoint>(units::angle::degree_t(ring->getX(index)),
                                    units::angle::degree_t(ring->getY(index)));
}

/**
 * @brief Check if two line endpoints match within tolerance.
 *
 * @param lon1 First longitude
 * @param lat1 First latitude
 * @param lon2 Second longitude
 * @param lat2 Second latitude
 * @param epsilon Tolerance for comparison
 * @return true if points are within epsilon of each other
 */
bool pointsMatch(double lon1, double lat1, double lon2, double lat2,
                 double epsilon = 1e-9)
{
    return std::abs(lon1 - lon2) < epsilon && std::abs(lat1 - lat2) < epsilon;
}

/**
 * @brief Tolerance for vertex proximity comparisons in meters.
 */
constexpr double VERTEX_TOLERANCE_METERS = 0.1;

/**
 * @brief Number of sample points for segment-through-hole testing.
 */
constexpr int HOLE_SAMPLING_COUNT = 10;

/**
 * @brief Threshold for short segment detection in meters.
 */
constexpr double SHORT_SEGMENT_THRESHOLD_METERS = 1000.0;

/**
 * @brief Threshold for detecting large longitude jumps (antimeridian crossing).
 */
constexpr double LONGITUDE_JUMP_THRESHOLD = 180.0;

/**
 * @brief Tolerance for full-span polygon detection in degrees.
 */
constexpr double FULL_SPAN_TOLERANCE = 2.0;

}  // anonymous namespace

// =============================================================================
// Constructors
// =============================================================================

Polygon::Polygon() {}

Polygon::Polygon(const QVector<std::shared_ptr<GPoint>>          &boundary,
                 const QVector<QVector<std::shared_ptr<GPoint>>> &holes,
                 const QString                                    ID)
    : mOutterBoundary(boundary)
    , mInnerHoles(holes)
    , mUserID(ID)
{
    setOuterPoints(boundary);
    setInnerHolesPoints(holes);
}

// =============================================================================
// Static Helper Methods
// =============================================================================

void Polygon::validateRing(const OGRLinearRing &ring, const QString &description)
{
    // Check for sufficient number of points
    if (ring.getNumPoints() < 3)
    {
        throw std::runtime_error(description.toStdString() +
                                 " is degenerate: requires at "
                                 "least 3 unique points.");
    }

    // If only 3 points are provided, check for collinearity
    if (ring.getNumPoints() == 3)
    {
        const OGRSpatialReference *SR = ring.getSpatialReference();

        GPoint p0 = GPoint(units::angle::degree_t(ring.getX(0)),
                           units::angle::degree_t(ring.getY(0)), *SR);
        GPoint p1 = GPoint(units::angle::degree_t(ring.getX(1)),
                           units::angle::degree_t(ring.getY(1)), *SR);
        GPoint p2 = GPoint(units::angle::degree_t(ring.getX(2)),
                           units::angle::degree_t(ring.getY(2)), *SR);

        if (GLine::orientation(std::make_shared<GPoint>(p0),
                               std::make_shared<GPoint>(p1),
                               std::make_shared<GPoint>(p2)))
        {
            throw std::runtime_error(description.toStdString() +
                                     " is degenerate: points are collinear.");
        }
    }
}

// =============================================================================
// Boundary Accessors and Mutators
// =============================================================================

void Polygon::setOuterPoints(const QVector<std::shared_ptr<GPoint>> &newOuter)
{
    mOutterBoundary = newOuter;

    // Create a ring (outer boundary)
    OGRLinearRing outerRing;

    // Temporarily store interior rings
    std::vector<OGRLinearRing *> tempInteriorRings;
    for (int i = 0; i < mPolygon.getNumInteriorRings(); ++i)
    {
        OGRLinearRing *interiorRing = static_cast<OGRLinearRing *>(
            mPolygon.getInteriorRing(i)->clone());
        tempInteriorRings.push_back(interiorRing);
    }

    // Clear the polygon and update the exterior ring
    mPolygon.empty();

    for (auto &p : newOuter)
    {
        auto pp = p->getGDALPoint();
        outerRing.addPoint(&pp);
    }
    outerRing.closeRings();
    outerRing.assignSpatialReference(
        newOuter[0]->getGDALPoint().getSpatialReference());

    mPolygon.addRing(&outerRing);
    mPolygon.assignSpatialReference(
        newOuter[0]->getGDALPoint().getSpatialReference());

    // Restore interior rings
    for (auto *interiorRing : tempInteriorRings)
    {
        mPolygon.addRingDirectly(interiorRing);  // Takes ownership
    }

    validateRing(outerRing, "Outer boundary");
}

QVector<std::shared_ptr<GPoint>> Polygon::outer() const
{
    return mOutterBoundary;
}

void Polygon::setInnerHolesPoints(
    QVector<QVector<std::shared_ptr<GPoint>>> newInners)
{
    mInnerHoles = newInners;

    // Temporarily store exterior ring
    OGRLinearRing *tempExteriorRing =
        static_cast<OGRLinearRing *>(mPolygon.getExteriorRing()->clone());

    // Clear the polygon and restore exterior ring
    mPolygon.empty();
    mPolygon.addRingDirectly(tempExteriorRing);  // Takes ownership

    // Add new interior rings
    for (const auto &hole : newInners)
    {
        OGRLinearRing holeRing;

        for (auto &p : hole)
        {
            auto pp = p->getGDALPoint();
            holeRing.addPoint(&pp);
        }
        holeRing.closeRings();
        holeRing.assignSpatialReference(
            hole[0]->getGDALPoint().getSpatialReference());

        mPolygon.addRing(&holeRing);
        mPolygon.assignSpatialReference(
            hole[0]->getGDALPoint().getSpatialReference());

        validateRing(holeRing, "Hole");
    }
}

QVector<QVector<std::shared_ptr<GPoint>>> Polygon::inners() const
{
    return mInnerHoles;
}

// =============================================================================
// Antimeridian Handling
// =============================================================================

bool Polygon::crossesAntimeridian() const
{
    // Use cached value if available
    if (mCrossesAntimeridianCache >= 0)
    {
        return mCrossesAntimeridianCache == 1;
    }

    auto *ring = mPolygon.getExteriorRing();
    if (!ring || ring->getNumPoints() < 2)
    {
        mCrossesAntimeridianCache = 0;
        return false;
    }

    // Calculate longitude bounds
    double minLon = ring->getX(0);
    double maxLon = ring->getX(0);
    for (int i = 1; i < ring->getNumPoints(); ++i)
    {
        double lon = ring->getX(i);
        minLon     = std::min(minLon, lon);
        maxLon     = std::max(maxLon, lon);
    }

    // CASE 1: Full-span polygons (approximately -180 to +180)
    // These cover the entire globe and do NOT cross the antimeridian
    bool isFullSpan = (minLon <= -180.0 + FULL_SPAN_TOLERANCE) &&
                      (maxLon >= 180.0 - FULL_SPAN_TOLERANCE);
    if (isFullSpan)
    {
        mCrossesAntimeridianCache = 0;
        return false;
    }

    // CASE 2: Detect antimeridian crossing via edge direction analysis
    // Look at the first edge that has a large longitude jump (> 180 degrees)
    // The direction determines if the polygon wraps around the antimeridian
    int numPoints = ring->getNumPoints();
    for (int i = 0; i < numPoints - 1; ++i)
    {
        double lon1    = ring->getX(i);
        double lon2    = ring->getX(i + 1);
        double lonDiff = std::abs(lon2 - lon1);

        if (lonDiff > LONGITUDE_JUMP_THRESHOLD)
        {
            // Positive -> Negative = crosses antimeridian
            // Negative -> Positive = goes around prime meridian
            if (lon1 > 0.0 && lon2 < 0.0)
            {
                mCrossesAntimeridianCache = 1;
                return true;
            }
            else
            {
                mCrossesAntimeridianCache = 0;
                return false;
            }
        }
    }

    mCrossesAntimeridianCache = 0;
    return false;
}

// =============================================================================
// Point Containment Tests
// =============================================================================

bool Polygon::isPointWithinExteriorRing(const GPoint &pointToCheck) const
{
    auto           r = mPolygon.getExteriorRing();
    const OGRPoint p = pointToCheck.getGDALPoint();

    // Check boundary first, then interior
    if (r->isPointOnRingBoundary(&p, TRUE))
    {
        return true;
    }
    if (r->isPointInRing(&p, TRUE))
    {
        return true;
    }

    return false;
}

bool Polygon::isPointWithinInteriorRings(const GPoint &pointToCheck) const
{
    return findContainingHoleIndex(pointToCheck) >= 0;
}

int Polygon::findContainingHoleIndex(const GPoint &pointToCheck) const
{
    const OGRPoint p = pointToCheck.getGDALPoint();

    for (int i = 0; i < mPolygon.getNumInteriorRings(); ++i)
    {
        auto r = mPolygon.getInteriorRing(i);

        if (r->isPointOnRingBoundary(&p, TRUE))
        {
            return i;
        }
        if (r->isPointInRing(&p, TRUE))
        {
            return i;
        }
    }

    return -1;
}

bool Polygon::isPointWithinPolygon(const GPoint &pointToCheck) const
{
    // Handle antimeridian-crossing polygons specially
    if (crossesAntimeridian())
    {
        double pointLon     = pointToCheck.getLongitude().value();
        double pointLat     = pointToCheck.getLatitude().value();
        double normPointLon = AngleUtils::normalizeLongitude360(pointLon);

        auto *ring = mPolygon.getExteriorRing();
        if (!ring)
        {
            return false;
        }

        // Create normalized ring for testing
        OGRLinearRing normRing;
        for (int i = 0; i < ring->getNumPoints(); ++i)
        {
            double normLon = AngleUtils::normalizeLongitude360(ring->getX(i));
            normRing.addPoint(normLon, ring->getY(i));
        }

        OGRPoint normPoint(normPointLon, pointLat);

        // Lambda to check if point is in any normalized hole
        auto isInNormalizedHole = [this, normPointLon, pointLat]() -> bool {
            OGRPoint np(normPointLon, pointLat);

            for (int h = 0; h < mPolygon.getNumInteriorRings(); ++h)
            {
                auto         *hole = mPolygon.getInteriorRing(h);
                OGRLinearRing normHole;

                for (int i = 0; i < hole->getNumPoints(); ++i)
                {
                    double normLon = AngleUtils::normalizeLongitude360(hole->getX(i));
                    normHole.addPoint(normLon, hole->getY(i));
                }

                if (normHole.isPointOnRingBoundary(&np, TRUE) ||
                    normHole.isPointInRing(&np, TRUE))
                {
                    return true;
                }
            }
            return false;
        };

        // Check boundary
        if (normRing.isPointOnRingBoundary(&normPoint, TRUE))
        {
            return !isInNormalizedHole();
        }

        // Check interior
        if (normRing.isPointInRing(&normPoint, TRUE))
        {
            return !isInNormalizedHole();
        }

        return false;
    }

    // Standard case: polygon doesn't cross antimeridian
    if (isPointWithinInteriorRings(pointToCheck))
    {
        return false;
    }
    return isPointWithinExteriorRing(pointToCheck);
}

bool Polygon::ringsContain(std::shared_ptr<GPoint> point) const
{
    auto           r = mPolygon.getExteriorRing();
    const OGRPoint p = point->getGDALPoint();

    // Check exterior ring boundary
    if (r->isPointOnRingBoundary(&p, TRUE))
    {
        return true;
    }

    // Check interior ring boundaries
    for (int i = 0; i < mPolygon.getNumInteriorRings(); ++i)
    {
        if (mPolygon.getInteriorRing(i)->isPointOnRingBoundary(&p, TRUE))
        {
            return true;
        }
    }

    return false;
}

// =============================================================================
// Geometric Calculations
// =============================================================================

units::area::square_meter_t Polygon::area() const
{
    const auto& geod = wgs84Geodesic();

    // Calculate exterior ring area
    GeographicLib::PolygonArea poly(geod);

    const auto *exteriorRing = mPolygon.getExteriorRing();
    for (int i = 0; i < exteriorRing->getNumPoints(); ++i)
    {
        poly.AddPoint(exteriorRing->getY(i), exteriorRing->getX(i));
    }

    double area, perimeter;
    poly.Compute(false, true, perimeter, area);

    // Subtract hole areas
    const int numInteriorRings = mPolygon.getNumInteriorRings();
    for (int j = 0; j < numInteriorRings; ++j)
    {
        const auto *interiorRing = mPolygon.getInteriorRing(j);
        GeographicLib::PolygonArea holePoly(geod);

        for (int k = 0; k < interiorRing->getNumPoints(); ++k)
        {
            holePoly.AddPoint(interiorRing->getY(k), interiorRing->getX(k));
        }

        double holeArea, holePerimeter;
        holePoly.Compute(false, true, holePerimeter, holeArea);
        area -= holeArea;
    }

    return units::area::square_meter_t(area);
}

units::length::meter_t Polygon::perimeter() const
{
    const auto& geod = wgs84Geodesic();

    GeographicLib::PolygonArea poly(geod);

    const auto *exteriorRing = mPolygon.getExteriorRing();
    for (int i = 0; i < exteriorRing->getNumPoints(); ++i)
    {
        poly.AddPoint(exteriorRing->getY(i), exteriorRing->getX(i));
    }

    double area, perimeter;
    poly.Compute(false, true, perimeter, area);

    return units::length::meter_t(perimeter);
}

units::length::meter_t Polygon::getMaxClearWidth(const GLine &line) const
{
    units::length::meter_t leftClearWidth =
        std::numeric_limits<units::length::meter_t>::max();
    units::length::meter_t rightClearWidth =
        std::numeric_limits<units::length::meter_t>::max();

    auto calculateClearWidths = [&leftClearWidth, &rightClearWidth,
                                 &line](const OGRLinearRing *ring) {
        int                        numPoints = ring->getNumPoints();
        const OGRSpatialReference *SR        = ring->getSpatialReference();

        for (int i = 0; i < numPoints; ++i)
        {
            OGRPoint vertexA, vertexB;
            ring->getPoint(i, &vertexA);
            ring->getPoint((i + 1) % numPoints, &vertexB);

            auto pointA = std::make_shared<GPoint>(
                units::angle::degree_t(vertexA.getX()),
                units::angle::degree_t(vertexA.getY()), *SR);

            auto pointB = std::make_shared<GPoint>(
                units::angle::degree_t(vertexB.getX()),
                units::angle::degree_t(vertexB.getY()), *SR);

            GLine edge(pointA, pointB);

            units::length::meter_t dist1 =
                edge.getPerpendicularDistance(*(line.startPoint()));
            units::length::meter_t dist2 =
                edge.getPerpendicularDistance(*(line.endPoint()));

            Line::LocationToLine isLeft1 = line.getlocationToLine(pointA);
            Line::LocationToLine isLeft2 = line.getlocationToLine(pointB);

            if (isLeft1 == Line::LocationToLine::left)
            {
                leftClearWidth = std::min(leftClearWidth, dist1);
            }
            else if (isLeft1 == Line::LocationToLine::right)
            {
                rightClearWidth = std::min(rightClearWidth, dist1);
            }

            if (isLeft2 == Line::LocationToLine::left)
            {
                leftClearWidth = std::min(leftClearWidth, dist2);
            }
            else if (isLeft2 == Line::LocationToLine::right)
            {
                rightClearWidth = std::min(rightClearWidth, dist2);
            }
        }
    };

    // Calculate for outer boundary
    const OGRLinearRing *outerRing =
        static_cast<const OGRLinearRing *>(mPolygon.getExteriorRing());
    calculateClearWidths(outerRing);

    // Calculate for each hole
    int numInteriorRings = mPolygon.getNumInteriorRings();
    for (int i = 0; i < numInteriorRings; ++i)
    {
        const OGRLinearRing *hole = mPolygon.getInteriorRing(i);
        calculateClearWidths(hole);
    }

    return rightClearWidth + leftClearWidth;
}

// =============================================================================
// Line and Segment Operations
// =============================================================================

bool Polygon::intersects(const std::shared_ptr<GLine> line)
{
    const OGRLineString &gdalLine = line->getGDALLine();

    // Quick check: if no intersection at all, return false
    if (!gdalLine.Intersects(&mPolygon))
    {
        return false;
    }

    // Compute actual intersection geometry
    OGRGeometry *intersection = gdalLine.Intersection(&mPolygon);
    if (!intersection)
    {
        return false;
    }

    bool               result   = false;
    OGRwkbGeometryType geomType = wkbFlatten(intersection->getGeometryType());

    if (geomType == wkbPoint || geomType == wkbMultiPoint)
    {
        // Intersection is just point(s) - check if any are NOT line endpoints
        const double eps      = 1e-9;
        double       startLon = line->startPoint()->getLongitude().value();
        double       startLat = line->startPoint()->getLatitude().value();
        double       endLon   = line->endPoint()->getLongitude().value();
        double       endLat   = line->endPoint()->getLatitude().value();

        auto checkIfEndpoint = [&](double x, double y) -> bool {
            return pointsMatch(x, y, startLon, startLat, eps) ||
                   pointsMatch(x, y, endLon, endLat, eps);
        };

        if (geomType == wkbPoint)
        {
            OGRPoint *pt = static_cast<OGRPoint *>(intersection);
            result       = !checkIfEndpoint(pt->getX(), pt->getY());
        }
        else
        {
            OGRMultiPoint *mpt = static_cast<OGRMultiPoint *>(intersection);
            for (int i = 0; i < mpt->getNumGeometries(); ++i)
            {
                OGRPoint *pt =
                    static_cast<OGRPoint *>(mpt->getGeometryRef(i));
                if (!checkIfEndpoint(pt->getX(), pt->getY()))
                {
                    result = true;
                    break;
                }
            }
        }
    }
    else
    {
        // Intersection is a line, polygon, or geometry collection
        result = true;
    }

    delete intersection;
    return result;
}

bool Polygon::isValidWaterSegment(const std::shared_ptr<GLine> &segment) const
{
    return !isSegmentDiagonalThroughHole(segment);
}

bool Polygon::segmentCrossesHoles(const std::shared_ptr<GLine> &segment) const
{
    return isSegmentDiagonalThroughHole(segment);
}

bool Polygon::isSegmentDiagonalThroughHole(
    const std::shared_ptr<GLine> &segment) const
{
    int numHoles = mPolygon.getNumInteriorRings();

    // Pre-compute segment bounding box
    double segStartLon = segment->startPoint()->getLongitude().value();
    double segEndLon   = segment->endPoint()->getLongitude().value();
    double segStartLat = segment->startPoint()->getLatitude().value();
    double segEndLat   = segment->endPoint()->getLatitude().value();
    double segMinLon   = std::min(segStartLon, segEndLon);
    double segMaxLon   = std::max(segStartLon, segEndLon);
    double segMinLat   = std::min(segStartLat, segEndLat);
    double segMaxLat   = std::max(segStartLat, segEndLat);

    for (int holeIndex = 0; holeIndex < numHoles; ++holeIndex)
    {
        const OGRLinearRing *hole = mPolygon.getInteriorRing(holeIndex);
        if (!hole)
        {
            continue;
        }

        // Quick bounding box check
        OGREnvelope holeEnv;
        hole->getEnvelope(&holeEnv);

        if (segMaxLon < holeEnv.MinX || segMinLon > holeEnv.MaxX ||
            segMaxLat < holeEnv.MinY || segMinLat > holeEnv.MaxY)
        {
            continue;  // No possible intersection
        }

        // Detailed checks
        if (isSegmentPassingThroughHole(segment, holeIndex))
        {
            return true;
        }

        if (isSegmentCrossingHoleBoundary(segment, holeIndex))
        {
            return true;
        }
    }

    return false;
}

bool Polygon::isSegmentPassingThroughHole(
    const std::shared_ptr<GLine> &segment, int holeIndex) const
{
    if (holeIndex >= mPolygon.getNumInteriorRings())
    {
        return false;
    }

    const OGRLinearRing *hole = mPolygon.getInteriorRing(holeIndex);
    if (!hole)
    {
        return false;
    }

    const OGRPoint startOGR = segment->startPoint()->getGDALPoint();
    const OGRPoint endOGR   = segment->endPoint()->getGDALPoint();

    bool startOnBoundary = hole->isPointOnRingBoundary(&startOGR, TRUE);
    bool endOnBoundary   = hole->isPointOnRingBoundary(&endOGR, TRUE);

    // If both points are on boundary and segment is short, allow it
    if (startOnBoundary && endOnBoundary)
    {
        if (segment->startPoint()->distance(*segment->endPoint()).value() <
            SHORT_SEGMENT_THRESHOLD_METERS)
        {
            return false;
        }
    }

    // Sample points along the segment
    auto   startPoint = segment->startPoint();
    auto   endPoint   = segment->endPoint();
    double startLon   = startPoint->getLongitude().value();
    double endLon     = endPoint->getLongitude().value();
    double startLat   = startPoint->getLatitude().value();
    double endLat     = endPoint->getLatitude().value();

    // Handle antimeridian crossing
    double lonDiff              = endLon - startLon;
    bool   crossesAntimeridian  = std::abs(lonDiff) > 180.0;
    double adjustedEndLon       = endLon;

    if (crossesAntimeridian)
    {
        if (lonDiff > 180.0)
        {
            adjustedEndLon = endLon - 360.0;
        }
        else if (lonDiff < -180.0)
        {
            adjustedEndLon = endLon + 360.0;
        }
    }

    for (int i = 1; i < HOLE_SAMPLING_COUNT; ++i)
    {
        double t = static_cast<double>(i) / HOLE_SAMPLING_COUNT;

        double lat = startLat * (1.0 - t) + endLat * t;
        double lon = startLon * (1.0 - t) + adjustedEndLon * t;

        // Normalize longitude using centralized function with epsilon tolerance
        lon = AngleUtils::normalizeLongitude(lon);

        // Optimized: pass coordinates directly instead of creating GPoint
        if (isPointInHoleByCoords(lon, lat, holeIndex))
        {
            return true;
        }
    }

    return false;
}

bool Polygon::isSegmentCrossingHoleBoundary(
    const std::shared_ptr<GLine> &segment, int holeIndex) const
{
    if (holeIndex >= mPolygon.getNumInteriorRings())
    {
        return false;
    }

    const OGRLinearRing *hole = mPolygon.getInteriorRing(holeIndex);
    if (!hole)
    {
        return false;
    }

    int numPoints = hole->getNumPoints();

    for (int i = 0; i < numPoints - 1; ++i)
    {
        auto holeEdgeStart = pointFromRing(hole, i);
        auto holeEdgeEnd   = pointFromRing(hole, i + 1);
        auto holeEdge = std::make_shared<GLine>(holeEdgeStart, holeEdgeEnd);

        if (segment->intersects(*holeEdge, false))
        {
            bool startPointOnHoleEdge =
                (*segment->startPoint() == *holeEdgeStart) ||
                (*segment->startPoint() == *holeEdgeEnd);
            bool endPointOnHoleEdge =
                (*segment->endPoint() == *holeEdgeStart) ||
                (*segment->endPoint() == *holeEdgeEnd);

            // Both endpoints on same hole edge = valid edge traversal
            if (startPointOnHoleEdge && endPointOnHoleEdge)
            {
                continue;
            }

            // One endpoint on hole edge - check if intersection is at vertex
            if (startPointOnHoleEdge || endPointOnHoleEdge)
            {
                if (!isIntersectionAtVertex(segment, holeEdge))
                {
                    return true;
                }
            }
            else
            {
                // Neither endpoint on hole edge = invalid crossing
                return true;
            }
        }
    }

    return false;
}

bool Polygon::isIntersectionAtVertex(
    const std::shared_ptr<GLine> &segment1,
    const std::shared_ptr<GLine> &segment2) const
{
    auto s1_start = segment1->startPoint();
    auto s1_end   = segment1->endPoint();
    auto s2_start = segment2->startPoint();
    auto s2_end   = segment2->endPoint();

    // Check all four vertex combinations
    if (s1_start->distance(*s2_start).value() < VERTEX_TOLERANCE_METERS ||
        s1_start->distance(*s2_end).value() < VERTEX_TOLERANCE_METERS ||
        s1_end->distance(*s2_start).value() < VERTEX_TOLERANCE_METERS ||
        s1_end->distance(*s2_end).value() < VERTEX_TOLERANCE_METERS)
    {
        return true;
    }

    return false;
}

bool Polygon::isPointInHoleByCoords(double lon, double lat,
                                    int    holeIndex) const
{
    if (holeIndex >= mPolygon.getNumInteriorRings())
    {
        return false;
    }

    const OGRLinearRing *hole = mPolygon.getInteriorRing(holeIndex);
    if (!hole)
    {
        return false;
    }

    int numPoints = hole->getNumPoints();
    if (numPoints < 4)  // Need at least 3 unique points + closing point
    {
        return false;
    }

    // Ray casting algorithm - directly on OGR ring coordinates
    // No GPoint allocation needed!
    int    intersections = 0;
    double px            = lon;
    double py            = lat;

    // Iterate through edges (numPoints - 1 because last point duplicates first)
    for (int i = 0; i < numPoints - 1; ++i)
    {
        double x1 = hole->getX(i);
        double y1 = hole->getY(i);
        double x2 = hole->getX(i + 1);
        double y2 = hole->getY(i + 1);

        // Check if ray from point crosses this edge
        if (((y1 > py) != (y2 > py)) &&
            (px < (x2 - x1) * (py - y1) / (y2 - y1) + x1))
        {
            intersections++;
        }
    }

    return (intersections % 2) == 1;
}

bool Polygon::isPointInHole(const std::shared_ptr<GPoint> &point,
                            int                            holeIndex) const
{
    // Delegate to optimized coordinate-based implementation
    return isPointInHoleByCoords(point->getLongitude().value(),
                                 point->getLatitude().value(),
                                 holeIndex);
}

// =============================================================================
// Bounding Box Operations
// =============================================================================

void Polygon::getEnvelope(double &minLon, double &maxLon, double &minLat,
                          double &maxLat) const
{
    OGREnvelope envelope;
    mPolygon.getEnvelope(&envelope);

    minLon = envelope.MinX;
    maxLon = envelope.MaxX;
    minLat = envelope.MinY;
    maxLat = envelope.MaxY;
}

bool Polygon::segmentBoundsIntersect(
    const std::shared_ptr<GLine> &segment) const
{
    double polyMinLon, polyMaxLon, polyMinLat, polyMaxLat;
    getEnvelope(polyMinLon, polyMaxLon, polyMinLat, polyMaxLat);

    double segStartLon = segment->startPoint()->getLongitude().value();
    double segEndLon   = segment->endPoint()->getLongitude().value();
    double segStartLat = segment->startPoint()->getLatitude().value();
    double segEndLat   = segment->endPoint()->getLatitude().value();

    double segMinLon = std::min(segStartLon, segEndLon);
    double segMaxLon = std::max(segStartLon, segEndLon);
    double segMinLat = std::min(segStartLat, segEndLat);
    double segMaxLat = std::max(segStartLat, segEndLat);

    // Check for non-overlap
    if (segMaxLon < polyMinLon || segMinLon > polyMaxLon ||
        segMaxLat < polyMinLat || segMinLat > polyMaxLat)
    {
        return false;
    }

    return true;
}

// =============================================================================
// Boundary Transformations
// =============================================================================

std::unique_ptr<OGRLinearRing>
Polygon::offsetBoundary(const OGRLinearRing &ring, bool inward,
                        units::length::meter_t offset) const
{
    const OGRSpatialReference           *currentSR = mPolygon.getSpatialReference();
    std::shared_ptr<OGRSpatialReference> targetSR =
        Point::getDefaultProjectionReference();

    // Create coordinate transformations
    OGRCoordinateTransformation *coordProjTransform =
        OGRCreateCoordinateTransformation(currentSR, targetSR.get());
    if (!coordProjTransform)
    {
        DESTROY_COORD_TRANSFORM(coordProjTransform);
        throw std::runtime_error(
            "Failed to create coordinate transformation.");
    }

    OGRCoordinateTransformation *coordReprojTransform =
        OGRCreateCoordinateTransformation(targetSR.get(), currentSR);
    if (!coordReprojTransform)
    {
        DESTROY_COORD_TRANSFORM(coordProjTransform);
        DESTROY_COORD_TRANSFORM(coordReprojTransform);
        throw std::runtime_error(
            "Failed to create coordinate transformation.");
    }

    // Transform ring to projected CRS
    OGRLinearRing *transformedRing =
        static_cast<OGRLinearRing *>(ring.clone());
    transformedRing->transform(coordProjTransform);

    // Apply buffer operation
    double       actualOffset     = inward ? -offset.value() : offset.value();
    OGRGeometry *bufferedGeometry = transformedRing->Buffer(actualOffset);
    delete transformedRing;

    // Transform back to WGS84
    bufferedGeometry->transform(coordReprojTransform);

    // Extract the linear ring
    OGRPolygon                    *bufferedPolygon =
        dynamic_cast<OGRPolygon *>(bufferedGeometry);
    std::unique_ptr<OGRLinearRing> newRing(nullptr);

    if (bufferedPolygon)
    {
        newRing.reset(static_cast<OGRLinearRing *>(
            bufferedPolygon->getExteriorRing()->clone()));
    }
    else
    {
        DESTROY_COORD_TRANSFORM(coordProjTransform);
        DESTROY_COORD_TRANSFORM(coordReprojTransform);
        delete bufferedGeometry;
        throw std::runtime_error("Buffered geometry is not a polygon.");
    }

    // Clean up
    DESTROY_COORD_TRANSFORM(coordProjTransform);
    DESTROY_COORD_TRANSFORM(coordReprojTransform);
    delete bufferedGeometry;

    return newRing;
}

void Polygon::transformOuterBoundary(bool                   inward,
                                     units::length::meter_t offset)
{
    const OGRLinearRing *currentOuterRing =
        static_cast<OGRLinearRing *>(mPolygon.getExteriorRing());

    std::unique_ptr<OGRLinearRing> newOuterRing =
        offsetBoundary(*currentOuterRing, inward, offset);

    // Create new polygon with transformed outer ring
    OGRPolygon newPolygon;
    newPolygon.addRing(newOuterRing.get());

    // Copy interior rings
    int numInteriorRings = mPolygon.getNumInteriorRings();
    for (int i = 0; i < numInteriorRings; ++i)
    {
        newPolygon.addRing(mPolygon.getInteriorRing(i));
    }

    mPolygon = newPolygon;
}

void Polygon::transformInnerHolesBoundaries(bool                   inward,
                                            units::length::meter_t offset)
{
    // Start with original outer boundary
    OGRPolygon newPolygon;
    newPolygon.addRing(mPolygon.getExteriorRing());

    // Offset each interior ring
    int numInteriorRings = mPolygon.getNumInteriorRings();
    for (int i = 0; i < numInteriorRings; ++i)
    {
        const OGRLinearRing *currentInnerRing = mPolygon.getInteriorRing(i);

        std::unique_ptr<OGRLinearRing> newInnerRing =
            offsetBoundary(*currentInnerRing, inward, offset);

        newPolygon.addRing(newInnerRing.get());
    }

    mPolygon = newPolygon;
}

// =============================================================================
// Simplification
// =============================================================================

std::shared_ptr<Polygon> Polygon::simplify(double toleranceMeters) const
{
    // Convert meters to approximate degrees for GDAL's Simplify()
    // At the equator: 1 degree ≈ 111,000 meters
    // This is an approximation; accuracy varies with latitude
    constexpr double METERS_PER_DEGREE = 111000.0;
    double toleranceDegrees = toleranceMeters / METERS_PER_DEGREE;

    // Use GDAL's Simplify which implements Douglas-Peucker
    std::unique_ptr<OGRGeometry> simplified(
        mPolygon.Simplify(toleranceDegrees));

    if (!simplified || simplified->getGeometryType() != wkbPolygon)
    {
        // If simplification fails, return a copy of the original
        return std::make_shared<Polygon>(mOutterBoundary, mInnerHoles, mUserID);
    }

    OGRPolygon* simplifiedPoly = static_cast<OGRPolygon*>(simplified.get());

    // Extract outer boundary points from simplified polygon
    const OGRLinearRing* extRing = simplifiedPoly->getExteriorRing();
    QVector<std::shared_ptr<GPoint>> newOuter;

    if (extRing)
    {
        int numPoints = extRing->getNumPoints();
        // Skip last point if it's the same as first (closed ring)
        if (numPoints > 1 &&
            extRing->getX(0) == extRing->getX(numPoints - 1) &&
            extRing->getY(0) == extRing->getY(numPoints - 1))
        {
            numPoints--;
        }

        for (int i = 0; i < numPoints; ++i)
        {
            newOuter.append(std::make_shared<GPoint>(
                units::angle::degree_t(extRing->getX(i)),
                units::angle::degree_t(extRing->getY(i))));
        }
    }

    // Extract inner holes from simplified polygon
    QVector<QVector<std::shared_ptr<GPoint>>> newHoles;
    int numInteriorRings = simplifiedPoly->getNumInteriorRings();

    for (int h = 0; h < numInteriorRings; ++h)
    {
        const OGRLinearRing* holeRing = simplifiedPoly->getInteriorRing(h);
        QVector<std::shared_ptr<GPoint>> holePoints;

        if (holeRing)
        {
            int numPoints = holeRing->getNumPoints();
            // Skip last point if it's the same as first (closed ring)
            if (numPoints > 1 &&
                holeRing->getX(0) == holeRing->getX(numPoints - 1) &&
                holeRing->getY(0) == holeRing->getY(numPoints - 1))
            {
                numPoints--;
            }

            for (int i = 0; i < numPoints; ++i)
            {
                holePoints.append(std::make_shared<GPoint>(
                    units::angle::degree_t(holeRing->getX(i)),
                    units::angle::degree_t(holeRing->getY(i))));
            }
        }

        if (!holePoints.isEmpty())
        {
            newHoles.append(holePoints);
        }
    }

    return std::make_shared<Polygon>(newOuter, newHoles, mUserID + "_simplified");
}

int Polygon::outerVertexCount() const
{
    return mOutterBoundary.size();
}

// =============================================================================
// String Representation
// =============================================================================

QString Polygon::toString(const QString &format, int decimalPercision) const
{
    QString perimeterStr =
        QString::number(perimeter().value(), 'f', decimalPercision);
    QString areaStr = QString::number(area().value(), 'f', decimalPercision);

    QString result = format;

    // Replace placeholders (case-insensitive)
    result.replace("%perimeter", perimeterStr, Qt::CaseInsensitive);
    result.replace("%area", areaStr, Qt::CaseInsensitive);

    return result;
}

}  // namespace ShipNetSimCore
