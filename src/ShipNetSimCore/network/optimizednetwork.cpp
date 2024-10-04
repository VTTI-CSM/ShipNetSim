#include "optimizednetwork.h"
#include "GPoint.h"
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
    variable.band = variable.dataset->GetRasterBand(1);
    return true;

}

QVector<std::shared_ptr<SeaPort>>
OptimizedNetwork::loadFirstAvailableSeaPorts()
{
    QVector<QString> seaPortsLocs = NetworkDefaults::seaPortsLocations();

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
        QVector<std::shared_ptr<SeaPort>> SeaPorts =
            readSeaPorts( filePath.toStdString().c_str());
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
{
    GDALAllRegister();

    loadTiffData();
}

OptimizedNetwork::OptimizedNetwork(
    QVector<std::shared_ptr<Polygon>> boundaries,
    BoundariesType boundariesType,
    QString regionName) :
    mBoundaries(boundaries),
    mBoundaryType(boundariesType),
    mRegionName(regionName)
{
    GDALAllRegister();

    mVisibilityGraph =
        std::make_shared<OptimizedVisibilityGraph>(
        std::as_const(mBoundaries),
        boundariesType);

    mRegionName = regionName;  // Set region name

    // load the sea ports
    bool loadedSeaPorts =
        loadFirstAvailableSeaPortsFile(NetworkDefaults::seaPortsLocations());
    if (! loadedSeaPorts) { qWarning("Sea Ports file could not be loaded!"); }
    else { mVisibilityGraph->loadSeaPortsPolygonCoordinates(mSeaPorts); }

    loadTiffData();
}

OptimizedNetwork::~OptimizedNetwork()
{}

OptimizedNetwork::OptimizedNetwork(QString filename)
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
        loadFirstAvailableSeaPortsFile(NetworkDefaults::seaPortsLocations());
    if (! loadedSeaPorts) { qWarning("Sea Ports file could not be loaded!"); }
    else { mVisibilityGraph->loadSeaPortsPolygonCoordinates(mSeaPorts); }


    // mVisibilityGraph->buildGraph();

    loadTiffData();

    // send signal that the network is loaded and valid
    emit NetworkLoaded();
}


void OptimizedNetwork::moveObjectToThread(QThread *thread)
{
    // Move Simulator object itself to the thread
    this->moveToThread(thread);

    // todo: define child object as QObject and move them to the new thread
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
        salinityTiffData, NetworkDefaults::SalinityTiffLocations());

    loadFirstAvailableTiffFile(
        waveHeightTiffData, NetworkDefaults::WaveHeightTiffLocations());

    loadFirstAvailableTiffFile(
        wavePeriodTiffData, NetworkDefaults::wavePeriodTiffLocations());

    loadFirstAvailableTiffFile(
        windNorthTiffData, NetworkDefaults::windSpeedNorthTiffLocations());

    loadFirstAvailableTiffFile(
        windEastTiffData, NetworkDefaults::windSpeedEastTiffLocations());

    loadFirstAvailableTiffFile(
        waterDepthTiffData, NetworkDefaults::waterDepthTiffLocations());
}

std::pair<size_t, size_t>
OptimizedNetwork::mapCoordinatesToTiffIndecies(tiffFileData& data, GPoint p)
{
    size_t pixelX, pixelY;

    double inv_det = 1.0 /
                     (data.adfGeoTransform[1] * data.adfGeoTransform[5] -
                      data.adfGeoTransform[2] * data.adfGeoTransform[4]);
    double x = p.getLongitude().value() - data.adfGeoTransform[0];
    double y = p.getLatitude().value() - data.adfGeoTransform[3];

    // Using the inverse of the geotransform matrix to convert
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

    // GDALRasterBand *band = dataset->GetRasterBand(1); // Get the first band

    // int width = band->GetXSize();
    // int height = band->GetYSize();

    // std::vector<std::vector<float>> imageData(height,
    //                                           std::vector<float>(width));

    // float *image = new float[width * height];
    // // Read the raster data into the buffer
    // if(band->RasterIO(GF_Read, 0, 0, width, height,
    //                    image, width, height, GDT_Float32, 0, 0) != CE_None)
    // {
    //     delete[] image;
    //     GDALClose(dataset);
    //     qFatal("Failed to read the raster data from the TIFF file."
    //            "The Tiff file must be grey scale with only one band!");
    // }

    // // Convert from the 1D buffer to 2D buffer
    // for (int row = 0; row < height; ++row) {
    //     for (int col = 0; col < width; ++col) {
    //         // Calculate the index for the 1D array (image)
    //         int index = row * width + col;
    //         // Assign the value from the 1D array to the 2D vector
    //         imageData[row][col] = image[index];
    //     }
    // }

    // delete[] image;
    // GDALClose(dataset); // Ensure the dataset is properly closed

    // return imageData; // Return the populated 2D vector


    // TinyTIFFReaderFile* tiffr = TinyTIFFReader_open(filename);
    // if (!tiffr) {
    //     std::cerr << "Error opening file: " << filename << std::endl;
    //     return {}; // Return an empty vector on failure
    // }

    // // Check for the number of frames/bands in the TIFF file
    // if (TinyTIFFReader_countFrames(tiffr) > 1) {
    //     TinyTIFFReader_close(tiffr); // Close the file before throwing
    //     throw std::runtime_error("TIFF file contains more than one band or "
    //                              "frame, which is not supported.");
    // }

    // uint32_t width = TinyTIFFReader_getWidth(tiffr);
    // uint32_t height = TinyTIFFReader_getHeight(tiffr);

    // std::vector<std::vector<float>> imageData(height,
    //                                           std::vector<float>(width));

    // float* image = (float*)calloc(width * height, sizeof(float));
    // // fill in all the array with nan values
    // std::fill_n(image, width * height, std::nan(""));

    // // load all the data from the tiff image
    // TinyTIFFReader_getSampleData(tiffr, image, 0);

    // // convert from the 1D buffer to 2D buffer
    // for (uint32_t row = 0; row < height; ++row) {
    //     for (uint32_t col = 0; col < width; ++col) {
    //         // Calculate the index for the 1D array (image)
    //         uint32_t index = row * width + col;
    //         // Assign the value from the 1D array to the 2D vector
    //         imageData[row][col] = image[index];
    //     }
    // }
    // free(image);

    // TinyTIFFReader_close(tiffr); // Ensure the file is properly closed

    // return imageData; // Return the populated 2D vector
}

