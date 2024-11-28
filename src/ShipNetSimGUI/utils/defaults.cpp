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
        basePath + "/placemark32.png",
    };
}

}
