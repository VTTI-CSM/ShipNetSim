#include "optimizednetwork.h"
#include "../utils/utils.h"
#include "gpoint.h"
#include "networkdefaults.h"
#include "seaportloader.h"
#include <QCoreApplication> // Core application utilities
#include <QFile>            // File input/output utilities
#include <QRegularExpression>
#include <QTextStream> // Text stream utilities
#include <gdal_priv.h>
#include <ogrsf_frmts.h>

namespace ShipNetSimCore
{

bool OptimizedNetwork::loadFirstAvailableTiffFile(
    tiffFileData &variable, QVector<QString> locations)
{
    QString loc = Utils::getFirstExistingPathFromList(
        locations, QVector<QString>{"tif", "tiff"});

    if (loc == "")
    {
        emit errorOccurred("Could not find the tiff file");
        return false;
    }
    variable.dataset = readTIFFFile(loc.toStdString().c_str());
    if (variable.dataset->GetGeoTransform(variable.adfGeoTransform)
        != CE_None)
    {
        emit errorOccurred("Tiff file may not have an assigned SRS!");
        return false;
    }

    // Calculate geographic extents (min/max lat/long)
    variable.calculateGeographicExtents();

    variable.band = variable.dataset->GetRasterBand(1);
    return true;
}

OptimizedNetwork::OptimizedNetwork() {}

void OptimizedNetwork::initializeNetwork(QString filename,
                                         QString regionName)
{
    GDALAllRegister();

    QFileInfo fileInfo(filename);

    // Check if the file exists before trying to get its extension
    if (!fileInfo.exists())
    {
        emit errorOccurred("File does not exist.");
        return;
    }

    // Get the extension and convert it to lower case
    // for case-insensitive comparison
    QString extension = fileInfo.suffix().toLower();

    // text based file
    if (extension == "txt" || extension == "dat")
    {
        loadTxtFile(filename);
    }
    // shp file
    else if (extension == "shp")
    {
        loadPolygonShapeFile(filename);
    }
    // other file extensions are not yet supported
    else
    {
        emit errorOccurred("file type is not supported!");
        return;
    }

    // load the sea ports
    bool loadedSeaPorts =
        SeaPortLoader::loadFirstAvailableSeaPortsFile(
            NetworkDefaults::seaPortsLocations(
                Utils::getDataDirectory()));
    mSeaPorts = SeaPortLoader::getPorts();
    if (!loadedSeaPorts)
    {
        emit errorOccurred("Sea Ports file could not be loaded!");
        return;
    }
    else
    {
        mVisibilityGraph->loadSeaPortsPolygonCoordinates(mSeaPorts);
    }

    // mVisibilityGraph->buildGraph();

    loadTiffData();

    mRegionName = regionName;

    // send signal that the network is loaded and valid
    emit NetworkLoaded();
}

void OptimizedNetwork::initializeNetwork(
    QVector<std::shared_ptr<Polygon>> boundaries,
    BoundariesType boundariesType, QString regionName)
{
    GDALAllRegister();

    mVisibilityGraph = std::make_shared<OptimizedVisibilityGraph>(
        std::as_const(mBoundaries), boundariesType);

    mRegionName = regionName; // Set region name

    // load the sea ports
    bool loadedSeaPorts =
        SeaPortLoader::loadFirstAvailableSeaPortsFile(
            NetworkDefaults::seaPortsLocations(
                Utils::getDataDirectory()));
    mSeaPorts = SeaPortLoader::getPorts();
    if (!loadedSeaPorts)
    {
        emit errorOccurred("Sea Ports file could not be loaded!");
        return;
    }
    else
    {
        mVisibilityGraph->loadSeaPortsPolygonCoordinates(mSeaPorts);
    }

    loadTiffData();

    emit NetworkLoaded();
}

OptimizedNetwork::OptimizedNetwork(
    QVector<std::shared_ptr<Polygon>> boundaries,
    BoundariesType boundariesType, QString regionName)
    : mBoundaries(boundaries)
    , mBoundaryType(boundariesType)
    , mRegionName(regionName)
{
    initializeNetwork(boundaries, boundariesType, regionName);
}

OptimizedNetwork::~OptimizedNetwork() {}

OptimizedNetwork::OptimizedNetwork(QString filename,
                                   QString regionName)
{
    initializeNetwork(filename, regionName);
}

void OptimizedNetwork::moveObjectToThread(QThread *thread)
{
    // Move Simulator object itself to the thread
    this->moveToThread(thread);

    if (mVisibilityGraph && mVisibilityGraph->parent() == nullptr)
    {
        mVisibilityGraph->moveToThread(thread);
    }
}

void OptimizedNetwork::loadTxtFile(const QString &filename)
{
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly))
    {
        emit errorOccurred("Failed to open the network file.");
        return;
    }

    QTextStream in(&file);
    QString     line; // Store each line from file

    // Outer boundary for current water body
    QVector<std::shared_ptr<GPoint>> outerBoundary;
    // Holes (land boundaries) for current water body
    QVector<QVector<std::shared_ptr<GPoint>>> holes;
    // Currently reading boundary (either outer boundary or hole)
    QVector<std::shared_ptr<GPoint>> currentBoundary;
    // Current section of the file being parsed
    QString currentSection;
    // ID of the current water body
    QString waterBodyId = "";

    QRegularExpression waterBodyRegex(
        "\\[\\s*WATERBODY\\s+(\\d+)\\s*\\]",
        QRegularExpression::CaseInsensitiveOption);
    QRegularExpression sectionRegex(
        "\\[\\s*([\\w\\s_]+)\\s*\\]",
        QRegularExpression::CaseInsensitiveOption);
    QRegularExpression pointRegex(
        "^(\\d+),\\s*(-?\\d+\\.?\\d*),\\s*(-?\\d+\\.?\\d*)$",
        QRegularExpression::CaseInsensitiveOption);

    auto cleanLine = [&in, &line](bool loadNextLine = true) {
        if (loadNextLine)
        {
            line = in.readLine();
        }
        // Remove comments (everything after '#')
        int commentIndex = line.indexOf('#');
        if (commentIndex != -1)
        {
            line = line.left(commentIndex).trimmed();
        }
    };

    while (!in.atEnd())
    {

        cleanLine(true);

        if (line.isEmpty())
        {
            continue; // Skip empty or comment-only lines
        }

        QRegularExpressionMatch waterBodyMatch =
            waterBodyRegex.match(line);

        if (waterBodyMatch.hasMatch())
        {
            if (!outerBoundary.isEmpty() || !holes.isEmpty())
            {
                try
                {
                    auto polygon = std::make_shared<Polygon>(
                        outerBoundary, holes, waterBodyId);
                    mBoundaries.push_back(polygon);
                }
                catch (const std::exception &e)
                {
                    QString errorMsg =
                        QString("Failed to create Polygon: %1")
                            .arg(e.what());
                    emit errorOccurred(errorMsg);
                    throw;
                }
                outerBoundary.clear();
                holes.clear();
            }
            waterBodyId = waterBodyMatch.captured(1);
        }
        else
        {
            QRegularExpressionMatch sectionMatch =
                sectionRegex.match(line);

            if (sectionMatch.hasMatch())
            {
                QString section = sectionMatch.captured(1).toLower();
                if (section == "water boundry")
                {
                    currentSection = section;
                    currentBoundary.clear();
                }
                else if (section == "land")
                {
                    currentSection = section;
                    currentBoundary.clear();
                }
                else if (section == "end")
                {
                    if (currentSection == "water boundry")
                    {
                        outerBoundary = currentBoundary;
                    }
                    else if (currentSection == "land")
                    {
                        holes.push_back(currentBoundary);
                    }
                    currentSection.clear();
                    currentBoundary.clear();
                }
            }
            else if (pointRegex.match(line).hasMatch()
                     && (!currentSection.isEmpty()))
            {
                QRegularExpressionMatch pointMatch =
                    pointRegex.match(line);

                QString id  = pointMatch.captured(1);
                double  lon = pointMatch.captured(2).toDouble();
                double  lat = pointMatch.captured(3).toDouble();

                try
                {
                    std::shared_ptr<GPoint> pt =
                        std::make_shared<GPoint>(
                            units::angle::degree_t(lon),
                            units::angle::degree_t(lat), id);
                    currentBoundary.append(pt);
                }
                catch (const std::exception &e)
                {
                    QString errorMsg =
                        QString("Failed to create GPoint: %1")
                            .arg(e.what());
                    emit errorOccurred(errorMsg);
                    throw; // Rethrow the exception after emitting the
                           // signal
                }
            }
            else
            {
                // Handle invalid or unexpected lines
                if (!line.isEmpty())
                {
                    emit errorOccurred("Unexpected format or "
                                       "content in line:"
                                       + line);
                }
            }
        }
    }

    // Finalize the last water body
    if (!outerBoundary.isEmpty() || !holes.isEmpty())
    {
        try
        {
            auto polygon = std::make_shared<Polygon>(
                outerBoundary, holes, waterBodyId);
            mBoundaries.push_back(polygon);
        }
        catch (const std::exception &e)
        {
            QString errorMsg =
                QString("Failed to create Polygon: %1").arg(e.what());
            emit errorOccurred(errorMsg);
            throw; // Rethrow the exception after emitting the signal
        }
    }

    file.close();

    try
    {
        mVisibilityGraph = std::make_shared<OptimizedVisibilityGraph>(
            std::as_const(mBoundaries), BoundariesType::Water);
    }
    catch (const std::exception &e)
    {
        QString errorMsg = QString("Failed to create"
                                   " OptimizedVisibilityGraph: %1")
                               .arg(e.what());
        emit errorOccurred(errorMsg);
        throw;
    }
}

