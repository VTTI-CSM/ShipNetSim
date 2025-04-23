/**
 * @file logger.h
 * @brief Logger Class Header
 *
 * This file contains the Logger class which provides functionality
 * for custom logging in a Qt application. The logger writes messages
 * to a specified log file and optionally to the standard output.
 *
 * @author Ahmed Aredah
 * @date 8/17/2023
 */

#ifndef LOGGER_H
#define LOGGER_H

#include "../export.h"
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QObject>
#include <QTextStream>
#include <iostream>
#include <qmutex.h>

namespace ShipNetSimCore
{
/**
 * @class Logger
 * @brief Custom logger for Qt applications
 *
 * The Logger class is designed to provide a custom logging mechanism
 * for Qt-based applications. It can write logs to a file and to the
 * standard output based on specified minimum log levels.
 */
class SHIPNETSIM_EXPORT Logger : public QObject
{
    Q_OBJECT
public:
    /**
     * @brief Constructor
     * @param parent Parent QObject
     */
    explicit Logger(QObject *parent = nullptr);

    /**
     * @brief keep multiple logs (log.1, log.2, etc.).
     */
    static void rotateLogs();

    /**
     * @brief Install the custom message handler to start logging.
     */
    static void attach(QString fileBaseName);

    /**
     * @brief Uninstall the custom message handler and stop
     * logging to the file.
     */
    static void detach();

    /**
     * @brief Custom message handler.
     *
     * Writes messages to the log file and/or standard output
     * depending on the current minimum log levels set.
     *
     * @param type Type of message.
     * @param context Contextual information about the source of the
     * message.
     * @param msg The message content.
     */
    static void handler(QtMsgType                 type,
                        const QMessageLogContext &context,
                        const QString            &msg);

    /**
     * @brief Stop logging operations.
     */
    static void turonOffLogger();

    /**
     * @brief Start/resume logging operations.
     */
    static void turnOnLogger();

    /**
     * @brief Set the minimum log level for file output.
     *
     * Messages with severity lower than this level will
     * not be written to the file.
     *
     * @param newType New minimum log level for file output.
     */
    static void setFileMinLogLevel(QtMsgType newType);

    /**
     * @brief Set the minimum log level for standard output.
     *
     * Messages with severity lower than this level will not
     * be written to the standard output.
     *
     * @param newType New minimum log level for standard output.
     */
    static void setStdOutMinLogLevel(QtMsgType newType);
signals:

private:
    /// Logging status flag.
    static bool logging;

    /// Path of the log file.
    static QString logPath;

    /// Log file object.
    static QFile file;

    /// Status flag indicating if the log file is open.
    static bool fileOpened;

    /// Minimum log level for file output.
    static QtMsgType fileMinLogLevel;

    /// Minimum log level for standard output.
    static QtMsgType stdOutMinLogLevel;

    /// Mutex for thread safety
    static QMutex g_logMutex;
};
}; // namespace ShipNetSimCore
#endif // LOGGER_H
