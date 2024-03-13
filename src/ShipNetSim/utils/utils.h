#ifndef UTILS_H
#define UTILS_H

#include <any>
#include <QString>
#include <QMap>
#include <QDir>

// The Utils namespace encapsulates utility functions and structures
namespace Utils
{

/**
 * Retrieve a value from a QMap with a specified key.
 *
 * @param parameters   The QMap containing the parameters.
 * @param key          The key to search for.
 * @param defaultValue The default value to return if the key is not found.
 * @returns Value associated with the key if found,
 *          otherwise returns the default value.
 */
template<typename T>
inline T getValueFromMap(const QMap<QString, std::any>& parameters,
                         const QString& key, const T& defaultValue)
{
    return parameters.contains(key) ?
               std::any_cast<T>(parameters[key]) : defaultValue;
}


// ValueGetter structures serve as utilities for key-value pairs operations
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

/**
 * Interpolates the value for a given key from a QMap.
 *
 * @param map  The QMap containing the data for interpolation.
 * @param key  The key for which interpolation is required.
 * @returns Interpolated value for the given key.
 */
template<typename KeyType, typename ValueType>
inline ValueType interpolate(const QMap<KeyType, ValueType> &map,
                             KeyType key)
{
    // Handle edge cases: single entry or zero key
    if (map.size() == 1) return map.first();
    if (ValueGetter<KeyType, ValueType>::getKey(key) == 0.0)
        return ValueGetter<KeyType, ValueType>::fromValue(0.0);

    // Establish interpolation bounds
    auto lower = map.lowerBound(key);
    if (lower == map.end()) {
        return (--lower).value();
    }
    if (lower.key() == key || lower == map.begin()) {
        return lower.value();
    }
    auto upper = lower;
    lower--;

    // Calculate the slope for interpolation
    double slope = (ValueGetter<KeyType,
                                ValueType>::getValue(upper.value() -
                                                     lower.value()) /
                    (ValueGetter<KeyType,
                                 ValueType>::getKey(upper.key()) -
                     ValueGetter<KeyType,
                                 ValueType>::getKey(lower.key())));

    // Return interpolated value
    return ValueGetter<KeyType, ValueType>::fromValue(
        ValueGetter<KeyType, ValueType>::getValue(lower.value()) +
        slope * (ValueGetter<KeyType, ValueType>::getKey(key) -
                 ValueGetter<KeyType, ValueType>::getKey(lower.key()))
        );
}

inline std::vector<double> linspace_step(double start,
                                         double end,
                                         double step = 1.0)
{
    std::vector<double> linspaced;
    int numSteps = static_cast<int>(std::ceil((end - start) / step));

    for(int i = 0; i <= numSteps; ++i){
        double currentValue = start + i * step;
        // To avoid floating point errors, we limit the value to 'end'
        if(currentValue > end) currentValue = end;
        linspaced.push_back(currentValue);
    }

    return linspaced;
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

/**
 * Format a number by adding a thousand separator and
 * preserving a specified number of decimals.
 *
 * @param n         The number to format.
 * @param decimals  The number of decimal places to retain (default is 3).
 * @returns Formatted string with thousand separators.
 */
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

/**
 * Split a string into pairs based on newline characters
 * and a specified delimiter.
 *
 * @param inputString The string to split.
 * @param delimiter   The delimiter to use for
 *                      splitting each line (default is ":").
 * @returns A QVector of QPairs containing the split string values.
 */
inline QVector<QPair<QString, QString>>
splitStringStream(const QString& inputString,
                  const QString& delimiter = ":")
{
    QVector<QPair<QString, QString>> result;

    // Split input string by newline characters
    const auto& lines = inputString.split("\n", Qt::SkipEmptyParts);
    for (const auto& line : lines)
    {
        // Split each line by the given delimiter and append to result
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

/**
 * Retrieve the home directory path and ensures a
 * specific sub-folder exists.
 *
 * @returns The path to the "ShipNetSim" folder inside
 *           the "Documents" directory.
 * @throws std::runtime_error if unable to retrieve the home directory.
 */
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
        // Creating a path to ShipNetSim folder in the Documents directory
        const QString documentsDir = QDir(homeDir).filePath("Documents");
        const QString folder = QDir(documentsDir).filePath("ShipNetSim");

        QDir().mkpath(folder); // Create the directory if it doesn't exist

        return folder;
    }

    throw std::runtime_error("Error: Cannot retrieve home directory!");
}

inline bool stringToBool(const QString& str, bool* ok = nullptr)
{
    QString lowerStr = str.toLower();
    if (lowerStr == "true" || str == "1") {
        if (ok != nullptr) *ok = true;
        return true;
    } else if (lowerStr == "false" || str == "0") {
        if (ok != nullptr) *ok = true;
        return false;
    } else {
        if (ok != nullptr) *ok = false;
        qWarning() << "Invalid boolean string:" << str;
        return false;
    }
}



}
#endif // UTILS_H
