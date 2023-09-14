#ifndef NETWORK_H
#define NETWORK_H


#include "line.h"
#include "polygon.h"
#include "../ship/ship.h"
#include "visibilitygraph.h"
class Network
{
private:
    Polygon *mWaterBoundries;
    VisibilityGraph *mVisibilityGraph;
    std::string mRegionName;
public:
    Network();
    Network(Polygon *waterBoundries,
            std::string regionName = "");
    ~Network();
    void seWaterBoundries(Polygon *waterBoundries);
    void setRegionName(std::string newName);
    std::vector<std::shared_ptr<Line>> dijkstraShortestPath(Point *startPoint,
                                             Point *endpoint);

    std::vector<units::length::meter_t>
    generateCumLinesLengths(Ship *ship);

    long double getTotalPathLength(const Ship *ship);
    Point getPositionByTravelledDistance(const Ship *ship);

};

#endif // NETWORK_H
