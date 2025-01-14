#include "data.h"
#include <QFile>
#include <QTextStream>
#include <QDomDocument>
#include <iostream>
#include <QtConcurrent>
#include <QFuture>
#include <QMutex>
#include <QSet>
#include <QVector>

namespace ShipNetSimCore
{

Data::CSV::CSV() : mFilePath("") {}

Data::CSV::CSV(const QString &filePath) :
    mFilePath(filePath), mFile(filePath) {
    std::cout << filePath.toStdString() << std::endl;
}

Data::CSV::~CSV() {
    close();
}

void Data::CSV::initCSV(const QString &filePath) {
    mFilePath = filePath;
    mFile.setFileName(mFilePath);

    std::cout << filePath.toStdString() << std::endl;
}

bool Data::CSV::writeLine(const QString &line) {
    if(!mFile.isOpen()) {
        if (!mFile.open(QIODevice::WriteOnly |
                        QIODevice::Text |
                        QIODevice::Append)) {
            return false;
        }
        mOutStream.setDevice(&mFile);
    }

    mOutStream << line << Qt::endl;  // Use Qt::endl to flush the stream
    mOutStream.flush();  // Explicitly flush the stream

    if (mOutStream.status() != QTextStream::Ok) {
        return false;
    }

    return true;
}

bool Data::CSV::writeLine(const QVector<QString> &lineDetails,
                          const QString separator)
{
    return writeLine(lineDetails.join(separator));
}

bool Data::CSV::clearFile()
{
    if (mFile.isOpen()) {
        mFile.close();  // Close the file if it’s open
    }

    // Open the file in write-only mode without append to truncate its content
    if (!mFile.open(QIODevice::WriteOnly |
                    QIODevice::Text |
                    QIODevice::Truncate)) {
        return false;  // Return false if unable to open the file
    }

    mFile.close();  // Close immediately after truncation
    return true;    // Indicate success
}

Data::Table Data::CSV::read(
    const bool hasHeaders,
    const QString& separator,
    std::function<bool(const QString&)> filterFunc,
    int filterColumnIndex)
{
    QFile file(mFilePath);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        throw std::runtime_error("Could not open file: "
                                 + mFilePath.toStdString());
    }

    QTextStream in(&file);

    QVector<QString> headers;

    // Read and process header if present
    if (hasHeaders && !in.atEnd()) {
        QString headerLine = in.readLine();
        headers = headerLine.split(separator);
    }

    // Define the number of lines per chunk
    const int LINES_PER_CHUNK = 10'000; // low for high performance and memory

    QList<QFuture<QVector<QVector<QVariant>>>> futures;

    // Define a mutex if needed for shared data (currently not required)
    // QMutex mutex;

    // Define a lambda for processing lines
    auto processLines = [&](const QStringList& lines) ->
        QVector<QVector<QVariant>>
    {
        QVector<QVector<QVariant>> localData;
        for (const QString& rawLine : lines) {
            QString line = rawLine.trimmed();
            if (line.endsWith('\r'))
                line.chop(1);  // Remove carriage return if present

            QVector<QString> rowStr = line.split(separator,
                                                 Qt::KeepEmptyParts);

            // Apply filtering function if provided
            if (filterFunc && filterColumnIndex >= 0 &&
                filterColumnIndex < rowStr.size() &&
                !filterFunc(rowStr[filterColumnIndex])) {
                continue;  // Skip this row
            }

            // Convert rowStr to QVariant
            QVector<QVariant> rowData;
            for (const QString& value : rowStr) {
                rowData.append(QVariant(value));
            }

            localData.append(rowData);
        }
        return localData;
    };

    // Read and dispatch chunks
    while (!in.atEnd()) {
        QStringList lines;
        lines.reserve(LINES_PER_CHUNK);

        // Collect lines for the current chunk
        for (int i = 0; i < LINES_PER_CHUNK && !in.atEnd(); ++i) {
            QString line = in.readLine();
            lines.append(line);
        }

        // Launch a thread to process these lines
        futures.append(QtConcurrent::run(processLines, lines));
    }

    file.close();

    // Construct the final Table
    Table table;

