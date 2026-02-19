#include "network/gpoint.h"
#include "network/hierarchicalvisibilitygraph.h"
#include "network/polygon.h"
#include <QCoreApplication>
#include <QElapsedTimer>
#include <QFileInfo>
#include <gdal_priv.h>
#include <ogrsf_frmts.h>
#include <iostream>
#include <memory>

using namespace ShipNetSimCore;

static QVector<std::shared_ptr<Polygon>>
loadPolygonsFromShapefile(const QString& filepath)
{
    QVector<std::shared_ptr<Polygon>> polygons;

    GDALDataset* poDS = static_cast<GDALDataset*>(
        GDALOpenEx(filepath.toStdString().c_str(), GDAL_OF_VECTOR,
                   nullptr, nullptr, nullptr));
    if (!poDS)
    {
        std::cerr << "Error: Failed to open shapefile: "
                  << filepath.toStdString() << "\n";
        return polygons;
    }

    OGRLayer* poLayer = poDS->GetLayer(0);
    if (!poLayer)
    {
        std::cerr << "Error: No layers found in shapefile.\n";
        GDALClose(poDS);
        return polygons;
    }

    // Validate WGS84 CRS
    const OGRSpatialReference* poSRS = poLayer->GetSpatialRef();
    if (poSRS)
    {
        OGRSpatialReference wgs84SRS;
        wgs84SRS.SetWellKnownGeogCS("WGS84");
        if (!poSRS->IsSameGeogCS(&wgs84SRS))
        {
            std::cerr << "Warning: Shapefile CRS is not WGS84. "
                      << "Results may be incorrect.\n";
        }
    }
    else
    {
        std::cerr << "Warning: No spatial reference found.\n";
    }

    poLayer->ResetReading();

    int shapeID = 0;
    OGRFeature* poFeature;
    while ((poFeature = poLayer->GetNextFeature()) != nullptr)
    {
        OGRGeometry* poGeometry = poFeature->GetGeometryRef();
        if (poGeometry != nullptr
            && wkbFlatten(poGeometry->getGeometryType()) == wkbPolygon)
        {
            OGRPolygon* poPolygon =
                static_cast<OGRPolygon*>(poGeometry);
            OGRLinearRing* poExteriorRing =
                poPolygon->getExteriorRing();

            // Read exterior ring
            QVector<std::shared_ptr<GPoint>> exteriorRing;
            if (poExteriorRing != nullptr)
            {
                int nPoints = poExteriorRing->getNumPoints();
                for (int i = 0; i < nPoints; i++)
                {
                    double x = poExteriorRing->getX(i);
                    double y = poExteriorRing->getY(i);
                    exteriorRing.push_back(
                        std::make_shared<GPoint>(
                            units::angle::degree_t(x),
                            units::angle::degree_t(y)));
                }
                shapeID++;
            }

            // Read interior rings (holes)
            int nInteriorRings = poPolygon->getNumInteriorRings();
            QVector<QVector<std::shared_ptr<GPoint>>> innerHoles(
                nInteriorRings);

            for (int i = 0; i < nInteriorRings; i++)
            {
                OGRLinearRing* poInteriorRing =
                    poPolygon->getInteriorRing(i);
                int nPoints = poInteriorRing->getNumPoints();
                for (int j = 0; j < nPoints; j++)
                {
                    double x = poInteriorRing->getX(j);
                    double y = poInteriorRing->getY(j);
                    innerHoles[i].push_back(
                        std::make_shared<GPoint>(
                            units::angle::degree_t(x),
                            units::angle::degree_t(y)));
                }
            }

            auto polygon = std::make_shared<Polygon>(
                exteriorRing, innerHoles,
                QString::number(shapeID));
            polygons.push_back(polygon);
        }
        OGRFeature::DestroyFeature(poFeature);
    }

    GDALClose(poDS);
    return polygons;
}

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("ShipNetSimAdjBuilder");

    // Parse arguments
    if (argc < 2 || argc > 3)
    {
        std::cerr << "Usage: ShipNetSimAdjBuilder <shapefile.shp> "
                  << "[output.hvg_adj]\n\n"
                  << "Builds a .hvg_adj adjacency cache file for the "
                  << "given shapefile.\n"
                  << "If output path is not specified, saves alongside "
                  << "the shapefile.\n";
        return 1;
    }

    QString shapefilePath = QString::fromLocal8Bit(argv[1]);
    QFileInfo shapefileInfo(shapefilePath);

    if (!shapefileInfo.exists())
    {
        std::cerr << "Error: Shapefile does not exist: "
                  << shapefilePath.toStdString() << "\n";
        return 1;
    }

    // Determine output path
    QString outputPath;
    if (argc == 3)
    {
        outputPath = QString::fromLocal8Bit(argv[2]);
    }
    else
    {
        outputPath = shapefileInfo.absolutePath() + "/"
                     + shapefileInfo.completeBaseName() + ".hvg_adj";
    }

    std::cout << "ShipNetSimAdjBuilder - Adjacency Cache Builder\n"
              << "================================================\n"
              << "Shapefile: " << shapefilePath.toStdString() << "\n"
              << "Output:    " << outputPath.toStdString() << "\n\n";

    // Initialize GDAL
    GDALAllRegister();

    // Step 1: Load polygons from shapefile
    std::cout << "[1/4] Loading polygons from shapefile...\n";
    QElapsedTimer timer;
    timer.start();

    auto polygons = loadPolygonsFromShapefile(shapefilePath);

    if (polygons.isEmpty())
    {
        std::cerr << "Error: No polygons loaded from shapefile.\n";
        return 1;
    }

    double loadTime = timer.elapsed() / 1000.0;
    std::cout << "      Loaded " << polygons.size() << " polygons in "
              << loadTime << "s\n\n";

    // Step 2: Build HierarchicalVisibilityGraph (builds all 4 levels,
    //         adjacency for levels 1-3)
    std::cout << "[2/4] Building HierarchicalVisibilityGraph "
              << "(levels 0-3)...\n";
    timer.restart();

    auto hvg = std::make_shared<HierarchicalVisibilityGraph>(
        std::as_const(polygons));

    double hvgTime = timer.elapsed() / 1000.0;
    std::cout << "      HVG built in " << hvgTime << "s\n\n";

    // Step 3: Build Level 0 adjacency (synchronous - this is what
    //         crashes in the main app)
    std::cout << "[3/4] Building Level 0 adjacency "
              << "(this may take a while)...\n";
    timer.restart();

    hvg->buildLevel0Adjacency();

    double adjTime = timer.elapsed() / 1000.0;
    std::cout << "      Level 0 adjacency built in " << adjTime
              << "s\n\n";

    // Step 4: Save adjacency cache
    std::cout << "[4/4] Saving adjacency cache...\n";
    timer.restart();

    bool saved = hvg->saveAdjacencyCache(outputPath);

    double saveTime = timer.elapsed() / 1000.0;

    if (!saved)
    {
        std::cerr << "Error: Failed to save adjacency cache to "
                  << outputPath.toStdString() << "\n";
        return 1;
    }

    QFileInfo outputInfo(outputPath);
    double fileSizeMB = outputInfo.size() / (1024.0 * 1024.0);

    std::cout << "      Saved in " << saveTime << "s\n\n";

    // Summary
    double totalTime = loadTime + hvgTime + adjTime + saveTime;
    std::cout << "================================================\n"
              << "Summary:\n"
              << "  Polygons:    " << polygons.size() << "\n"
              << "  File size:   " << fileSizeMB << " MB\n"
              << "  Total time:  " << totalTime << "s\n"
              << "  Output:      " << outputPath.toStdString() << "\n"
              << "================================================\n"
              << "Done.\n";

    return 0;
}
