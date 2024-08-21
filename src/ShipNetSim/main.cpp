/*!
 * @file main.cpp
 * @brief Entry point for the ShipNetSim application.
 *
 * This file contains the main function, which initializes the application,
 * sets up internationalization, processes command-line options, and launches
 * the simulator.
 *
 * @author Ahmed Aredah
 * @date 11/3/2023
 */

#include <QCoreApplication>
#include <ogr_geometry.h>
#include "simulator.h"
#include "./VersionConfig.h"
#include "utils/logger.h"
#include <QLocale>
#include <QTranslator>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include "ship/shipsList.h"
#include "utils/utils.h"
#include "utils/updatechecker.h"

// Compilation date and time are set by the preprocessor.
const std::string compilation_date = __DATE__;
const std::string compilation_time = __TIME__;

using namespace ShipNetSimCore;

/*!
 * @brief Validates the presence of a command-line option and
 * prints an error if required.
 * @param parser The command-line parser to check for the option.
 * @param option The specific command-line option to validate.
 * @param s The error message to display if the option is
 * missing and required.
 * @param isRequired Indicates if the command-line option is required.
 * @return True if the option is set, false otherwise.
 */
bool checkParserValue(QCommandLineParser& parser,
                      const QCommandLineOption &option,
                      std::string s, bool isRequired = true)
{
    // Check if the option is set in the command-line arguments.
    if(parser.isSet(option)) {
        return true;
    }
    // If the option is required but not set, print an error message.
    if (isRequired)
    {
        qWarning() << QString::fromStdString(s);
        // Add a newline for clear separation in the console.
        fputs("\n", stdout);
        // Print the help text to assist the user.
        fputs(qPrintable(parser.helpText()), stdout);
    }
    return false;
}

/*!
 * @brief The main function, entry point of the application.
 * @param argc Argument count.
 * @param argv Argument vector.
 * @return Application's exit code.
 */
