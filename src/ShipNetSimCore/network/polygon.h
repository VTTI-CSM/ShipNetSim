/**
 * @file polygon.h
 *
 * @brief Definition of the Polygon class.
 *
 * This file contains the definition of the Polygon class,
 * which is used to represent a two-dimensional polygon.
 *
 * @author Ahmed Aredah
 * @date 10.12.2023
 */
#ifndef POLYGON_H
#define POLYGON_H

#include "basegeometry.h"
#include "gpoint.h"
#include <QVector>
#include "gdal_priv.h"
#include "ogr_spatialref.h"
#include <memory> // for std::shared_ptr
#include "../../third_party/units/units.h"
#include "gline.h"

namespace ShipNetSimCore
{
/**
 * @class Polygon
 *
 * @brief Represents a two-dimensional polygon.
 *
 * The Polygon class represents a polygon defined by a series of points.
 * The polygon can have a single outer boundary and multiple inner holes.
 * Each boundary or hole is represented as a QVector of std::shared_ptr
 * to GPoint objects. It provides methods to calculate area, perimeter,
 * and check if a point is inside the polygon.
 *
 * @extends BaseGeometry
 */
class Polygon : public BaseGeometry
{
private:
    QVector<std::shared_ptr<GPoint>> mOutterBoundary;
    QVector<QVector<std::shared_ptr<GPoint>>> mInnerHoles;

    OGRPolygon mPolygon;

    QString mUserID;

    /**
     * @brief Offset the boundary by a given distance.
     *
     * @param boundary The boundary to offset.
     * @param inward True to offset inward, false for outward.
     * @param offset The distance to offset.
     * @return The offset boundary.
     */
    std::unique_ptr<OGRLinearRing> offsetBoundary(
        const OGRLinearRing& ring,
        bool inward,
        units::length::meter_t offset
        ) const;

    // check if the polygon has correct points (degenerate/colocated points)
    static void validateRing(
        const OGRLinearRing& ring,
        const QString& description);

public:
    /**
     * @brief Default constructor.
     *
     * Initializes an empty polygon.
     */
    Polygon();

    /**
     * @brief Parameterized constructor.
     *
     * Initializes the polygon with the given outer boundary and inner holes.
     *
     * @param boundary The outer boundary of the polygon.
     * @param holes The inner holes of the polygon.
     */
    Polygon(const QVector<std::shared_ptr<GPoint>>& boundary,
            const QVector<QVector<std::shared_ptr<GPoint>>>& holes = {},
            const QString ID = "");


    /**
     * @brief set a new vector that defines the outer points of the polygon
     * @param newOuter a vector of the points that defines the polygon
     */
    void setOuterPoints(const QVector<std::shared_ptr<GPoint> > &newOuter);
    /**
     * @brief Get the outer boundary of the polygon.
     *
     * @return A QVector of std::shared_ptr to GPoint objects representing the
     * outer boundary.
     */
    QVector<std::shared_ptr<GPoint>> outer() const;

    /**
     * @brief set a new vector that defines the outer points of the polygon
     * @param holes a vector of vectors of the points that defines
     * the holes in the polygons.
     */
    void setInnerHolesPoints(const
                             QVector<QVector<std::shared_ptr<GPoint>>> holes);
    /**
     * @brief Get the inner holes of the polygon.
     *
     * @return A QVector of QVector of std::shared_ptr to GPoint objects
     * representing the inner holes.
     */
    QVector<QVector<std::shared_ptr<GPoint>>> inners() const;

    /**
     * @brief Check if a point is within the polygon's exterior ring.
     *
     * @param pointToCheck The point to check.
     * @return True if the point is within the polygon's exterior ring,
     *  false otherwise.
     */
    bool isPointWithinExteriorRing(const GPoint& pointToCheck) const;

    /**
     * @brief Check if a point is within the polygon's interior rings.
     *
     * @param pointToCheck The point to check.
     * @return True if the point is within the polygon's interior rings,
     *  false otherwise.
     */
    bool isPointWithinInteriorRings(const GPoint& pointToCheck) const;

    /**
     * @brief check if a point is within the polygon but not its holes.
     *
     * @details The function checks if the point is within the polygon.
     * If the point is within the holes, it returns false. If the point is
     * outside the holes but inside the polygon, it returns true.
     *
     * @param pointToCheck The point to check.
     * @return True if the point is within the polygon's interior rings,
     *  false otherwise.
     */
    bool isPointWithinPolygon(const GPoint& pointToCheck) const;

    /**
     * @brief Check if a line intersects the polygon.
     *
     * @param line The line to check.
     * @return True if the line intersects the polygon, false otherwise.
     */
    bool intersects(const std::shared_ptr<GLine> line);


    /**
     * @brief Offset the outer polygon boundary by a given distance.
     *
     * @param inward True to offset inward, false for outward.
     * @param offset The distance to offset.
     */
    void transformOuterBoundary(bool inward,
                                units::length::meter_t offset);

    /**
     * @brief Offset all the inner holes polygon boundaries by a given distance.
     *
     * @param inward True to offset inward, false for outward.
     * @param offset The distance to offset.
     */
    void transformInnerHolesBoundaries(bool inward,
                                       units::length::meter_t offset);

    /**
     * @brief Get the maximum clear width of a line inside the polygon.
     *
     * @param line The line to check.
     * @return The maximum clear width of the line inside the polygon.
     */
    units::length::meter_t getMaxClearWidth(const GLine &line) const;

    /**
     * @brief Get the area of the polygon.
     *
     * @return The area of the polygon.
     */
    units::area::square_meter_t area() const;

    /**
     * @brief Get the perimeter of the polygon.
     *
     * @return The perimeter of the polygon.
     */
    units::length::meter_t perimeter() const;

    /**
     * @brief Get a string representation of the polygon.
     *
     * @return A string representing the polygon.
     */
    QString toString() const override;

    /**
     * @brief Check if the polygon contains the point either in their
     *          boundary or inner holes structure.
     * @param point The point to check
     * @return True if the point is found, false otherwise.
     */
    bool contains(std::shared_ptr<GPoint> point) const;
};
};
#endif // POLYGON_H