void OptimizedNetwork::loadPolygonShapeFile(const QString &filepath)
{

    mBoundaries.clear();
    GDALDataset *poDS = static_cast<GDALDataset *>(
        GDALOpenEx(filepath.toStdString().c_str(), GDAL_OF_VECTOR,
                   NULL, NULL, NULL));
    if (poDS == NULL)
    {
        emit errorOccurred("Open shaefile failed.");
        return;
    }

    OGRLayer *poLayer = poDS->GetLayer(0);

    // Validate that the spatial reference is WGS84 (EPSG:4326)
    // All geodetic calculations in this application assume WGS84 ellipsoid
    const OGRSpatialReference *poSRS = poLayer->GetSpatialRef();
    if (poSRS == NULL)
    {
        GDALClose(poDS);
        emit errorOccurred(
            "Spatial reference system is unknown. "
            "Only WGS84 (EPSG:4326) is supported.");
        return;
    }

    // Create WGS84 reference for comparison
    OGRSpatialReference wgs84SRS;
    wgs84SRS.SetWellKnownGeogCS("WGS84");

    // Check if the shapefile's CRS matches WGS84
    if (!poSRS->IsSameGeogCS(&wgs84SRS))
    {
        GDALClose(poDS);
        emit errorOccurred(
            "Only WGS84 (EPSG:4326) coordinate reference system is supported. "
            "Please convert your shapefile using: "
            "ogr2ogr -t_srs EPSG:4326 output.shp input.shp");
        return;
    }

    OGRFeature *poFeature;

    poLayer->ResetReading();

    int shapeID = 0;
    while ((poFeature = poLayer->GetNextFeature()) != NULL)
    {
        OGRGeometry *poGeometry;
        poGeometry = poFeature->GetGeometryRef();
        if (poGeometry != NULL
            && wkbFlatten(poGeometry->getGeometryType())
                   == wkbPolygon)
        {
            OGRPolygon *poPolygon =
                static_cast<OGRPolygon *>(poGeometry);
            OGRLinearRing *poExteriorRing =
                poPolygon->getExteriorRing();

            // read exterior ring
            QVector<std::shared_ptr<GPoint>> exterirorRinge =
                QVector<std::shared_ptr<GPoint>>();
            if (poExteriorRing != NULL)
            {
                int nPoints = poExteriorRing->getNumPoints();
                for (int i = 0; i < nPoints; i++)
                {
                    double x = poExteriorRing->getX(i);
                    double y = poExteriorRing->getY(i);

                    try
                    {
                        exterirorRinge.push_back(
                            std::make_shared<GPoint>(
                                GPoint(units::angle::degree_t(x),
                                       units::angle::degree_t(y))));
                    }
                    catch (const std::exception &e)
                    {
                        QString errorMsg =
                            QString("Failed to create GPoint: %1")
                                .arg(e.what());
                        emit errorOccurred(errorMsg);
                        throw;
                    }
                }

                shapeID++; // increment the id
            }

            // read interior ring
            // Iterate over interior rings (holes) if necessary
            int nInteriorRings = poPolygon->getNumInteriorRings();
            QVector<QVector<std::shared_ptr<GPoint>>> innerHoles =
                QVector<QVector<std::shared_ptr<GPoint>>>(
                    nInteriorRings);

            for (int i = 0; i < nInteriorRings; i++)
            {
                OGRLinearRing *poInteriorRing =
                    poPolygon->getInteriorRing(i);
                int nPoints = poInteriorRing->getNumPoints();
                for (int j = 0; j < nPoints; j++)
                {
                    double x = poInteriorRing->getX(j);
                    double y = poInteriorRing->getY(j);

                    try
                    {
                        innerHoles[i].push_back(
                            std::make_shared<GPoint>(
                                GPoint(units::angle::degree_t(x),
                                       units::angle::degree_t(y))));
                    }
                    catch (const std::exception &e)
                    {
                        QString errorMsg =
                            QString("Failed to create GPoint: %1")
                                .arg(e.what());
                        emit errorOccurred(errorMsg);
                        throw;
                    }
                }
            }

            try
            {
                auto polygon = std::make_shared<Polygon>(
                    exterirorRinge, innerHoles,
                    QString::number(shapeID));
                mBoundaries.push_back(polygon);
            }
            catch (const std::exception &e)
            {
                QString errorMsg =
                    QString("Failed to create Polygon: %1")
                        .arg(e.what());
                emit errorOccurred(errorMsg);
                throw;
            }
        }
        OGRFeature::DestroyFeature(poFeature);
    }

    GDALClose(poDS);

    try
    {
        mVisibilityGraph = std::make_shared<OptimizedVisibilityGraph>(
            std::as_const(mBoundaries), BoundariesType::Water);
    }
    catch (const std::exception &e)
    {
        QString errorMsg =
            QString("Failed to create OptimizedVisibilityGraph: %1")
                .arg(e.what());
        emit errorOccurred(errorMsg);
        throw;
    }
}

