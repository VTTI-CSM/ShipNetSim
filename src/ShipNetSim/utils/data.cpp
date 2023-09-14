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

Data::Table Data::CSV::read(const QString& filePath,
                         const std::vector<std::string>& typeSequence,
                         const std::string& separator)
{
    Table table;
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        throw std::runtime_error("Could not open file: " +
                                 filePath.toStdString());
    }

    QTextStream in(&file);
    bool firstLine = true;
    while (!in.atEnd()) {
        QString line = in.readLine();
        std::string lineStr = line.toStdString();

        if (firstLine) {
            boost::split(table.headers,
                         lineStr,
                         boost::is_any_of(separator));

            for (const auto& header : table.headers)
            {
                table.tableMap[header] = std::vector<Cell>();
            }
            firstLine = false;
            continue;
        }

        std::vector<std::string> rowStr;
        boost::split(rowStr,
                     lineStr,
                     boost::is_any_of(separator));

        if (rowStr.size() != typeSequence.size()) {
            throw std::runtime_error(
                "Number of columns does not "
                "match the provided type sequence");
        }

        for (std::size_t i = 0; i < table.headers.size(); ++i) {
            if (typeSequence[i] == "int")
            {
                table.tableMap[table.headers[i]].
                    push_back(std::stoi(rowStr[i]));
            }
            else if (typeSequence[i] == "double")
            {
                table.tableMap[table.headers[i]].
                    push_back(std::stod(rowStr[i]));
            }
            else if (typeSequence[i] == "string")
            {
                table.tableMap[table.headers[i]].
                    push_back(rowStr[i]);
            }
            else
            {
                throw std::runtime_error(
                    "Unknown data type in type sequence: " +
                    typeSequence[i]);
            }
        }
    }
    return table;
}


Data::Table Data::CSV::filterTable(
    const Table& originalTable,
    const std::string& columnName,
    std::function<bool(const Cell&)> filterFunction)
{
    Table filteredTable;

    // Copy the headers
    filteredTable.headers = originalTable.headers;

    // Initialize the columns in filteredTable
    for (const auto& header : filteredTable.headers) {
        filteredTable.tableMap[header] = std::vector<Cell>();
    }

    // Check if column exists in the original table
    if (originalTable.tableMap.find(columnName) ==
        originalTable.tableMap.end()) {
        throw std::runtime_error("Column not found: "
                                 + columnName);
    }

    // Loop through rows to apply the filter
    std::size_t numRows =
        originalTable.tableMap.begin()->second.size();

    for (std::size_t i = 0; i < numRows; ++i)
    {
        // Apply filter function
        if (filterFunction(originalTable.tableMap.at(columnName)[i]))
        {
            // Copy this row to filteredTable
            for (const auto& header : filteredTable.headers)
            {
                filteredTable.tableMap[header].
                    push_back(originalTable.tableMap.at(header)[i]);
            }
        }
    }

    return filteredTable;
}
