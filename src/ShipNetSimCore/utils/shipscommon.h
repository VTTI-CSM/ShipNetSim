#ifndef SHIPSCOMMON_H
#define SHIPSCOMMON_H

#include "../export.h"
#include "qjsonarray.h"
#include "qjsonobject.h"
#include <QFile>
#include <QObject>
#include <QFileInfo>
#include <QTextStream>
#include <QString>
#include <QVector>
#include <QDataStream>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>

struct SHIPNETSIM_EXPORT ShipsResults {
private:
    // Constants
    static const qint64 MAX_TRAJECTORY_SIZE = 1024 * 1024; // 1 MB in bytes
    static const int COMPRESSION_LEVEL = 9; // Highest compression level


    QVector<QPair<QString, QString>> summaryData;  // Stores the summary
        // data (key-value pairs)
    QByteArray trajectoryFileData;// Stores the content of the trajectory file
    QString trajectoryFileName;  // Stores the full path of the trajectory file
    QString summaryFileName;     // Stores the full path of the summary file

public:

    // Default constructor
    ShipsResults() : summaryData(), trajectoryFileData(),
        trajectoryFileName(), summaryFileName() {}

    // Constructor with summary and trajectory file paths
    ShipsResults(QVector<QPair<QString, QString>> summary,
                 QString trajectoryFilePath, QString summaryFilePath)
        : summaryData(summary), summaryFileName(summaryFilePath) {
        // Load the trajectory file content if the file path is valid
        if (!trajectoryFilePath.isEmpty() &&
            QFile::exists(trajectoryFilePath)) {
            trajectoryFileData.clear();
            // Store the entire trajectory file path
            trajectoryFileName = trajectoryFilePath;
        } else {
            trajectoryFileData.clear(); // Handle invalid file path case
            trajectoryFileName.clear();
            qWarning("Invalid trajectory file path or file does not exist.");
        }
    }

    QVector<QPair<QString, QString>> getSummaryData() { return summaryData; }
    QByteArray getTrajectoryFileData() { return trajectoryFileData; }

    // Function to load the trajectory file content into QByteArray
    void loadTrajectoryFile(const QString& filePath = QString()) {
        QString fPath;
        fPath = (filePath.isEmpty()) ? trajectoryFileName : filePath;
        if (fPath.isEmpty()) return;

        QFile file(fPath);
        if (!file.open(QIODevice::ReadOnly)) {
            qWarning("Cannot open trajectory file for reading.");
            return;
        }

        QByteArray rawData = file.readAll();
        trajectoryFileData = qCompress(rawData, COMPRESSION_LEVEL);
    }

    // Function to save the trajectory file content to an optional new path
    bool saveTrajectoryFile(const QString& newPath = QString()) const {
        QString savePath = newPath.isEmpty() ? trajectoryFileName : newPath;

        if (savePath.isEmpty() || trajectoryFileData.isEmpty()) {
            qWarning("No valid trajectory file data to save.");
            return false;
        }

        QFile file(savePath);
        if (!file.open(QIODevice::WriteOnly)) {
            qWarning("Cannot open trajectory file for writing.");
            return false;
        }
        file.write(qUncompress(trajectoryFileData));
        return true;
    }

    // Function to save the summary data to an optional new path
    bool saveSummaryFile(const QString& newPath = QString()) const {
        QString savePath = newPath.isEmpty() ? summaryFileName : newPath;

        if (savePath.isEmpty() || summaryData.isEmpty()) {
            qWarning("No valid summary data to save.");
            return false;
        }

        QFile file(savePath);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            qWarning("Cannot open summary file for writing.");
            return false;
        }

        QTextStream out(&file);
        for (const auto& pair : summaryData) {
            // Write each key-value pair as a line
            out << pair.first << ": " << pair.second << "\n";
        }

        return true;
    }

    // Function to get only the filename from the
    // full path of the trajectory file
    QString getTrajectoryFileName() const {
        QFileInfo fileInfo(trajectoryFileName);
        // Returns only the filename without the path
        return fileInfo.fileName();
    }

    // Function to get only the filename from the
    // full path of the summary file
    QString getSummaryFileName() const {
        QFileInfo fileInfo(summaryFileName);
        // Returns only the filename without the path
        return fileInfo.fileName();
    }

    // Function to get the summary data
    QVector<QPair<QString, QString>> getSummaryData() const
    { return summaryData; }

    // Function to get the trajectory data
    QByteArray getTrajectoryFileData() const {
        return trajectoryFileData;
    }

    // New function to convert ShipsResults to JSON
    QJsonObject toJson() const {
        QJsonObject json;

        // Convert summaryData to JSON array
        QJsonArray summaryArray;
        for (const auto& pair : summaryData) {
            QJsonObject pairObject;
            pairObject[pair.first] = pair.second;
            summaryArray.append(pairObject);
        }
        json["summaryData"] = summaryArray;

        // Check size of trajectoryFileData
        if (trajectoryFileData.size() <= MAX_TRAJECTORY_SIZE) {
            // Convert trajectoryFileData to base64 string
            json["trajectoryFileData"] = QString(trajectoryFileData.toBase64());
            json["trajectoryFileDataIncluded"] = true;
        } else {
            json["trajectoryFileDataIncluded"] = false;
        }

        json["trajectoryFileName"] = trajectoryFileName;
        json["summaryFileName"] = summaryFileName;

        return json;
    }

    // New function to convert JSON to ShipsResults
    static ShipsResults fromJson(const QJsonObject& json) {
        ShipsResults results;

        // Parse summaryData
        QJsonArray summaryArray = json["summaryData"].toArray();
        for (const auto& value : summaryArray) {
            QJsonObject pairObject = value.toObject();
            QString key = pairObject.keys().first();
            results.summaryData.append(qMakePair(key, pairObject[key].toString()));
        }

        // Parse trajectoryFileData if included
        if (json["trajectoryFileDataIncluded"].toBool(false)) {
            QByteArray base64Data = json["trajectoryFileData"].toString().toUtf8();
            results.trajectoryFileData = QByteArray::fromBase64(base64Data);
        } else {
            results.trajectoryFileData.clear();
        }

        results.trajectoryFileName = json["trajectoryFileName"].toString();
        results.summaryFileName = json["summaryFileName"].toString();

        return results;
    }

    // Function to get JSON string representation
    QString toJsonString() const {
        QJsonDocument doc(toJson());
        return QString(doc.toJson(QJsonDocument::Compact));
    }

    // Serialization operator (<<)
    friend QDataStream &operator<<(QDataStream &out,
                                   const ShipsResults &results) {
        out << results.summaryData << results.trajectoryFileData
            << results.trajectoryFileName << results.summaryFileName;
        return out;
    }

    // Deserialization operator (>>)
    friend QDataStream &operator>>(QDataStream &in,
                                   ShipsResults &results) {
        in >> results.summaryData >> results.trajectoryFileData
            >> results.trajectoryFileName >> results.summaryFileName;
        return in;
    }
};
Q_DECLARE_METATYPE(ShipsResults)


#endif // SHIPSCOMMON_H
