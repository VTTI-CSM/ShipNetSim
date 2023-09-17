#ifndef DATA_H
#define DATA_H

#include <fstream>
#include <vector>
#include <map>
#include <variant>
#include <string>
#include <stdexcept>
#include <QString>
#include <QFile>
#include <QTextStream>

using Cell = std::variant<int, double, std::string>;

namespace Data
{

class Table
{
public:
    std::vector<std::string> headers;
    // this is effecient since i will need to loop over the whole ds anyways
    // and we do not know the size of the table
    std::map<std::string, std::vector<Cell>> tableMap;

    // Templated function to access a column by its header name
    template <typename T>
    std::vector<T> getColumn(const std::string& headerName) const;

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
    inline Table filterTable(const std::string& columnName,
                             std::function<bool(const Cell&)> filterFunction);

    // Nested iterator class
    class iterator
    {
    public:
        using inner_iterator = std::map<std::string,
                                        std::vector<Cell>>::iterator;

        iterator(inner_iterator it) : it_(it) {}

        iterator& operator++() {
            ++it_;
            return *this;
        }

        std::pair<const std::string, std::vector<Cell>>& operator*()
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

    // Nested const_iterator class
    class const_iterator {
    public:
        using inner_const_iterator =
            std::map<std::string,
                     std::vector<Cell>>::const_iterator;

        const_iterator(inner_const_iterator it) : it_(it) {}

        const_iterator& operator++()
        {
            ++it_;
            return *this;
        }

        const std::pair<const std::string,
                        std::vector<Cell>>& operator*() const
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

class CSV {
private:
    QString mFilePath;
    QFile mFile;
    QTextStream mOutStream; // For writing

public:
    CSV();
    CSV(const QString &filePath);
    ~CSV(); // Destructor
    void initCSV(const QString &filePath);
    Table read(const std::vector<std::string>& typeSequence,
               const bool hasHeaders = false,
               const std::string& separator = ",");
    bool writeLine(const std::string &line);
};

class TXT
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
    inline Table read(const std::vector<std::string>& typeSequence,
                      const std::string& separator = ",");
    inline bool writeFile(std::string &data);
};

}

#endif // DATA_H
