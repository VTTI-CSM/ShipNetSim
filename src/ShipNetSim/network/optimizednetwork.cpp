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

bool OptimizedNetwork::loadFirstAvailableTiffFile(
    std::vector<std::vector<float>>& variable,
    QVector<QString> locations)
{
    bool fileLoaded = false;

    for (qsizetype i = 0; i < locations.size(); i++) {
        if (fileLoaded) { break; }
        // Check the file exists and its extension is valid
        QFileInfo fileInfo(locations[i]);
        if (fileInfo.exists()) {
            // Get the file extension in lowercase
            QString extension = fileInfo.suffix().toLower();
            if (extension == "tif" || extension == "tiff") {
                variable = readTIFFAs2DArray(
                    fileInfo.absoluteFilePath().toStdString().c_str());
                fileLoaded = true;
            }
        }
    }
    return fileLoaded;
}

bool OptimizedNetwork::loadFirstAvailableSeaPortsFile(
    QVector<QString> locations)
{
    bool fileLoaded = false;
    for (qsizetype i = 0; i < locations.size(); i++) {
        if (fileLoaded) { break; }
        // Check the file exists and its extension is valid
        QFileInfo fileInfo(locations[i]);
        if (fileInfo.exists()) {
            // Get the file extension in lowercase
            QString extension = fileInfo.suffix().toLower();
            if (extension == "geojson") {
                mSeaPorts = readSeaPorts(
                    fileInfo.absoluteFilePath().toStdString().c_str());
                fileLoaded = true;
            }
        }
    }
    return fileLoaded;
}

OptimizedNetwork::OptimizedNetwork()
{
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
    mVisibilityGraph =
        std::make_shared<OptimizedVisibilityGraph>(
        std::as_const(mBoundaries),
        boundariesType);

    mRegionName = regionName;  // Set region name

    // load the sea ports
    bool loadedSeaPorts = loadFirstAvailableSeaPortsFile(seaPortsLocations());
    if (! loadedSeaPorts) { qWarning("Sea Ports file could not be loaded!"); }
    else { mVisibilityGraph->loadSeaPortsPolygonCoordinates(mSeaPorts); }

    loadTiffData();
}

OptimizedNetwork::~OptimizedNetwork()
{}

OptimizedNetwork::OptimizedNetwork(QString filename)
{

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
    bool loadedSeaPorts = loadFirstAvailableSeaPortsFile(seaPortsLocations());
    if (! loadedSeaPorts) { qWarning("Sea Ports file could not be loaded!"); }
    else { mVisibilityGraph->loadSeaPortsPolygonCoordinates(mSeaPorts); }


    // mVisibilityGraph->buildGraph();

    loadTiffData();
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
        salinity, OptimizedNetwork::SalinityTiffLocations());

    loadFirstAvailableTiffFile(
        waveHeight, OptimizedNetwork::WaveHeightTiffLocations());

    loadFirstAvailableTiffFile(
        wavePeriod, OptimizedNetwork::wavePeriodTiffLocations());

    loadFirstAvailableTiffFile(
        windNorth, OptimizedNetwork::windSpeedNorthTiffLocations());

    loadFirstAvailableTiffFile(
        windEast, OptimizedNetwork::windSpeedEastTiffLocations());

    loadFirstAvailableTiffFile(
        waterDepth, OptimizedNetwork::waterDepthTiffLocations());
}


std::vector<std::vector<float>> OptimizedNetwork::readTIFFAs2DArray(
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

    GDALRasterBand *band = dataset->GetRasterBand(1); // Get the first band
    int width = band->GetXSize();
    int height = band->GetYSize();

    std::vector<std::vector<float>> imageData(height,
                                              std::vector<float>(width));

    float *image = new float[width * height];
    // Read the raster data into the buffer
    if(band->RasterIO(GF_Read, 0, 0, width, height,
                       image, width, height, GDT_Float32, 0, 0) != CE_None)
    {
        delete[] image;
        GDALClose(dataset);
        qFatal("Failed to read the raster data from the TIFF file."
               "The Tiff file must be grey scale with only one band!");
    }

    // Convert from the 1D buffer to 2D buffer
    for (int row = 0; row < height; ++row) {
        for (int col = 0; col < width; ++col) {
            // Calculate the index for the 1D array (image)
            int index = row * width + col;
            // Assign the value from the 1D array to the 2D vector
            imageData[row][col] = image[index];
        }
    }

    delete[] image;
    GDALClose(dataset); // Ensure the dataset is properly closed

    return imageData; // Return the populated 2D vector


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
            OGRGeometry* geometry = feature->GetGeometryRef();

            if (geometry != nullptr &&
                wkbFlatten(geometry->getGeometryType()) == wkbPoint) {
                OGRPoint* point = static_cast<OGRPoint*>(geometry);
                double longitude = point->getY();
                double latitude = point->getX();
                GPoint p = GPoint(units::angle::degree_t(longitude),
                                  units::angle::degree_t(latitude));
                SeaPort sp = SeaPort(p);
                try {
                    sp.setCountryName(country);
                    sp.setPortCode(locode);
                    sp.setPortName(nameWoDiac);
                } catch (std::exception& e)
                {
                    qWarning(e.what());
                }

                seaPorts.push_back(std::make_shared<SeaPort>(SeaPort(p)));

                // std::cout << "Country: " << country << ", NameWoDiac: " << nameWoDiac
                //           << ", LOCODE: " << locode << ", Coordinates: ("
                //           << longitude << ", " << latitude << ")" << std::endl;
            }
        }

        OGRFeature::DestroyFeature(feature);
    }

    GDALClose(dataset);
    return seaPorts;
}

