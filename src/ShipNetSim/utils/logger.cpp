// Include necessary headers
#include "logger.h"
#include <QStandardPaths>

// Set the project name based on if it's defined.
// If PROJECT_NAME is defined, set it as the project name.
// Otherwise, default to "Unknown Project".
#ifdef PROJECT_NAME
const char* projectName = PROJECT_NAME;
#else
const char* projectName = "Unknown Project";
#endif

// Determine the log path based on the platform.
// For Windows and Linux, store the logs in the AppLocalDataLocation.
// For macOS, store it in the AppDataLocation.
#if _WIN32 || __linux__
QString Logger::logPath =
    QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) +
    QDir::separator() + projectName + QDir::separator() + "log.txt";
#elif defined(__APPLE__) && defined(__MACH__)
QString Logger::logPath =
    QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) +
    QDir::separator() + projectName + QDir::separator() + "log.txt";
#endif

// Initialize static member variables

// Determine if the logger is currently active
bool Logger::logging = false;
// Create a QFile object for logging
QFile Logger::file(Logger::logPath);
// Track if the file is open for writing
bool Logger::fileOpened = false;
// Track which level of logging is considered
QtMsgType Logger::fileMinLogLevel = QtDebugMsg;
QtMsgType Logger::stdOutMinLogLevel = QtDebugMsg;

// Store the default Qt message handler
static const QtMessageHandler QT_DEFAULT_MESSAGE_HANDLER =
    qInstallMessageHandler(nullptr);

// Default Logger constructor
Logger::Logger(QObject *parent) : QObject{parent} {}

// Enable the logger
void Logger::attach()
{
    // Activate logging
    Logger::logging = true;
    // Install our custom message handler
    qInstallMessageHandler(Logger::handler);
    // Open the log file in append mode
    fileOpened = file.open(QIODevice::Append);
}

// Disable the logger and close the file
void Logger::detach()
{
    file.close();                    // Close the log file
    fileOpened = false;              // Indicate the file is no longer open
}

// Custom message handler function
void Logger::handler(QtMsgType type,
                     const QMessageLogContext &context,
                     const QString &msg)
{
    // Only proceed if logging is active
    if (Logger::logging && type >= Logger::fileMinLogLevel)
    {
        QString logType;

        // Determine the type of log message
        switch (type) {
        case QtInfoMsg: logType = "Info"; break;
        case QtDebugMsg: logType = "Debug"; break;
        case QtWarningMsg: logType = "Warning"; break;
        case QtCriticalMsg: logType = "Critical"; break;
        case QtFatalMsg: logType = "Fatal"; break;
        }

        // Construct the log entry string
        QString txt = QString("%1 - %2: %3 %4 line: %5\r\n")
                          .arg(
                              QDateTime::currentDateTime().toString(),
                              logType,
                              msg,
                              context.file,
                              QString::number(context.line)
                              );
        // If the log file is open, write the entry
        if (fileOpened) {
            QTextStream ts(&file);
            ts << txt;
            ts.flush();
        }
    }

    //
    if (type >= stdOutMinLogLevel)
    {
        // Call the default Qt message handler
        // (ensuring any default behavior still occurs)
        (*QT_DEFAULT_MESSAGE_HANDLER)(type, context, msg);
    }
}

// Turn off the Logger and only follow the default logging behavior
// (as per Qt Framework)
void Logger::turonOffLogger()
{
    Logger::logging = false;
}

// Turn on the Logger and also follow the default logging behavior
void Logger::turnOnLogger()
{
    Logger::logging = true;
}

// Sets the Min level of logging for the file
void Logger::setFileMinLogLevel(QtMsgType newType)
{
    Logger::fileMinLogLevel = newType;
}

// Sets the Min level of logging for the std output (as per Qt Framework)
void Logger::setStdOutMinLogLevel(QtMsgType newType)
{
    Logger::stdOutMinLogLevel = newType;
}
