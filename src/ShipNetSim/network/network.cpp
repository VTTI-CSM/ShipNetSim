#include "network.h"

Network::Network()
{

}

Network::Network(std::shared_ptr<Polygon> waterBoundries,
        std::string regionName)
{
    mWaterBoundries = waterBoundries;
    mVisibilityGraph = std::make_shared<VisibilityGraph>(mWaterBoundries);
    mRegionName = regionName;
}

void Network::seWaterBoundries(std::shared_ptr<Polygon> waterBoundries)
{
    mWaterBoundries = waterBoundries;
    mVisibilityGraph = std::make_shared<VisibilityGraph>(mWaterBoundries);
}

void Network::setRegionName(std::string newName)
{
    mRegionName = newName;
}

ShortestPathResult Network::dijkstraShortestPath(
    std::shared_ptr<Point> startPoint,
    std::shared_ptr<Point> endpoint)
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
Network::generateCumLinesLengths(std::shared_ptr<Ship> ship)
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
