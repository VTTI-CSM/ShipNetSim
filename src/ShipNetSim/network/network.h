#ifndef NETWORK_H
#define NETWORK_H


#include "polygon.h"
#include "../ship/ship.h"
#include "visibilitygraph.h"
class Network
{
private:
    std::shared_ptr<Polygon> mWaterBoundries;
    std::shared_ptr<VisibilityGraph> mVisibilityGraph;
    std::string mRegionName;
public:
    Network();
    Network(std::shared_ptr<Polygon> waterBoundries,
            std::string regionName = "");
    ~Network();
    void seWaterBoundries(std::shared_ptr<Polygon> waterBoundries);
    void setRegionName(std::string newName);
    ShortestPathResult dijkstraShortestPath(std::shared_ptr<Point> startPoint,
                                             std::shared_ptr<Point> endpoint);

    std::vector<units::length::meter_t>
    generateCumLinesLengths(std::shared_ptr<Ship> ship);

    long double getTotalPathLength(std::shared_ptr<Ship> ship);
    Point getPositionByTravelledDistance(std::shared_ptr<Ship> ship);

};

#endif // NETWORK_H
