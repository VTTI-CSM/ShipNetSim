#ifndef UTILS_H
#define UTILS_H

#include <any>
#include <QString>
#include <QMap>
#include <QDir>

namespace Utils
{
    template<typename T>
    inline T getValueFromMap(const QMap<QString, std::any>& parameters,
                             const QString& key, const T& defaultValue)
    {
        return parameters.contains(key) ?
                   std::any_cast<T>(parameters[key]) : defaultValue;
    }

    inline QString getHomeDirectory()
    {
        QString homeDir;

        // OS-dependent home directory retrieval
#if defined(Q_OS_WIN)
        homeDir = QDir::homePath();
#elif defined(Q_OS_LINUX) || defined(Q_OS_MAC)
        homeDir = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
#endif

        if (!homeDir.isEmpty())
        {
            // Creating a path to NeTrainSim folder in the Documents directory
            const QString documentsDir = QDir(homeDir).filePath("Documents");
            const QString folder = QDir(documentsDir).filePath("ShipNetSim");

            QDir().mkpath(folder); // Create the directory if it doesn't exist

            return folder;
        }

        throw std::runtime_error("Error: Cannot retrieve home directory!");
    }
}
#endif // UTILS_H
