#include "defaults.h"

namespace Defaults
{

QVector<QString> getEarthTifPaths(const QString &basePath) {
    return {
        basePath + "/NE1_HR_LC_SR_W_DR.earth",
    };
}

QVector<QString> getIconPaths(const QString &basePath) {
    return {
        basePath + "/portIcon25px.png",
    };
}

QVector<QString> getHighlightedIconPaths(const QString &basePath) {
    return {
        basePath + "/portIcon25pxHighlight.png"
    };
}

QVector<QString> getTemporaryIconPaths(const QString &basePath) {
    return {
        basePath + "/PortIcon25pxTemp.png"
    };
}

QVector<QString> getShipIconPaths(const QString& basePath) {
    return {
        basePath + "/shipIcon.png",
    };
}

}
