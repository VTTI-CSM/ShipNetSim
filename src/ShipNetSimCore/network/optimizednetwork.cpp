#include "optimizednetwork.h"
#include "gpoint.h"
#include "qdebug.h"      // Debugging utilities
#include <QCoreApplication>  // Core application utilities
#include <QFile>             // File input/output utilities
#include <QTextStream>       // Text stream utilities
#include <QDebug>            // Debugging utilities
#include <QRegularExpression>
#include <gdal_priv.h>
#include <ogrsf_frmts.h>
#include "../utils/utils.h"
#include "networkdefaults.h"

namespace ShipNetSimCore
{

bool OptimizedNetwork::loadFirstAvailableTiffFile(
    tiffFileData& variable,
    QVector<QString> locations)
{
    QString loc =
        Utils::getFirstExistingPathFromList(locations,
                                                      QVector<QString>{"tif", "tiff"});

    if (loc == "")
    {
        qFatal("Could not find the tiff file");
    }
    variable.dataset = readTIFFFile(loc.toStdString().c_str());
    if (variable.dataset->GetGeoTransform(
            variable.adfGeoTransform) != CE_None)
    {
        qFatal("Tiff file may not have an assigned SRS!");
        return false;
    }

    // Calculate geographic extents (min/max lat/long)
    variable.calculateGeographicExtents();

    variable.band = variable.dataset->GetRasterBand(1);
    return true;

}

QVector<std::shared_ptr<SeaPort>>
OptimizedNetwork::loadFirstAvailableSeaPorts()
{
    static QVector<std::shared_ptr<SeaPort>> SeaPorts;
    if (!SeaPorts.isEmpty()) {
        return SeaPorts;
    }

    QVector<QString> seaPortsLocs =
        NetworkDefaults::seaPortsLocations(Utils::getDataDirectory());

    QString filePath =
        Utils::getFirstExistingPathFromList(seaPortsLocs,
                                            QVector<QString>("geojson"));
    // if there is a path found, load it
    if (filePath == "")
    {
        return QVector<std::shared_ptr<SeaPort>>();
    }
    else
    {
        SeaPorts = readSeaPorts( filePath.toStdString().c_str());
        return SeaPorts;
    }

}

bool OptimizedNetwork::loadFirstAvailableSeaPortsFile(
    QVector<QString> locations)
{
    QString filePath =
        Utils::getFirstExistingPathFromList(locations,
                                            QVector<QString>("geojson"));

    // if there is a path found, load it
    if (filePath == "")
    {
        return false;
    }
    else
    {
        mSeaPorts = readSeaPorts(
            filePath.toStdString().c_str());
        return true;
    }
}

OptimizedNetwork::OptimizedNetwork()
{}

void OptimizedNetwork::initializeNetwork(QString filename)
{
    GDALAllRegister();

    QFileInfo fileInfo(filename);

    // Check if the file exists before trying to get its extension
    if (!fileInfo.exists()) {
        qDebug() << "File does not exist.";
        return;
    }

    // Get the extension and convert it to lower case
    // for case-insensitive comparison
    QString extension = fileInfo.suffix().toLower();

    // text based file
    if (extension == "txt" || extension == "dat") {
        loadTxtFile(filename);
    }
    // shp file
    else if (extension == "shp") {
        loadShapeFile(filename);
    }
    // other file extensions are not yet supported
    else {
        qFatal("file type is not supported!");
    }

    // load the sea ports
    bool loadedSeaPorts =
        loadFirstAvailableSeaPortsFile(
            NetworkDefaults::seaPortsLocations(Utils::getDataDirectory()));
    if (! loadedSeaPorts) { qWarning("Sea Ports file could not be loaded!"); }
    else { mVisibilityGraph->loadSeaPortsPolygonCoordinates(mSeaPorts); }


    // mVisibilityGraph->buildGraph();

    loadTiffData();

    // send signal that the network is loaded and valid
    emit NetworkLoaded();
}

void OptimizedNetwork::initializeNetwork(
    QVector<std::shared_ptr<Polygon> > boundaries,
    BoundariesType boundariesType,
    QString regionName)
{
    GDALAllRegister();

    mVisibilityGraph =
        std::make_shared<OptimizedVisibilityGraph>(
            std::as_const(mBoundaries),
            boundariesType);

    mRegionName = regionName;  // Set region name

    // load the sea ports
    bool loadedSeaPorts =
        loadFirstAvailableSeaPortsFile(
            NetworkDefaults::seaPortsLocations(Utils::getDataDirectory()));
    if (! loadedSeaPorts) { qWarning("Sea Ports file could not be loaded!"); }
    else { mVisibilityGraph->loadSeaPortsPolygonCoordinates(mSeaPorts); }

    loadTiffData();

    emit NetworkLoaded();
}


OptimizedNetwork::OptimizedNetwork(
    QVector<std::shared_ptr<Polygon>> boundaries,
    BoundariesType boundariesType,
    QString regionName) :
    mBoundaries(boundaries),
    mBoundaryType(boundariesType),
    mRegionName(regionName)
{
    initializeNetwork(boundaries, boundariesType, regionName);
}

OptimizedNetwork::~OptimizedNetwork()
{}

OptimizedNetwork::OptimizedNetwork(QString filename)
{
    initializeNetwork(filename);
}


void OptimizedNetwork::moveObjectToThread(QThread *thread)
{
    // Move Simulator object itself to the thread
    this->moveToThread(thread);

    if (mVisibilityGraph && mVisibilityGraph->parent() == nullptr) {
        mVisibilityGraph->moveToThread(thread);
    }
}

void OptimizedNetwork::loadTxtFile(const QString& filename)
{
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly))
    {
        qFatal("Failed to open the network file.");  // Log error message
        return;
    }

    QTextStream in(&file);
    QString line;  // Store each line from file

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

    auto cleanLine = [&in, &line](bool loadNextLine = true)
    {
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
            if (!outerBoundary.isEmpty() ||
                !holes.isEmpty())
            {
                auto polygon =
                    std::make_shared<Polygon>(outerBoundary,
                                              holes,
                                              waterBodyId);
                mBoundaries.push_back(polygon);
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
            else if (pointRegex.match(line).hasMatch() &&
                     (!currentSection.isEmpty()))
            {
                QRegularExpressionMatch pointMatch =
                    pointRegex.match(line);

                QString id = pointMatch.captured(1);
                double lon = pointMatch.captured(2).toDouble();
                double lat = pointMatch.captured(3).toDouble();

                std::shared_ptr<GPoint> pt =
                    std::make_shared<GPoint>(units::angle::degree_t(lon),
                                             units::angle::degree_t(lat),
                                             id);
                currentBoundary.append(pt);
            }
            else
            {
                // Handle invalid or unexpected lines
                if (!line.isEmpty()) {
                    qDebug() << "Unexpected format or "
                                "content in line:" << line;
                }
            }
        }
    }

    // Finalize the last water body
    if (!outerBoundary.isEmpty() ||
        !holes.isEmpty())
    {
        auto polygon =
            std::make_shared<Polygon>(outerBoundary,
                                      holes,
                                      waterBodyId);
        mBoundaries.push_back(polygon);
    }

    file.close();

    mVisibilityGraph =
        std::make_shared<OptimizedVisibilityGraph>(
            std::as_const(mBoundaries),
            BoundariesType::Water);
}

