#ifndef UTILS_H
#define UTILS_H

#include <any>
#include <QString>
#include <QMap>
#include <QDir>

namespace Utils
{
    template<typename T>
    inline T getValueFromMap(const QMap<QString, std::any>& parameters,
                             const QString& key, const T& defaultValue)
    {
        return parameters.contains(key) ?
                   std::any_cast<T>(parameters[key]) : defaultValue;
    }

    template<typename KeyType, typename ValueType>
    struct ValueGetter {
        static double getValue(const ValueType& val) {
            return val.value();
        }
        static ValueType fromValue(double val) {
            return ValueType(val);
        }
        static double getKey(const KeyType& key) {
            return key.value();
        }
    };

    // Specialization for double as ValueType
    template<typename KeyType>
    struct ValueGetter<KeyType, double> {
        static double getValue(const double& val) {
            return val;
        }
        static double fromValue(double val) {
            return val;
        }
        static double getKey(const KeyType& key) {
            return key.value();
        }
    };

    // Specialization for double as KeyType
    template<typename ValueType>
    struct ValueGetter<double, ValueType> {
        static double getValue(const ValueType& val) {
            return val.value();
        }
        static ValueType fromValue(double val) {
            return ValueType(val);
        }
        static double getKey(const double& key) {
            return key;
        }
    };

    // Specialization for both KeyType and ValueType as double
    template<>
    struct ValueGetter<double, double> {
        static double getValue(const double& val) {
            return val;
        }
        static double fromValue(double val) {
            return val;
        }
        static double getKey(const double& key) {
            return key;
        }
    };

    template<typename KeyType, typename ValueType>
    inline ValueType interpolate(const QMap<KeyType, ValueType> &map,
                                 KeyType key)
    {
        if (map.size() == 1) return map.first();
        if (ValueGetter<KeyType, ValueType>::getKey(key) == 0.0)
            return ValueGetter<KeyType, ValueType>::fromValue(0.0);

        auto lower = map.lowerBound(key);
        if (lower == map.end()) {
            return (--lower).value();
        }
        if (lower.key() == key || lower == map.begin()) {
            return lower.value();
        }
        auto upper = lower;
        lower--;

        double slope = (ValueGetter<KeyType,
                                    ValueType>::getValue(upper.value() -
                                                         lower.value()) /
                        (ValueGetter<KeyType,
                                     ValueType>::getKey(upper.key()) -
                         ValueGetter<KeyType,
                                     ValueType>::getKey(lower.key())));

        return ValueGetter<KeyType, ValueType>::fromValue(
            ValueGetter<KeyType, ValueType>::getValue(lower.value()) +
            slope * (ValueGetter<KeyType, ValueType>::getKey(key) -
                     ValueGetter<KeyType, ValueType>::getKey(lower.key()))
            );
    }


    /**
     * Format duration
     *
     * @author	Ahmed Aredah
     * @date	2/28/2023
     *
     * @param 	seconds	The seconds.
     *
     * @returns	The formatted duration.
     *
     * @tparam	T	Generic type parameter.
     */
    template<typename T>
    inline QString formatDuration(T seconds) {
        int minutes = static_cast<int>(seconds) / 60;
        int hours = minutes / 60;
        int days = hours / 24;
        int remainingSeconds = static_cast<int>(seconds) % 60;
        int remainingMinutes = minutes % 60;
        int remainingHours = hours % 24;

        QString result;
        QTextStream stream(&result);
        stream << days << ":";
        stream << QString("%1").arg(remainingHours, 2, 10, QChar('0')) << ":";
        stream << QString("%1").arg(remainingMinutes, 2, 10, QChar('0')) << ":";
        stream << QString("%1").arg(remainingSeconds, 2, 10, QChar('0'));

        return result;
    }

    template <typename T>
    inline QString thousandSeparator(T n, int decimals = 3) {
        // Get the sign of the number and remove it
        int sign = (n < 0) ? -1 : 1;
        double approx = std::pow(static_cast<double>(10.0), decimals);
        n *= sign;

        // Get the integer part of the number
        qint64 intPart = static_cast<qint64>(n);

        // Check if the fractional part has any value
        bool hasFracPart = (n - intPart > 0);

        // Get the fractional part of the number and trim
        // it to n decimal places
        double fracPart = std::round((n - intPart) * approx) / approx;

        // Convert the integer part to a string and apply
        // thousand separator
        QString intStr = QString::number(intPart);
        int insertPos = intStr.length() - 3;
        while (insertPos > 0) {
            intStr.insert(insertPos, ',');
            insertPos -= 3;
        }

        // Convert the fractional part to a string and trim it
        // to n decimal places
        QString fracStr;
        if (hasFracPart) {
            fracStr = QString::asprintf("%.*f", decimals, fracPart);
            // Remove the leading "0" in the fractional string
            fracStr.remove(0, 1);
        }

        // Combine the integer and fractional parts into a single string
        QString result = intStr + fracStr;

        // Add the sign to the beginning of the string
        // if the number was negative
        if (sign == -1) {
            result.prepend("-");
        }

        return result;
    }

    inline QVector<QPair<QString, QString>>
    splitStringStream(const QString& inputString,
                      const QString& delimiter = ":")
    {
        QVector<QPair<QString, QString>> result;

        const auto& lines = inputString.split("\n", Qt::SkipEmptyParts);
        for (const auto& line : lines)
        {
            int delimiterPos = line.indexOf(delimiter);
            if (delimiterPos != -1)
            {
                QString first = line.left(delimiterPos);
                QString second = line.mid(delimiterPos + delimiter.size());
                result.append(qMakePair(first, second));
            }
            else
            {
                result.append(qMakePair(line, ""));
            }
        }

        return result;
    }

    inline QString getHomeDirectory()
    {
        QString homeDir;

        // OS-dependent home directory retrieval
#if defined(Q_OS_WIN)
        homeDir = QDir::homePath();
#elif defined(Q_OS_LINUX) || defined(Q_OS_MAC)
        homeDir = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
#endif

        if (!homeDir.isEmpty())
        {
            // Creating a path to NeTrainSim folder in the Documents directory
            const QString documentsDir = QDir(homeDir).filePath("Documents");
            const QString folder = QDir(documentsDir).filePath("ShipNetSim");

            QDir().mkpath(folder); // Create the directory if it doesn't exist

            return folder;
        }

        throw std::runtime_error("Error: Cannot retrieve home directory!");
    }




}
#endif // UTILS_H