    // Initialize tableMap with headers
    if (hasHeaders) {
        table.headers = headers;
    } else {
        // Headers will be generated based on the first row
        // This will be handled during data mapping
    }

    // Initialize tableMap after all data is collected
    // To determine the maximum number of columns
    QVector<QVector<QVariant>> allData;

    // Wait for all threads to finish and collect data
    for (auto &future : futures) {
        QVector<QVector<QVariant>> chunkData = future.result();
        allData.append(chunkData);
    }

    // Determine final headers
    if (!hasHeaders) {
        if (!allData.isEmpty()) {
            int maxColumns = 0;
            for (const auto& row : allData) {
                if (row.size() > maxColumns)
                    maxColumns = row.size();
            }
            for (int i = 0; i < maxColumns; ++i) {
                headers.append("Column" + QString::number(i + 1));
            }
            table.headers = headers;
        }
    } else {
        // If headers are provided, check if any row has more columns
        int headerCount = headers.size();
        int maxColumns = headerCount;
        for (const auto& row : allData) {
            if (row.size() > maxColumns)
                maxColumns = row.size();
        }

        if (maxColumns > headerCount) {
            // Extend headers to match maxColumns
            for (int i = headerCount; i < maxColumns; ++i) {
                headers.append("Column" + QString::number(i + 1));
            }
            table.headers = headers;
        } else {
            table.headers = headers;
        }
    }

    // Initialize tableMap with headers
    for (const QString& header : table.headers) {
        table.tableMap[header] = QVector<QVariant>();
    }

    // Fill tableMap with data
    for (const QVector<QVariant>& row : allData) {
        for (int i = 0; i < table.headers.size(); ++i) {
            if (i < row.size()) {
                table.tableMap[table.headers[i]].append(row[i]);
            } else {
                table.tableMap[table.headers[i]].
                    append(QVariant("")); // Fill missing with empty QVariant
            }
        }

        // Handle extra columns if any (only if headers were extended)
        if (row.size() > table.headers.size()) {
            for (int i = table.headers.size(); i < row.size(); ++i) {
                QString newHeader = "Column" + QString::number(i + 1);
                table.headers.append(newHeader);
                table.tableMap[newHeader] = QVector<QVariant>();
                table.tableMap[newHeader].append(row[i]);
            }
        }
    }

    return table;
}



QVector<QString> Data::CSV::getDistinctValuesFromCSV(
    const bool hasHeaders,
    int columnIndex,
    const QString& separator)
{
    QFile file(mFilePath);  // Local instance

    if (!file.open(QIODevice::ReadOnly)) {
        throw std::runtime_error("Could not open file: "
                                 + mFilePath.toStdString());
    }

    QTextStream in(&file);
    bool skipHeader = hasHeaders;

    constexpr qint64 CHUNK_SIZE = 10 * 1024 * 1024;  // Process 10MB at a time
    QList<QFuture<QSet<QString>>> futures; // Store futures for parallel processing
    QMutex mutex; // Mutex for safe file reading

    while (!in.atEnd()) {
        QByteArray chunkData;

        // Lock file access to read a chunk
        {
            QMutexLocker locker(&mutex);
            chunkData = file.read(CHUNK_SIZE); // Read a chunk of the file
        }

        // Process chunk in a separate thread
        futures.append(QtConcurrent::run([chunkData, separator,
                                          columnIndex, skipHeader]() ->
                                         QSet<QString> {
            QSet<QString> localSet;
            QTextStream chunkStream(chunkData);
            bool localSkipHeader = skipHeader;

            while (!chunkStream.atEnd()) {
                QString line = chunkStream.readLine();
                if (line.endsWith("\r")) {
                    line.chop(1); // Handle Windows \r\n issue
                }

                if (localSkipHeader) {
                    localSkipHeader = false;
                    continue;
                }

                QVector<QString> rowStr;
                int start = 0;
                for (int i = 0; i < line.size(); ++i) {
                    if (line[i] == separator) {
                        rowStr.push_back(line.mid(start, i - start));
                        start = i + 1;
                    }
                }
                rowStr.push_back(line.mid(start)); // Last column

                if (columnIndex >= 0 && columnIndex < rowStr.size()) {
                    localSet.insert(std::move(rowStr[columnIndex]));
                }
            }

            return localSet;
        }));
    }

    file.close();

    // Merge results from all threads
    QSet<QString> distinctValues;
    for (auto &future : futures) {
        distinctValues.unite(future.result()); // Combine results
    }

    return QVector<QString>(distinctValues.begin(), distinctValues.end());
}

