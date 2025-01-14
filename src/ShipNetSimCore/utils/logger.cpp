// Include necessary headers
#include "logger.h"
#include <QStandardPaths>
#include <qmutex.h>


namespace ShipNetSimCore
{

// Define the static mutex:
QMutex Logger::g_logMutex;


// we will generate the logPath dynamically in attach().
QString Logger::logPath = QString();

// Initialize static member variables

// Determine if the logger is currently active
bool Logger::logging = false;

// Create a QFile object for logging
QFile Logger::file(Logger::logPath);

// Track if the file is open for writing
bool Logger::fileOpened = false;

// Track which level of logging is considered
QtMsgType Logger::fileMinLogLevel = QtDebugMsg;
QtMsgType Logger::stdOutMinLogLevel = QtWarningMsg;

// Store the default Qt message handler
static const QtMessageHandler QT_DEFAULT_MESSAGE_HANDLER =
    qInstallMessageHandler(nullptr);

// Default Logger constructor
Logger::Logger(QObject *parent) : QObject{parent} {}

void Logger::rotateLogs()
{
    const qint64 MAX_FILE_SIZE = 10 * 1024 * 1024; // 10MB per log
    const int MAX_BACKUP_FILES = 5; // Keep up to 5 backups

    QFileInfo fileInfo(Logger::logPath);
    if (fileInfo.exists() && fileInfo.size() > MAX_FILE_SIZE)
    {
        for (int i = MAX_BACKUP_FILES - 1; i > 0; --i)
        {
            QString oldFile = Logger::logPath + "." + QString::number(i);
            QString newFile = Logger::logPath + "." + QString::number(i + 1);
            QFile::remove(newFile); // Remove last backup
            QFile::rename(oldFile, newFile); // Rename older backups
        }
        QFile::rename(Logger::logPath,
                      Logger::logPath + ".1"); // Current log becomes log.1
    }
}

// Enable the logger
void Logger::attach(QString fileBaseName)
{
    // Activate logging
    Logger::logging = true;

    // Use the system's temp directory for the log file.
    // The resulting filename might look like:
    //    /tmp/ShipNetSim.log on Linux/macOS, or
    // C:\Users\<user>\AppData\Local\Temp\ShipNetSim.log
    // on Windows.
    Logger::logPath =
        QStandardPaths::writableLocation(QStandardPaths::TempLocation) +
        QDir::separator() + fileBaseName + ".log";

    rotateLogs(); // Rotate logs before opening the file

    // Assign this new path to our QFile object
    file.setFileName(Logger::logPath);

    // Ensure the directory exists (particularly important on Windows)
    QFileInfo fileInfo(Logger::logPath);
    QDir dir = fileInfo.dir();
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    // Install our custom message handler
    qInstallMessageHandler(Logger::handler);

    // Attempt to open the file in append mode
    fileOpened = file.open(QIODevice::Append | QIODevice::Text);
    if (!fileOpened) {
        qWarning() << "Logger could not open file at" << Logger::logPath;
    }
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
    // Lock the mutex to ensure we don't interleave writes
    QMutexLocker locker(&g_logMutex);

    // Only proceed if logging is active
    if (Logger::logging && type >= Logger::fileMinLogLevel)
    {
        QString logType;

        // Determine the type of log message
        switch (type) {
        case QtInfoMsg:    logType = "Info";    break;
        case QtDebugMsg:   logType = "Debug";   break;
        case QtWarningMsg: logType = "Warning"; break;
        case QtCriticalMsg:logType = "Critical";break;
        case QtFatalMsg:   logType = "Fatal";   break;
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

    // Forward messages >= stdOutMinLogLevel to the default handler
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
}