void OptimizedNetwork::loadShapeFile(const QString& filepath) {

    mBoundaries.clear();
    GDALDataset* poDS = static_cast<GDALDataset*>
        (GDALOpenEx(filepath.toStdString().c_str(),
                    GDAL_OF_VECTOR, NULL, NULL, NULL));
    if(poDS == NULL) {
        qFatal("Open shaefile failed.");
    }

    OGRLayer* poLayer = poDS->GetLayer(0);

    // Check the reference system is geodetic
    OGRSpatialReference* poSRS = poLayer->GetSpatialRef();
    if (poSRS != NULL) {
        char *pszSRS_WKT = NULL;
        // Get the WKT representation of the SRS
        poSRS->exportToWkt(&pszSRS_WKT);

        // Check if the SRS is geographic (not projected)
        if (! poSRS->IsGeographic()) {
            CPLFree(pszSRS_WKT);
            GDALClose(poDS);
            qFatal("The spatial reference system is not geographic. "
                   "Exiting...");
        }

        CPLFree(pszSRS_WKT);
    } else {
        GDALClose(poDS);
        qFatal("Spatial reference system is unknown. Exiting...");
    }


    OGRFeature* poFeature;

    poLayer->ResetReading();

    int shapeID = 0;
    while((poFeature = poLayer->GetNextFeature()) != NULL)
    {
        OGRGeometry* poGeometry;
        poGeometry = poFeature->GetGeometryRef();
        if(poGeometry != NULL &&
            wkbFlatten(poGeometry->getGeometryType()) == wkbPolygon)
        {
            OGRPolygon* poPolygon = static_cast<OGRPolygon*>(poGeometry);
            OGRLinearRing* poExteriorRing = poPolygon->getExteriorRing();

            // read exterior ring
            QVector<std::shared_ptr<GPoint>> exterirorRinge =
                QVector<std::shared_ptr<GPoint>>();
            if(poExteriorRing != NULL)
            {
                int nPoints = poExteriorRing->getNumPoints();
                for(int i = 0; i < nPoints; i++) {
                    double x = poExteriorRing->getX(i);
                    double y = poExteriorRing->getY(i);

                    exterirorRinge.push_back(
                        std::make_shared<GPoint>(
                            GPoint(units::angle::degree_t(x),
                                   units::angle::degree_t(y))));
                }

                shapeID ++; // increment the id
            }

            // read interior ring
            // Iterate over interior rings (holes) if necessary
            int nInteriorRings = poPolygon->getNumInteriorRings();
            QVector<QVector<std::shared_ptr<GPoint>>> innerHoles =
                QVector<QVector<std::shared_ptr<GPoint>>>(nInteriorRings);

            for (int i = 0; i < nInteriorRings; i++) {
                OGRLinearRing* poInteriorRing = poPolygon->getInteriorRing(i);
                int nPoints = poInteriorRing->getNumPoints();
                for (int j = 0; j < nPoints; j++) {
                    double x = poInteriorRing->getX(j);
                    double y = poInteriorRing->getY(j);

                    innerHoles[i].push_back(
                        std::make_shared<GPoint>(
                            GPoint(units::angle::degree_t(x),
                                   units::angle::degree_t(y))));
                }
            }

            auto polygon =
                std::make_shared<Polygon>(exterirorRinge,
                                          innerHoles,
                                          QString::number(shapeID));
            mBoundaries.push_back(polygon);

        }
        OGRFeature::DestroyFeature(poFeature);
    }

    GDALClose(poDS);

    mVisibilityGraph =
        std::make_shared<OptimizedVisibilityGraph>(
            std::as_const(mBoundaries),
            BoundariesType::Water);
}

