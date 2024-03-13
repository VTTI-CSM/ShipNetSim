#include "boundingbox.h"

BoundingBox::BoundingBox() {}

BoundingBox::BoundingBox(Point btmLft, Point tpRgt)
    : Polygon(
        // bottom left
        {std::make_shared<Point>(btmLft),
         // bottom right
         std::make_shared<Point>(Point(btmLft.x() +
                                           units::math::abs(tpRgt.x() -
                                                            btmLft.x()),
                                       btmLft.y())),
         // top right
         std::make_shared<Point>(tpRgt),
         // top left
         std::make_shared<Point>(Point(btmLft.x(), tpRgt.y()))}
        ) {}


BoundingBox::BoundingBox(Point btmLft, Point btmRgt, Point tpRgt, Point tpLft)
    : Polygon(
        {std::make_shared<Point>(btmLft),
         std::make_shared<Point>(btmRgt),
         std::make_shared<Point>(tpRgt),
         std::make_shared<Point>(tpLft)}
        ) {}

void BoundingBox::setTpRgtPoint(Point tpRgt)
{
    auto boundary = Polygon::outer();
    auto btmLft = *(boundary[0]);
    auto newBoundary =
        {std::make_shared<Point>(btmLft),
         // bottom right
         std::make_shared<Point>(Point(btmLft.x() +
                                           units::math::abs(tpRgt.x() -
                                                            btmLft.x()),
                                       btmLft.y())),
         // top right
         std::make_shared<Point>(tpRgt),
         // top left
         std::make_shared<Point>(Point(btmLft.x(), tpRgt.y()))};
    Polygon::setOuterPoints(newBoundary);
}

void BoundingBox::setBtmLftPoint(Point btmLft)
{
    auto boundary = Polygon::outer();
    auto tpRgt = *(boundary[2]);
    auto newBoundary =
        {std::make_shared<Point>(btmLft),
         // bottom right
         std::make_shared<Point>(Point(btmLft.x() +
                                           units::math::abs(tpRgt.x() -
                                                            btmLft.x()),
                                       btmLft.y())),
         // top right
         std::make_shared<Point>(tpRgt),
         // top left
         std::make_shared<Point>(Point(btmLft.x(), tpRgt.y()))};
    Polygon::setOuterPoints(newBoundary);
}


bool BoundingBox::intersects(const std::shared_ptr<Line> &line)
{
    return Polygon::intersects(line);
}

