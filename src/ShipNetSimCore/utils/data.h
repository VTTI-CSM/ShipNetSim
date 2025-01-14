#ifndef DATA_H
#define DATA_H

#include "../export.h"
#include <QMap>
#include <QVariant>
#include <QString>
#include <QVector>
#include <QList>
#include <QPair>
#include <functional>


#include <QString>
#include <QFile>
#include <QTextStream>

namespace ShipNetSimCore
{
using Cell = QVariant;

namespace Data
{

class CSV;  // Forward declaration
class SHIPNETSIM_EXPORT Table
{
    friend class CSV;
    friend class TXT;
protected:
    QVector<QString> headers;
    // this is effecient since i will need to loop over the whole ds anyways
    // and we do not know the size of the table
    QMap<QString, QVector<Cell>> tableMap;

public:
    QVector<QString> getHeaders() const { return headers; }

    bool hasColumn(int columnIndex) const
    {
        return columnIndex >= 0 && columnIndex < headers.size();
    }

    bool hasColumn(QString columnName) const
    {
        return headers.contains(columnName);
    }

    // Templated function to access a column by its header name
    template <typename T>
    inline QVector<T> getColumn(const QString& headerName) const
    {
        auto it = tableMap.find(headerName);
        if (it != tableMap.end()) {
            QVector<T> column;
            for (const auto& cell : it.value()) {
                if (cell.canConvert<T>()) {
                    column.push_back(cell.value<T>());
                } else {
                    throw std::runtime_error("Type mismatch in column: " +
                                             headerName.toStdString());
                }
            }
            return column;
        }
        throw std::runtime_error("Header not found: " +
                                 headerName.toStdString());
    }

    template <typename T>
    inline QVector<T> getColumn(const int& columnIndex) const
    {
        QString headerName = headers[columnIndex];
        return getColumn<T>(headerName);
    }

    template<typename T>
    inline T getCellData(const QString& headerName, qsizetype index) const
    {
        const QVariant& cell = tableMap[headerName][index];
        if (!cell.canConvert<T>()) {
            // Or more specific:
            throw std::runtime_error(
                QString("Cannot convert cell data at header '%1' index %2 "
                        "from type %3 to requested type")
                    .arg(headerName)
                    .arg(index)
                    .arg(cell.typeName())
                    .toStdString()
                );
        }
        return cell.value<T>();
    }

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
    inline Table filterTable(
        const QString& columnName,
        std::function<bool(const Cell&)> filterFunction) const
    {
        // Validate column existence
        auto it = tableMap.find(columnName);
        if (it == tableMap.end()) {
            throw std::runtime_error("Column not found: " +
                                     columnName.toStdString());
        }

        Table filteredTable;

        // Copy headers
        filteredTable.headers = headers;

        // Initialize columns in the filtered table map
        for (const auto& header : headers) {
            filteredTable.tableMap[header] = QVector<Cell>();
        }

        // Get the column to filter
        const QVector<Cell>& column = it.value();

        // Loop through each row
        for (qsizetype rowIdx = 0; rowIdx < column.size(); ++rowIdx) {
            const Cell& cell = column[rowIdx];

            // Apply the filter function
            if (filterFunction(cell)) {
                // If the row matches the filter, add all column values
                // of this row to the filtered table
                for (const auto& header : headers) {
                    filteredTable.tableMap[header].append(
                        tableMap[header][rowIdx]);
                }
            }
        }

        return filteredTable;
    }

    inline Table filterTable(
        const int& columnIndex,
        std::function<bool(const Cell&)> filterFunction) const
    {
        if (columnIndex > headers.size() - 1) {
            throw std::runtime_error("column index is out of bound!");
        }
        QString headerName = headers[columnIndex];
        return filterTable(headerName, filterFunction);
    }


