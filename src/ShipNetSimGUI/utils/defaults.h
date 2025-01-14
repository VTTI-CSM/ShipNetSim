#ifndef DEFAULTS_H
#define DEFAULTS_H

#include <QString>
#include <QVector>

namespace Defaults
{

QVector<QString> getEarthTifPaths(const QString &basePath);

QVector<QString> getIconPaths(const QString &basePath);

QVector<QString> getHighlightedIconPaths(const QString &basePath);

QVector<QString> getTemporaryIconPaths(const QString &basePath);

QVector<QString> getShipIconPaths(const QString& basePath);
}

#endif // DEFAULTS_H
