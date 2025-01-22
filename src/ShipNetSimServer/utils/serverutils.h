#ifndef SERVERUTILS_H
#define SERVERUTILS_H

#include <optional>
#include <QString>
#include <QJsonObject>

namespace ServerUtils {

inline QString getOptionalString(const QJsonObject &json,
                                                const QString &key)
{
    if (!json.contains(key) || !json[key].isString()) {
        // Return empty optional if key is missing or not a string
        return "default";
    }

    QString value = json[key].toString().trimmed();
    return (value.isEmpty()) ? "default" : value;
}

}
#endif // SERVERUTILS_H
