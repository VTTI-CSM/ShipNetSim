#include "simulator.h"
#include "network/network.h"
#include "utils/utils.h"
#include "ship/ship.h"
#include "../../VersionConfig.h"

Simulator::Simulator(std::shared_ptr<Network> network,
                     QVector<std::shared_ptr<Ship>> shipList,
                     units::time::second_t simulatorTimeStep,
                     QObject *parent)
    : mNetwork(network), mShips(shipList),
    mTimeStep(simulatorTimeStep), QObject{parent},
    mSimulationEndTime(DefaultEndTime),
    mSimulationTime(units::time::second_t(0.0))
{
    mOutputLocation = Utils::getHomeDirectory();
    // Get a high-resolution time point
    auto now = std::chrono::high_resolution_clock::now();

    // Convert the time point to a numerical value
    auto serial_number =
        std::chrono::duration_cast<std::chrono::milliseconds>(
                             now.time_since_epoch()).count();

    // Define default names for trajectory and summary files with current time
    mTrajectoryFilename = DefaultInstantaneousTrajectoryFilename +
                          QString::number(serial_number) + ".csv";
    mSummaryFileName = DefaultSummaryFilename +
                       QString::number(serial_number) + ".txt";

}

// Setter for the time step of the simulation
void Simulator::setTimeStep(units::time::second_t newTimeStep) {
    this->mTimeStep = newTimeStep;
}

// Setter for the end time of the simulation
void Simulator::setEndTime(units::time::second_t newEndTime) {
    this->mSimulationEndTime = newEndTime;
}

// Setter for the plot frequency
void Simulator::setPlotFrequency(int newPlotFrequency) {
    this->mPlotFrequency = newPlotFrequency;
}

// Setter for the output folder location
void Simulator::setOutputFolderLocation(QString newOutputFolderLocation)
{
    this->mOutputLocation = newOutputFolderLocation;
}
// The getOutputFolder function returns the path to the
// directory where the output files are stored.
QString Simulator::getOutputFolder() {
    return this->mOutputLocation;
}

// Setter for the summary file name
void Simulator::setSummaryFilename(QString newfilename)
{
    QString filename = newfilename;
    if (!filename.isEmpty()){
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
        this->mSummaryFileName = DefaultSummaryFilename;
    }
}

// Setter for the instantaneous trajectory export flag and filename
void Simulator::setExportInstantaneousTrajectory(
    bool exportInstaTraject,
    QString newInstaTrajectFilename)
{
    this->mExportTrajectory = exportInstaTraject;
    if (newInstaTrajectFilename != "")
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
        // Get a high-resolution time point
        auto now = std::chrono::high_resolution_clock::now();

        // Convert the time point to a numerical value
        auto serial_number =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                                 now.time_since_epoch()).count();

        this->mTrajectoryFilename =
            DefaultInstantaneousTrajectoryFilename +
                                    QString::number(serial_number) + ".csv";
    }
}

// The checkAllShipsReachedDestination function checks if
// all ships have reached their destinations.
// If a ship has not reached its destination, it returns false;
// otherwise, it returns true.
bool Simulator::checkAllShipsReachedDestination() {
    for (std::shared_ptr<Ship>& s : (mShips)) {
        if (s->isOutOfEnergy())
        {
            continue;
        }
        if (! s->isReachedDestination()) {
            return false;
        }
    }
    return true;
}

void Simulator::initializeAllShips()
{
    for (auto& ship: mShips)
    {
        // find the shortest path for the ship
        ShortestPathResult r =
            mNetwork->dijkstraShortestPath(ship->startPoint(),
                                           ship->endPoint());
        ship->setPath(r.points, r.lines);

    }
}