void OptimizedNetwork::loadTiffData() {
    loadFirstAvailableTiffFile(
        salinityTiffData,
        NetworkDefaults::SalinityTiffLocations(Utils::getDataDirectory()));

    loadFirstAvailableTiffFile(
        waveHeightTiffData,
        NetworkDefaults::WaveHeightTiffLocations(Utils::getDataDirectory()));

    loadFirstAvailableTiffFile(
        wavePeriodTiffData,
        NetworkDefaults::wavePeriodTiffLocations(Utils::getDataDirectory()));

    loadFirstAvailableTiffFile(
        windNorthTiffData,
        NetworkDefaults::windSpeedNorthTiffLocations(Utils::getDataDirectory()));

    loadFirstAvailableTiffFile(
        windEastTiffData,
        NetworkDefaults::windSpeedEastTiffLocations(Utils::getDataDirectory()));

    loadFirstAvailableTiffFile(
        waterDepthTiffData,
        NetworkDefaults::waterDepthTiffLocations(Utils::getDataDirectory()));
}

std::pair<size_t, size_t>
OptimizedNetwork::mapCoordinatesToTiffIndecies(tiffFileData& data, GPoint p)
{
    size_t pixelX, pixelY;

    // Get the longitude and latitude from the GPoint p
    double longitude = p.getLongitude().value();
    double latitude = p.getLatitude().value();

    // Handle the longitude wrapping within the GeoTIFF's bounds (minLong, maxLong)
    if (longitude < data.minLong) {
        longitude = data.maxLong - (data.minLong - longitude);
    }
    else if (longitude > data.maxLong) {
        longitude = data.minLong + (longitude - data.maxLong);
    }

    // Clamp the latitude to the nearest valid value if it goes outside the bounds
    if (latitude < data.minLat) {
        latitude = data.minLat;
    }
    else if (latitude > data.maxLat) {
        latitude = data.maxLat;
    }

    // Adjust the coordinates using the geo-transform array
    double x = longitude - data.adfGeoTransform[0];
    double y = latitude - data.adfGeoTransform[3];

    // Compute the determinant inverse for transformation
    double inv_det = 1.0 / (data.adfGeoTransform[1] * data.adfGeoTransform[5] -
                            data.adfGeoTransform[2] * data.adfGeoTransform[4]);

    // Use the inverse of the geo-transform matrix to map
    // coordinates to pixel indices
    pixelX = static_cast<size_t>((x * data.adfGeoTransform[5] -
                                  y * data.adfGeoTransform[2]) * inv_det);
    pixelY = static_cast<size_t>((-x * data.adfGeoTransform[4] +
                                  y * data.adfGeoTransform[1]) * inv_det);

    return std::make_pair(pixelX, pixelY);
}

