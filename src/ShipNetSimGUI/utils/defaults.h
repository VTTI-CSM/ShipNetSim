#ifndef DEFAULTS_H
#define DEFAULTS_H

#include <QString>
#include <QVector>

namespace Defaults
{

QVector<QString> getEarthTifPaths(const QString &basePath);

QVector<QString> getIconPaths(const QString &basePath);

}

#endif // DEFAULTS_H
