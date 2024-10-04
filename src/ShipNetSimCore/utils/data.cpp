#include "data.h"
#include <QFile>
#include <QTextStream>
#include <QDomDocument>

namespace ShipNetSimCore
{

template <typename T>
QVector<T> Data::Table::getColumn(const QString& headerName) const
{
    auto it = tableMap.find(headerName);
    if (it != tableMap.end()) {
        QVector<T> column;
        for (const auto& cell : it.value()) {
            if (std::holds_alternative<T>(cell)) {
                column.push_back(std::get<T>(cell));
            } else {
                throw std::runtime_error("Type mismatch in column: " +
                                         headerName.toStdString());
            }
        }
        return column;
    }
    throw std::runtime_error("Header not found: " + headerName.toStdString());
}

Data::CSV::CSV() : mFilePath("") {}

Data::CSV::CSV(const QString &filePath) :
    mFilePath(filePath), mFile(filePath) {}

Data::CSV::~CSV() {
    close();
}

void Data::CSV::initCSV(const QString &filePath) {
    mFilePath = filePath;
    mFile.setFileName(mFilePath);
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

Data::Table Data::CSV::read(const QVector<QString>& typeSequence,
                            const bool hasHeaders,
                            const QString& separator)
{
    Table table;

    if (!mFile.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        throw std::runtime_error("Could not open file: " +
                                 mFilePath.toStdString());
    }

    QTextStream in(&mFile);

    if (hasHeaders) {
        QString headerLine = in.readLine();

        headerLine.split(separator).toVector();

        for (const auto& header : table.headers) {
            table.tableMap[header] = QVector<Cell>();
        }
    }
    else
    {
        for (qsizetype i = 0; i < typeSequence.size(); ++i)
        {
            QString header = "Column" + QString::number(i);
            table.headers.push_back(header);
            table.tableMap[header] = QVector<Cell>();
        }
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
                table.tableMap[table.headers[i]].push_back(rowStr[i]);
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

void Data::CSV::close()
{
    if (mFile.isOpen())
    {
        mOutStream.flush();  // Ensure stream is flushed
        mFile.close();
    }
}

Data::Table Data::Table::filterTable(
    const QString& columnName,
    std::function<bool(const Cell&)> filterFunction)
{
    Table filteredTable;

    // Copy the headers
    filteredTable.headers = headers;

    // Initialize the columns in filteredTable
    for (const auto& header : filteredTable.headers) {
        filteredTable.tableMap[header] = QVector<Cell>();
    }

    // Check if column exists in the original table
    if (tableMap.find(columnName) == tableMap.end())
    {
        throw std::runtime_error("Column not found: "
                                 + columnName.toStdString());
    }

    // Loop through rows to apply the filter
    std::size_t numRows = tableMap[columnName].size();

    for (std::size_t i = 0; i < numRows; ++i)
    {
        // Apply filter function
        if (filterFunction(tableMap[columnName][i]))
        {
            // Copy this row to filteredTable
            for (const auto& header : filteredTable.headers)
            {
                filteredTable.tableMap[header].
                    push_back(tableMap[header][i]);
            }
        }
    }

    return filteredTable;
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
                table.tableMap[table.headers[i]].push_back(rowStr[i]);
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
    QDomElement shipsElement       = root.firstChildElement("ShipssFileName");
    QDomElement simEndTimeElement   = root.firstChildElement("simEndTime");
    QDomElement simTimestepElement  = root.firstChildElement("simTimestep");
    QDomElement simPlotTimeElement  = root.firstChildElement("simPlotTime");

    Data::ProjectFile::ProjectDataFile pf;
    // Get the text values
    pf.projectName       = projectElement.text();
    pf.networkName       = networkElement.text();
    pf.authorName        = authorElement.text();
    pf.shipsFileName    = shipsElement.text();
    pf.simEndTime        = simEndTimeElement.text();
    pf.simTimestep       = simTimestepElement.text();
    pf.simPlotTime       = simPlotTimeElement.text();

    // Return the extracted values as a tuple
    return pf;
};
};
