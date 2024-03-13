#ifndef BOUNDINGBOX_H
#define BOUNDINGBOX_H

#include "polygon.h"

class BoundingBox : private Polygon
{
public:
    BoundingBox();

    BoundingBox(Point btmLft, Point tpRgt);

    BoundingBox(Point btmLft, Point btmRgt, Point tpRgt, Point tpLft);

    void setTpRgtPoint(Point tpRgt);

    void setBtmLftPoint(Point btmLft);


    // Wrapper method to access Polygon's intersects method
    bool intersects(const std::shared_ptr<Line> &line);
};

#endif // BOUNDINGBOX_H
