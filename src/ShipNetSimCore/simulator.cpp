#include "simulator.h"
#include "ship/hydrology.h"
#include "ship/ishipcalmresistancestrategy.h"
#include "utils/shipscommon.h"
#include "utils/utils.h"
#include "ship/ship.h"
#include "VersionConfig.h"
#include "network/seaportloader.h"

namespace ShipNetSimCore
{

Simulator::Simulator(OptimizedNetwork* network,
                     QVector<std::shared_ptr<Ship>> shipList,
                     units::time::second_t simulatorTimeStep,
                     bool isExternallyControlled,
                     QObject *parent)
    : QObject{parent},
    mShips(shipList),
    mSimulationTime(units::time::second_t(0.0)),
    mSimulationEndTime(DefaultEndTime),
    mTimeStep(simulatorTimeStep),
    mNetwork(network),
    mIsExternallyControlled(isExternallyControlled)
{
    qDebug() << "Simulator initialized with" << mShips.size() << "ships.";

    if (!mNetwork) {
        throw std::runtime_error("Simulator initialization failed: "
                                 "Network pointer is null.");
    }

    mOutputLocation = Utils::getHomeDirectory();
    // Get a high-resolution time point
    auto now = std::chrono::high_resolution_clock::now();

    // Convert the time point to a numerical value
    simulation_serial_number =
        std::chrono::duration_cast<std::chrono::milliseconds>(
                             now.time_since_epoch()).count();

    // Define default names for trajectory and summary files with current time
    mTrajectoryFilename = DefaultInstantaneousTrajectoryFilename +
                          QString::number(simulation_serial_number) + ".csv";
    mSummaryFileName = DefaultSummaryFilename +
                       QString::number(simulation_serial_number) + ".txt";

    qRegisterMetaType<ShipsResults>("ShipsResults");
}

Simulator::~Simulator() {
    qDebug() << "Simulator destructor called. Cleaning up network and ships.";

    mNetwork = nullptr;
}

void Simulator::moveObjectToThread(QThread *thread)
{
    qDebug() << "Moving Simulator and associated objects to new thread.";


    // Move Simulator object itself to the thread
    this->moveToThread(thread);

    // Move network to the new thread
    if (mNetwork) {
        mNetwork->moveObjectToThread(thread);
    }

    // Move each Ship in mShips to the new thread
    for (auto& ship : mShips) {
        if (ship) {
            ship->moveObjectToThread(thread);
        }
    }
}

void Simulator::studyShipsResistance()
{
    qDebug() << "Starting studyShipsResistance for "
             << mShips.size() << " ships.";


    QString trajectoryFullPath = (mExportTrajectory)?
                                     QDir(mOutputLocation).filePath(
                                         mTrajectoryFilename) : "";
    mTrajectoryFile.initCSV(trajectoryFullPath);

    QString exportLine;
    QTextStream stream(&exportLine);
    stream << "ShipNo,"
              "Speed_knots,"
              "FroudeNumber,"
              "Fr_i,"
              "AirResistance_kN,"
              "BulbousBowResistance_kN,"
              "ImmersedTransomPressureResistance_kN,"
              "AppendageResistance_N,WaveResistance_kN,"
              "FrictionalResistance_kN,"
              "ModelCorrelationResistance_kN,"
              "TotalResistance_kN,"
              "ResistanceCoefficient";

    // Make sure to flush the QTextStream and write the header
    this->mTrajectoryFile.writeLine(exportLine);
    exportLine.clear(); // Clear after writing
    stream.seek(0);  // Reset the stream position to avoid errors

    for (auto &ship: mShips)
    {
        qDebug() << "Calculating resistance for Ship ID:" << ship->getUserID();

        // double s[] = {15.0, 15.5, 16.0, 16.5, 17.0, 17.5, 18.0, 18.5, 19.0};

        for (auto speedStep :
             Utils::linspace_step(0, ship->getMaxSpeed().
                                     convert<units::velocity::knot>().value()))
        {
            exportLine.clear();  // Ensure exportLine is cleared at the start of each speed
            QTextStream stream1(&exportLine);

            units::velocity::meters_per_second_t ss =
                units::velocity::knot_t(speedStep).
                convert<units::velocity::meters_per_second>();
            ship->setSpeed(ss);

            if (!ship->getCalmResistanceStrategy()) {
                QString errorMsg = QString("Ship ID: %1 - Missing calm "
                                           "resistance strategy.")
                                       .arg(ship->getUserID());
                throw std::runtime_error(errorMsg.toStdString());
            }

            // ship->setSpeed(units::velocity::meters_per_second_t(speedStep));
            auto speed = ss; //units::velocity::knot_t(speedStep).
                         //convert<units::velocity::meters_per_second>();
            auto fraudNo =
                hydrology::F_n(speed, ship->getLengthInWaterline());

            auto airRes =
                ship->getCalmResistanceStrategy()->getAirResistance(*ship);
            auto bulbousRes =
                ship->getCalmResistanceStrategy()->
                getBulbousBowResistance(*ship);
            auto transomRes =
                ship->getCalmResistanceStrategy()->
                getImmersedTransomPressureResistance(*ship);
            auto appendageRes =
                ship->getCalmResistanceStrategy()->
                getAppendageResistance(*ship);
            auto waveRes =
                ship->getCalmResistanceStrategy()->
                getWaveResistance(*ship);
            auto frictionalRes =
                ship->getCalmResistanceStrategy()->
                getfrictionalResistance(*ship);
            auto modelCorRes =
                ship->getCalmResistanceStrategy()->
                getModelShipCorrelationResistance(*ship);
            auto totalRes =
                ship->calculateTotalResistance();

            auto totalResCoef =
                ship->getCalmResistanceStrategy()->
                getCoefficientOfResistance(*ship);
            
            auto FRI = ship->getCalmResistanceStrategy()->calc_F_n_i(*ship);


            stream1 << ship->getUserID() << ","
                   << speed.convert<units::velocity::knot>().value() << ","
                   << fraudNo << ","
                   << FRI <<","
                   << airRes.convert<units::force::kilonewton>().value() << ","
                   << bulbousRes.convert<units::force::kilonewton>().value() << ","
                   << transomRes.convert<units::force::kilonewton>().value() << ","
                   << appendageRes.convert<units::force::kilonewton>().value() << ","
                   << waveRes.convert<units::force::kilonewton>().value() << ","
                   << frictionalRes.convert<units::force::kilonewton>().value() << ","
                   << modelCorRes.convert<units::force::kilonewton>().value() << ","
                   << totalRes.convert<units::force::kilonewton>().value() << ","
                   << totalResCoef;

            // Write the data to the CSV file
            mTrajectoryFile.writeLine(exportLine);
        }
    }

    mTrajectoryFile.writeLine("");
    mTrajectoryFile.writeLine("");

    exportLine.clear();
    stream << "ShipNo,"
              "Speed_knots,"
              "RPM,"
              "PropellerRequiredPower_kWh,"
              "EnginePower_kWh,"
              "EngineTorque_N.m";
    mTrajectoryFile.writeLine(exportLine);
    exportLine.clear();
    stream.seek(0);  // Reset the stream position to avoid errors

    for (auto &ship: mShips) {

        // Step 1: Get the max speed of the ship
        auto maxSpeed = ship->getMaxSpeed();

        for (auto& speedStep :
             Utils::linspace_step(
                 0.0, maxSpeed.convert<units::velocity::knot>().value(), 1.0))
        {

            exportLine.clear();  // Ensure exportLine is cleared at the start of each speed
            QTextStream stream2(&exportLine);

            // start with ID
            stream2 << ship->getUserID() << ",";

            units::velocity::meters_per_second_t ss =
                units::velocity::knot_t(speedStep).
                convert<units::velocity::meters_per_second>();
            ship->setSpeed(ss);

            // Use fixed-point notation with 2 decimals
            stream2 << QString::number(speedStep, 'f', 2) << ",";

            // Step 2: Calculate the speed of advance (SOA) using the
            // ship's calm resistance strategy
            auto SOA = ship->getCalmResistanceStrategy()->
                       calc_SpeedOfAdvance(*ship);

            // Step 3: Calculate the max RPM of the propeller based
            // on max speed and shaft power
            auto propeller = ship->getPropellers()->at(0);
            units::angular_velocity::revolutions_per_minute_t rpm =
                units::angular_velocity::revolutions_per_minute_t(
                    60.0 * SOA.value() /
                    (propeller->getPropellerPitch().value() *
                     (1.0 - propeller->getPropellerSlip())));

            // Write RPM to stream, with fixed-point format
            stream2 << QString::number(rpm.value(), 'f', 2) << ",";

            // Step 4: Write propeller shaft power at the calculated RPM
            stream2 << QString::number(
                propeller->getRequiredShaftPowerAtRPM(rpm).value(),
                'f', 2) << ",";

            for (auto& engine : propeller->getDrivingEngines()) {
                if (engine->isRPMWithinOperationalRange(rpm)) {
                    stream2 << QString::number(
                        engine->getEnginePropertiesAtRPM(rpm).breakPower.value(),
                        'f', 2) << ",";

                    stream2 << QString::number(
                        engine->getEngineTorqueByRPM(rpm).value(),
                        'f', 2) << ",";
                }
                else { // if it is outside the range, do not export it
                    continue;
                }
            }

            mTrajectoryFile.writeLine(exportLine);
        }
    }
}

void Simulator::addShipToSimulation(std::shared_ptr<Ship> ship) {

    qDebug() << "Adding ship " << ship->getUserID() << " to the simulator.";

    {
        // Lock the mutex to protect the mShips list
        QMutexLocker locker(&mutex);

        mShips.push_back(ship);
    } // Unlocks mutex here

    // Call resumeSimulation outside of the locked section to prevent deadlock
    if (mIsSimulatorPaused) this->resumeSimulation(false);

}

void Simulator::addShipsToSimulation(QVector< std::shared_ptr<Ship> > ships)
{
    for (auto ship : ships) {
        qDebug() << "Adding ship " << ship->getUserID()
                 << " to the simulator.";
    }

    {
        // Lock the mutex to protect the mShips list
        QMutexLocker locker(&mutex);

        for (auto &ship: ships) {
            mShips.push_back(ship);
        }
    } // Unlocks mutex here

    // Call resumeSimulation outside of the locked section to prevent deadlock
    if (mIsSimulatorPaused) this->resumeSimulation(false);
}


// Setter for the time step of the simulation
void Simulator::setTimeStep(units::time::second_t newTimeStep) {
    qDebug() << "Setting simulation time step to " << newTimeStep.value();

    // Lock the mutex to protect the mShips list
    QMutexLocker locker(&mutex);

    this->mTimeStep = newTimeStep;
}

units::time::second_t Simulator::getSimulatorTimeStep()
{
    return this->mTimeStep;
}

units::time::second_t Simulator::getCurrentSimulatorTime()
{
    return this->mSimulationTime;
}

// Setter for the end time of the simulation
void Simulator::setEndTime(units::time::second_t newEndTime) {
    qDebug() << "Setting simulation time to " << newEndTime.value();

    // Lock the mutex to protect the mShips list
    QMutexLocker locker(&mutex);

    this->mSimulationEndTime = newEndTime;
}

// Setter for the plot frequency
void Simulator::setPlotFrequency(int newPlotFrequency) {
    qDebug() << "Setting plotting frequency to " << newPlotFrequency;

    // Lock the mutex to protect the mShips list
    QMutexLocker locker(&mutex);

    this->mPlotFrequency = newPlotFrequency;
}

// Setter for the output folder location
void Simulator::setOutputFolderLocation(QString newOutputFolderLocation)
{
    qDebug() << "Setting output directory to " << newOutputFolderLocation;

    // Lock the mutex to protect the mShips list
    QMutexLocker locker(&mutex);

    if (newOutputFolderLocation.trimmed().isEmpty())
    {
        mOutputLocation = Utils::getHomeDirectory();
    }
    else
    {
        this->mOutputLocation = newOutputFolderLocation;
    }
}

// The getOutputFolder function returns the path to the
// directory where the output files are stored.
QString Simulator::getOutputFolder() {
    return this->mOutputLocation;
}

// Setter for the summary file name
void Simulator::setSummaryFilename(QString newfilename)
{

    qDebug() << "Setting summary file name to " << newfilename;

    // Lock the mutex to protect the mShips list
    QMutexLocker locker(&mutex);

    QString filename = newfilename;
    if (!filename.trimmed().isEmpty()){
        QFileInfo fileInfo(filename);
        // Check if the file name has an extension
        if (!fileInfo.completeSuffix().isEmpty())
        {
            this->mSummaryFileName = newfilename;
        }
        else
        {
            this->mSummaryFileName = newfilename + ".txt";
        }
    }
    else
    {
        DefaultSummaryFilename +
            QString::number(simulation_serial_number) + ".txt";
    }
}

// Setter for the instantaneous trajectory export flag and filename
void Simulator::setExportInstantaneousTrajectory(
    bool exportInstaTraject,
    QString newInstaTrajectFilename)
{

    qDebug() << "Setting enable instantaneous file generation to "
             << exportInstaTraject
             << " with output file name to "
             << newInstaTrajectFilename;

    // Lock the mutex to protect the mShips list
    QMutexLocker locker(&mutex);

    this->mExportTrajectory = exportInstaTraject;
    if (!newInstaTrajectFilename.trimmed().isEmpty())
    {
        QString filename = newInstaTrajectFilename;

        QFileInfo fileInfo(filename);
        // Check if the file name has an extension
        if (!fileInfo.completeSuffix().isEmpty())
        {
            // The new filename has an extension
            this->mTrajectoryFilename =
                newInstaTrajectFilename;
        }
        else
        {
            this->mTrajectoryFilename =
                newInstaTrajectFilename + ".csv";
        }
    }
    else
    {
        mTrajectoryFilename =
            DefaultInstantaneousTrajectoryFilename +
            QString::number(simulation_serial_number) + ".csv";
    }
}


void
Simulator::setExportIndividualizedShipsSummary(bool exportAllShipsSummary) {

    qDebug() << "Setting enable detailed summary to "
             << exportAllShipsSummary;

    // Lock the mutex to protect the mShips list
    QMutexLocker locker(&mutex);

    mExportIndividualizedShipsSummary = exportAllShipsSummary;
}

QJsonObject Simulator::getCurrentStateAsJson()
{
    QMutexLocker locker(&mutex);

    QJsonObject json;
    QJsonArray shipsArray;

    for (const auto& ship : mShips) {
        if (ship) {
            shipsArray.append(ship->getCurrentStateAsJson());
        }
    }
    json["Ships"] = shipsArray;

    json["CurrentSimulationTime"] = mSimulationTime.value();
    json["Progress"] = mProgressStep;

    return json;
}

// The checkAllShipsReachedDestination function checks if
// all ships have reached their destinations.
// If a ship has not reached its destination, it returns false;
// otherwise, it returns true.
bool Simulator::checkAllShipsReachedDestination()
{
    // give an opportunity for the external controller to add ships
    if (mShips.empty() && mIsExternallyControlled) {
        return false;
    }


    for (std::shared_ptr<Ship>& s : (mShips))
    {
        if (s->isOutOfEnergy() || !s->isShipStillMoving())
        {
            continue;
        }
        if (! s->isReachedDestination())
        {
            return false;
        }
    }
    return true;
}

void Simulator::initializeAllShips()
{
//    for (auto& ship: mShips)
//    {

//        // find the shortest path for the ship
//        ShortestPathResult r =
//            mNetwork->dijkstraShortestPath(ship->startPoint(),
//                                           ship->endPoint());
//        ship->setPath(r.points, r.lines);

//    }
}

void Simulator::initializeSimulation(bool emitSignal)
{
    qDebug() << "Initializing the simulation!";

    connect(this, &Simulator::simulationFinished,
            this, [this]() {
                generateSummaryData();
                exportSummaryToTXTFile();
                finalizeSimulation();
            });

    mTrajectoryFullPath = (mExportTrajectory)?
                                     QDir(mOutputLocation).filePath(
                                         mTrajectoryFilename) : "";

    // define trajectory file and set it up
    if (this->mExportTrajectory)
    {
        this->mTrajectoryFile.initCSV(mTrajectoryFullPath);
        QString exportLine;
        QTextStream stream(&exportLine);
        stream << "TStep_s,"                                 // 1. double
                  "ShipNo,"                                  // 2. int
                  "WaterSalinity_ppt,"                       // 3. double
                  "WaveHeight_m,"                            // 4. double
                  "WaveFrequency_hz,"                        // 5. double
                  "WaveLength_m,"                            // 6. double
                  "NorthwardWindSpeed_mps,"                  // 7. double
                  "EastwardWindSpeed_mps,"                   // 8. double
                  "TotalShipThrust_N,"                       // 9. double
                  "TotalShipResistance_N,"                   // 10. double
                  "maxAcceleration_mps2,"                    // 11. double
                  "TravelledDistance_m,"                     // 12. double
                  "Acceleration_mps2,"                       // 13. double
                  "Speed_knots,"                             // 14. double
                  "CumEnergyConsumption_KWH,"                // 15. double
                  "MainEnergySourceCapacityState_percent,"   // 16. double
                  "Position(long;lat),"                      // 17. string
                  "Course_deg,";                             // 18. double

        this->mTrajectoryFile.writeLine(exportLine);
    }

    mInitTime = std::chrono::system_clock::to_time_t(
        std::chrono::system_clock::now());

    mSimulatorInitialized = true;

    if (emitSignal) emit simulationInitialized();
}

void Simulator::runSimulation(units::time::second_t runFor,
                              bool endSimulationAfterRun,
                              bool emitEndStepSignal)
{
    qDebug() << "Starting simulation.";

    // initialize the simulator only if it was not initialized earlier
    if (!mSimulatorInitialized) {
        initializeSimulation(false);
    }

    while ((this->mSimulationTime <= this->mSimulationTime + runFor) &&
           (this->mSimulationTime <= this->mSimulationEndTime) )
    {
        {
            QMutexLocker locker(&mutex);  // Lock the mutex automatically
        
            // This will block the thread if pauseFlag is true
            while (mIsSimulatorPaused) {    // Use a loop to re-check
                                            // the condition
                qWarning() << "Simulation has been paused externally.";
                pauseCond.wait(&mutex);  // Automatically releases
                                         // and reacquires the mutex
            }
        }   // The mutex is automatically unlocked here
            // when `locker` goes out of scope

        // Check if the simulation should continue running
        if (!mIsSimulatorRunning) {
            qWarning() << "Simulation has been stopped externally.";

            break;
        }

        if (this->checkAllShipsAreNotMoving()) {

            if (mIsExternallyControlled) {
                qWarning() << "All ships have stopped moving.";

                continue;
            }
            qWarning() << "All ships have stopped moving. Ending simulation.";

            break;
        }

        if (this->checkAllShipsReachedDestination()) {
            // Emit the signal when all ships have reached their destination
            emit allShipsReachedDestination();

            if (mIsExternallyControlled) {
                // Pause the simulation to wait for new ships to be added
                qDebug() << "All ships have reached their "
                            "destination, pausing simulation.";
                this->pauseSimulation(false);
                continue;
            }

            qDebug() << "All ships have reached their destination.";

            break;
        }

        // one step simulation
        runOneTimeStep();

        // ##################################################################
        // #             start: show progress on console                    #
        // ##################################################################

        qDebug() << "Simulation ended.";

        this->ProgressBar(mShips, 100, emitEndStepSignal);

    }

    if (!std::isinf(runFor.value())){
        emit simulationReachedReportingTime(mSimulationTime,
                                            mProgressPercentage);
    }

    if (endSimulationAfterRun) {
        endSimulation();
    }
}

bool Simulator::checkAllShipsAreNotMoving() {
    return std::all_of(mShips.begin(), mShips.end(), [](const auto& s) {
        return !s->isShipStillMoving();
    });
}

void Simulator::endSimulation() {
    emit simulationFinished();
}

void Simulator::restartSimulation()
{
    mInactiveShipsCount = 0;
    mSimulationTime = units::time::second_t(0);
    mProgressStep = -1;
    mProgressPercentage = 0.0;
    mSummaryTextData = "";
    for (auto &ship : mShips) {
        ship->reset();
    }
    mIsSimulatorPaused = false;
    mIsSimulatorRunning = true;
    mTrajectoryFile.clearFile();
    mSummaryFile.clearFile();

    emit simulationRestarted();
}

QVector<std::shared_ptr<SeaPort>>
Simulator::getAvailablePorts(bool considerShipsPathPortsOnly)
{
    QVector<QString> portsArray;

    QVector<std::shared_ptr<SeaPort>> ports;

    if (considerShipsPathPortsOnly) {
        ports = SeaPortLoader::getPorts();
    }
    else {
        for (auto &ship : mShips) {
            auto points = ship->getShipPathPoints();

            for (auto &point : *points) {
                auto port =
                    SeaPortLoader::getClosestPortToPoint(
                        point, units::length::meter_t(3000.0));
                if (port) {
                    ports.append(port);
                }
            }

        }
    }

    for (const auto& port : ports) {
        portsArray.append(port->getPortCode());
    }
    emit availablePorts(portsArray);
    return ports;
}

void Simulator::generateSummaryData()
{
    QMutexLocker locker(&mutex);

    // ##################################################################
    // #                       start: summary file                      #
    // ##################################################################
    time_t fin_time =
        std::chrono::system_clock::to_time_t(
        std::chrono::system_clock::now());
    double difTime = difftime(fin_time, mInitTime);
    QString mSummaryTextData;
    QTextStream stream(&mSummaryTextData);

    stream
        << "~.~.~.~.~.~.~.~.~.~.~.~.~.~.~.~.~.~.~.~.~.~.~.~.~.~\n"
        << "ShipNetSim SIMULATION SUMMARY\n"
        << "Version: " << ShipNetSim_VERSION << "\n"
        << "Simulation Time: " << Utils::formatDuration(difTime) << " (dd:hh:mm:ss)\n"
        << "~.~.~.~.~.~.~.~.~.~.~.~.~.~.~.~.~.~.~.~.~.~.~.~.~.~\n\n"
        << "+ NETWORK STATISTICS:\n"
        << "  |_ Total Number of Ships on Network                                           \x1D : " << Utils::thousandSeparator(this->mShips.size()) << "\n"
        << "....................................................\n\n"
        << "\n";
    stream
        << "+ AGGREGATED/ACCUMULATED SHIPS STATISTICS:\n"
        << "    |-> Moved Commodity:\n"
        << "        |_ Total Moved Cargo (ton)                                              \x1D : " << Utils::thousandSeparator(ShipNetSimCore::Utils::accumulateShipValuesDouble(mShips, std::function<double(const std::shared_ptr<Ship>&)>([](const auto& s) { return s->getCargoWeight().value(); }))) << "\n"
        << "        |_ Total ton.km (ton.Km)                                                \x1D : " << Utils::thousandSeparator(ShipNetSimCore::Utils::accumulateShipValuesDouble(mShips, std::function<double(const std::shared_ptr<Ship>&)>([](const auto& s) { return s->getTotalCargoTonKm().value(); }))) << "\n\n"
        << "  |-> Route Information:\n"
        << "    |_ Ships Reached Destination                                                \x1D : " << ShipNetSimCore::Utils::accumulateShipValuesInt(mShips, std::function<int(const std::shared_ptr<Ship>&)>([](const auto& s) { return s->isReachedDestination(); })) << "\n"
        << "    |_ Ships Total Path Length (km)                                             \x1D : " << Utils::thousandSeparator(ShipNetSimCore::Utils::accumulateShipValuesDouble(mShips, std::function<double(const std::shared_ptr<Ship>&)>([](const auto& s) { return s->getTotalPathLength().value(); }))) << "\n\n"
        << "  |-> Ships Performance:\n"
        << "    |_ Operating Time (d:h::m::s)                                               \x1D : " << Utils::formatDuration(ShipNetSimCore::Utils::accumulateShipValuesDouble(mShips, std::function<double(const std::shared_ptr<Ship>&)>([](const auto& s) { return s->getTripTime().value(); }))) << "\n"
        << "    |_ Average Speed (meter/second)                                             \x1D : " << ShipNetSimCore::Utils::accumulateShipValuesDouble(mShips, std::function<double(const std::shared_ptr<Ship>&)>([](const auto& s) { return s->getTripRunningAvergageSpeed().value(); })) / this->mShips.size() << "\n"
        << "    |_ Average Acceleration (meter/square second)                               \x1D : " << Utils::thousandSeparator(ShipNetSimCore::Utils::accumulateShipValuesDouble(mShips, std::function<double(const std::shared_ptr<Ship>&)>([](const auto& s) { return s->getTripRunningAverageAcceleration().value(); })) / this->mShips.size(), 4) << "\n"
        << "    |_ Average Travelled Distance (km)                                          \x1D : " << Utils::thousandSeparator(ShipNetSimCore::Utils::accumulateShipValuesDouble(mShips, std::function<double(const std::shared_ptr<Ship>&)>([](const auto& s) { return s->getTraveledDistance().value() / 1000.0; })) / this->mShips.size()) << "\n"
        << "    |_ Consumed and Regenerated Energy:\n"
        << "        |_ Total Net Energy Consumed (KW.h)                                     \x1D : " << Utils::thousandSeparator(ShipNetSimCore::Utils::accumulateShipValuesDouble(mShips, std::function<double(const std::shared_ptr<Ship>&)>([](const auto& s) { return s->getCumConsumedEnergy().value(); }))) << "\n"
        << "            |_ Total Energy Consumed (KW.h)                                     \x1D : " << Utils::thousandSeparator(ShipNetSimCore::Utils::accumulateShipValuesDouble(mShips, std::function<double(const std::shared_ptr<Ship>&)>([](const auto& s) { return s->getCumConsumedEnergy().value(); }))) << "\n"
        << "            |_ Total Energy Regenerated (KW.h)                                  \x1D : " << Utils::thousandSeparator(0.0) << "\n"
        << "            |_ Average Net Energy Consumption per Net Weight (KW.h/ton)         \x1D : " << Utils::thousandSeparator(ShipNetSimCore::Utils::accumulateShipValuesDouble(mShips, std::function<double(const std::shared_ptr<Ship>&)>([](const auto& s) { return s->getEnergyConsumptionPerTon().value(); }))) << "\n"
        << "            |_ Average Net Energy Consumption per Net ton.km (KW.hx10^3/ton.km) \x1D : " << Utils::thousandSeparator(ShipNetSimCore::Utils::accumulateShipValuesDouble(mShips, std::function<double(const std::shared_ptr<Ship>&)>([](const auto& s) { return s->getEnergyConsumptionPerTonKM().value() * 1000.0; }))) << "\n"
        << "        |_ Tank Consumption:\n";

    std::map<ShipFuel::FuelType, units::volume::liter_t> totalFuelConsumed;
    for (auto& s: mShips) {
        for (auto& fc: s->getCumConsumedFuel()) {
            totalFuelConsumed[fc.first] += fc.second;
        }
    }

    stream
        << "            |_ Total Overall Fuel Consumed (liters)                             \x1D : " << Utils::thousandSeparator(ShipNetSimCore::Utils::accumulateShipValuesDouble(mShips, std::function<double(const std::shared_ptr<Ship>&)>([](const auto& s) { return s->getOverallCumFuelConsumption().value(); }))) << "\n";

    for (auto& fc: totalFuelConsumed) {
        stream
            << Utils::formatString(
                   "                |_ ",
                   ShipFuel::convertFuelTypeToString(fc.first) + " (liters) ",
                   "\x1D : ", " ", 84) << Utils::thousandSeparator(fc.second.value()) << "\n";
    }

    stream
        << "            |_ Average Fuel Consumed per Net Weight (litersx10^3/ton)           \x1D : " << Utils::thousandSeparator(ShipNetSimCore::Utils::accumulateShipValuesDouble(mShips, std::function<double(const std::shared_ptr<Ship>&)>([](const auto& s) { return s->getOverallCumFuelConsumptionPerTon().value(); }))) << "\n"
        << "            |_ Average Fuel Consumed per Net ton.km (litersx10^3/ton.km)       \x1D : " << Utils::thousandSeparator(ShipNetSimCore::Utils::accumulateShipValuesDouble(mShips, std::function<double(const std::shared_ptr<Ship>&)>([](const auto& s) { return s->getOverallCumFuelConsumptionPerTonKM().value(); }))) << "\n"
        << "    |_ Environmental Impact:\n"
        << "        |_ Total CO2 Emissions (kg)                                             \x1D : " << Utils::thousandSeparator(ShipNetSimCore::Utils::accumulateShipValuesDouble(mShips, std::function<double(const std::shared_ptr<Ship>&)>([](const auto& s) { return s->getTotalCO2Emissions().value(); }))) << "\n"
        << "        |_ Average CO2 Emissions per Net Weight (kg/ton)                        \x1D : " << Utils::thousandSeparator(ShipNetSimCore::Utils::accumulateShipValuesDouble(mShips, std::function<double(const std::shared_ptr<Ship>&)>([](const auto& s) { return s->getTotalCO2EmissionsPerTon(); }))) << "\n"
        << "        |_ Average CO2 Emissions per Net ton.km (kg/ton.km)                     \x1D : " << Utils::thousandSeparator(ShipNetSimCore::Utils::accumulateShipValuesDouble(mShips, std::function<double(const std::shared_ptr<Ship>&)>([](const auto& s) { return s->getCO2EmissionsPerTonKM(); }))) << "\n"
        << "....................................................\n\n";

    for (auto &ship: mShips)
    {
        stream
            << "Ship ID: " << ship->getUserID() << "\n"
            << "SHIP GENERAL INFORMATION\n"
            << "    |-> Moved Commodity:\n"
            << "        |_ Total Moved Cargo (ton)                                              \x1D : " << Utils::thousandSeparator(ship->getCargoWeight().value()) << "\n"
            << "        |_ Total ton.km (ton.km)                                                \x1D : " << Utils::thousandSeparator(ship->getTotalCargoTonKm().value()) << "\n\n"
            << "  |-> Route Information:\n"
            << "    |_ Ships Reached Destination                                                \x1D : " << (ship->isReachedDestination() ? "Yes" : "No") << "\n"
            << "    |_ Ships Total Path Length (km)                                             \x1D : " << Utils::thousandSeparator(ship->getTotalPathLength().value()) << "\n\n"
            << "  |-> Ships Performance:\n"
            << "    |_ Operating Time (d:h::m::s)                                               \x1D : " << Utils::formatDuration(ship->getTripTime().value()) << "\n"
            << "    |_ Average Speed (meter/second)                                             \x1D : " << ship->getTripRunningAvergageSpeed().value() << "\n"
            << "    |_ Average Acceleration (meter/square second)                               \x1D : " << Utils::thousandSeparator(ship->getTripRunningAverageAcceleration().value(), 4) << "\n"
            << "    |_ Average Travelled Distance (km)                                          \x1D : " << Utils::thousandSeparator(ship->getTraveledDistance().value() / 1000.0) << "\n\n"
            << "    |_ Consumed and Regenerated Energy:\n"
            << "        |_ Total Net Energy Consumed (KW.h)                                     \x1D : " << Utils::thousandSeparator(ship->getCumConsumedEnergy().value()) << "\n"
            << "            |_ Total Energy Consumed (KW.h)                                     \x1D : " << Utils::thousandSeparator(ship->getCumConsumedEnergy().value()) << "\n"
            << "            |_ Total Energy Regenerated (KW.h)                                  \x1D : " << Utils::thousandSeparator(0.0) << "\n"
            << "            |_ Average Net Energy Consumption per Net Weight (KW.h/ton)         \x1D : " << Utils::thousandSeparator(ship->getEnergyConsumptionPerTon().value()) << "\n"
            << "            |_ Average Net Energy Consumption per Net ton.km (KW.hx10^3/ton.km) \x1D : " << Utils::thousandSeparator(ship->getEnergyConsumptionPerTonKM().value() * 1000.0) << "\n\n"
            << "        |_ Tank Consumption:\n";

        std::map<ShipFuel::FuelType, units::volume::liter_t> shipFuelConsumed;
        for (const auto &fc: ship->getCumConsumedFuel()) {
            shipFuelConsumed[fc.first] += fc.second;
        }

        stream
            << "            |_ Total Overall Fuel Consumed (liters)                            \x1D : " << Utils::thousandSeparator(ship->getOverallCumFuelConsumption().value()) << "\n";

        for (const auto &fc: shipFuelConsumed) {
            stream << Utils::formatString(
                "                |_ ",
                ShipFuel::convertFuelTypeToString(fc.first) + " (liters) ",
                "\x1D : ", " ", 84) << Utils::thousandSeparator(fc.second.value()) << "\n";
        }

        stream
            << "            |_ Average Fuel Consumed per Net Weight (litersx10^3/ton)           \x1D : " << Utils::thousandSeparator(ship->getOverallCumFuelConsumptionPerTon().value()) << "\n"
            << "            |_ Average Fuel Consumed per Net ton.km (litersx10^3/ton.km)        \x1D : " << Utils::thousandSeparator(ship->getOverallCumFuelConsumptionPerTonKM().value()) << "\n\n"
            << "    |_ Environmental Impact:\n"
            << "        |_ Total CO2 Emissions (kg)                                             \x1D : " << Utils::thousandSeparator(ship->getTotalCO2Emissions().value()) << "\n"
            << "        |_ Average CO2 Emissions per Net Weight (kg/ton)                        \x1D : " << Utils::thousandSeparator(ship->getTotalCO2EmissionsPerTon()) << "\n"
            << "        |_ Average CO2 Emissions per Net ton.km (kg/ton.km)                     \x1D : " << Utils::thousandSeparator(ship->getCO2EmissionsPerTonKM()) << "\n"
            << "....................................................\n\n";
    }

    mSummaryFullPath =
        QDir(mOutputLocation).filePath(mSummaryFileName);

    // ##################################################################
    // #                       end: summary file                      #
    // ##################################################################

    QVector<QPair<QString, QString>> shipsSummaryData;
    shipsSummaryData = Utils::splitStringStream(mSummaryTextData, "\x1D :");

    ShipsResults rs =
        ShipsResults(shipsSummaryData, mTrajectoryFullPath, mSummaryFullPath);

    // fire the signal of available simulation results
    emit this->simulationResultsAvailable(rs);
}

void Simulator::exportSummaryToTXTFile() {
    this->mSummaryFile.initTXT(mSummaryFullPath);
    // setup the summary file
    this->mSummaryFile.writeFile(mSummaryTextData.replace("\x1D", ""));
    // Close the file
    this->mSummaryFile.close();
}

void Simulator::finalizeSimulation() {
    this->mTrajectoryFile.close();
}

void Simulator::runOneTimeStep() {

    QVector<std::shared_ptr<Ship>> shipsToSimulate;


    // Lock the mutex only for the duration of accessing or modifying the mShips list
    {
        QMutexLocker locker(&mutex);
        shipsToSimulate = mShips;
    }


    for (std::shared_ptr<Ship>& s : shipsToSimulate) {
        if (s->isReachedDestination() || !s->isShipStillMoving()) { continue; }
        this->playShipOneTimeStep(s);
    }

    if (mPlotFrequency > 0.0 &&
        ((int(this->mSimulationTime) * 10) % (mPlotFrequency * 10)) == 0)
    {
        QVector<std::pair<QString, GPoint>> shipsLocs;

        for (std::shared_ptr <Ship>& s : (shipsToSimulate)) {
            if (! s->isLoaded()) { continue; }

            GPoint nP = s->getCurrentPosition();
            auto data = std::make_pair(s->getUserID(),
                                       nP);
            shipsLocs.push_back(data);
        }
        emit this->plotShipsUpdated(shipsLocs);
    }

    this->mSimulationTime += this->mTimeStep;

    // Minimize waiting when no ships are on network
    if (!mIsExternallyControlled && this->checkNoShipIsOnNetwork())
    {
        // get the next ships to enter the network time
        auto shiftTime = this->getNotLoadedShipsMinStartTime();
        if (shiftTime > this->mSimulationTime)
        {
            this->mSimulationTime = shiftTime;
        }
    }
}

// This function simulates one time step for a given ship
// in the simulation environment
void Simulator::playShipOneTimeStep(std::shared_ptr<Ship> ship)
{
    // Check if the ship start time is passed
    // if such, load ship first and then run the simulation
    if (this->mSimulationTime >= ship->getStartTime() &&
        !ship->isLoaded())
    {
        bool skipLoadingship = false;

        // check if there is a ship that is already loaded in
        // the same node and still on the same starting node
        for (std::shared_ptr<Ship>& otherShip : this->mShips)
        {
            // check if the ship is loaded and not reached destination
            if (otherShip == ship ||
                !otherShip->isLoaded() ||
                otherShip->isReachedDestination()) { continue; }

            // check if the ship has the same starting node and
            // the otherShip still on the same starting node
            if (otherShip->getShipPathPoints()->at(0) ==
                ship->getShipPathPoints()->at(0) &&
                otherShip->getTraveledDistance() <=
                    otherShip->getLengthInWaterline())
            {
                skipLoadingship = true;
                break;
            }
        }
        // skip loading the ship if there is any ship at the same point
        if (!skipLoadingship)
        {
            // load ship
            ship->load();
        }
    }

    // Continue if the ship is loaded and its start time
    // is past the current simulation time
    if ((ship->getStartTime() <= this->mSimulationTime) &&
        ship->isLoaded())
    {

        AlgebraicVector::Environment currentEnvironment =
            mNetwork->getEnvironmentFromPosition(ship->getCurrentPosition());

        // get the next port or stopping point
        Ship::stopPointDefinition stopPoint = ship->getNextStoppingPoint();

        // get the lower speeds links by their starting point
        // auto aheadLowerSpeeds = ship->getAheadLowerSpeeds(
        //     stopPoint.pointIndex);

        // define the critical points
        // a critical point is
        // 1. any point that defines a lower max speed
        // 2. stopping point (port)
        criticalPoints cp;

        // // add all lower speed points to their corresponding lists
        // for (auto &index : aheadLowerSpeeds.keys())
        // {
        //     cp.gapToCriticalPoint.push_back(
        //         ship->distanceFromCurrentPositionToNodePathIndex(index));
        //     cp.speedAtCriticalPoint.push_back(aheadLowerSpeeds[index]);
        //     cp.isFollowingAnotherShip.push_back(false);
        // }

        // add the stopping station to the list
        cp.gapToCriticalPoint.push_back(
            ship->distanceFromCurrentPositionToNodePathIndex(
                stopPoint.pointIndex));
        cp.speedAtCriticalPoint.push_back(
            units::velocity::meters_per_second_t(0.0));
        cp.isFollowingAnotherShip.push_back(false);


        // check if the ship stopped a little earlier than it should,
        // if yes, give it a kick forward... This only happens if
        // the ship is not dwelling
        if ((!ship->isCurrentlyDwelling()) &&
            (cp.gapToCriticalPoint.size() == 1) &&
            (ship->getAcceleration().value() <= 0.0) &&
            ((std::round(ship->getPreviousSpeed().value() * 1000.0) /
              1000.0) == 0.0) &&
            ((std::round(ship->getSpeed().value() * 1000.0) / 1000.0) == 0.0) &&
            (ship->getSpeed().value() >= 0.0) &&
            (ship->getSpeed().value() * mTimeStep.value() * 3.0 >=
             cp.gapToCriticalPoint.back().value()))
        {
            ship->kickForwardADistance(cp.gapToCriticalPoint.back(),
                                       mTimeStep);
        }

        // move the ship forward by ship dynamics
        auto currentMaxSpeed =
            units::velocity::meters_per_second_t(100.0); //ship->getCurrentMaxSpeed();

        ship->sail(mSimulationTime,
                   mTimeStep,
                   currentMaxSpeed,
                   cp.gapToCriticalPoint,
                   stopPoint.point,
                   cp.isFollowingAnotherShip,
                   cp.speedAtCriticalPoint,
                   currentEnvironment);


        // calculate stats even if the ship did not move
        ship->calculateGeneralStats(mTimeStep);

        if (mExportTrajectory)
        {
            QString s;
            QTextStream stream(&s);
            stream << mSimulationTime.value() << ","
                   << ship->getUserID() << ","
                   << currentEnvironment.salinity.value() << ","
                   << currentEnvironment.waveHeight.value() << ","
                   << currentEnvironment.waveFrequency.value() << ","
                   << currentEnvironment.waveLength.value() << ","
                   << currentEnvironment.windSpeed_Northward.value() << ","
                   << currentEnvironment.windSpeed_Eastward.value() << ","
                   << ship->getTotalThrust().value() << ","
                   << ship->calculateTotalResistance().value() << ","
                   << ship->getMaxAcceleration().value() << ","
                   << QString::number(ship->getTraveledDistance().value(), 'f', 3) << ","
                   << QString::number(ship->getAcceleration().value(), 'f', 3) << ","
                   << QString::number(ship->getSpeed().convert<units::velocity::knot>().value(), 'f', 3) << ","
                   << QString::number(ship->getCumConsumedEnergy().value(), 'f', 3) << ","
                   << QString::number(ship->getMainTankCurrentCapacity(), 'f', 3) << ","
                   << ship->getCurrentPosition().toString("(%x; %y)") << ","
                   << QString::number(ship->getCurrentHeading().value(), 'f', 3);

            mTrajectoryFile.writeLine(s);
        }
    }

}

bool Simulator::checkNoShipIsOnNetwork() {
    for (std::shared_ptr<Ship>& s : (this->mShips))
    {
        if (s->isLoaded() && ! s->isReachedDestination())
        {
            return false;
        }
    }
    qWarning() << "No ship is active on the network.";
    return true;
}

units::time::second_t Simulator::getNotLoadedShipsMinStartTime()
{
    QVector<units::time::second_t> st;
    for (std::shared_ptr<Ship>& s : (mShips))
    {
        if (!s->isLoaded())
        {
            st.push_back(s->getStartTime());
        }
    }
    if (st.empty()) { return units::time::second_t(-1.0); }
    return *(std::min_element(st.begin(), st.end()));
}

void Simulator::ProgressBar(QVector<std::shared_ptr<Ship>> ships,
                            int bar_length, bool emitProgressSignal)
{
    double sum = 0.0;
    for (const auto& ship : ships) {
        sum += ship->progress();
    }

    double fraction = sum / ships.size();
    mProgressPercentage = fraction * 100.0;
    int progressValue = static_cast<int>(fraction * bar_length);
    int progressPercent = static_cast<int>(fraction * 100);

    // Only print and update when the progress percent changes
    if (progressPercent != this->mProgressStep) {
        QTextStream out(stdout);
        QString bar = QString(progressValue, '-').append('>')
                          .append(QString(bar_length - progressValue, ' '));

        QChar ending = (progressPercent >= 100) ? '\n' : '\r';

#ifdef Q_OS_WIN
        // --------- Windows approach (SetConsoleTextAttribute) -----------
        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

        // Save the current console attributes
        CONSOLE_SCREEN_BUFFER_INFO consoleInfo;
        GetConsoleScreenBufferInfo(hConsole, &consoleInfo);
        WORD saved_attributes = consoleInfo.wAttributes;

        // Set text color to bright green
        // FOREGROUND_GREEN | FOREGROUND_INTENSITY = bright green
        SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN | FOREGROUND_INTENSITY);

        // Print the progress
        out << "Progress: [" << bar << "] " << progressPercent << "%" << ending;

        // Reset to original console attributes
        SetConsoleTextAttribute(hConsole, saved_attributes);

#else \
    /** --------- macOS & Linux approach (ANSI escape codes) ----------- \
     \033[1;32m = Bright Green \
     \033[0m    = Reset color **/
        out << "\033[1;32m"
            << "Progress: [" << bar << "] "
            << progressPercent << "%"
            << "\033[0m"
            << ending;
#endif

        this->mProgressStep = progressPercent;
        if (emitProgressSignal) {
            emit this->progressUpdated(this->mProgressStep);
        }
    }
}


void Simulator::pauseSimulation(bool emitSignal) {
    QMutexLocker locker(&mutex);  // Use QMutexLocker to prevent potential deadlocks
    mIsSimulatorPaused = true;
    if (emitSignal) emit simulationPaused();
}

void Simulator::resumeSimulation(bool emitSignal) {
    QMutexLocker locker(&mutex);  // Use QMutexLocker to prevent potential deadlocks
    mIsSimulatorPaused = false;
    pauseCond.wakeAll();
    if (emitSignal) emit simulationResumed();
}

void Simulator::terminateSimulation(bool emitSignal) {
    qWarning() << "Terminating simulation.";

    QMutexLocker locker(&mutex);  // Use QMutexLocker to prevent potential deadlocks
    mIsSimulatorRunning = false;  // Stop the simulation loop
    mIsSimulatorPaused = false;   // Ensure the simulation is not paused

    pauseCond.wakeAll();  // Wake up any paused threads

    if (emitSignal) emit simulationTerminated();
}

}
