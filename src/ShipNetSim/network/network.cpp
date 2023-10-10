#include "network.h"

Network::Network() : QObject(nullptr)
{

}

Network::Network(std::shared_ptr<Polygon> waterBoundries,
                 QString regionName) : QObject(nullptr)
{
    mWaterBoundries = waterBoundries;
    mVisibilityGraph = std::make_shared<VisibilityGraph>(mWaterBoundries);
    mRegionName = regionName;
}

Network::~Network()
{
}

void Network::seWaterBoundries(std::shared_ptr<Polygon> waterBoundries)
{
    mWaterBoundries = waterBoundries;
    mVisibilityGraph = std::make_shared<VisibilityGraph>(mWaterBoundries);
}

QString Network::getRegionkName()
{
    return mRegionName;
}

void Network::setRegionName(QString newName)
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
    mVisibilityGraph->buildGraph();

    return mVisibilityGraph->dijkstraShortestPath();
}


