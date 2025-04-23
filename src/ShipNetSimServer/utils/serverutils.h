#ifndef SERVERUTILS_H
#define SERVERUTILS_H

#include <optional>
#include <QString>
#include <QJsonObject>

namespace ServerUtils {

inline QString getOptionalString(const QJsonObject &json,
                                 const QString     &key)
{
    // First check directly in the root object
    if (json.contains(key) && json[key].isString())
    {
        QString value = json[key].toString().trimmed();
        return (value.isEmpty()) ? "default" : value;
    }

    // Then check in the params object if it exists
    if (json.contains("params")
        && json["params"].isObject())
    {
        QJsonObject params = json["params"].toObject();
        if (params.contains(key) && params[key].isString())
        {
            QString value =
                params[key].toString().trimmed();
            return (value.isEmpty()) ? "default" : value;
        }
    }

    // Return default if key is not found in either location
    return "default";
}
}
#endif // SERVERUTILS_H