void Simulator::runSimulation()
{
    // define trajectory file and set it up
    if (this->mExportTrajectory) {
        this->mTrajectoryFile.initCSV(mTrajectoryFilename);
        QString exportLine;
        QTextStream stream(&exportLine);
        stream << "ShipNo,TStep_s,TravelledDistance_m,Acceleration_mps2,"
               << "Speed_mps,LinkMaxSpeed_mps,"
               << "EnergyConsumption_KWH,DelayTimeToEach_s,DelayTime_s,"
               << "Stoppings,tractiveForce_N,"
               << "ResistanceForces_N,CurrentUsedTractivePower_kw,";

        this->mTrajectoryFile.writeLine(exportLine);
    }

    std::time_t init_time = std::chrono::system_clock::to_time_t(
        std::chrono::system_clock::now());

    while (this->mSimulationTime <= this->mSimulationEndTime ||
           this->mRunSimulationEndlessly)
    {
        mutex.lock();
            // This will block the thread if pauseFlag is true
        if (mPauseFlag) pauseCond.wait(&mutex);
        mutex.unlock();

        if (this->checkAllShipsReachedDestination()) {
            break;
        }
        for (std::shared_ptr<Ship>& s : (this->mShips)) {
            if (s->isReachedDestination()) { continue;  }

            this->playShipOneTimeStep(s);
        }

        if (mPlotFrequency > 0.0 &&
            ((int(this->mSimulationTime) * 10) % (mPlotFrequency * 10)) == 0)
        {
            QVector<std::pair<QString, Point>> shipsLocs;

            for (std::shared_ptr <Ship>& s : (this->mShips)) {
                if (! s->isLoaded()) { continue; }

                auto data = std::make_pair(s->getUserID(),
                                           s->getCurrentPosition());
                shipsLocs.push_back(data);
            }
            emit this->plotShipsUpdated(shipsLocs);
        }
        // ##################################################################
        // #             start: show progress on console                    #
        // ##################################################################


        this->ProgressBar(mShips);

        this->mSimulationTime += this->mTimeStep;

    }
    // ##################################################################
    // #                       start: summary file                      #
    // ##################################################################
    time_t fin_time =
        std::chrono::system_clock::to_time_t(
        std::chrono::system_clock::now());
    double difTime = difftime(fin_time, init_time);
    QString exportLine;
    QTextStream stream(&exportLine);

    stream << "~.~.~.~.~.~.~.~.~.~.~.~.~.~.~.~.~.~.~.~.~.~.~.~.~.~\n"
           << "NeTrainSim SIMULATION SUMMARY\n"
           << "Version: " << ShipNetSim_VERSION << "\n"
           << "Simulation Time: " << Utils::formatDuration(difTime) << " (dd:hh:mm:ss)\n"
           << "~.~.~.~.~.~.~.~.~.~.~.~.~.~.~.~.~.~.~.~.~.~.~.~.~.~\n\n"
           << "+ NETWORK STATISTICS:\n"
           << "  |_ Region Name                                                                \x1D : " << this->mNetwork->getRegionName() << "\n"
           << "  |_ Total Number of Ships on Network                                           \x1D : " << Utils::thousandSeparator(this->mShips.size()) << "\n"
           << "....................................................\n\n"
           << "\n";


    // setup the summary file
    this->mSummaryFile.writeFile(exportLine.replace("\x1D", ""));
    // ##################################################################
    // #                       end: summary file                      #
    // ##################################################################

    QVector<std::pair<QString, QString>> shipsSummaryData;
    shipsSummaryData = Utils::splitStringStream(exportLine, "\x1D :");

    QString trajectoryFilePath = "";
    if (this->mExportTrajectory) {
        trajectoryFilePath =
            QDir(mOutputLocation).filePath(this->mTrajectoryFilename);
    }

    emit this->finishedSimulation(shipsSummaryData, trajectoryFilePath);

}