QVector<std::shared_ptr<SeaPort> > OptimizedNetwork::readSeaPorts(const char* filename)
{
    QVector<std::shared_ptr<SeaPort>> seaPorts;

    GDALDataset* dataset = static_cast<GDALDataset*>(
        GDALOpenEx(filename, GDAL_OF_VECTOR, nullptr, nullptr, nullptr));
    if (dataset == nullptr) {
        qWarning("Failed to open file: %", qPrintable(filename));
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
                SeaPort sp = SeaPort(p);
                try {
                    sp.setCountryName(country);
                    sp.setPortCode(locode);
                    sp.setPortName(nameWoDiac);
                    sp.setHasRailTerminal(function[1] != '-');
                    sp.setHasRoadTerminal(function[2] != '-');


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

                    sp.setStatusOfEntry(statusText);
                } catch (std::exception& e)
                {
                    qWarning(e.what());
                }

                seaPorts.push_back(std::make_shared<SeaPort>(sp));

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
        data.band->RasterIO(GF_Read, index.first, index.second, 1, 1,
                       &value, 1, 1, GDT_Float32, 0, 0);
        return value;
    };

    double salinityValue = getValue(salinityTiffData);
    salinityValue = std::isnan(salinityValue)? 35.0: salinityValue; // default value if nan
    env.salinity = units::concentration::pptd_t(salinityValue);

    double waveHeightValue = getValue(waveHeightTiffData);
    waveHeightValue = std::isnan(waveHeightValue)? 0.0 : waveHeightValue;
    env.waveHeight = units::length::meter_t(waveHeightValue);

    double wavePeriodValue = getValue(wavePeriodTiffData);
    wavePeriodValue = (wavePeriodValue <= 0.0 || std::isnan(wavePeriodValue))?
        40.0 : wavePeriodValue;  //40 seconds
    double waveFrequency = 1.0f / wavePeriodValue;
    env.waveFrequency = units::frequency::hertz_t(waveFrequency);
    env.waveAngularFrequency =
        units::angular_velocity::radians_per_second_t(
        (double)2.0 * units::constants::pi.value() * waveFrequency);

    double windSpeed_Northward = getValue(windNorthTiffData);
    if (std::isnan(windSpeed_Northward)) { windSpeed_Northward = 0.0; };
    env.windSpeed_Northward =
        units::velocity::meters_per_second_t(windSpeed_Northward);

    double windSpeed_Eastward = getValue(windEastTiffData);
    if (std::isnan(windSpeed_Eastward)) { windSpeed_Eastward = 0.0; };
    env.windSpeed_Eastward =
        units::velocity::meters_per_second_t(windSpeed_Eastward);

    double waveSpeedResultant = sqrt(std::pow(windSpeed_Northward, 2) +
                                     std::pow(windSpeed_Eastward, 2));

    double waveLength = waveSpeedResultant / waveFrequency;
    env.waveLength = units::length::meter_t(waveLength);


    double depthV = getValue(waterDepthTiffData);
    if (std::isnan(depthV)) { depthV = 50.0; };
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
    return mVisibilityGraph->findShortestPath(points, algorithm);
}

QString OptimizedNetwork::getRegionName()
{
    return mRegionName;
}
};
