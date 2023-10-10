#ifndef ALGEBRAICVECTOR_H
#define ALGEBRAICVECTOR_H

#include <QVector>
#include "point.h"
#include "../../third_party/units/units.h"

class AlgebraicVector
{
public:
    AlgebraicVector();
    AlgebraicVector(const Point startPoint, const Point& endPoint);
//    AlgebraicVector(const Point startPoint);
//    void changeHeading(units::angle::degree_t deltaThetaDegrees);

    void setTargetAndMaxROT(const Point& target,
                            units::angle::degree_t maxROTPerSec);

    void moveByDistance(units::length::meter_t distance,
                        units::time::second_t timeStep);
    units::angle::degree_t orientation() const;
    Point getCurrentPosition();
    bool isRotating() const;
    units::angle::degree_t angleTo(const Point& otherPoint) const;

private:
    Point mTargetPoint_;
    units::angle::degree_t mMaxROTPerSec_;
    Point mPosition_;
    QVector<units::length::meter_t> mOrientation;
    bool mIsRotating;

    void setOrientationByEndPoint(const Point& endPoint);
    void rotateToTargetByMaxROT(const Point& target,
                                units::angle::degree_t maxROTPerSec,
                                units::time::second_t timeStep);
};

#endif // ALGEBRAICVECTOR_H