void OptimizedNetwork::loadTiffData()
{
    loadFirstAvailableTiffFile(salinityTiffData,
                               NetworkDefaults::SalinityTiffLocations(
                                   Utils::getDataDirectory()));

    loadFirstAvailableTiffFile(
        waveHeightTiffData, NetworkDefaults::WaveHeightTiffLocations(
                                Utils::getDataDirectory()));

    loadFirstAvailableTiffFile(
        wavePeriodTiffData, NetworkDefaults::wavePeriodTiffLocations(
                                Utils::getDataDirectory()));

    loadFirstAvailableTiffFile(
        windNorthTiffData,
        NetworkDefaults::windSpeedNorthTiffLocations(
            Utils::getDataDirectory()));

    loadFirstAvailableTiffFile(
        windEastTiffData, NetworkDefaults::windSpeedEastTiffLocations(
                              Utils::getDataDirectory()));

    loadFirstAvailableTiffFile(
        waterDepthTiffData, NetworkDefaults::waterDepthTiffLocations(
                                Utils::getDataDirectory()));
}

std::pair<size_t, size_t>
OptimizedNetwork::mapCoordinatesToTiffIndecies(tiffFileData &data,
                                               GPoint        p)
{
    size_t pixelX, pixelY;

    // Get the longitude and latitude from the GPoint p
    double longitude = p.getLongitude().value();
    double latitude  = p.getLatitude().value();

    // Handle the longitude wrapping within the GeoTIFF's bounds
    // (minLong, maxLong)
    if (longitude < data.minLong)
    {
        longitude = data.maxLong - (data.minLong - longitude);
    }
    else if (longitude > data.maxLong)
    {
        longitude = data.minLong + (longitude - data.maxLong);
    }

    // Clamp the latitude to the nearest valid value if it goes
    // outside the bounds
    if (latitude < data.minLat)
    {
        latitude = data.minLat;
    }
    else if (latitude > data.maxLat)
    {
        latitude = data.maxLat;
    }

    // Adjust the coordinates using the geo-transform array
    double x = longitude - data.adfGeoTransform[0];
    double y = latitude - data.adfGeoTransform[3];

    // Compute the determinant inverse for transformation
    double inv_det =
        1.0
        / (data.adfGeoTransform[1] * data.adfGeoTransform[5]
           - data.adfGeoTransform[2] * data.adfGeoTransform[4]);

    // Use the inverse of the geo-transform matrix to map
    // coordinates to pixel indices
    pixelX = static_cast<size_t>(
        (x * data.adfGeoTransform[5] - y * data.adfGeoTransform[2])
        * inv_det);
    pixelY = static_cast<size_t>(
        (-x * data.adfGeoTransform[4] + y * data.adfGeoTransform[1])
        * inv_det);

    return std::make_pair(pixelX, pixelY);
}

