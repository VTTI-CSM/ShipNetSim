#ifndef NETWORK_H
#define NETWORK_H


#include "polygon.h"
#include "visibilitygraph.h"
#include "QObject"

class Ship;

class Network : QObject
{
    Q_OBJECT

private:
    std::shared_ptr<Polygon> mWaterBoundries;
    std::shared_ptr<VisibilityGraph> mVisibilityGraph;
    QString mRegionName;
public:
    Network();
    Network(std::shared_ptr<Polygon> waterBoundries,
            QString regionName = "");
    ~Network();
    void seWaterBoundries(std::shared_ptr<Polygon> waterBoundries);
    QString getRegionkName();
    void setRegionName(QString newName);
    ShortestPathResult dijkstraShortestPath(std::shared_ptr<Point> startPoint,
                                             std::shared_ptr<Point> endpoint);

};

#endif // NETWORK_H
