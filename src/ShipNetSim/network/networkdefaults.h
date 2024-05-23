#ifndef NETWORKDEFAULTS_H
#define NETWORKDEFAULTS_H

#include <QVector>
#include <QString>

namespace ShipNetSimCore
{
namespace NetworkDefaults
{


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

};
};
#endif // NETWORKDEFAULTS_H
