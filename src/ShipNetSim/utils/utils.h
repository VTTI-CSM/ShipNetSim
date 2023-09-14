#ifndef UTILS_H
#define UTILS_H

#include <any>
#include <QString>
#include <QMap>

namespace Utils
{
    template<typename T>
    inline T getValueFromMap(const QMap<QString, std::any>& parameters,
                             const QString& key, const T& defaultValue)
    {
        return parameters.contains(key) ?
                   std::any_cast<T>(parameters[key]) : defaultValue;
    }
}
#endif // UTILS_H