void Data::CSV::close()
{
    if (mFile.isOpen())
    {
        mOutStream.flush();  // Ensure stream is flushed
        mFile.close();
    }
}



Data::TXT::TXT() : mFilePath("") {}

Data::TXT::TXT(const QString &filePath) :
    mFilePath(filePath), mFile(filePath) {}

Data::TXT::~TXT() {
    close();
}

void Data::TXT::initTXT(const QString &filePath) {
    mFilePath = filePath;
    mFile.setFileName(mFilePath);
}

Data::Table Data::TXT::read(const QVector<QString>& typeSequence,
                            const QString& separator)
{
    Table table;

    if (!mFile.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        throw std::runtime_error("Could not open file: " +
                                 mFilePath.toStdString());
    }

    QTextStream in(&mFile);

    for (qsizetype i = 0; i < typeSequence.size(); ++i) {
        QString header = "Column" + QString::number(i);
        table.headers.push_back(header);
        table.tableMap[header] = QVector<Cell>();
    }


    while (!in.atEnd()) {
        QString line = in.readLine();

        QVector<QString> rowStr;
        rowStr = line.split(separator).toVector();

        if (rowStr.size() != typeSequence.size()) {
            throw std::runtime_error(
                "Number of columns does not match "
                "the provided type sequence");
        }

        for (qsizetype i = 0; i < table.headers.size(); ++i) {
            bool conversionSuccessful = false;

            if (typeSequence[i] == "int") {
                int value = rowStr[i].toInt(&conversionSuccessful);
                if (conversionSuccessful) {
                    table.tableMap[table.headers[i]].push_back(value);
                } else {
                    throw std::runtime_error("Failed to convert to int: " +
                                             rowStr[i].toStdString());
                }
            }
            else if (typeSequence[i] == "double") {
                double value = rowStr[i].toDouble(&conversionSuccessful);
                if (conversionSuccessful) {
                    table.tableMap[table.headers[i]].push_back(value);
                } else {
                    throw std::runtime_error("Failed to convert to double: " +
                                             rowStr[i].toStdString());
                }
            }
            else if (typeSequence[i] == "string") {
                table.tableMap[table.getHeaders()[i]].push_back(rowStr[i]);
            }
            else {
                throw std::runtime_error(
                    "Unknown data type in type sequence: " +
                    typeSequence[i].toStdString());
            }
        }
    }

    // Close the file after reading
    mFile.close();

    return table;
}

bool Data::TXT::writeFile(QString &data)
{
    if(!mFile.isOpen()) {
        if (!mFile.open(QIODevice::WriteOnly |
                        QIODevice::Text |
                        QIODevice::Append)) {
            return false;
        }
        mOutStream.setDevice(&mFile);
    }

    mOutStream << data << Qt::endl;  // Use Qt::endl to flush the stream
    mOutStream.flush();  // Explicitly flush the stream

    if (mOutStream.status() != QTextStream::Ok) {
        return false;
    }

    return true;
}

bool Data::TXT::clearFile()
{
    if (mFile.isOpen()) {
        mFile.close();  // Close the file if it’s open
    }

    // Open the file in write-only mode without append to truncate its content
    if (!mFile.open(QIODevice::WriteOnly |
                    QIODevice::Text |
                    QIODevice::Truncate)) {
        return false;  // Return false if unable to open the file
    }

    mFile.close();  // Close immediately after truncation
    return true;    // Indicate success
}

void Data::TXT::close()
{
    if (mFile.isOpen())
    {
        mFile.close();
    }
}

