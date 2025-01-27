/**
 * @file polygon.cpp
 * @brief Implementation of the polygon class and its utilities.
 *
 * This file contains the implementation for the Polygon class, which
 * defines a polygon with an outer boundary and an optional set of
 * inner holes. The class provides methods for calculating the
 * perimeter, area, and for checking if a point is inside the polygon
 * or is part of its structure (either on the boundary or in the holes).
 *
 * Author: Ahmed Aredah
 * Date: 10.12.2023
 */

#include "polygon.h" // Include the definition of the Polygon class.
#include "qdebug.h"  // Include for qCritical().
#include <sstream>   // Include for string stream operations.
#include <cmath>     // Include for mathematical operations.
#include <limits>    // Include for numeric limits.
#include <QVector>   // Include the QVector class from the Qt framework.
#include <GeographicLib/Geodesic.hpp>
#include <GeographicLib/PolygonArea.hpp>

namespace ShipNetSimCore
{
// Default constructor
Polygon::Polygon() {}

// Parameterized constructor to initialize the outer boundary and inner holes
Polygon::Polygon(
    const QVector<std::shared_ptr<GPoint>>&
        boundary, // points defining outer boundary
    const QVector<QVector<std::shared_ptr<GPoint>>>&
        holes,  // vector of vectors of points defining holes
    const QString ID) // ID of the polygon (land of water body)
    : mOutterBoundary(boundary), mInnerHoles(holes), mUserID(ID)
{
    setOuterPoints(boundary);
    setInnerHolesPoints(holes);
}

void Polygon::validateRing(const OGRLinearRing& ring,
                              const QString& description)
{
    // Check for sufficient number of points
    if (ring.getNumPoints() < 3)
    {
        throw std::runtime_error( description.toStdString() +
                                 "is degenerate: requires at "
                                 "least 3 unique points.");
    }

    // Check for unique points to avoid co-location
    // QSet<QPair<double, double>> uniquePoints;
    // for (int i = 0; i < ring.getNumPoints(); ++i)
    // {
    //     QPair<double, double> pointPair(ring.getX(i),
    //                                     ring.getY(i));
    //     if (uniquePoints.contains(pointPair))
    //     {
    //         continue;
    //         //qFatal("% has co-located points.", qPrintable(description));
    //     }
    //     uniquePoints.insert(pointPair);
    // }

    // If only 3 points are provided, check for collinearity
    if (ring.getNumPoints() == 3)
    {
        const OGRSpatialReference* SR = ring.getSpatialReference();

        GPoint p0 = GPoint(units::angle::degree_t(ring.getX(0)),
                           units::angle::degree_t(ring.getY(0)),
                           *SR);
        GPoint p1 = GPoint(units::angle::degree_t(ring.getX(1)),
                           units::angle::degree_t(ring.getY(1)),
                           *SR);
        GPoint p2 = GPoint(units::angle::degree_t(ring.getX(2)),
                           units::angle::degree_t(ring.getY(2)),
                           *SR);

        SR = nullptr;

        if (GLine::orientation(std::make_shared<GPoint>(p0),
                               std::make_shared<GPoint>(p1),
                               std::make_shared<GPoint>(p2)))
        // if (arePointsCollinear(points[0], points[1], points[2]))
        {
            throw std::runtime_error(description.toStdString() +
                                     "is degenerate: points "
                                     "are collinear.");
        }
    }
}


void Polygon::setOuterPoints(const QVector<std::shared_ptr<GPoint>>& newOuter)
{
    mOutterBoundary = newOuter;

    // Create a ring (outer boundary)
    OGRLinearRing outerRing;

    // Temporarily store interior rings
    std::vector<OGRLinearRing*> tempInteriorRings;
    for(int i = 0; i < mPolygon.getNumInteriorRings(); ++i) {
        OGRLinearRing* interiorRing = mPolygon.getInteriorRing(i)->clone();
        tempInteriorRings.push_back(interiorRing);
    }

    // Clear the polygon and update the exterior ring
    mPolygon.empty();
    for (auto &p: newOuter)
    {
        auto pp = p->getGDALPoint();
        outerRing.addPoint(&pp);
    }
    outerRing.closeRings();
    outerRing.assignSpatialReference(
        newOuter[0]->getGDALPoint().getSpatialReference());
    mPolygon.addRing(&outerRing); // Add the new exterior ring
    mPolygon.assignSpatialReference(
        newOuter[0]->getGDALPoint().getSpatialReference());

    // Restore interior rings
    for (auto* interiorRing : tempInteriorRings) {
        mPolygon.addRingDirectly(interiorRing); // Takes ownership of the pointer
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

    // Temporarily store exterior rings
    OGRLinearRing* tempExteriorRing = mPolygon.getExteriorRing()->clone();

    // Clear the polygon and update the exterior ring
    mPolygon.empty();
    mPolygon.addRingDirectly(tempExteriorRing); // Takes ownership of the pointer
    for (const auto& hole : newInners)
    {
        OGRLinearRing holeRing;
        for (auto &p: hole)
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


bool Polygon::isPointWithinExteriorRing(const GPoint& pointToCheck) const
{
    auto r = mPolygon.getExteriorRing();
    const OGRPoint p = pointToCheck.getGDALPoint();
    // Check if the point is on the boundary of the ring
    if (r->isPointOnRingBoundary(&p, TRUE)) {
        return true;
    }
    // Check if the point is inside the ring (including boundaries)
    if (r->isPointInRing(&p, TRUE)) {
        return true;
    }
    // If neither on the boundary nor inside, the point is outside the polygon
    return false;
}

bool Polygon::isPointWithinInteriorRings(const GPoint& pointToCheck) const
{
    const OGRPoint p = pointToCheck.getGDALPoint();

    for(int i = 0; i < mPolygon.getNumInteriorRings(); ++i)
    {
        auto r = mPolygon.getInteriorRing(i);
        // Check if the point is on the boundary of the ring
        if (r->isPointOnRingBoundary(&p, TRUE)) {
            return true;
        }
        // Check if the point is inside the ring (including boundaries)
        if (r->isPointInRing(&p, TRUE)) {
            return true;
        }
    }
    return false;
}

bool Polygon::isPointWithinPolygon(const GPoint& pointToCheck) const
{
    if (isPointWithinInteriorRings(pointToCheck))
    {
        return false;
    }
    return isPointWithinExteriorRing(pointToCheck);
}

units::area::square_meter_t Polygon::area() const
{
    // Get the spatial reference of the polygon
    const OGRSpatialReference* thisSR = mPolygon.getSpatialReference();

    double semiMajorAxis = thisSR->GetSemiMajor();
    double flattening = 1.0 / thisSR->GetInvFlattening();

    // Create a GeographicLib::Geodesic object with the ellipsoid parameters
    const GeographicLib::Geodesic geod(semiMajorAxis, flattening);

    // Initialize a PolygonArea object for the WGS84 ellipsoid
    GeographicLib::PolygonArea poly(geod);

    // Handle the exterior ring
    auto* exteriorRing = mPolygon.getExteriorRing();
    for (int i = 0; i < exteriorRing->getNumPoints(); ++i) {
        double lon = exteriorRing->getX(i);
        double lat = exteriorRing->getY(i);
        poly.AddPoint(lat, lon);
    }

    // Compute the area and perimeter of the outer boundary
    double area, perimeter;
    poly.Compute(false, true, area, perimeter);

    // Handle the interior rings (holes)
    int numInteriorRings = mPolygon.getNumInteriorRings();
    for (int j = 0; j < numInteriorRings; ++j) {
        auto* interiorRing = mPolygon.getInteriorRing(j);
        // Each hole treated separately
        GeographicLib::PolygonArea holePoly(geod);

        for (int k = 0; k < interiorRing->getNumPoints(); ++k) {
            double lon = interiorRing->getX(k);
            double lat = interiorRing->getY(k);
            holePoly.AddPoint(lat, lon);
        }

        // Compute the area and perimeter of the hole and
        // subtract from the total area
        double holeArea, holePerimeter;
        holePoly.Compute(false, true, holeArea, holePerimeter);
        area -= holeArea; // Subtract the hole area from the outer area
    }

    // Return the net area of the polygon with holes subtracted,
    // in square meters
    return units::area::square_meter_t(area);
}


units::length::meter_t Polygon::perimeter() const {
    // Get the spatial reference of the polygon
    const OGRSpatialReference* thisSR = mPolygon.getSpatialReference();

    double semiMajorAxis = thisSR->GetSemiMajor();
    double flattening = 1.0 / thisSR->GetInvFlattening();

    // Create a GeographicLib::Geodesic object with the ellipsoid parameters
    const GeographicLib::Geodesic geod(semiMajorAxis, flattening);

    // Initialize a PolygonArea object for the WGS84 ellipsoid
    GeographicLib::PolygonArea poly(geod);

    // Handle the exterior ring
    auto* exteriorRing = mPolygon.getExteriorRing();
    for (int i = 0; i < exteriorRing->getNumPoints(); ++i) {
        double lon = exteriorRing->getX(i);
        double lat = exteriorRing->getY(i);
        poly.AddPoint(lat, lon);
    }

    // Compute the area and perimeter of the outer boundary
    double area, perimeter;
    poly.Compute(false, true, area, perimeter);

    return units::length::meter_t(perimeter);
}

bool Polygon::intersects(const std::shared_ptr<GLine> line)
{
    return line->getGDALLine().Intersects(&mPolygon);
}

std::unique_ptr<OGRLinearRing> Polygon::offsetBoundary(
    const OGRLinearRing& ring, bool inward,
    units::length::meter_t offset) const
{
    const OGRSpatialReference* currentSR = mPolygon.getSpatialReference();
    std::shared_ptr<OGRSpatialReference> targetSR =
        Point::getDefaultProjectionReference();

    // Create a coordinate transformation from the current
    // geographic CRS to the target projected CRS
    OGRCoordinateTransformation* coordProjTransform =
        OGRCreateCoordinateTransformation(currentSR, targetSR.get());
    if (!coordProjTransform) {
        OCTDestroyCoordinateTransformation(coordProjTransform);
        throw std::runtime_error("Failed to create coordinate transformation.");
    }

    OGRCoordinateTransformation* coordReprojTransform =
        OGRCreateCoordinateTransformation(targetSR.get(), currentSR);
    if (!coordReprojTransform) {
        OCTDestroyCoordinateTransformation(coordReprojTransform);
        throw std::runtime_error("Failed to create coordinate transformation.");
    }

    OGRLinearRing* transformedRing = static_cast<OGRLinearRing*>(ring.clone());
    transformedRing->transform(coordProjTransform);
    // Apply the buffer operation
    double actualOffset = inward ? -offset.value() : offset.value();
    OGRGeometry* bufferedGeometry = transformedRing->Buffer(actualOffset);
    delete transformedRing;

    // Transform the buffered geometry back to WGS84
    bufferedGeometry->transform(coordReprojTransform);

    // Extract the linear ring from the buffered geometry
    OGRPolygon* bufferedPolygon = dynamic_cast<OGRPolygon*>(bufferedGeometry);
    std::unique_ptr<OGRLinearRing> newRing(nullptr);

    if (bufferedPolygon) {
        newRing.reset(bufferedPolygon->getExteriorRing()->clone());
    } else {
        throw std::runtime_error("Buffered geometry is not a polygon.");
    }


    // Clean up
    OCTDestroyCoordinateTransformation(coordProjTransform);
    OCTDestroyCoordinateTransformation(coordReprojTransform);
    delete bufferedGeometry;

    return newRing;
}

void Polygon::transformOuterBoundary(bool inward,
                                     units::length::meter_t offset)
{
    // Retrieve the current outer boundary (exterior ring) of the polygon
    const OGRLinearRing* currentOuterRing = mPolygon.getExteriorRing();

    // Offset the current outer boundary to create a new ring
    std::unique_ptr<OGRLinearRing> newOuterRing =
        offsetBoundary(*currentOuterRing, inward, offset);

    // Create a new polygon to hold the transformed geometry
    OGRPolygon newPolygon;

    // Add the new outer ring to the new polygon
    newPolygon.addRing(newOuterRing.get());

    // Copy interior rings (holes)
    // from the original polygon to the new polygon
    int numInteriorRings = mPolygon.getNumInteriorRings();
    for (int i = 0; i < numInteriorRings; ++i) {
        newPolygon.addRing(mPolygon.getInteriorRing(i));
    }

    // Replace the existing polygon geometry with the new polygon
    mPolygon = newPolygon;
}

void Polygon::transformInnerHolesBoundaries(bool inward,
                                            units::length::meter_t offset)
{
    // Create a new polygon to hold the transformed geometry,
    // starting with the original outer boundary
    OGRPolygon newPolygon;
    newPolygon.addRing(mPolygon.getExteriorRing());

    // Offset each interior ring (hole) and add to the new polygon
    int numInteriorRings = mPolygon.getNumInteriorRings();
    for (int i = 0; i < numInteriorRings; ++i) {
        const OGRLinearRing* currentInnerRing = mPolygon.getInteriorRing(i);
        std::unique_ptr<OGRLinearRing> newInnerRing =
            offsetBoundary(*currentInnerRing, inward, offset);

        // Add the new inner ring to the new polygon.
        // Use get() to access the raw pointer without releasing ownership.
        newPolygon.addRing(newInnerRing.get());
    }

    // Replace the existing polygon geometry with the new polygon
    mPolygon = newPolygon;
}


units::length::meter_t Polygon::getMaxClearWidth(const GLine& line) const
{
    units::length::meter_t leftClearWidth =
        std::numeric_limits<units::length::meter_t>::max();
    units::length::meter_t rightClearWidth =
        std::numeric_limits<units::length::meter_t>::max();

    auto calculateClearWidths = [&leftClearWidth,
                                 &rightClearWidth,
                                 &line](const OGRLinearRing* ring)
    {
        int numPoints = ring->getNumPoints();
        for (int i = 0; i < numPoints; ++i)
        {
            OGRPoint vertexA, vertexB;
            ring->getPoint(i, &vertexA);
            ring->getPoint((i + 1) % numPoints, &vertexB); // loop closed

            const OGRSpatialReference* SR = ring->getSpatialReference();

            auto pointA = std::make_shared<GPoint>(
                units::angle::degree_t(vertexA.getX()),
                units::angle::degree_t(vertexA.getY()),
                *SR);

            auto pointB = std::make_shared<GPoint>(
                units::angle::degree_t(vertexB.getX()),
                units::angle::degree_t(vertexB.getY()),
                *SR);

            SR = nullptr;

            GLine edge(pointA, pointB);

            units::length::meter_t dist1 =
                edge.getPerpendicularDistance(*(line.startPoint()));
            units::length::meter_t dist2 =
                edge.getPerpendicularDistance(*(line.endPoint()));

            Line::LocationToLine isLeft1 = line.getlocationToLine(pointA);
            Line::LocationToLine isLeft2 = line.getlocationToLine(pointB);

            if (isLeft1 == Line::LocationToLine::left) {
                leftClearWidth = std::min(leftClearWidth, dist1);
            } else if (isLeft1 == Line::LocationToLine::right) {
                rightClearWidth = std::min(rightClearWidth, dist1);
            }

            if (isLeft2 == Line::LocationToLine::left) {
                leftClearWidth = std::min(leftClearWidth, dist2);
            } else if (isLeft2 == Line::LocationToLine::right) {
                rightClearWidth = std::min(rightClearWidth, dist2);
            }
        }
    };

    // Calculate clear widths for the outer boundary
    const OGRLinearRing* outerRing = mPolygon.getExteriorRing();
    calculateClearWidths(outerRing);

    // Calculate clear widths for each inner hole
    int numInteriorRings = mPolygon.getNumInteriorRings();
    for (int i = 0; i < numInteriorRings; ++i) {
        const OGRLinearRing* hole = mPolygon.getInteriorRing(i);
        calculateClearWidths(hole);
    }

    return rightClearWidth + leftClearWidth;
}


QString Polygon::toString(const QString &format, int decimalPercision) const
{
    QString perimeterStr =
        QString::number(perimeter().value(), 'f', decimalPercision);
    QString areaStr = QString::number(area().value(), 'f', decimalPercision);

    QString result = format;

    // Replace placeholders (case-insensitive)
    result.replace("%perimeter", perimeterStr, Qt::CaseInsensitive);
    result.replace("%area", areaStr, Qt::CaseInsensitive);

    return result; // Return the formatted string
}

bool Polygon::contains(std::shared_ptr<GPoint> point) const
{
    auto r = mPolygon.getExteriorRing();
    const OGRPoint p = point->getGDALPoint();
    // Check if the point is on the boundary of the ring
    if (r->isPointOnRingBoundary(&p, TRUE)) {
        return true;
    }

    for (int i = 0; i < mPolygon.getNumInteriorRings(); ++i)
    {
        if (mPolygon.getInteriorRing(i)->isPointOnRingBoundary(&p, TRUE))
        {
            return true;
        }
    }
    return false;
}
};
