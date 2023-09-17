#include "simulator.h"
#include "network/network.h"
#include "utils/utils.h"

Simulator::Simulator(std::shared_ptr<Network> network,
                     std::vector<std::shared_ptr<Ship>> shipList,
                     double simulatorTimeStep,
                     QObject *parent)
    : mNetwork(network), mShips(shipList),
    mTimeStep(simulatorTimeStep), QObject{parent},
    mSimulationEndTime(DefaultEndTime), mSimulationTime(0.0)
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
                          std::to_string(serial_number) + ".csv";
    mSummaryFileName = DefaultSummaryFilename +
                       std::to_string(serial_number) + ".txt";

    setShipsSimulationPath();

}

// Setter for the time step of the simulation
void Simulator::setTimeStep(double newTimeStep) {
    this->mTimeStep = newTimeStep;
}

// Setter for the end time of the simulation
void Simulator::setEndTime(double newEndTime) {
    this->mSimulationEndTime = newEndTime;
}

// Setter for the plot frequency
void Simulator::setPlotFrequency(int newPlotFrequency) {
    this->mPlotFrequency = newPlotFrequency;
}

// Setter for the output folder location
void Simulator::setOutputFolderLocation(std::string newOutputFolderLocation)
{
    this->mOutputLocation = QString::fromStdString(newOutputFolderLocation);
}
// The getOutputFolder function returns the path to the
// directory where the output files are stored.
std::string Simulator::getOutputFolder() {
    return this->mOutputLocation.toStdString();
}

// Setter for the summary file name
void Simulator::setSummaryFilename(std::string newfilename)
{
    QString filename = QString::fromStdString(newfilename);
    if (!filename.isEmpty()){
        QFileInfo fileInfo(filename);
        // Check if the file name has an extension
        if (!fileInfo.completeSuffix().isEmpty()){
            this->mSummaryFileName = newfilename;
        }
        else{
            this->mSummaryFileName = newfilename + ".txt";
        }
    }
    else {
        this->mSummaryFileName = DefaultSummaryFilename;
    }
}

// Setter for the instantaneous trajectory export flag and filename
void Simulator::setExportInstantaneousTrajectory(
    bool exportInstaTraject,
    std::string newInstaTrajectFilename)
{
    this->mExportTrajectory = exportInstaTraject;
    if (newInstaTrajectFilename != "")
    {
        QString filename =
            QString::fromStdString(newInstaTrajectFilename);

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
                                    std::to_string(serial_number) + ".csv";
    }
}

// The checkAllShipsReachedDestination function checks if
// all ships have reached their destinations.
// If a ship has not reached its destination, it returns false;
// otherwise, it returns true.
bool Simulator::checkAllShipsReachedDestination() {
    for (std::shared_ptr<Ship>& s : (mShips)) {
        if (s->outOfEnergy()) {
            continue;
        }
        if (! s->reachedDestination()) {
            return false;
        }
    }
    return true;
}


// The loadShip function sets up a ship for simulation
// by setting its starting point,
// and connecting it to the network.
void Simulator::loadShip(std::shared_ptr<Ship> ship)
{
    ship->setLoaded(true);
    ship->setLinksCumLengths(mNetwork->generateCumLinesLengths(ship));
}