GDALDataset *OptimizedNetwork::readTIFFFile(
    const char* filename)
{

    GDALDataset *dataset;
    GDALAllRegister();

    dataset = (GDALDataset *) GDALOpen(filename, GA_ReadOnly);
    if(dataset == nullptr) {
        std::cerr << "Error opening file: " << filename << std::endl;
        return {}; // Return an empty vector on failure
    }

    // Check for the number of bands in the TIFF file
    if(dataset->GetRasterCount() > 1) {
        GDALClose(dataset); // Close the dataset before throwing
        qFatal("TIFF file contains more than one band, "
               "which is not supported.");
    }

    return dataset;

}

QVector<std::shared_ptr<SeaPort> >
OptimizedNetwork::readSeaPorts(const char* filename)
{
    QVector<std::shared_ptr<SeaPort>> seaPorts;

    GDALDataset* dataset = static_cast<GDALDataset*>(
        GDALOpenEx(filename, GDAL_OF_VECTOR, nullptr, nullptr, nullptr));
    if (dataset == nullptr) {
        qWarning("Failed to open file: %s", qPrintable(filename));
        return seaPorts;
    }

    // Assuming the data is in the first layer
    OGRLayer* layer = dataset->GetLayer(0);
    if (layer == nullptr) {
        qWarning("Failed to get layer from dataset.");
        GDALClose(dataset);
        return seaPorts;
    }

    OGRFeature* feature;
    layer->ResetReading();

    while ((feature = layer->GetNextFeature()) != nullptr) {
        const char* function = feature->GetFieldAsString("Function");
        const char* status = feature->GetFieldAsString("Status");

        if (function[0] == '1' && strcmp(status, "AI") == 0) {
            const char* country = feature->GetFieldAsString("Country");
            const char* nameWoDiac = feature->GetFieldAsString("NameWoDiac");
            const char* locode = feature->GetFieldAsString("LOCODE");
            QString qStatus = QString::fromUtf8(status).toUpper();
            OGRGeometry* geometry = feature->GetGeometryRef();

            if (geometry != nullptr &&
                wkbFlatten(geometry->getGeometryType()) == wkbPoint) {
                OGRPoint* point = static_cast<OGRPoint*>(geometry);
                double longitude = point->getX();
                double latitude = point->getY();
                GPoint p = GPoint(units::angle::degree_t(longitude),
                                  units::angle::degree_t(latitude));
                auto sp = std::make_shared<SeaPort>(p);
                // SeaPort sp = SeaPort(p);
                try {
                    sp->setCountryName(country);
                    sp->setPortCode(locode);
                    sp->setPortName(nameWoDiac);
                    sp->setHasRailTerminal(function[1] != '-');
                    sp->setHasRoadTerminal(function[2] != '-');


                    QString statusText;

                    if (qStatus == "AA") {
                        statusText = "Approved by competent national "
                                     "government agency";
                    } else if (qStatus == "AC") {
                        statusText = "Approved by Customs Authority";
                    } else if (qStatus == "AF") {
                        statusText = "Approved by national facilitation body";
                    } else if (qStatus == "AI") {
                        statusText = "Code adopted by international "
                                     "organisation (IATA or ECLAC)";
                    } else if (qStatus == "AS") {
                        statusText = "Approved by national standardisation "
                                     "body";
                    } else if (qStatus == "RL") {
                        statusText = "Recognised location - Existence and "
                                     "representation of location name "
                                     "confirmed by check against nominated "
                                     "gazetteer or other reference work";
                    } else if (qStatus == "RN") {
                        statusText = "Request from credible national sources "
                                     "for locations in their own country";
                    } else if (qStatus == "RQ") {
                        statusText = "Request under consideration";
                    } else if (qStatus == "RR") {
                        statusText = "Request rejected";
                    } else if (qStatus == "QQ") {
                        statusText = "Original entry not verified since date "
                                     "indicated";
                    } else if (qStatus == "XX") {
                        statusText = "Entry that will be removed from the "
                                     "next issue of UN/LOCODE";
                    } else {
                        statusText = "Unknown status code";
                    }

                    sp->setStatusOfEntry(statusText);
                } catch (std::exception& e)
                {
                    qWarning("%s", e.what());
                }

                seaPorts.push_back(sp);
            }
        }

        OGRFeature::DestroyFeature(feature);
    }

    GDALClose(dataset);
    return seaPorts;
}


