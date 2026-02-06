#include "utils.h"
#include "../../third_party/units/units.h"
#include "../ship/ship.h"
#include <QMutex>
#include <QMutexLocker>
#include <QStandardPaths>
#include <QTextStream>
#include <QThread>

#ifdef Q_OS_WIN
#include <windows.h>
#endif
namespace ShipNetSimCore
{
namespace Utils
{

QString getExecutableDirectory()
{
    // Return the absolute path
    return QCoreApplication::applicationDirPath();
}

QString getRootDirectory()
{
    QDir execDir(QCoreApplication::applicationDirPath());

#ifdef NDEBUG // Release build
    execDir.cdUp();
#endif

    // Return the absolute path
    return execDir.absolutePath();
}

QString getDataDirectory()
{   
    // First, check for 'data' directory in the same directory as executable
    QString execDir = getExecutableDirectory();
    QDir execDataDir(QDir(execDir).filePath("data"));
    if (execDataDir.exists())
    {
        return execDataDir.absolutePath();
    }

    // Then check the parent/data location
    QString parentDir = getRootDirectory();
    
    // Navigate to the 'data' directory within the parent
    QDir dataDir(QDir(parentDir).filePath("data"));

    // Check if the 'data' directory exists in the runtime location
    if (dataDir.exists())
    {
        return dataDir.absolutePath();
    }

// Fall back to the 'src/data' directory in the source directory
#ifdef SOURCE_DIRECTORY
    QDir sourceDataDir(QDir(SOURCE_DIRECTORY).filePath("src/data"));
    if (sourceDataDir.exists())
    {
        qDebug() << "Using data directory from source: "
                 << sourceDataDir.absolutePath();
        return sourceDataDir.absolutePath();
    }
#endif

    qDebug() << "Data directory not found.";
    return QString(); // Return an empty string or handle as
                      // appropriate
}

QString getDataFile(const QString &fileName)
{
    // Get the data directory path
    QString dataDir = getDataDirectory();

    // Check if the data directory was resolved
    if (!dataDir.isEmpty())
    {
        // Construct the full path for the file
        QString filePath = QDir(dataDir).filePath(fileName);

        // Check if the file exists
        if (QFile::exists(filePath))
        {
            return filePath;
        }
    }

    // Fatal error if the file does not exist
    QString errorMsg =
        QString("Data file '%1' not found in the resolved "
                "data directory: %2")
            .arg(fileName)
            .arg(dataDir);
    throw std::runtime_error(errorMsg.toStdString());
    return QString(); // This will never be reached because of throw
                      // error
}

// Definition of the function
QString getFirstExistingPathFromList(QVector<QString> filePaths,
                                     QVector<QString> extensions)
{
    for (const QString &loc : filePaths)
    {
        QFileInfo fileInfo(loc);

        // If the path is relative, resolve it to an absolute path
        QString fullPath;
        if (fileInfo.isRelative())
        {
            fullPath = QDir::current().absoluteFilePath(loc);
        }
        else
        {
            fullPath = loc;
        }

        // Check if the file exists
        if (QFile::exists(fullPath))
        {
            // Get the extension of the file
            QString ext = fileInfo.suffix().toLower();

            // if it has the needed extension or no required
            // extension, return the path
            if (extensions.empty()
                || extensions.contains(ext, Qt::CaseInsensitive))
            {
                return fullPath;
            }
        }
    }
    return QString("");
};

QString formatString(const QString preString,
                     const QString mainString,
                     const QString postString, const QString filler,
                     int length)
{
    // Start by concatenating preString and mainString
    QString result = preString + mainString;

    // Calculate remaining length to fill
    int remainingLength =
        length - postString.length() - result.length();

    // Append the filler string as many times as needed to reach the
    // desired length excluding postString
    while (remainingLength > 0)
    {
        // Append only the necessary part of the filler
        result.append(filler.left(remainingLength));
        // Update remaining length
        remainingLength =
            length - postString.length() - result.length();
    }

    result += postString; // Append postString after filling

    return result; // Return the formatted string
};

std::vector<double> linspace_step(double start, double end,
                                  double step)
{
    std::vector<double> linspaced;
    int numSteps = static_cast<int>(std::ceil((end - start) / step));

    for (int i = 0; i <= numSteps; ++i)
    {
        double currentValue = start + i * step;
        // To avoid floating point errors, we limit the value to 'end'
        if (currentValue > end)
            currentValue = end;
        linspaced.push_back(currentValue);
    }

    return linspaced;
};

QVector<QPair<QString, QString>>
splitStringStream(const QString &inputString,
                  const QString &delimiter)
{
    QVector<QPair<QString, QString>> result;

    // Split input string by newline characters
    const auto &lines = inputString.split("\n", Qt::SkipEmptyParts);
    for (const auto &line : lines)
    {
        // Split each line by the given delimiter and append to result
        int delimiterPos = line.indexOf(delimiter);
        if (delimiterPos != -1)
        {
            QString first = line.left(delimiterPos);
            QString second =
                line.mid(delimiterPos + delimiter.size());
            result.append(qMakePair(first, second));
        }
        else
        {
            result.append(qMakePair(line, ""));
        }
    }

    return result;
};

QString getHomeDirectory()
{
    QString homeDir;

// OS-dependent home directory retrieval
#if defined(Q_OS_WIN)
    homeDir = QDir::homePath();
#elif defined(Q_OS_LINUX) || defined(Q_OS_MAC)
    homeDir = QStandardPaths::writableLocation(
        QStandardPaths::HomeLocation);
#endif

    if (!homeDir.isEmpty())
    {
        // Creating a path to ShipNetSim folder in the Documents
        // directory
        const QString documentsDir =
            QDir(homeDir).filePath("Documents");
        const QString folder =
            QDir(documentsDir).filePath("ShipNetSim");

        QDir().mkpath(
            folder); // Create the directory if it doesn't exist

        return folder;
    }

    throw std::runtime_error(
        "Error: Cannot retrieve home directory!");
};

bool stringToBool(const QString &str, bool *ok)
{
    QString lowerStr = str.toLower();
    if (lowerStr == "true" || str == "1")
    {
        if (ok != nullptr)
            *ok = true;
        return true;
    }
    else if (lowerStr == "false" || str == "0")
    {
        if (ok != nullptr)
            *ok = true;
        return false;
    }
    else
    {
        if (ok != nullptr)
            *ok = false;
        qWarning() << "Invalid boolean string:" << str;
        return false;
    }
};

double accumulateShipValuesDouble(
    const QVector<std::shared_ptr<Ship>>                &ships,
    std::function<double(const std::shared_ptr<Ship> &)> func)
{
    return std::accumulate(
        ships.begin(), ships.end(), 0.0,
        [&func](double total, const std::shared_ptr<Ship> &ship) {
            return total + func(ship);
        });
}

int accumulateShipValuesInt(
    const QVector<std::shared_ptr<Ship>>             &ships,
    std::function<int(const std::shared_ptr<Ship> &)> func)
{
    return std::accumulate(
        ships.begin(), ships.end(), 0,
        [&func](int total, const std::shared_ptr<Ship> &ship) {
            return total + func(ship);
        });
}

void displayPathFindingProgress(int segmentIndex, int totalSegments,
                                double elapsedSeconds, int barLength)
{
    // Calculate percentage based on segments completed
    int percent = (totalSegments > 0)
                      ? (segmentIndex * 100 / totalSegments)
                      : 0;
    int filled = percent * barLength / 100;

    QString bar =
        QString(filled, '=') + QString(barLength - filled, ' ');
    QChar ending = (percent >= 100) ? '\n' : '\r';

    QTextStream out(stdout);

#ifdef Q_OS_WIN
    // Windows color handling
    HANDLE  hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO consoleInfo;
    GetConsoleScreenBufferInfo(hConsole, &consoleInfo);
    WORD saved_attributes = consoleInfo.wAttributes;

    // Set to cyan (FOREGROUND_GREEN | FOREGROUND_BLUE | INTENSITY)
    SetConsoleTextAttribute(
        hConsole, FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);

    out << "Finding path [" << bar << "] " << percent << "% ("
        << QString::number(elapsedSeconds, 'f', 1) << "s)" << ending;

    // Reset to original attributes
    SetConsoleTextAttribute(hConsole, saved_attributes);
#else
    // Linux/macOS: use ANSI escape codes
    // \033[1;36m = Bright Cyan, \033[0m = Reset
    out << "\033[1;36m"
        << "Finding path [" << bar << "] " << percent << "% ("
        << QString::number(elapsedSeconds, 'f', 1) << "s)"
        << "\033[0m" << ending;
#endif

    out.flush();
}

} // namespace Utils

namespace ThreadConfig
{

namespace
{
// Static thread pool instance for parallel operations
static QThreadPool* s_sharedThreadPool = nullptr;
static int s_maxThreads = -1; // -1 means not initialized
static QMutex s_mutex;
} // anonymous namespace

void setMaxThreads(int maxThreads)
{
    QMutexLocker locker(&s_mutex);

    int availableCores = QThread::idealThreadCount();

    if (maxThreads <= 0)
    {
        // Use half of available cores (minimum 1)
        s_maxThreads = std::max(1, availableCores / 2);
    }
    else if (maxThreads > availableCores)
    {
        // Cap at available cores
        s_maxThreads = availableCores;
    }
    else
    {
        s_maxThreads = maxThreads;
    }

    // Update the shared thread pool if it exists
    if (s_sharedThreadPool != nullptr)
    {
        s_sharedThreadPool->setMaxThreadCount(s_maxThreads);
    }
}

int getMaxThreads()
{
    QMutexLocker locker(&s_mutex);

    // Initialize if not set
    if (s_maxThreads < 0)
    {
        int availableCores = QThread::idealThreadCount();
        s_maxThreads = std::max(1, availableCores / 2);
    }

    return s_maxThreads;
}

int getAvailableCores()
{
    return QThread::idealThreadCount();
}

QThreadPool* getSharedThreadPool()
{
    QMutexLocker locker(&s_mutex);

    if (s_sharedThreadPool == nullptr)
    {
        s_sharedThreadPool = new QThreadPool();

        // Initialize max threads if not set
        if (s_maxThreads < 0)
        {
            int availableCores = QThread::idealThreadCount();
            s_maxThreads = std::max(1, availableCores / 2);
        }

        s_sharedThreadPool->setMaxThreadCount(s_maxThreads);
    }

    return s_sharedThreadPool;
}

void resetToDefault()
{
    QMutexLocker locker(&s_mutex);

    int availableCores = QThread::idealThreadCount();
    s_maxThreads = std::max(1, availableCores / 2);

    if (s_sharedThreadPool != nullptr)
    {
        s_sharedThreadPool->setMaxThreadCount(s_maxThreads);
    }
}

} // namespace ThreadConfig

} // namespace ShipNetSimCore