// This function simulates one time step for a given train
// in the simulation environment
void Simulator::playShipOneTimeStep(std::shared_ptr<Ship> ship)
{
    // Indicator to skip loading the train
    bool skipShipMove = false;

    // Check if the train start time is passed
    // if such, load train first and then run the simulation
    if (this->mSimulationTime >= ship->getStartTime() &&
        !ship->isLoaded())
    {
        bool skipLoadingship = false;

        // check if there is a train that is already loaded in
        // the same node and still on the same starting node
        for (std::shared_ptr<Ship>& otherShip : this->mShips)
        {
            // check if the train is loaded and not reached destination
            if (otherShip == ship ||
                !otherShip->isLoaded() ||
                otherShip->isReachedDestination()) { continue; }

            // check if the train has the same starting node and
            // the otherTrain still on the same starting node
            if (otherShip->getShipPathPoints()->at(0) ==
                ship->getShipPathPoints()->at(0) &&
                otherShip->getTraveledDistance() <=
                    otherShip->getLengthInWaterline())
            {
                skipLoadingship = true;
                break;
            }
        }
        // skip loading the train if there is any train at the same point
        if (!skipLoadingship)
        {
            // load train
            ship->load();
        }
    }

    // Continue if the train is loaded and its start time
    // is past the current simulation time
    if ((ship->getStartTime() <= this->mSimulationTime) &&
        ship->isLoaded())
    {

        QPair<qsizetype, std::shared_ptr<Point>> stopPoint =
            ship->getNextStoppingPoint();

        auto rest =
            ship->distanceFromCurrentPositionToNodePathIndex(stopPoint.first);

        auto currentMaxSpeed = ship->getCurrentMaxSpeed();

        auto aheadLowerSpeeds = ship->getAheadLowerSpeeds(stopPoint.first);

        criticalPoints cp;

        // add all lower speed points to their corresponding lists
        for (auto index : aheadLowerSpeeds.keys())
        {
            cp.gapToCriticalPoint.push_back(
                ship->distanceFromCurrentPositionToNodePathIndex(index));
            cp.speedAtCriticalPoint.push_back(aheadLowerSpeeds[index]);
            cp.isFollowingAnotherShip.push_back(false);
        }

        // add the stopping station to the list
        cp.gapToCriticalPoint.push_back(
            ship->distanceFromCurrentPositionToNodePathIndex(stopPoint.first));
        cp.speedAtCriticalPoint.push_back(
            units::velocity::meters_per_second_t(0.0));
        cp.isFollowingAnotherShip.push_back(false);


        // check if the ship stopped a little earlier than it should,
        // if yes, give it a kick forward
        if ((cp.gapToCriticalPoint.size() == 1) &&
            (ship->getAcceleration().value()< 0.0) &&
            ((std::round(ship->getPreviousSpeed().value() * 1000.0) /
              1000.0) == 0.0) &&
            ((std::round(ship->getSpeed().value() * 1000.0) / 1000.0) == 0.0))
        {
            ship->kickForwardADistance(cp.gapToCriticalPoint.back(),
                                       mTimeStep);
        }

        // if the ship should move in this time step,
        // move it forward by ship dynamics
        if (!skipShipMove)
        {
            auto maxSpeed = ship->getCurrentMaxSpeed();

            ship->moveShip(mTimeStep, maxSpeed,
                           cp.gapToCriticalPoint,
                           cp.isFollowingAnotherShip,
                           cp.speedAtCriticalPoint);
        }


        if (mExportTrajectory)
        {
            QString s;
            QTextStream stream(&s);
            stream << mSimulationTime.value() << ","
                   << ship->getUserID() << ","
                   << ship->getSpeed().value() << ","
                   << ship->getAcceleration().value() << ","
                   << ship->getCurrentMaxSpeed().value() << ","
                   << ship->getConsumedEnergy().value();
            mTrajectoryFile.writeLine(s);
        }

    }

    // Minimize waiting when no ships are on network
    if (this->checkNoShipIsOnNetwork())
    {
        // get the next ships to enter the network time
        auto shiftTime = this->getNotLoadedShipsMinStartTime();
        if (shiftTime > this->mSimulationTime)
        {
            this->mSimulationTime = shiftTime;
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
                            int bar_length)
{

    double sum = 0.0;
    for(const auto& ship : ships) {
        sum += ship->progress();
    }

    double fraction = sum / ships.size();
    int progressValue = static_cast<int>(fraction * bar_length);
    int progressPercent = static_cast<int>(fraction * 100);

    QTextStream out(stdout);
    QString bar = QString(progressValue, '-').append('>').
                  append(QString(bar_length - progressValue, ' '));

    QChar ending = (progressPercent = 100) ? '\n' : '\r';

    out << "Progress: [" << bar << "] " << progressPercent << "%" << ending;

    if (progressPercent != this->mProgress) {
        this->mProgress = progressPercent;
        emit this->progressUpdated(this->mProgress);
    }
}

void Simulator::pauseSimulation() {
    mutex.lock();
    mPauseFlag = true;
    mutex.unlock();
}

void Simulator::resumeSimulation() {
    mutex.lock();
    mPauseFlag = false;
    mutex.unlock();
    pauseCond.wakeAll(); // This will wake up the thread
}

