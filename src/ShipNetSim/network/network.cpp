#include "network.h"

Network::Network()
{

}

Network::Network(Polygon *waterBoundries,
        std::string regionName)
{
    mWaterBoundries = waterBoundries;
    mVisibilityGraph = new VisibilityGraph(mWaterBoundries);
    mRegionName = regionName;
}

void Network::seWaterBoundries(Polygon *waterBoundries)
{
    mWaterBoundries = waterBoundries;
    mVisibilityGraph = new VisibilityGraph(mWaterBoundries);
}

void Network::setRegionName(std::string newName)
{
    mRegionName = newName;
}

std::vector<std::shared_ptr<Line>> Network::dijkstraShortestPath(
    Point *startPoint,
    Point *endpoint)
{
    if (!mWaterBoundries)
    {
        throw std::exception("Water boundry is not defined yet!");
    }

    mVisibilityGraph->setStartPoint(startPoint);
    mVisibilityGraph->setEndPoint(endpoint);
    mVisibilityGraph->buildGraph(mWaterBoundries->getMaxAllowedSpeed());

    return mVisibilityGraph->dijkstraShortestPath();
}

std::vector<units::length::meter_t>
Network::generateCumLinesLengths(Ship *ship)
{
    int n = ship->shipPath().size();
    std::vector<units::length::meter_t>
        linksCumLengths(n, units::length::meter_t(0.0));

    for (std::size_t i = 1; i < n; i++)
    {
        linksCumLengths[i] = linksCumLengths[i-1] +
                             units::length::meter_t(
                                 ship->shipPath().at(i-1)->length());
    }

    return linksCumLengths;
}
