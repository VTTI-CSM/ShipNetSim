#ifndef GUIUTILS_H
#define GUIUTILS_H

#include <QDir>
#include <QFileInfo>
#include <QString>
#include <QVector>
#include <QDebug>

namespace GUIUtils {

inline QString constructFullPath(const QString& directory,
                          const QString& filename,
                          const QString& extension)
{
    // Ensure the extension starts with a dot
    QString ext = extension;
    if (!ext.startsWith('.')) {
        ext.prepend('.');
    }

    // Use QFileInfo to analyze the filename
    QFileInfo fileInfo(filename);
    QString baseName = fileInfo.completeBaseName();
    QString currentExt = fileInfo.suffix();

    QString finalFileName;
    if (QString("." + currentExt).compare(ext, Qt::CaseInsensitive) != 0) {
        // Append the extension if it's different or not present
        finalFileName = baseName + ext;
    } else {
        // Use the original filename if the extension matches
        finalFileName = fileInfo.fileName();
    }

    // Combine using QDir
    QDir dir(directory);
    QString fullPath = dir.filePath(finalFileName);
    fullPath = QDir::cleanPath(fullPath);

    return fullPath;
}

inline QVector<double> factorQVector(const QVector<double>& l1, double factor) {
    QVector<double> result;
    result.reserve(l1.size());  // Preallocate memory
    for (double value : l1) {
        result.push_back(value * factor);
    }
    return result;
}

}
#endif // GUIUTILS_H