AlgebraicVector::Environment OptimizedNetwork::
    getEnvironmentFromPosition(GPoint p)
{
    AlgebraicVector::Environment env;

    auto getValue =
        [this, &p]( tiffFileData& data) ->
        float {    // {new index.height, new index.width}

        auto index = mapCoordinatesToTiffIndecies(data, p);
        float value;
        double noDataValue = data.band->GetNoDataValue(nullptr);
        data.band->RasterIO(GF_Read, index.first, index.second, 1, 1,
                       &value, 1, 1, GDT_Float32, 0, 0);
        // Check if the value matches the NoData value
        if (std::isnan(value) || value == static_cast<float>(noDataValue)) {
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
    env.waveHeight = units::length::meter_t(waveHeightValue);

    // Wave period (seconds) and frequency (hertz)
    double wavePeriodValue = getValue(wavePeriodTiffData);
    double waveFrequency = 0.0;
    if (std::isnan(wavePeriodValue)) {
        waveFrequency = std::nan("noData");
    }
    else {
        waveFrequency = 1.0 / wavePeriodValue;
    }
    env.waveFrequency = units::frequency::hertz_t(waveFrequency);
    env.waveAngularFrequency = units::angular_velocity::radians_per_second_t(
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
    if (std::isnan(wavePeriodValue)) {
        waveLength = std::nan("noData");
    }
    else {
        waveLength = (units::constants::g.value() *
                     std::pow(wavePeriodValue, 2.0)) /
                     (2.0 * units::constants::pi);
    }
    env.waveLength = units::length::meter_t(waveLength);

    // Water depth (meters)
    double depthV = getValue(waterDepthTiffData);
    env.waterDepth = units::length::meter_t(depthV);

    return env;
}

void OptimizedNetwork::setBoundaries(
    QVector<std::shared_ptr<Polygon>> boundaries)
{
    mBoundaries = boundaries;

    mVisibilityGraph->clear();
    OptimizedNetwork(mBoundaries,
                     BoundariesType::Water,
                     mRegionName);  // Init Network with water body and region.
}

ShortestPathResult OptimizedNetwork::findShortestPath(
    std::shared_ptr<GPoint> startPoint,
    std::shared_ptr<GPoint> endpoint,
    PathFindingAlgorithm algorithm)
{
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
    PathFindingAlgorithm algorithm)
{
    // If already in the correct thread, call the function directly
    if (QThread::currentThread() == this->thread()) {
        return mVisibilityGraph->findShortestPath(points, algorithm);
    }

    // Otherwise, use invokeMethod to execute in the correct thread
    ShortestPathResult result;
    QMetaObject::invokeMethod(this, [&]() {
        result = mVisibilityGraph->findShortestPath(points, algorithm);
    }, Qt::BlockingQueuedConnection); // Wait for the result before proceeding

    return result;
}

QString OptimizedNetwork::getRegionName()
{
    return mRegionName;
}

void OptimizedNetwork::setRegionName(QString newName)
{
    mRegionName = newName;
}
};