    /**
     * @brief Get distinct values from a specified column with type checking
     * @tparam T The type of values to retrieve
     * @param columnName Name of the column to get distinct values from
     * @return QSet of distinct values of type T from the specified column
     * @throws std::runtime_error if column name is not found or if type mismatch occurs
     * @code
     *    QSet<int> uniqueAges = table.getDistinctValues<int>("age");
     *    QSet<QString> uniqueNames = table.getDistinctValues<QString>("name");
     */
    template <typename T>
    inline QSet<T> getDistinctValues(const QString& columnName) const
    {
        auto it = tableMap.find(columnName);
        if (it == tableMap.end()) {
            throw std::runtime_error("Column not found: "
                                     + columnName.toStdString());
        }

        const QVector<Cell>& column = it.value();

        // Use QSet constructor to efficiently create the set in one step
        QSet<T> distinctValues(column.begin(), column.end());

        return distinctValues;
    }

    template <typename T>
    inline QSet<T> getDistinctValues(const int& columnIndex) const
    {
        if (columnIndex > headers.size() - 1) {
            throw std::runtime_error("column index is out of bound!");
        }
        QString headerName = headers[columnIndex];
        return getDistinctValues<T>(headerName);
    }

    template <typename T>
    inline static Table createFromColumns(const QVector<QString>& headers,
                                          const QVector<QVector<T>>& data)
    {
        // Validate input
        if (headers.isEmpty()) {
            throw std::runtime_error("Headers cannot be empty");
        }

        if (!data.isEmpty() && data[0].size() != headers.size()) {
            throw std::runtime_error("Number of headers must match number "
                                     "of columns in data");
        }

        Table table;
        table.headers = headers;

        // Initialize columns in the table map
        for (const auto& header : headers) {
            table.tableMap[header] = QVector<Cell>();
        }

        // If there's no data, return empty table with headers
        if (data.isEmpty()) {
            return table;
        }

        // Get number of rows from first vector
        const size_t numRows = data[0].size();

        // Validate that all data vectors have the same size
        for (const auto& column : data) {
            if (column.size() != numRows) {
                throw std::runtime_error("All columns must have "
                                         "the same number of rows");
            }
        }

        // Fill the table with data (rows)
        for (int colIdx = 0; colIdx < headers.size(); ++colIdx) {
            const QString& header = headers[colIdx];
            auto& tableColumn = table.tableMap[header];
            tableColumn.reserve(numRows);

            for (int rowIdx = 0; rowIdx < numRows; ++rowIdx) {
                tableColumn.push_back(QVariant::fromValue(data[colIdx][rowIdx]));
            }
        }

        return table;

    }

    template <typename T>
    inline static Table createFromRows(const QVector<QString>& headers,
                                       const QVector<QVector<T>>& data)
    {
        // Validate input
        if (headers.isEmpty()) {
            throw std::runtime_error("Headers cannot be empty");
        }

        // Validate data consistency
        for (const auto& row : data) {
            if (row.size() != headers.size()) {
                throw std::runtime_error("Each row must have the same number of "
                                         "elements as the number of headers");
            }
        }

        Table table;
        table.headers = headers;

        // Initialize columns in the table map
        for (const auto& header : headers) {
            table.tableMap[header] = QVector<Cell>();
        }

        // Fill the table with data
        for (int rowIdx = 0; rowIdx < data.size(); ++rowIdx) {
            const auto& row = data[rowIdx];
            for (int colIdx = 0; colIdx < headers.size(); ++colIdx) {
                const QString& header = headers[colIdx];
                table.tableMap[header].push_back(
                    QVariant::fromValue(row[colIdx]));
            }
        }

        return table;
    }