GDALDataset *OptimizedNetwork::readTIFFFile(const char *filename)
{

    GDALDataset *dataset;
    GDALAllRegister();

    dataset = (GDALDataset *)GDALOpen(filename, GA_ReadOnly);
    if (dataset == nullptr)
    {
        emit errorOccurred(QString("Error opening file: ")
                           + filename);
        return {}; // Return an empty vector on failure
    }

    // Check for the number of bands in the TIFF file
    if (dataset->GetRasterCount() > 1)
    {
        GDALClose(dataset); // Close the dataset before throwing
        emit errorOccurred("TIFF file contains more than one band, "
                           "which is not supported.");
    }

    return dataset;
}

AlgebraicVector::Environment
OptimizedNetwork::getEnvironmentFromPosition(GPoint p)
{
    AlgebraicVector::Environment env;

    auto getValue = [this, &p](tiffFileData &data)
        -> float { // {new index.height, new index.width}
        auto   index = mapCoordinatesToTiffIndecies(data, p);
        float  value;
        double noDataValue = data.band->GetNoDataValue(nullptr);
        data.band->RasterIO(GF_Read, index.first, index.second, 1, 1,
                            &value, 1, 1, GDT_Float32, 0, 0);
        // Check if the value matches the NoData value
        if (std::isnan(value)
            || value == static_cast<float>(noDataValue))
        {
            return std::nan("noData"); // Return NaN
        }
        return value; // Otherwise, return the valid value
    };

    // Salinity (pptd)
    double salinityValue = getValue(salinityTiffData) / 100.0;
    salinityValue = (salinityValue > 1.0) ? 1.0 : salinityValue;
    env.salinity = units::concentration::pptd_t(salinityValue);

    // Wave height (meters)
    double waveHeightValue = getValue(waveHeightTiffData);
    env.waveHeight         = units::length::meter_t(waveHeightValue);

    // Wave period (seconds) and frequency (hertz)
    double wavePeriodValue = getValue(wavePeriodTiffData);
    double waveFrequency   = 0.0;
    if (std::isnan(wavePeriodValue))
    {
        waveFrequency = std::nan("noData");
    }
    else
    {
        waveFrequency = 1.0 / wavePeriodValue;
    }
    env.waveFrequency = units::frequency::hertz_t(waveFrequency);
    env.waveAngularFrequency =
        units::angular_velocity::radians_per_second_t(
            2.0 * units::constants::pi.value() * waveFrequency);

    // Wind speed (Northward and Eastward, m/s)
    double windSpeed_Northward = getValue(windNorthTiffData);
    env.windSpeed_Northward =
        units::velocity::meters_per_second_t(windSpeed_Northward);

    double windSpeed_Eastward = getValue(windEastTiffData);
    env.windSpeed_Eastward =
        units::velocity::meters_per_second_t(windSpeed_Eastward);

    // Calculate resultant wave speed and wavelength
    double waveLength;
    if (std::isnan(wavePeriodValue))
    {
        waveLength = std::nan("noData");
    }
    else
    {
        waveLength = (units::constants::g.value()
                      * std::pow(wavePeriodValue, 2.0))
                     / (2.0 * units::constants::pi);
    }
    env.waveLength = units::length::meter_t(waveLength);

    // Water depth (meters)
    double depthV  = getValue(waterDepthTiffData);
    env.waterDepth = units::length::meter_t(depthV);

    return env;
}