int main(int argc, char *argv[])
{
    // ####################################################
    // #                     values                       #
    // ####################################################

    // Initialize the core application with given args.
    QCoreApplication app(argc, argv);

    // Define the update checker to check new available versions
    UpdateChecker updateChecker;
    QEventLoop loop; // event loop for the checker


    // Internationalization support: Translator object
    // and loading translation files.
    QTranslator translator;
    const QStringList uiLanguages = QLocale::system().uiLanguages();
    for (const QString &locale : uiLanguages) {
        const QString baseName = "ShipNetSim_" + QLocale(locale).name();
        // Attempt to load a translation file for the detected locale.
        if (translator.load(":/i18n/" + baseName))
        {
            // Install the translator into the application.
            app.installTranslator(&translator);
            break;
        }
    }

    // Set application metadata (used for command-line
    // parsing and system integration).
    std::string GithubLink = "https://github.com/VTTI-CSM/ShipNetSim";
    QCoreApplication::setApplicationName(ShipNetSim_NAME);
    QCoreApplication::setApplicationVersion(ShipNetSim_VERSION );
    QCoreApplication::setOrganizationName(
        QString::fromStdString(ShipNetSim_VENDOR));

    // Logger setup to attach to application events for logging.
    Logger::attach();

    // Command-line argument parsing setup.
    QCommandLineParser parser;
    parser.setApplicationDescription("Open-source network ships simulator");

    // Help option configuration.
    QCommandLineOption helpOption(QStringList() << "h" << "help" << "?",
                                  "Display this help message.");
    parser.addOption(helpOption);
    parser.addVersionOption();

    // Define command-line option for specifying water boundaries file.
    const QCommandLineOption waterBoundariesOption(
        QStringList() <<
            "b" << "water-boundaries-file",
        QCoreApplication::translate("main",
                                    "[Cond-Required] the water boundaries "
                                    "filename. "
                                    "This file is only required if you are "
                                    "running the full scale simulation and is "
                                    "not required if you are studying "
                                    "resistance only with command p or "
                                    "resistance-parametric-analysis"),
        "nodesFile", "");
    parser.addOption(waterBoundariesOption);

    // Define command-line option for specifying the ships file.
    const QCommandLineOption shipsFileOption(
        QStringList() <<
            "s" << "ships-file",
        QCoreApplication::translate("main",
                                    "[Required] the nodes filename."),
        "nodesFile", "");
    parser.addOption(shipsFileOption);

    // Define command-line option for specifying the output folder.
    const QCommandLineOption outputLocationOption(
        QStringList() << "o" << "output-folder",
        QCoreApplication::translate(
            "main",
            "[Optional] the output folder address. "
            "\nDefault is 'C:\\Users\\<USERNAME>\\Documents\\ShipNetSim\\'."),
        "outputLocation", "");
    parser.addOption(outputLocationOption);

    // Define command-line option for specifying the summary filename.
    const QCommandLineOption summaryFilenameOption(
        QStringList() << "r" << "result-summaries",
        QCoreApplication::translate(
            "main",
            "[Optional] the summary filename. "
            "\nDefault is 'shipSummary_timeStamp.txt'."),
        "summaryFilename", "");
    parser.addOption(summaryFilenameOption);

    // Define command-line option to show summaries for all ships.
    const QCommandLineOption summaryExportAllOption(
        QStringList() << "a" << "show-all-ships-summaries",
        QCoreApplication::translate(
            "main",
            "[Optional] bool to show summary of "
            "all ships in the summary file. \nDefault is 'false'."),
        "summarizeAllTrains", "false");
    parser.addOption(summaryExportAllOption);

    // Define command-line option for exporting instantaneous trajectory.
    const QCommandLineOption exportInstaTrajOption(
        QStringList() << "e" << "export-insta-file",
        QCoreApplication::translate(
            "main",
            "[Optional] bool to export instantaneous trajectory. "
            "\nDefault is 'false'."), "exportTrajectoryOptions" ,"false");
    parser.addOption(exportInstaTrajOption);

    // Define command-line option for specifying the insta-trajectory file.
    const QCommandLineOption instaTrajOption(
        QStringList() << "i" << "insta-file",
        QCoreApplication::translate(
            "main",
            "[Optional] the instantaneous trajectory filename. "
            "\nDefault is 'shipTrajectory_timeStamp.csv'."),
        "instaTrajectoryFile", "");
    parser.addOption(instaTrajOption);

    // Define command-line option for specifying the simulator time step.
    const QCommandLineOption timeStepOption(
        QStringList() << "t" << "timeStep",
        QCoreApplication::translate(
            "main",
            "[Optional] the simulator time step. "
            "\nDefault is '1.0'."), "simulatorTimeStep", "1.0");
    parser.addOption(timeStepOption);

    // Define command-line option for studying resistance exerted on the ship
    const QCommandLineOption studyResistances(
        QStringList() << "p" << "resistance-parametric-analysis",
        QCoreApplication::translate(
            "main",
            "[Optional] A flag to study the resistance exerted on "
            "the ship from zero speed to max ship speed! "
            "The simulator does not run if this flag is passed!"));
    parser.addOption(studyResistances);

    // process all the arguments
    parser.process(app);


    // display the help if requested and exit
    if (parser.isSet(helpOption)) {
        parser.showHelp(0);
    }

    // show app details
    std::stringstream hellos;
    hellos << ShipNetSim_NAME <<
        " [Version " << ShipNetSim_VERSION << ", "  <<
        compilation_date << " " << compilation_time <<
        " Build" <<  "]" << std::endl;
    hellos << ShipNetSim_VENDOR << std::endl;
    hellos << GithubLink << std::endl;
    std::cout << hellos.str() << "\n";

    // Connect the updateAvailable signal to a lambda that handles the result
    QObject::connect(&updateChecker, &UpdateChecker::updateAvailable,
                     [&](bool available) {
                         if (available) {
                             qDebug() << "An Update is Available! \n"
                                         "Download from: https://github.com/VTTI-CSM/ShipNetSim/releases\n\n";
                         }
                         loop.quit();
                     });

    // Start checking for updates
    updateChecker.checkForUpdates();

    // Start the event loop and wait for the update check to complete
    loop.exec();

    // Start checking for updates
    updateChecker.checkForUpdates();

    // Define variables for storing command-line option values.
    QString waterBoundariesFile, shipsFile,
        exportLocation, summaryFilename,
        instaTrajFilename;
    bool exportInstaTraj = false;
    double timeStep = 1.0;

    // Simulator and network object setup.
    OptimizedNetwork* net = nullptr;
    QVector<std::shared_ptr<Ship>> ships;
    std::unique_ptr<Simulator> sim;

    // Exception handling for simulator initialization.
    try
    {
        GDALAllRegister();
        // Assuming the default constructor for OGRSpatialReference is sufficient for WGS84
        // OGRSpatialReference crc;
        // crc.SetWellKnownGeogCS("WGS84");

        // // Create the GPoint object with a unique ID, e.g., "Point1"
        // GPoint p1( units::angle::degree_t(50.0), units::angle::degree_t(20.0), "Point1", crc);
        // GPoint p2( units::angle::degree_t(20.0), units::angle::degree_t(20.0), "Point1", crc);
        // std::cout << p1.distance(p2).value();
        // exit(0);


        // Read and validate command-line options.

        // parse the ships file
        if (checkParserValue(parser, shipsFileOption,
                             "ships file is missing!", true))
        {
            shipsFile =
                parser.value(shipsFileOption);
        }
        else
        {
            throw std::runtime_error("ships file is missing!");
        }

        // read optional values

        // parse the output location
        if (checkParserValue(parser, outputLocationOption, "" ,false)){
            exportLocation = parser.value(outputLocationOption);
            QDir directory(exportLocation);
            // check if directory exists; if not, attempt to create it
            if (!directory.exists()) {
                bool success = directory.mkpath(".");
                if (!success) {
                    // Handle the case where the directory
                    // could not be created.
                    throw std::runtime_error("Failed to create "
                                             "export directory!");
                }
            }

        }
        else { exportLocation = ""; }

        // parse the summary file name
        if (checkParserValue(parser, summaryFilenameOption, "", false))
        { summaryFilename = parser.value(summaryFilenameOption); }
        else { summaryFilename = ""; }

        // parse the export check of insta file
        if (checkParserValue(parser, exportInstaTrajOption, "", false))
        {
            QString value = parser.value(exportInstaTrajOption);
            bool ok;
            exportInstaTraj = Utils::stringToBool(value, &ok);
            if (!ok)
            {
                throw std::runtime_error("could not convert " +
                                         value.toStdString() +
                                         " to boolean!");
            }

        }
        else { exportInstaTraj = false; }

        // parse the insta file name
        if (checkParserValue(parser, instaTrajOption, "", false))
        { instaTrajFilename = parser.value(instaTrajOption); }
        else { instaTrajFilename = ""; }

        // parse the time step
        if (checkParserValue(parser, timeStepOption, "", false))
        { timeStep = parser.value(timeStepOption).toDouble(); }
        else { timeStep = 1.0; }

        // Check if the parametric resistance flag is set
        if (parser.isSet(studyResistances))
        {
            auto shipsDetails =
                ShipsList::readShipsFile(shipsFile, nullptr, true);
            ships = ShipsList::loadShipsFromParameters(shipsDetails);
            sim = std::make_unique<Simulator>(nullptr,
                                              ships,
                                              units::time::second_t(timeStep));
            // The flag is set, study resistance
            sim->setExportInstantaneousTrajectory(true,
                                                  instaTrajFilename);
            // Set up simulator output location.
            sim->setOutputFolderLocation(exportLocation);
            sim->setSummaryFilename(summaryFilename);

            std::cout <<"\nRunning Calculations!          \n";
            sim->studyShipsResistance();
            std::cout << "Finished Successfully!          \n";
        }
        else
        {

            // parse the waterboundaries file
            if (checkParserValue(parser, waterBoundariesOption,
                                 "Water boundaries file is missing!", true))
            {
                waterBoundariesFile =
                    parser.value(waterBoundariesOption);
            }
            else
            {
                throw std::runtime_error("Water boundaries file is missing!");
            }

            std::cout <<"\nLoading Networks!              \n";

            // Initialize network and simulator with config.
            net = new OptimizedNetwork(waterBoundariesFile);

            std::cout <<"\nLoading Ships!                 \n";
            auto shipsDetails =
                ShipsList::readShipsFile(shipsFile, net, false);
            ships = ShipsList::loadShipsFromParameters(shipsDetails);

            std::cout <<"\nPutting Things Together!       \n";
            sim = std::make_unique<Simulator>(net,
                                              ships,
                                              units::time::second_t(timeStep));
            // Set up simulator output location.
            sim->setOutputFolderLocation(exportLocation);
            sim->setSummaryFilename(summaryFilename);

            sim->setExportInstantaneousTrajectory(exportInstaTraj,
                                                  instaTrajFilename);
            // run the actual simulation
            std::cout <<"\nStarting Simulation!           \n";
            sim->runSimulation();
        }
        std::cout << "\nOutput folder: " <<
            sim->getOutputFolder().toStdString() << std::endl;
    }
    catch (const std::exception& e) {
        if (net) delete net;
        // Log setup errors and exit application.
        qWarning() << "An error occurred: " << e.what();
        Logger::detach(); // Ensure logger is detached.
        return 1; // Exit with an error code.
    }

    // Detach logger and start event loop.
    Logger::detach();
    return app.exec();


}
