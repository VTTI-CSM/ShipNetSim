#ifndef UTILS_H
#define UTILS_H

#include <any>
#include <QString>
#include <QMap>
#include <QDir>
#include "../export.h"

namespace ShipNetSimCore
{
// The Utils namespace encapsulates utility functions and structures
namespace Utils
{

SHIPNETSIM_EXPORT QString getFirstExistingPathFromList(
    QVector<QString> filePaths,
    QVector<QString> extensions = QVector<QString>());

/**
 * Format a QString by prepending a prefix and appending a filler string
 * until a specified length is reached.
 *
 * @param preString The string to prepend.
 * @param mainString The main content string.
 * @param postString The string to append after the main string.
 * @param filler The string to append repeatedly to fill up to the
 * desired length.
 * @param length The target total length of the final string.
 * @return A formatted QString of the specified length.
 */
SHIPNETSIM_EXPORT QString formatString(const QString preString,
                                       const QString mainString,
                                       const QString postString,
                                       const QString filler,
                                       int length);

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

// Template function for linear interpolation
template<typename T>
T linearInterpolate(T x0, T y0, T x1, T y1, T x)
{
    if (x1 == x0) {
        throw std::invalid_argument("x0 and x1 cannot be the same, "
                                    "division by zero is not allowed!");
    }

    // Compute the slope and perform interpolation
    return y0 + (y1 - y0) * (x - x0) / (x1 - x0);
};

// Function to perform interpolation on given x and y
// vectors at a given x point
template<typename T>
inline T linearInterpolateAtX(const QVector<T>& x_vals,
                              const QVector<T>& y_vals,
                              T x)
{
    // check both vectors have the same size
    if (x_vals.size() != y_vals.size()) {
        throw std::invalid_argument("x_vals and y_vals must "
                                    "be of the same size!");
    }

    // check x_vals is not empty
    if (x_vals.empty()) {
        throw std::invalid_argument("x_vals and y_vals cannot be empty!");
    }

    // check the x_vals is sorted
    if (!std::is_sorted(x_vals.begin(), x_vals.end())) {
        throw std::invalid_argument("x_vals must be sorted in "
                                    "non-decreasing order!");
    }

    // Check if x is before the first element, interpolate with assumed (0,0)
    if (x < x_vals.front()) {
        return linearInterpolate(T(0), T(0),
                                 x_vals.front(), y_vals.front(),
                                 x);
    }

    // Ensure x is within the bounds
    if (x > x_vals.back()) {
        throw std::out_of_range("x is out of the range of x_vals!");
    }

    // Find the correct interval
    for (std::size_t i = 0; i < x_vals.size() - 1; ++i) {
        if (x >= x_vals[i] && x <= x_vals[i + 1]) {
            return linearInterpolate(x_vals[i],
                                     y_vals[i],
                                     x_vals[i + 1],
                                     y_vals[i + 1], x);
        }
    }

    throw std::logic_error("Interpolation interval not found, "
                           "which should be impossible!");
};

std::vector<double> linspace_step(double start,
                                         double end,
                                         double step = 1.0);

/**
 * Format duration from seconds into a customized string format.
 *
 * @author	Ahmed Aredah
 * @date	2/28/2023
 *
 * @param 	seconds	The duration in seconds to format.
 * @param	format	The format string specifying the output format,
 *                  using placeholders:
 *                  %dd for days,
 *                  %hh for hours,
 *                  %mm for minutes,
 *                  %ss for seconds.
 *
 * @returns	The duration formatted according to the specified format.
 *
 * @tparam	T	Generic type parameter, should be capable of conversion to int.
 */
template<typename T>
inline QString formatDuration(T seconds,
                       const QString format = "%dd days %hh:%mm:%ss")
{
    int minutes = static_cast<int>(seconds) / 60;
    int hours = minutes / 60;
    int days = hours / 24;
    int remainingSeconds = static_cast<int>(seconds) % 60;
    int remainingMinutes = minutes % 60;
    int remainingHours = hours % 24;

    QString result;
    QTextStream stream(&result);

    // Using placeholders in format: %dd for days, %hh for hours, %mm for minutes, %ss for seconds
    QString tempFormat = format;

    // Replace day, hour, minute, and second placeholders
    tempFormat.replace("%dd",
                       QString::number(days));
    tempFormat.replace("%hh",
                       QString("%1").arg(remainingHours, 2, 10, QChar('0')));
    tempFormat.replace("%mm",
                       QString("%1").arg(remainingMinutes, 2, 10, QChar('0')));
    tempFormat.replace("%ss",
                       QString("%1").arg(remainingSeconds, 2, 10, QChar('0')));

    // Stream the formatted string
    stream << tempFormat;

    return result;
};

/**
 * Format a number by adding a thousand separator and
 * preserving a specified number of decimals.
 *
 * @param n         The number to format.
 * @param decimals  The number of decimal places to retain (default is 3).
 * @returns Formatted string with thousand separators.
 */
template <typename T>
inline QString thousandSeparator(T n, int decimals = 3)
{
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
};

/**
 * Split a string into pairs based on newline characters
 * and a specified delimiter.
 *
 * @param inputString The string to split.
 * @param delimiter   The delimiter to use for
 *                      splitting each line (default is ":").
 * @returns A QVector of QPairs containing the split string values.
 */
SHIPNETSIM_EXPORT QVector<QPair<QString, QString>>
splitStringStream(const QString& inputString,
                  const QString& delimiter = ":");

/**
 * Retrieve the home directory path and ensures a
 * specific sub-folder exists.
 *
 * @returns The path to the "ShipNetSim" folder inside
 *           the "Documents" directory.
 * @throws std::runtime_error if unable to retrieve the home directory.
 */
SHIPNETSIM_EXPORT QString getHomeDirectory();

/**
 * Convert boolean string to a boolean value.
 * @param str   The string that contains the bool value
 * @param ok    A variable that holds the conversion result.
 * 
 * @return The boolean value of the string.
 */
SHIPNETSIM_EXPORT bool stringToBool(const QString& str, bool* ok = nullptr);

}
};
#endif // UTILS_H