void OptimizedNetwork::setBoundaries(
    QVector<std::shared_ptr<Polygon>> boundaries)
{
    mBoundaries = boundaries;

    mVisibilityGraph->clear();
    OptimizedNetwork(
        mBoundaries, BoundariesType::Water,
        mRegionName); // Init Network with water body and region.
}

ShortestPathResult
OptimizedNetwork::findShortestPath(std::shared_ptr<GPoint> startPoint,
                                   std::shared_ptr<GPoint> endpoint,
                                   PathFindingAlgorithm    algorithm)
{
    if (!mVisibilityGraph)
    {
        emit errorOccurred("Visibility graph not initialized");
        return ShortestPathResult();
    }

    if (algorithm == PathFindingAlgorithm::AStar)
    {
        return mVisibilityGraph->findShortestPathAStar(startPoint,
                                                       endpoint);
    }
    else // if (algorithm == PathFindingAlgorithm::Dijkstra)
    {
        return mVisibilityGraph->findShortestPathDijkstra(startPoint,
                                                          endpoint);
    }
}

ShortestPathResult OptimizedNetwork::findShortestPath(
    QVector<std::shared_ptr<GPoint>> points,
    PathFindingAlgorithm             algorithm)
{
    // Check for null visibility graph
    if (!mVisibilityGraph)
    {
        emit errorOccurred("Visibility graph not initialized");
        return ShortestPathResult();
    }

    // Same-thread case
    if (QThread::currentThread() == this->thread())
    {
        return mVisibilityGraph->findShortestPath(points, algorithm);
    }

    // Cross-thread case - use QFuture for safer data passing
    QFutureInterface<ShortestPathResult> futureInterface;
    futureInterface.reportStarted();

    QMetaObject::invokeMethod(
        this,
        [this, points, algorithm, &futureInterface]() {
            // Execute in the correct thread
            ShortestPathResult localResult =
                mVisibilityGraph->findShortestPath(points, algorithm);
            futureInterface.reportResult(localResult);
            futureInterface.reportFinished();
        },
        Qt::QueuedConnection);

    QFuture<ShortestPathResult> future = futureInterface.future();
    future.waitForFinished();
    return future.result();
}

QString OptimizedNetwork::getRegionName()
{
    return mRegionName;
}

void OptimizedNetwork::setRegionName(QString newName)
{
    mRegionName = newName;
}
}; // namespace ShipNetSimCore
