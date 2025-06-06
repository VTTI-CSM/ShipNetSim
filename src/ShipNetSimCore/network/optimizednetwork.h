#ifndef OPTIMIZEDNETWORK_H
#define OPTIMIZEDNETWORK_H

#include "../export.h"
#include "./algebraicvector.h"
#include "QObject"
#include "optimizedvisibilitygraph.h"
#include "polygon.h"
#include "seaport.h"

namespace ShipNetSimCore
{
class Ship;

struct tiffFileData
{
    /**
     * Array to hold geotransformation parameters of a raster dataset.
     * These parameters define the affine transformation that maps the
     * pixel/line coordinates of the raster into georeferenced space.
     * The array consists of six double precision values, each
     * representing a specific parameter in the transformation
     * process:
     *
     * adfGeoTransform[0] - Top left x-coordinate of the raster. This
     * is the x-coordinate of the upper-left corner of the top-left
     * pixel in the raster, which represents the georeferenced
     * longitude (in a geographic coordinate system) or easting (in a
     * projected coordinate system) of that point.
     *
     * adfGeoTransform[1] - Pixel width in the georeferenced units of
     * the raster's coordinate system. This is the distance in the
     * x-direction (longitude or easting) covered by one pixel in the
     * raster.
     *
     * adfGeoTransform[2] - Rotation parameter 1, which represents the
     *                      row rotation about the top-left corner of
     *                      the raster. For north-up images,
     *                      this is typically 0.
     *
     * adfGeoTransform[3] - Top left y-coordinate of the raster. This
     * is the y-coordinate of the upper-left corner of the top-left
     * pixel in the raster, which represents the georeferenced
     * latitude (in a geographic coordinate system) or northing (in a
     * projected coordinate system) of that point.
     *
     * adfGeoTransform[4] - Rotation parameter 2, which represents the
     *                      column rotation about the top-left
     *                      corner of the raster. For north-up images,
     *                      this is typically 0.
     *
     * adfGeoTransform[5] - Pixel height in the georeferenced units of
     *                      the raster's coordinate system,
     *                      which is the distance in the y-direction
     *                      (latitude or northing) covered
     *                      by one pixel in the raster. For north-up
     *                      images, this value is typically
     *                      negative, indicating that the numeric
     * value of the y-coordinate decreases as you move down the rows
     * of the raster.
     *
     * Together, these parameters are used to convert between
     * pixel/line (row/column) coordinates and geographic coordinates.
     * It is essential for tasks such as georeferencing, where the
     * spatial location of the raster data in a coordinate system is
     * defined, and for accurately overlaying raster data in mapping
     * applications.
     */
    double adfGeoTransform[6];

    /**
     * Holds the tiff dataset from GDAL
     */
    GDALDataset *dataset;

    /**
     * Holds the tiff band from GDAL
     */
    GDALRasterBand *band;

    double minLong; ///< Minimum longitude (x)
    double maxLong; ///< Maximum longitude (x)
    double minLat;  ///< Minimum latitude (y)
    double maxLat;  ///< Maximum latitude (y)

    tiffFileData()
        : dataset(nullptr)
        , band(nullptr)
        , minLong(std::nan("no value"))
        , maxLong(std::nan("no value"))
        , minLat(std::nan("no value"))
        , maxLat(std::nan("no value"))
    {
        std::fill_n(adfGeoTransform, 6, std::nan("no value"));
    }

    ~tiffFileData()
    {
        // Cleanup
        GDALClose(dataset);
    }

    /**
     * Calculate min/max longitude and latitude from the geo-transform
     * and dataset size
     */
    void calculateGeographicExtents()
    {
        if (!dataset)
        {
            throw std::runtime_error(
                "Dataset is null, "
                "cannot calculate geographic extents.");
            return;
        }

        // Get image dimensions (width and height)
        int rasterWidth  = dataset->GetRasterXSize();
        int rasterHeight = dataset->GetRasterYSize();

        // Extract geo-transform values
        double *gt = adfGeoTransform;

        // Top-left corner (origin)
        minLong = gt[0]; // Top-left x (longitude)
        maxLat  = gt[3]; // Top-left y (latitude)

        // Bottom-right corner
        maxLong = gt[0] + rasterWidth * gt[1] + rasterHeight * gt[2];
        minLat  = gt[3] + rasterWidth * gt[4] + rasterHeight * gt[5];
    }
};

class SHIPNETSIM_EXPORT OptimizedNetwork : public QObject
{
    Q_OBJECT
private:
    // Hold the tiff data
    tiffFileData salinityTiffData;
    tiffFileData waveHeightTiffData;
    tiffFileData wavePeriodTiffData;
    tiffFileData windNorthTiffData;
    tiffFileData windEastTiffData;
    tiffFileData waterDepthTiffData;

    // Holds all the boundaries as polygons
    QVector<std::shared_ptr<Polygon>> mBoundaries;

    // The visibility graph representing navigable paths in the
    // region.
    std::shared_ptr<OptimizedVisibilityGraph> mVisibilityGraph;

    // Differentiate between land and water bodies
    BoundariesType mBoundaryType;

    // Holds the sea ports
    QVector<std::shared_ptr<SeaPort>> mSeaPorts;

    // The name of the region.
    QString mRegionName;

    // Loads the TIFF file
    bool loadFirstAvailableTiffFile(tiffFileData    &variable,
                                    QVector<QString> locations);

    // reads the tiff file as a GDAL dataset
    GDALDataset *readTIFFFile(const char *filename);

    // convert the coordinates to its equivilant tiff x, y indicies
    std::pair<size_t, size_t>
    mapCoordinatesToTiffIndecies(tiffFileData &data, GPoint p);

    // load the text file for the structure
    void loadTxtFile(const QString &filename);

    // load the shape file for the structure
    void loadPolygonShapeFile(const QString &filepath);

    // load the tiff data
    void loadTiffData();

public:
    // Default constructor.
    OptimizedNetwork();

    // Full constructor
    OptimizedNetwork(QVector<std::shared_ptr<Polygon>> boundaries,
                     BoundariesType                    boundariesType,
                     QString regionName = "");

    void initializeNetwork(QString filename, QString regionName);

    void
    initializeNetwork(QVector<std::shared_ptr<Polygon>> boundaries,
                      BoundariesType boundariesType,
                      QString        regionName = "");

    // Constructor to load network data from a file.
    OptimizedNetwork(QString filename, QString regionName);

    AlgebraicVector::Environment getEnvironmentFromPosition(GPoint p);

    void moveObjectToThread(QThread *thread);

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
    [[nodiscard]] ShortestPathResult
    findShortestPath(std::shared_ptr<GPoint> startPoint,
                     std::shared_ptr<GPoint> endpoint,
                     PathFindingAlgorithm    algorithm);

    /**
     * @brief Calculates the shortest path between points of a
     *          vector using dijkstraShortestPath
     * @param points A vector of must traversed points.
     * @return A ShortestPathResult struct of 2 vectors
     *          1) points Vector: All points along the shortest path
     *          2) Links Vector : All Links along the shortest path
     */
    [[nodiscard]] ShortestPathResult
    findShortestPath(QVector<std::shared_ptr<GPoint>> points,
                     PathFindingAlgorithm             algorithm);

signals:
    void NetworkLoaded();
    void errorOccurred(QString error);
};
}; // namespace ShipNetSimCore
#endif // OPTIMIZEDNETWORK_H
