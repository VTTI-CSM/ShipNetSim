#include "simulator.h"
#include "ship/hydrology.h"
#include "ship/ishipcalmresistancestrategy.h"
#include "utils/utils.h"
#include "ship/ship.h"
#include "VersionConfig.h"

namespace ShipNetSimCore
{

Simulator::Simulator(std::shared_ptr<OptimizedNetwork> network,
                     QVector<std::shared_ptr<Ship>> shipList,
                     units::time::second_t simulatorTimeStep,
                     QObject *parent)
    : QObject{parent},
    mShips(shipList),
    mSimulationTime(units::time::second_t(0.0)),
    mSimulationEndTime(DefaultEndTime),
    mTimeStep(simulatorTimeStep),

    mNetwork(network)
{
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

}

void Simulator::studyShipsResistance()
{
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

    this->mTrajectoryFile.writeLine(exportLine);

    for (auto &ship: mShips)
    {
        double s[] = {15.0, 15.5, 16.0, 16.5, 17.0, 17.5, 18.0, 18.5, 19.0};

        for (double speedStep : s)  //Utils::linspace_step(0, ship->getMaxSpeed().value())
        {
            units::velocity::meters_per_second_t ss = units::velocity::knot_t(speedStep).convert<units::velocity::meters_per_second>();
            ship->setSpeed(ss);
            // ship->setSpeed(units::velocity::meters_per_second_t(speedStep));
            auto speed = ss; //units::velocity::knot_t(speedStep).
                         //convert<units::velocity::meters_per_second>();
            auto fraudNo =
                hydrology::F_n(speed, ship->getLengthInWaterline());

            auto airRes =
                ship->getCalmResistanceStrategy()->getAirResistance(*ship);
            auto bulbousRes =
                ship->getCalmResistanceStrategy()->getBulbousBowResistance(*ship);
            auto transomRes =
                ship->getCalmResistanceStrategy()->
                              getImmersedTransomPressureResistance(*ship);
            auto appendageRes =
                ship->getCalmResistanceStrategy()->getAppendageResistance(*ship);
            auto waveRes =
                ship->getCalmResistanceStrategy()->getWaveResistance(*ship);
            auto frictionalRes =
                ship->getCalmResistanceStrategy()->getfrictionalResistance(*ship);
            auto modelCorRes =
                ship->getCalmResistanceStrategy()->
                               getModelShipCorrelationResistance(*ship);
            auto totalRes =
                ship->calculateTotalResistance();

            auto totalResCoef =
                ship->getCalmResistanceStrategy()->
                                getCoefficientOfResistance(*ship);
            
            auto FRI = ship->getCalmResistanceStrategy()->calc_F_n_i(*ship);

            QString line;
            QTextStream lineStream(&line);
            lineStream << ship->getUserID() << ","
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


            mTrajectoryFile.writeLine(line);
        }

    }
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

// The checkAllShipsReachedDestination function checks if
// all ships have reached their destinations.
// If a ship has not reached its destination, it returns false;
// otherwise, it returns true.
bool Simulator::checkAllShipsReachedDestination()
{
    for (std::shared_ptr<Ship>& s : (mShips))
    {
        if (s->isOutOfEnergy())
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


void Simulator::runSimulation()
{
    QString trajectoryFullPath = (mExportTrajectory)?
                                     QDir(mOutputLocation).filePath(
                                     mTrajectoryFilename) : "";

    // define trajectory file and set it up
    if (this->mExportTrajectory)
    {
        this->mTrajectoryFile.initCSV(trajectoryFullPath);
        QString exportLine;
        QTextStream stream(&exportLine);
        stream << "TStep_s,"
                  "ShipNo,"
                  "WaterSalinity_ppt,"
                  "WaveHeight_m,"
                  "WaveFrequency_hz,"
                  "WaveLength_m,"
                  "NorthwardWindSpeed_mps,"
                  "EastwardWindSpeed_mps,"
                  "TotalShipThrust_N,"
                  "TotalShipResistance_N,"
                  "maxAcceleration_mps2,"
                  "TravelledDistance_m,"
                  "Acceleration_mps2,"
                  "Speed_knots,"
                  "CumEnergyConsumption_KWH,"
                  "MainEnergySourceCapacityState_percent,"
                  "Position(id;long;lat),"
                  "Course_deg";

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

                Point nP = s->getCurrentPosition().projectTo(
                    Point::getDefaultProjectionReference().get());
                auto data = std::make_pair(s->getUserID(),
                                           nP);
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
        << "+ AGGREGATED/ACCUMULATED TRAINS STATISTICS:\n"
        << "    |-> Moved Commodity:\n"
        << "        |_ Total Moved Cargo (ton)                                              \x1D : " << Utils::thousandSeparator(std::accumulate(this->mShips.begin(), this->mShips.end(), 0.0,
                                                                                                                                                 [](double total, const auto& s) {
                                                                                                                                                     return total + (s->getCargoWeight().value());
                                                                                                                                                 })) << "\n"
        << "        |_ Total ton.km (ton.Km)                                                \x1D : " << Utils::thousandSeparator(std::accumulate(this->mShips.begin(), this->mShips.end(), 0.0,
                                                                                                                                                 [](double total, const auto& s) {
                                                                                                                                                     return total + (s->getTotalCargoTonKm().value());
                                                                                                                                                 })) << "\n\n"
        << "  |-> Route Information:\n"
        << "    |_ Ships Reached Destination                                                \x1D : " << std::accumulate(this->mShips.begin(), this->mShips.end(), 0.0,
                                                                                                                        [](int total, const auto& s) {
                                                                                                                            return total + s->isReachedDestination();
                                                                                                                        }) << "\n"
        << "    |_ Ships Total Path Length (km)                                             \x1D : " << Utils::thousandSeparator(std::accumulate(this->mShips.begin(), this->mShips.end(), 0.0,
                                                                                                                                                 [](double total, const auto& s) {
                                                                                                                                                     return total + (s->getTotalPathLength().value());
                                                                                                                                                 })) << "\n\n"
        << "  |-> Ships Performance:\n"
        << "    |_ Operating Time (d:h::m::s)                                               \x1D : " << Utils::formatDuration(std::accumulate(this->mShips.begin(), this->mShips.end(), 0.0,
                                                                                                                                              [](double total, const auto& s) {
                                                                                                                                                  return total + s->getTripTime().value();
                                                                                                                                              })) << "\n"
        << "    |_ Average Speed (meter/second)                                             \x1D : " << std::accumulate(this->mShips.begin(), this->mShips.end(), 0.0,
                                                                                                                        [](double total, const auto& s) {
                                                                                                                            return total + s->getTripRunningAvergageSpeed().value();
                                                                                                                        })/this->mShips.size() << "\n"
        << "    |_ Average Acceleration (meter/square second)                               \x1D : " << Utils::thousandSeparator(std::accumulate(this->mShips.begin(), this->mShips.end(), 0.0,
                                                                                                                                                 [](double total, const auto& s) {
                                                                                                                                                     return total + s->getTripRunningAverageAcceleration().value();
                                                                                                                                                 })/this->mShips.size(), 4) << "\n"
        << "    |_ Average Travelled Distance (km)                                          \x1D : " << Utils::thousandSeparator(std::accumulate(this->mShips.begin(), this->mShips.end(), 0.0,
                                                                                                                                                 [](double total, const auto& s) {
                                                                                                                                                     return total + (s->getTraveledDistance().convert<units::length::kilometers>().value());
                                                                                                                                                 })/this->mShips.size()) << "\n"
        << "    |_ Consumed and Regenerated Energy:\n"
        << "        |_ Total Net Energy Consumed (KW.h)                                     \x1D : " << Utils::thousandSeparator(std::accumulate(this->mShips.begin(), this->mShips.end(), 0.0,
                                                                                                                                                 [](double total, const auto& s) {
                                                                                                                                                     return total + s->getCumConsumedEnergy().value();
                                                                                                                                                 })) << "\n"
        << "            |_ Total Energy Consumed (KW.h)                                     \x1D : " << Utils::thousandSeparator(std::accumulate(this->mShips.begin(), this->mShips.end(), 0.0,
                                                                                                                                                 [](double total, const auto& s) {
                                                                                                                                                     return total + s->getCumConsumedEnergy().value();
                                                                                                                                                 })) << "\n"
        << "            |_ Total Energy Regenerated (KW.h)                                  \x1D : " << Utils::thousandSeparator(0.0) << "\n"
        << "            |_ Average Net Energy Consumption per Net Weight (KW.h/ton)         \x1D : " << Utils::thousandSeparator(std::accumulate(this->mShips.begin(), this->mShips.end(), 0.0,
                                                                                                                                                 [](double total, const auto& s) {
                                                                                                                                                     return total + (s->getEnergyConsumptionPerTon().value());
                                                                                                                                                 })) << "\n"
        << "            |_ Average Net Energy Consumption per Net ton.km (KW.hx10^3/ton.km) \x1D : " << Utils::thousandSeparator(std::accumulate(this->mShips.begin(), this->mShips.end(), 0.0,
                                                                                                                                                 [](double total, const auto& s) {
                                                                                                                                                     return total +
                                                                                                                                                            (s->getEnergyConsumptionPerTonKM().value() * (double) 1000.0);
                                                                                                                                                 })) << "\n"
        << "        |_ Tank Consumption:\n";

    std::map<ShipFuel::FuelType, units::volume::liter_t> totalFuelConsumed;
    for (auto& s: mShips) {
        for (auto& fc: s->getCumConsumedFuel() ) {
            totalFuelConsumed[fc.first] += fc.second;
        }
    }

    stream
        << "            |_ Total Overall Fuel Consumed (litters)                            \x1D : " << Utils::thousandSeparator(std::accumulate(this->mShips.begin(), this->mShips.end(), 0.0,
                                                                                                                                                 [](double total, const auto& s) {
                                                                                                                                                     return total + s->getOverallCumFuelConsumption().value();
                                                                                                                                                 })) << "\n";

    for (auto& fc: totalFuelConsumed) {
    stream
            << Utils::formatString(
                   "                |_ ",
                    ShipFuel::convertFuelTypeToString(fc.first) + " (litters) ",
                   "\x1D : ", " ", 84) << Utils::thousandSeparator(fc.second.value()) << "\n";
    }

    stream
        << "            |_ Average Fuel Consumed per Net Weight (litterx10^3/ton)           \x1D : " << Utils::thousandSeparator(std::accumulate(this->mShips.begin(), this->mShips.end(), 0.0,
                                                                                                                                                 [](double total, const auto& train) {
                                                                                                                                                     return total + train->getOverallCumFuelConsumptionPerTon().value();
                                                                                                                                                 })) << "\n"
        << "            |_ Average Fuel Consumed per Net ton.km (littersx10^3/ton.km)       \x1D : " << Utils::thousandSeparator(std::accumulate(this->mShips.begin(), this->mShips.end(), 0.0,
                                                                                                                                                 [](double total, const auto& train) {
                                                                                                                                                     return total + train->getOverallCumFuelConsumptionPerTonKM().value();
                                                                                                                                                 })) << "\n"
        << "        |_ Tank Status:\n"
        << "....................................................\n\n";

    for (auto &ship: mShips)
    {
        stream
           << "Ship ID: " << ship->getUserID() << "\n"
           << "SHIP GENERAL INFORMATION\n"
           << "Average Max Propeller Effeciency                                               \x1D : " << ship->getPropellers()->front()->getDrivingEngines()[0]->getEngineMaxPowerRatio() << "\n"
            << "Total Energy Consumed                                                          \x1D : " << ship->getCumConsumedEnergy().value() << "\n";
    }

    // Set up the summary file information
    QString summaryFullPath =
        QDir(mOutputLocation).filePath(mSummaryFileName);
    this->mSummaryFile.initTXT(summaryFullPath);
    // setup the summary file
    this->mSummaryFile.writeFile(exportLine.replace("\x1D", ""));
    // ##################################################################
    // #                       end: summary file                      #
    // ##################################################################

    QVector<std::pair<QString, QString>> shipsSummaryData;
    shipsSummaryData = Utils::splitStringStream(exportLine, "\x1D :");

    // fire the signal of finished simulation
    emit this->finishedSimulation(shipsSummaryData, trajectoryFullPath);

    // make sure the files are closed!
    mTrajectoryFile.close();
    mSummaryFile.close();
}


// This function simulates one time step for a given ship
// in the simulation environment
void Simulator::playShipOneTimeStep(std::shared_ptr<Ship> ship)
{
    // Indicator to skip loading the ship
    bool skipShipMove = false;

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
            auto currentMaxSpeed =
                units::velocity::meters_per_second_t(100.0); //ship->getCurrentMaxSpeed();

            ship->moveShip(mTimeStep, currentMaxSpeed,
                           cp.gapToCriticalPoint,
                           cp.isFollowingAnotherShip,
                           cp.speedAtCriticalPoint,
                            currentEnvironment);
        }

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
                   << ship->getCurrentPosition().toString() << ","
                   << QString::number(ship->getCurrentHeading().value(), 'f', 3);
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

    QChar ending = (progressPercent >= 100) ? '\n' : '\r';

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

}
