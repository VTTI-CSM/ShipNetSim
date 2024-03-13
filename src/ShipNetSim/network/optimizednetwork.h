#ifndef OPTIMIZEDNETWORK_H
#define OPTIMIZEDNETWORK_H


#include "polygon.h"
#include "optimizedvisibilitygraph.h"
#include "QObject"
#include "seaport.h"
// #include "tinytiffreader.hxx"
#include "./algebraicvector.h"


class Ship;

class OptimizedNetwork : QObject
{
    Q_OBJECT
private:

    static const QVector<QString>& SalinityTiffLocations() {
        static QVector<QString> v
            {
                "D:/OneDrive - Virginia Tech/03.Work/02.VTTI/02.ResearchWork/"
                "04.ShipModelling/01.Code/00.CPP/ShipNetSim/src/data/"
                "salinity.tif"
            };
        return v;
    };

    static const QVector<QString>& WaveHeightTiffLocations() {
        static QVector<QString> v
            {
                "D:/OneDrive - Virginia Tech/03.Work/02.VTTI/02.ResearchWork/"
                "04.ShipModelling/01.Code/00.CPP/ShipNetSim/src/data/"
                "waveHeight.tif"
            };
        return v;
    };

    static const QVector<QString>& wavePeriodTiffLocations() {
        static QVector<QString> v
            {
                "D:/OneDrive - Virginia Tech/03.Work/02.VTTI/02.ResearchWork/"
                "04.ShipModelling/01.Code/00.CPP/ShipNetSim/src/data/"
                "wavePeriod.tif"
            };
        return v;
    };

    static const QVector<QString>& windSpeedNorthTiffLocations() {
        static QVector<QString> v
            {
                "D:/OneDrive - Virginia Tech/03.Work/02.VTTI/02.ResearchWork/"
                "04.ShipModelling/01.Code/00.CPP/ShipNetSim/src/data/"
                "WindSpeedNorth.tif"
            };
        return v;
    };

    static const QVector<QString>& windSpeedEastTiffLocations() {
        static QVector<QString> v
            {
                "D:/OneDrive - Virginia Tech/03.Work/02.VTTI/02.ResearchWork/"
                "04.ShipModelling/01.Code/00.CPP/ShipNetSim/src/data/"
                "WindSpeedEast.tif"
            };
        return v;
    };

    static const QVector<QString>& waterDepthTiffLocations() {
        static QVector<QString> v
            {
                "D:/OneDrive - Virginia Tech/03.Work/02.VTTI/02.ResearchWork/"
                "04.ShipModelling/01.Code/00.CPP/ShipNetSim/src/data/"
                "WaterDepth.tif"
            };
        return v;
    };

    static const QVector<QString>& seaPortsLocations() {
        static QVector<QString> v
            {
                "D:/OneDrive - Virginia Tech/03.Work/02.VTTI/02.ResearchWork/"
                "04.ShipModelling/01.Code/00.CPP/ShipNetSim/src/data/"
                "ports.geojson"
            };
        return v;
    }


    bool loadFirstAvailableTiffFile(std::vector<std::vector<float>>& variable,
        QVector<QString> locations);
    bool loadFirstAvailableSeaPortsFile(QVector<QString> locations);

    QVector<std::shared_ptr<Polygon>> mBoundaries;

    // The visibility graph representing navigable paths in the region.
    std::shared_ptr<OptimizedVisibilityGraph> mVisibilityGraph;

    // Differentiate between land and water bodies
    BoundariesType mBoundaryType;

    // Holds the sea ports
    QVector<std::shared_ptr<SeaPort>> mSeaPorts;

    std::vector<std::vector<float>> salinity;
    std::vector<std::vector<float>> waveHeight;
    std::vector<std::vector<float>> wavePeriod;
    std::vector<std::vector<float>> windNorth;
    std::vector<std::vector<float>> windEast;
    std::vector<std::vector<float>> waterDepth;

    // The name of the region.
    QString mRegionName;

    std::vector<std::vector<float>> readTIFFAs2DArray(const char* filename);
    QVector<std::shared_ptr<SeaPort>> readSeaPorts(const char* filename);

    std::pair<size_t, size_t> mapCoordinateTo2DArray(
        GPoint coord, GPoint mapMinPoint, GPoint mapMaxPoint,
        size_t arrayWidth, size_t arrayHeight);

    void loadTxtFile(const QString& filename);
    void loadShapeFile(const QString& filepath);
    void loadTiffData();

    // // Private method to check if a point is within a given polygon.
    // bool containsPoint(const QVector<std::shared_ptr<Point>>& polygon,
    //                    const Point& pt);
public:
    // Default constructor.
    OptimizedNetwork();

    OptimizedNetwork(QVector<std::shared_ptr<Polygon>> boundaries,
                     BoundariesType boundariesType,
                     QString regionName = "");

    // Constructor to load network data from a file.
    OptimizedNetwork(QString filename);

    AlgebraicVector::Environment getEnvironmentFromPosition(GPoint p);

    // Destructor.
    ~OptimizedNetwork();

    // Sets the water boundaries of the region.
    void setBoundaries(QVector<std::shared_ptr<Polygon>> boundaries);

    // Gets the name of the region.
    [[nodiscard]] QString getRegionName();

    // Sets the name of the region.
    void setRegionName(QString newName);

    /**
     * @brief Calculates the shortest path between two points
     *          using dijkstraShortestPath
     * @param startPoint The start point of the shortest path.
     * @param endpoint The end point of the shortest path.
     * @return A ShortestPathResult struct of 2 vectors
     *          1) points Vector: All points along the shortest path
     *          2) Links Vector : All Links along the shortest path
     */
    [[nodiscard]] ShortestPathResult findShortestPath(
        std::shared_ptr<GPoint> startPoint,
        std::shared_ptr<GPoint> endpoint,
        PathFindingAlgorithm algorithm);

    /**
     * @brief Calculates the shortest path between points of a
     *          vector using dijkstraShortestPath
     * @param points A vector of must traversed points.
     * @return A ShortestPathResult struct of 2 vectors
     *          1) points Vector: All points along the shortest path
     *          2) Links Vector : All Links along the shortest path
     */
    [[nodiscard]] ShortestPathResult findShortestPath(
        QVector<std::shared_ptr<GPoint>> points,
        PathFindingAlgorithm algorithm);
};

#endif // OPTIMIZEDNETWORK_H
