#ifndef DATA_H
#define DATA_H

#include <vector>
#include <map>
#include <variant>
#include <string>
#include <stdexcept>
#include <QString>

using Cell = std::variant<int, double, std::string>;

namespace Data
{

class Table
{
public:
    std::vector<std::string> headers;
    // this is effecient since i will need to loop over the whole ds anyways
    // and i do not know the size of the table
    std::map<std::string, std::vector<Cell>> tableMap;

    // Templated function to access a column by its header name
    template <typename T>
    std::vector<T> getColumn(const std::string& headerName) const;
};

namespace CSV
{

/**
 * @brief read
 * @param filePath
 * @param typeSequence
 * @param separator
 * @return
 */
inline Table read(const QString& filePath,
                  const std::vector<std::string>& typeSequence,
                  const std::string& separator = ",");

/**
 * @brief CsvReader::filterTable
 * @param originalTable
 * @param columnName
 * @param filterFunction
 * @return
 * @code
 *    CsvTable filteredTable = reader.filterTable(table, "age",
 *                                          [](const Cell& cell) -> bool {
 *      if (cell.type() == typeid(int)) {
 *          return boost::get<int>(cell) > 30;
 *      }
 *      return false;
 *  });
 */
inline Table filterTable(const Table& originalTable,
                         const std::string& columnName,
                         std::function<bool(const Cell&)> filterFunction);
};


}

#endif // DATA_H
