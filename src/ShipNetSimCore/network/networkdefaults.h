#ifndef NETWORKDEFAULTS_H
#define NETWORKDEFAULTS_H

#include "qdir.h"
#include <QVector>
#include <QString>

namespace ShipNetSimCore
{
namespace NetworkDefaults
{

static const QVector<QString>& worldNetworkLocation(QString baseDirectory) {
    static QVector<QString> v
        {
            QDir(baseDirectory).filePath("ne_110m_ocean.shp")
        };
    return v;
};

static const QVector<QString>& SalinityTiffLocations(QString baseDirectory) {
    static QVector<QString> v
        {
            QDir(baseDirectory).filePath("salinity.tif")
        };
    return v;
};

static const QVector<QString>& WaveHeightTiffLocations(QString baseDirectory) {
    static QVector<QString> v
        {
            QDir(baseDirectory).filePath("waveHeight.tif")
        };
    return v;
};

static const QVector<QString>& wavePeriodTiffLocations(QString baseDirectory) {
    static QVector<QString> v
        {
            QDir(baseDirectory).filePath("wavePeriod.tif")
        };
    return v;
};

static const QVector<QString>&
windSpeedNorthTiffLocations(QString baseDirectory) {
    static QVector<QString> v
        {
            QDir(baseDirectory).filePath("WindSpeedNorth.tif")
        };
    return v;
};

static const QVector<QString>&
windSpeedEastTiffLocations(QString baseDirectory) {
    static QVector<QString> v
        {
            QDir(baseDirectory).filePath("WindSpeedEast.tif")
        };
    return v;
};

static const QVector<QString>& waterDepthTiffLocations(QString baseDirectory) {
    static QVector<QString> v
        {
            QDir(baseDirectory).filePath("WaterDepth.tif")
        };
    return v;
};

static const QVector<QString>& seaPortsLocations(QString baseDirectory) {
    static QVector<QString> v
        {
            QDir(baseDirectory).filePath("ports.geojson")
        };
    return v;
}

};
};
#endif // NETWORKDEFAULTS_H