std::pair<size_t, size_t> OptimizedNetwork::mapCoordinateTo2DArray(
    GPoint coord, GPoint mapMinPoint, GPoint mapMaxPoint,
    size_t arrayWidth, size_t arrayHeight)
{
    // Normalize x and y within the coordinate system boundaries
    double x_norm =
        (coord.getLongitude().value() -
         mapMinPoint.getLongitude().value()) /
        (mapMaxPoint.getLongitude().value() -
         mapMinPoint.getLongitude().value());
    double y_norm =
        (coord.getLatitude().value() -
         mapMinPoint.getLatitude().value()) /
        (mapMaxPoint.getLatitude().value() -
         mapMinPoint.getLatitude().value());

    // Scale normalized coordinates to array indices
    size_t i =
        static_cast<size_t>(y_norm * (arrayHeight - 1)); // 0-based index
    size_t j =
        static_cast<size_t>(x_norm * (arrayWidth - 1)); // 0-based index

    return {i, j};
}

AlgebraicVector::Environment OptimizedNetwork::
    getEnvironmentFromPosition(GPoint p)
{
    AlgebraicVector::Environment env;

    auto getArrayIndex =
        [this, &p](const std::vector<std::vector<float>>& vec) ->
        std::pair<size_t, size_t> {    // {new index.height, new index.width}

        size_t height = vec.size();
        size_t width = vec.empty() ? 0 : vec[0].size();

        if (height <= 0 || width <= 0) { return {-1, -1}; }

        return this->mapCoordinateTo2DArray(
            p, mVisibilityGraph->getMinMapPoint(),
            mVisibilityGraph->getMaxMapPoint(),
            width, height);

    };

    auto saIndex = getArrayIndex(salinity);
    double salinityValue =
        (saIndex.first > 0) ? salinity[saIndex.first][saIndex.second] : 0;
    if (std::isnan(salinityValue)) { salinityValue = 0.0; };
    env.salinity = units::concentration::pptd_t(salinityValue);

    auto whIndex = getArrayIndex(waveHeight);
    double waveHeightValue =
        (whIndex.first > 0) ? waveHeight[whIndex.first][whIndex.second] : 0.0;
    if (std::isnan(waveHeightValue)) { waveHeightValue = 0.0; }
    env.waveHeight = units::length::meter_t(waveHeightValue);

    auto wpIndex = getArrayIndex(wavePeriod);
    double wavePeriodValue =
        (wpIndex.first > 0) ? wavePeriod[wpIndex.first][wpIndex.second] : 0;
    if (std::isnan(wavePeriodValue)) { wavePeriodValue = 40.0; };  //40 seconds
    double waveFrequency = 1.0f / wavePeriodValue;
    env.waveFrequency = units::frequency::hertz_t(waveFrequency);
    env.waveAngularFrequency =
        units::angular_velocity::radians_per_second_t(
        (double)2.0 * units::constants::pi.value() * waveFrequency);

    auto wsnIndex = getArrayIndex(windNorth);
    double windSpeed_Northward =
        (wsnIndex.first > 0) ? windNorth[wsnIndex.first][wsnIndex.second] : 0;
    if (std::isnan(windSpeed_Northward)) { windSpeed_Northward = 0.0; };
    env.windSpeed_Northward =
        units::velocity::meters_per_second_t(windSpeed_Northward);

    auto wseIndex = getArrayIndex(windEast);
    double windSpeed_Eastward =
        (wsnIndex.first > 0) ? windEast[wseIndex.first][wseIndex.second] : 0;
    if (std::isnan(windSpeed_Eastward)) { windSpeed_Eastward = 0.0; };
    env.windSpeed_Eastward =
        units::velocity::meters_per_second_t(windSpeed_Eastward);

    double waveSpeedResultant = sqrt(std::pow(windSpeed_Northward, 2) +
                                     std::pow(windSpeed_Eastward, 2));

    double waveLength = waveSpeedResultant / waveFrequency;
    env.waveLength = units::length::meter_t(waveLength);


    auto waterDIndex = getArrayIndex(waterDepth);
    double depthV =
        (waterDIndex.first > 0) ? waterDepth[waterDIndex.first][waterDIndex.second] : 50.0;
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
