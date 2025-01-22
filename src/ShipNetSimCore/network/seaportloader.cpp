#include "seaportloader.h"
#include "networkdefaults.h"
#include "gpoint.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <limits>
#include <ogrsf_frmts.h>
#include "../utils/utils.h"
#include "seaport.h"

namespace ShipNetSimCore {

// Initialize static members
QThreadStorage<QVector<std::shared_ptr<SeaPort>>> SeaPortLoader::mPorts;

SeaPortLoader::SeaPortLoader() {}

bool SeaPortLoader::loadPortsFromFile(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to open port data file:" << filePath;
        return false;
    }

    // Clear existing ports
    mPorts.localData().clear();

    try {
        mPorts.localData() =
            readSeaPorts(filePath.toStdString().c_str());
    } catch (std::exception &e) {
        qWarning() << "Error loading sea ports from file: " << filePath;
    }

    return true;
}

std::shared_ptr<SeaPort>
SeaPortLoader::getClosestPortToPoint(const std::shared_ptr<GPoint> point,
                                     units::length::meter_t maxDistance) {
    if (!point) {
        return nullptr;
    }

    if (mPorts.localData().isEmpty()) {
        mPorts.localData() = loadFirstAvailableSeaPorts();
    }

    if (mPorts.localData().isEmpty()) {
        return nullptr;
    }

    std::shared_ptr<SeaPort> closestPort = nullptr;
    units::length::meter_t minDistance =
        units::length::meter_t(std::numeric_limits<double>::max());

    for (const auto& port : mPorts.localData()) {
        // If port has a water point, use that for distance calculation
        std::shared_ptr<GPoint> portPoint =
            port->getClosestPointOnWaterPolygon();
        if (!portPoint) {
            // If no water point, use port coordinates
            auto portCoord =
                std::make_shared<GPoint>(port->getPortCoordinate());
            portPoint = portCoord;
        }

        try {
            units::length::meter_t distance = point->distance(*portPoint);
            if (distance < minDistance && distance <= maxDistance) {
                minDistance = distance;
                closestPort = port;
            }
        } catch (const std::exception& e) {
            qWarning() << "Error calculating distance to port"
                       << port->getPortName()
                       << ":" << e.what();
            continue;
        }
    }

    return closestPort;
}

QVector<std::shared_ptr<SeaPort> >
SeaPortLoader::readSeaPorts(const char* filename)
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

QVector<std::shared_ptr<SeaPort>>
SeaPortLoader::loadFirstAvailableSeaPorts()
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

bool SeaPortLoader::loadFirstAvailableSeaPortsFile(
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
        mPorts.localData() = readSeaPorts(
            filePath.toStdString().c_str());
        return true;
    }
}

QVector<std::shared_ptr<SeaPort> > SeaPortLoader::getPorts()
{
    if (mPorts.localData().isEmpty()) {
        mPorts.localData() = loadFirstAvailableSeaPorts();
    }
    return mPorts.localData();
}

} // namespace ShipNetSimCore