void Data::ProjectFile::createProjectFile(const ProjectDataFile pf,
                                          QString& filename)
{
    // Create a QDomDocument object
    QDomDocument doc;

    // Create the root element
    QDomElement root = doc.createElement("Data");
    doc.appendChild(root);

    // Create project name element and set its text
    QDomElement projectElement = doc.createElement("ProjectName");
    QDomText projectText = doc.createTextNode(pf.projectName);
    projectElement.appendChild(projectText);
    root.appendChild(projectElement);

    // Create network name element and set its text
    QDomElement networkElement = doc.createElement("NetworkName");
    QDomText networkText = doc.createTextNode(pf.networkName);
    networkElement.appendChild(networkText);
    root.appendChild(networkElement);

    // Create author name element and set its text
    QDomElement authorElement = doc.createElement("AuthorName");
    QDomText authorText = doc.createTextNode(pf.authorName);
    authorElement.appendChild(authorText);
    root.appendChild(authorElement);

    // Create ships file name element and set its text
    QDomElement shipsElement = doc.createElement("ShipsFileName");
    QDomText shipsText = doc.createTextNode(pf.shipsFileName);
    shipsElement.appendChild(shipsText);
    root.appendChild(shipsElement);

    // Create simEndTime name element and set its text
    QDomElement ElementSimEndTime = doc.createElement("simEndTime");
    QDomText simEndTimeText = doc.createTextNode(pf.simEndTime);
    ElementSimEndTime.appendChild(simEndTimeText);
    root.appendChild(ElementSimEndTime);

    // Create simTimestep name element and set its text
    QDomElement ElementsimTimestep = doc.createElement("simTimestep");
    QDomText simTimestepText = doc.createTextNode(pf.simTimestep);
    ElementsimTimestep.appendChild(simTimestepText);
    root.appendChild(ElementsimTimestep);

    // Create simTimestep name element and set its text
    QDomElement ElementsimPlotTime = doc.createElement("simPlotTime");
    QDomText simPlotTimeText = doc.createTextNode(pf.simPlotTime);
    ElementsimPlotTime.appendChild(simPlotTimeText);
    root.appendChild(ElementsimPlotTime);

    // Create a QFile object to write the XML file
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        throw std::runtime_error("Error: "
                                 "Failed to open the file for writing.");
    }

    // Create a QTextStream object to write the XML content to the file
    QTextStream out(&file);
    out.setDevice(&file); // Set the device of the QTextStream
    out.setEncoding(QStringConverter::Utf8);
    doc.save(out, 4); // Save the XML document with indentation

    // Close the file
    file.close();
}

Data::ProjectFile::ProjectDataFile
Data::ProjectFile::readProjectFile(const QString& filename)
{
    // Create a QDomDocument object
    QDomDocument doc;

    // Create a QFile object to read the XML file
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        throw std::runtime_error("Error: Failed to open the file for reading.");
    }

    // Set the content of the QDomDocument with the file content
    if (!doc.setContent(&file)) {
        file.close();
        throw std::runtime_error("Error: Failed to parse the XML file.");
    }

    // Close the file
    file.close();

    // Get the root element
    QDomElement root = doc.documentElement();

    // Get the child elements
    QDomElement projectElement      = root.firstChildElement("ProjectName");
    QDomElement networkElement      = root.firstChildElement("NetworkName");
    QDomElement authorElement       = root.firstChildElement("AuthorName");
    QDomElement shipsElement        = root.firstChildElement("ShipsFileName");
    QDomElement simEndTimeElement   = root.firstChildElement("simEndTime");
    QDomElement simTimestepElement  = root.firstChildElement("simTimestep");
    QDomElement simPlotTimeElement  = root.firstChildElement("simPlotTime");

    Data::ProjectFile::ProjectDataFile pf;
    // Get the text values
    pf.projectName       = projectElement.text();
    pf.networkName       = networkElement.text();
    pf.authorName        = authorElement.text();
    pf.shipsFileName     = shipsElement.text();
    pf.simEndTime        = simEndTimeElement.text();
    pf.simTimestep       = simTimestepElement.text();
    pf.simPlotTime       = simPlotTimeElement.text();

    // Return the extracted values as a tuple
    return pf;
};

};
