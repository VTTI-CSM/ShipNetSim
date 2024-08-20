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


#include <fstream>
#include <vector>
#include <map>
#include <variant>
#include <string>
#include <stdexcept>
#include <QString>
#include <QFile>
#include <QTextStream>

namespace ShipNetSimCore
{
using Cell = QVariant;

namespace Data
{

class SHIPNETSIM_EXPORT Table
{
public:
    QVector<QString> headers;
    // this is effecient since i will need to loop over the whole ds anyways
    // and we do not know the size of the table
    QMap<QString, QVector<Cell>> tableMap;

    // Templated function to access a column by its header name
    template <typename T>
    QVector<T> getColumn(const QString& headerName) const;

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
    inline Table filterTable(const QString& columnName,
                             std::function<bool(const Cell&)> filterFunction);

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
    Table read(const QVector<QString>& typeSequence,
               const bool hasHeaders = false,
               const QString& separator = ",");
    bool writeLine(const QString &line);
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
