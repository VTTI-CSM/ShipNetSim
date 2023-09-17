#include "data.h"
#include <QFile>
#include <QTextStream>
#include <boost/algorithm/string.hpp>

template <typename T>
std::vector<T> Data::Table::getColumn(const std::string& headerName) const
{
    auto it = tableMap.find(headerName);
    if (it != tableMap.end()) {
        std::vector<T> column;
        for (const auto& cell : it->second) {
            if (std::holds_alternative<T>(cell)) {
                column.push_back(std::get<T>(cell));
            } else {
                throw std::runtime_error("Type mismatch in column: " +
                                         headerName);
            }
        }
        return column;
    }
    throw std::runtime_error("Header not found: " + headerName);
}

Data::CSV::CSV() : mFilePath("") {}

Data::CSV::CSV(const QString &filePath) :
    mFilePath(filePath), mFile(filePath) {}

Data::CSV::~CSV() {
    if(mFile.isOpen()) {
        mFile.close();
    }
}

void Data::CSV::initCSV(const QString &filePath) {
    mFilePath = filePath;
    mFile.setFileName(mFilePath);
}

bool Data::CSV::writeLine(const std::string &line) {
    if(!mFile.isOpen()) {
        if (!mFile.open(QIODevice::WriteOnly |
                        QIODevice::Text |
                        QIODevice::Append)) {
            return false;
        }
        mOutStream.setDevice(&mFile);
    }

    mOutStream << QString::fromStdString(line) << "\n";
    return true;
}

Data::Table Data::CSV::read(const std::vector<std::string>& typeSequence,
                            const bool hasHeaders,
                            const std::string& separator)
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
        std::string headerStr = headerLine.toStdString();

        boost::split(table.headers,
                     headerStr,
                     boost::is_any_of(separator));

        for (const auto& header : table.headers) {
            table.tableMap[header] = std::vector<Cell>();
        }
    }
    else
    {
        for (std::size_t i = 0; i < typeSequence.size(); ++i) {
            std::string header = "Column" + std::to_string(i);
            table.headers.push_back(header);
            table.tableMap[header] = std::vector<Cell>();
        }
    }

    while (!in.atEnd()) {
        QString line = in.readLine();
        std::string lineStr = line.toStdString();

        std::vector<std::string> rowStr;
        boost::split(rowStr,
                     lineStr,
                     boost::is_any_of(separator));

        if (rowStr.size() != typeSequence.size()) {
            throw std::runtime_error(
                "Number of columns does not match "
                "the provided type sequence");
        }

        for (std::size_t i = 0; i < table.headers.size(); ++i) {
            if (typeSequence[i] == "int") {
                table.tableMap[table.headers[i]].
                    push_back(std::stoi(rowStr[i]));
            }
            else if (typeSequence[i] == "double") {
                table.tableMap[table.headers[i]].
                    push_back(std::stod(rowStr[i]));
            }
            else if (typeSequence[i] == "string") {
                table.tableMap[table.headers[i]].
                    push_back(rowStr[i]);
            }
            else {
                throw std::runtime_error(
                    "Unknown data type in type sequence: " +
                    typeSequence[i]);
            }
        }
    }

    // Close the file after reading
    mFile.close();

    return table;
}


Data::Table Data::Table::filterTable(
    const std::string& columnName,
    std::function<bool(const Cell&)> filterFunction)
{
    Table filteredTable;

    // Copy the headers
    filteredTable.headers = headers;

    // Initialize the columns in filteredTable
    for (const auto& header : filteredTable.headers) {
        filteredTable.tableMap[header] = std::vector<Cell>();
    }

    // Check if column exists in the original table
    if (tableMap.find(columnName) == tableMap.end())
    {
        throw std::runtime_error("Column not found: "
                                 + columnName);
    }

    // Loop through rows to apply the filter
    std::size_t numRows = tableMap.begin()->second.size();

    for (std::size_t i = 0; i < numRows; ++i)
    {
        // Apply filter function
        if (filterFunction(tableMap.at(columnName)[i]))
        {
            // Copy this row to filteredTable
            for (const auto& header : filteredTable.headers)
            {
                filteredTable.tableMap[header].
                    push_back(tableMap.at(header)[i]);
            }
        }
    }

    return filteredTable;
}

Data::TXT::TXT() : mFilePath("") {}

Data::TXT::TXT(const QString &filePath) :
    mFilePath(filePath), mFile(filePath) {}

Data::TXT::~TXT() {
    if(mFile.isOpen()) {
        mFile.close();
    }
}

void Data::TXT::initTXT(const QString &filePath) {
    mFilePath = filePath;
    mFile.setFileName(mFilePath);
}

Data::Table Data::TXT::read(const std::vector<std::string>& typeSequence,
                            const std::string& separator)
{
    Table table;

    if (!mFile.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        throw std::runtime_error("Could not open file: " +
                                 mFilePath.toStdString());
    }

    QTextStream in(&mFile);

    for (std::size_t i = 0; i < typeSequence.size(); ++i) {
        std::string header = "Column" + std::to_string(i);
        table.headers.push_back(header);
        table.tableMap[header] = std::vector<Cell>();
    }


    while (!in.atEnd()) {
        QString line = in.readLine();
        std::string lineStr = line.toStdString();

        std::vector<std::string> rowStr;
        boost::split(rowStr,
                     lineStr,
                     boost::is_any_of(separator));

        if (rowStr.size() != typeSequence.size()) {
            throw std::runtime_error(
                "Number of columns does not match "
                "the provided type sequence");
        }

        for (std::size_t i = 0; i < table.headers.size(); ++i) {
            if (typeSequence[i] == "int") {
                table.tableMap[table.headers[i]].
                    push_back(std::stoi(rowStr[i]));
            }
            else if (typeSequence[i] == "double") {
                table.tableMap[table.headers[i]].
                    push_back(std::stod(rowStr[i]));
            }
            else if (typeSequence[i] == "string") {
                table.tableMap[table.headers[i]].
                    push_back(rowStr[i]);
            }
            else {
                throw std::runtime_error(
                    "Unknown data type in type sequence: " +
                    typeSequence[i]);
            }
        }
    }

    // Close the file after reading
    mFile.close();

    return table;
}

bool Data::TXT::writeFile(std::string &data)
{
    if(!mFile.isOpen()) {
        if (!mFile.open(QIODevice::WriteOnly |
                        QIODevice::Text |
                        QIODevice::Append)) {
            return false;
        }
        mOutStream.setDevice(&mFile);
    }

    mOutStream << QString::fromStdString(data) << "\n";
    mFile.close();

    return true;
}
