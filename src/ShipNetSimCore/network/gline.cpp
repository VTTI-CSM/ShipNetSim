#include "gline.h"
#include "galgebraicvector.h"
#include <GeographicLib/Geodesic.hpp>
#include <GeographicLib/GeodesicLine.hpp>

namespace ShipNetSimCore
{
GLine::GLine()
{
    start =
        std::make_shared<GPoint>(GPoint(units::angle::degree_t(0.0),
                                        units::angle::degree_t(0.0)));
    end =
        std::make_shared<GPoint>(GPoint(units::angle::degree_t(0.0),
                                        units::angle::degree_t(0.0)));
    OGRPoint sp = start->getGDALPoint();
    line.addPoint(&sp);

    OGRPoint ep = end->getGDALPoint();
    line.addPoint(&ep);

    mLength = units::length::meter_t(0.0);
    mForwardAzimuth = units::angle::degree_t(0.0);
    mBackwardAzimuth = units::angle::degree_t(0.0);
}

GLine::GLine(std::shared_ptr<GPoint> start,
             std::shared_ptr<GPoint> end) :
    start(start),  // Initialize start point.
    end(end)      // Initialize end point.
{
    if (! start->getGDALPoint().getSpatialReference()->IsSame(
            end->getGDALPoint().getSpatialReference()))
    {
        throw std::runtime_error("Mismatch spatial reference for the two points!");
    }
    OGRPoint sp = start->getGDALPoint();
    line.addPoint(&sp);

    OGRPoint ep = end->getGDALPoint();
    line.addPoint(&ep);

    mLength = start->distance(*end);  // Calculate line segment length.
    mForwardAzimuth = start->forwardAzimuth(*end); // calculate frwd azimuth
    mBackwardAzimuth = start->backwardAzimuth(*end); // calculate bck azimuth
}

// Destructor.
GLine::~GLine()
{
    // No dynamic memory to free.
}

OGRLineString GLine::getGDALLine() const
{
    return line;
}

// Get length of the line segment.
units::length::meter_t GLine::length() const
{
    return mLength;  // Return length of the line segment.
}

units::angle::degree_t GLine::forwardAzimuth() const
{
    return mForwardAzimuth;
}

units::angle::degree_t GLine::backwardAzimuth() const
{
    return mBackwardAzimuth;
}

// Get starting point of the line segment.
std::shared_ptr<GPoint> GLine::startPoint() const
{
    return start;  // Return start point.
}

// Get ending point of the line segment.
std::shared_ptr<GPoint> GLine::endPoint() const
{
    return end;  // Return end point.
}

void GLine::setStartPoint(std::shared_ptr<GPoint> sPoint)
{
    start = sPoint;
    OGRPoint sp = start->getGDALPoint();
    line.setPoint(0, &sp);

    mLength = start->distance(*end);  // Calculate line segment length.
    mForwardAzimuth = start->forwardAzimuth(*end); // calculate frwd azimuth
    mBackwardAzimuth = start->backwardAzimuth(*end); // calculate bck azimuth
}

void GLine::setEndPoint(std::shared_ptr<GPoint> ePoint)
{
    end = ePoint;
    OGRPoint ep = end->getGDALPoint();
    line.setPoint(1, &ep);

    mLength = start->distance(*end);  // Calculate line segment length.
    mForwardAzimuth = start->forwardAzimuth(*end); // calculate frwd azimuth
    mBackwardAzimuth = start->backwardAzimuth(*end); // calculate bck azimuth
}

Line GLine::projectTo(OGRSpatialReference* targetSR) const
{
    if (! targetSR || ! targetSR->IsProjected())
    {
        throw std::runtime_error("Target Spatial Reference "
                                 "is not valid or not a projected CRS.");
    }
    std::shared_ptr<Point> ps =
        std::make_shared<Point>(start->projectTo(targetSR));
    std::shared_ptr<Point> pe =
        std::make_shared<Point>(end->projectTo(targetSR));

    return Line(ps, pe);

}

// Determine orientation of three points.
Line::Orientation GLine::orientation(std::shared_ptr<GPoint> p,
                                    std::shared_ptr<GPoint> q,
                                    std::shared_ptr<GPoint> r)
{
    // create the projection SR
    std::shared_ptr<OGRSpatialReference> SR =
        Point::getDefaultProjectionReference();

    // project the points to a projected map and check normal projection
    std::shared_ptr<Point> newp =
        std::make_shared<Point>(p->projectTo(SR.get()));
    std::shared_ptr<Point> newq =
        std::make_shared<Point>(q->projectTo(SR.get()));
    std::shared_ptr<Point> newr =
        std::make_shared<Point>(r->projectTo(SR.get()));

    return Line::orientation(newp, newq, newr);
}

bool GLine::intersects(GLine &other, bool ignoreEdgePoints) const
{
    auto pointsAreClose = [=](
                              const GPoint& p1, const GPoint& p2) -> bool {
        return p1.distance(p2).value() <= TOLERANCE;
    };

    if (ignoreEdgePoints &&
        (pointsAreClose(*start, *other.start) ||
         pointsAreClose(*start, *other.end) ||
         pointsAreClose(*end, *other.start) ||
         pointsAreClose(*end, *other.end)))
    {
        return false; // Ignore intersection as it's at the endpoint
    }

    auto otherL = other.getGDALLine();
    return getGDALLine().Intersects(&otherL);
}

// Calculate angle between two line segments.
units::angle::radian_t GLine::smallestAngleWith(GLine& other) const
{
    // Identify common point between line segments.
    std::shared_ptr<GPoint> commonPoint;

    // Identify the common point
    if (*(this->startPoint()) == *(other.startPoint()) ||
        *(this->startPoint()) == *(other.endPoint()))
    {
        commonPoint = this->startPoint();
    }
    else if (*(this->endPoint()) == *(other.startPoint()) ||
             *(this->endPoint()) == *(other.endPoint()))
    {
        commonPoint = this->endPoint();
    }
    else
    {
        // Line segments do not share a common point.
        // TODO: Solve this to not throw error in the middle of a simulation.
        throw std::invalid_argument(
            "The lines do not share a common point.");
    }

    // Identify non-common points for line segments.
    std::shared_ptr<GPoint> a, c;

    // Get non-common points for both line segments.
    if (*(this->startPoint()) == *commonPoint)
    {
        a = this->endPoint();
    }
    else
    {
        a = this->startPoint();
    }

    if (*(other.startPoint()) == *commonPoint)
    {
        c = other.endPoint();
    }
    else
    {
        c = other.startPoint();
    }

    // Compute the azimuths of the two lines from their shared starting point
    double azi1, azi2;
    azi1 = commonPoint->forwardAzimuth(*a).value();
    azi2 = commonPoint->forwardAzimuth(*c).value();

    // Calculate the angle between the two azimuths
    double angle = azi2 - azi1;


    // Normalize the angle to [0, 180]
    angle = fmod(angle + 360, 360);
    if (angle > 180) angle = 360 - angle;

    return units::angle::degree_t(angle).convert<units::angle::radian>();
}


// Function to calculate the perpendicular distance from a point to the line.
units::length::meter_t GLine::getPerpendicularDistance(
    const GPoint& point) const  // Point from which distance is to be measured.
{
    // create the projection SR
    std::shared_ptr<OGRSpatialReference> SR =
        Point::getDefaultProjectionReference();
    std::shared_ptr<OGRSpatialReference> GSR =
        GPoint::getDefaultReprojectionReference();

    // project the line and the point to the cartesian system
    Line newLine = projectTo(SR.get());
    Point newPoint = point.projectTo(SR.get());

    // get the projected point on the line
    Point p = newLine.getProjectionFrom(newPoint);

    // reproject the point on the geodetic system
    GPoint GPointP = newPoint.reprojectTo(GSR.get());

    return GPointP.distance(point);
}
units::length::meter_t GLine::distanceToPoint(
    const std::shared_ptr<GPoint>& point) const
{
    // create the projection SR
    std::shared_ptr<OGRSpatialReference> SR =
        Point::getDefaultProjectionReference();
    std::shared_ptr<OGRSpatialReference> GSR =
        GPoint::getDefaultReprojectionReference();

    // project the line and the point to the cartesian system
    Line newLine = projectTo(SR.get());
    std::shared_ptr<Point> newPoint =
        std::make_shared<Point>(point->projectTo(SR.get()));

    GPoint nearestPoint =
        newLine.getNearestPoint(newPoint).reprojectTo(GSR.get());

    return point->distance(nearestPoint);
}


// Function to get the theoretical width of the line.
units::length::meter_t GLine::getTheoriticalWidth() const
{
    return mWidth;  // Return the value of mWidth.
}

// Function to set the theoretical width of the line.
void GLine::setTheoriticalWidth(const units::length::meter_t newWidth)
{
    mWidth = newWidth;  // Set the value of mWidth to newWidth.
}

// Function to check the location of a point relative to the line.
Line::LocationToLine GLine::getlocationToLine(
    const std::shared_ptr<GPoint>& point) const
{
    // create the projection SR
    std::shared_ptr<OGRSpatialReference> SR =
        Point::getDefaultProjectionReference();

    // project the line and the point to the cartesian system
    Line newLine = projectTo(SR.get());
    std::shared_ptr<Point> newPoint =
        std::make_shared<Point>(point->projectTo(SR.get()));

    return newLine.getlocationToLine(newPoint);
}

// Overloaded equality operator to compare two lines.
bool GLine::operator==(const GLine& other) const
{
    // Return true if the starting and ending points
    // of both lines are the same.
    return *start == *(other.start) && *end == *(other.end);
}

GPoint GLine::midpoint() const
{
    const GPoint endPoint = *end.get();
    return start->getMiddlePoint(endPoint);
}

GLine GLine::reverse() const
{
    return GLine(end, start);  // Swap start and end points
}

GAlgebraicVector GLine::toAlgebraicVector(
    const std::shared_ptr<GPoint> startPoint) const
{
    GPoint begin, finish;  // Declare start and end points of the vector.
    // Check if startPoint is the same as the starting point of the line.
    if (*startPoint == *start)
    {
        begin = *start;  // Set begin to start.
        finish = *end;   // Set finish to end.
    }
    // If startPoint is not the starting point of the line.
    else
    {
        begin = *end;     // Set begin to end.
        finish = *start;  // Set finish to start.
    }
    // Create an algebraic vector from begin to finish and return it.
    GAlgebraicVector result(begin, finish);
    return result;
}

// Function to convert the line to a string representation.
QString GLine::toString(const QString &format, int decimalPercision) const
{
    // Get the string representations of the start and end points
    QString startStr =
        start ? start->toString("(%x, %y)", decimalPercision) : "N/A";
    QString endStr =
        end ? end->toString("(%x, %y)", decimalPercision) : "N/A";

    // Create the formatted string
    QString result = format;

    // Replace placeholders (case-insensitive)
    result.replace("%start", startStr, Qt::CaseInsensitive);
    result.replace("%end", endStr, Qt::CaseInsensitive);

    return result; // Return the formatted string
}
};