    template <typename firstColumnType, typename secondColumnType>
    inline static Data::Table createFromQPairRows(
        const QPair<QString, QString>& headers,
        const QVector<QPair<firstColumnType, secondColumnType>>& data)
    {
        // Validate headers
        if (headers.first.isEmpty() || headers.second.isEmpty()) {
            throw std::runtime_error("Headers cannot be empty");
        }

        Table table;
        table.headers = {headers.first, headers.second};

        // Initialize columns in the table map
        table.tableMap[headers.first] = QVector<Cell>();
        table.tableMap[headers.second] = QVector<Cell>();

        // If there's no data, return an empty table with headers
        if (data.isEmpty()) {
            return table;
        }

        // Populate the table with data
        for (const auto& pair : data) {
            // Add `pair.first` to the column corresponding to `headers.first`
            if constexpr (std::is_convertible<firstColumnType,
                                              QVariant>::value) {
                table.tableMap[headers.first].append(
                    QVariant::fromValue(pair.first));
            } else {
                throw std::runtime_error("firstColumnType must "
                                         "be convertible to QVariant");
            }

            // Add `pair.second` to the column corresponding to `headers.second`
            if constexpr (std::is_convertible<secondColumnType, QVariant>::value) {
                table.tableMap[headers.second].append(
                    QVariant::fromValue(pair.second));
            } else {
                throw std::runtime_error("secondColumnType must be "
                                         "convertible to QVariant");
            }
        }

        return table;
    }


    class iterator
    {
    public:
        using inner_iterator = QMap<QString, QVector<Cell>>::iterator;

        iterator(inner_iterator it) : it_(it) {}

        iterator& operator++() {
            ++it_;
            return *this;
        }

        QList<Cell>& operator*()
        {
            return *it_;
        }

        bool operator==(const iterator& other) const
        {
            return it_ == other.it_;
        }

        bool operator!=(const iterator& other) const
        {
            return it_ != other.it_;
        }

    private:
        inner_iterator it_;
    };

    class const_iterator
    {
    public:
        using inner_const_iterator = QMap<QString, QVector<Cell>>::const_iterator;

        const_iterator(inner_const_iterator it) : it_(it) {}

        const_iterator& operator++()
        {
            ++it_;
            return *this;
        }

        const QList<Cell>& operator*() const
        {
            return *it_;
        }

        bool operator==(const const_iterator& other) const
        {
            return it_ == other.it_;
        }

        bool operator!=(const const_iterator& other) const
        {
            return it_ != other.it_;
        }

    private:
        inner_const_iterator it_;
    };

    iterator begin()
    {
        return iterator(tableMap.begin());
    }

    iterator end() {
        return iterator(tableMap.end());
    }

    const_iterator begin() const
    {
        return const_iterator(tableMap.cbegin());
    }

    const_iterator end() const
    {
        return const_iterator(tableMap.cend());
    }

    const_iterator cbegin() const
    {
        return const_iterator(tableMap.cbegin());
    }

    const_iterator cend() const
    {
        return const_iterator(tableMap.cend());
    }
};

class SHIPNETSIM_EXPORT CSV {
private:
    QString mFilePath;
    QFile mFile;
    QTextStream mOutStream; // For writing

public:
    CSV();
    CSV(const QString &filePath);
    ~CSV(); // Destructor
    void initCSV(const QString &filePath);
    Table read(const bool hasHeaders = false,
               const QString& separator = ",",
               std::function<bool(const QString&)> filterFunc = nullptr,
               int filterColumnIndex = -1);
    QVector<QString> getDistinctValuesFromCSV(const bool hasHeaders,
                                              int columnIndex,
                                              const QString& separator);

    bool writeLine(const QString &line);
    bool writeLine(const QVector<QString> &lineDetails,
                   const QString separator = ",");
    bool clearFile();
    void close();
};

class SHIPNETSIM_EXPORT TXT
{
private:
    QString mFilePath;
    QFile mFile;
    QTextStream mOutStream; // For writing

public:
    TXT();
    TXT(const QString &filePath);
    ~TXT(); // Destructor to close the file
    void initTXT(const QString &filePath);
    Table read(const QVector<QString>& typeSequence,
                      const QString& separator = ",");
    bool writeFile(QString &data);
    bool clearFile();
    void close();
};

namespace ProjectFile {
    struct SHIPNETSIM_EXPORT ProjectDataFile {
        QString projectName;
        QString networkName;
        QString authorName;
        QString shipsFileName;
        QString simEndTime;
        QString simTimestep;
        QString simPlotTime;
    };

    SHIPNETSIM_EXPORT void createProjectFile(const ProjectDataFile pf, QString& filename);

    SHIPNETSIM_EXPORT ProjectDataFile readProjectFile(const QString &filename);
};
}
};
#endif // DATA_H
