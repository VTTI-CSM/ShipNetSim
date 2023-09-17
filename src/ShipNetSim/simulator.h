#ifndef SIMULATOR_H
#define SIMULATOR_H

#include "qmutex.h"
#include "qwaitcondition.h"
#include "network/network.h"
#include <string>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <QObject>
#include <QDir>
#include "utils/data.h"

class Simulator : public QObject
{
    Q_OBJECT
private:
    /** (Immutable) the default time step */
    static constexpr double DefaultTimeStep = 1.0;
    /** (Immutable) the default end time */
    static constexpr double DefaultEndTime = 0.0;
    /** (Immutable) true to default export instantaneous trajectory */
    static constexpr bool
        DefaultExportInstantaneousTrajectory = true;
    /** (Immutable) the default instantaneous trajectory empty filename */
    inline static const std::string
        DefaultInstantaneousTrajectoryEmptyFilename = "";
    /** (Immutable) the default summary empty filename */
    inline static const std::string DefaultSummaryEmptyFilename = "";
    /** (Immutable) the default instantaneous trajectory filename */
    inline static const std::string
        DefaultInstantaneousTrajectoryFilename = "trainTrajectory_";
    /** (Immutable) the default summary filename */
    inline static const std::string
        DefaultSummaryFilename =  "trainSummary_";


    /** The trains */
    std::vector<std::shared_ptr<Ship>> mShips;
    /** The simulation time */
    double mSimulationTime;
    /** The simulation end time */
    double mSimulationEndTime;
    /** The time step */
    double mTimeStep;
    /** The frequency of plotting the trains */
    int mPlotFrequency;
    /** The output location */
    QString mOutputLocation;
    /** Filename of the summary file */
    std::string mSummaryFileName;
    /** Filename of the trajectory file */
    std::string mTrajectoryFilename;
    /** The network */
    std::shared_ptr<Network> mNetwork;
    /** The progress */
    double mProgress;
    /** True to run simulation endlessly */
    bool mRunSimulationEndlessly;
    /** True to export trajectory */
    bool mExportTrajectory;
    /** The trajectory file */
    Data::CSV mTrajectoryFile = Data::CSV();
    /** The summary file */
    Data::TXT mSummaryFile = Data::TXT();
    /** export individualized trains summary in the summary file*/
    bool mExportIndividualizedTrainsSummary = false;

    /**
     * Determines if we can check all ships reached destination
     *
     * @author	Ahmed Aredah
     * @date	2/28/2023
     *
     * @returns	True if it succeeds, false if it fails.
     */
    bool checkAllShipsReachedDestination();

    /**
     * Play ship one time step
     *
     * @author	Ahmed Aredah
     * @date	2/28/2023
     *
     * @param 	train	The train.
     */
    void playShipOneTimeStep(std::shared_ptr <Ship> ship);

    /**
     * Loads a ship
     *
     * @author	Ahmed Aredah
     * @date	2/28/2023
     *
     * @param 	train	The train.
     */
    void loadShip(std::shared_ptr<Ship> ship);


    /**
     * Progress bar
     *
     * @author	Ahmed Aredah
     * @date	2/28/2023
     *
     * @param 	current   	The current.
     * @param 	total	  	Number of.
     * @param 	bar_length	(Optional) Length of the bar.
     */
    void ProgressBar(double current, double total, int bar_length = 100);

public:
    explicit Simulator(std::shared_ptr<Network> network,
                       std::vector<std::shared_ptr<Ship>> shipList,
                       double simulatorTimeStep = DefaultTimeStep,
                       QObject *parent = nullptr);


    /**
     * @brief set simulator time step
     *
     * @author	Ahmed Aredah
     * @date	2/28/2023
     *
     * @param newTimeStep
     */
    void setTimeStep(double newTimeStep);

    /**
     * @brief Get the output folder directory.
     *
     * @author	Ahmed Aredah
     * @date	2/28/2023
     *
     * @return the directory.
     */
    std::string getOutputFolder();
    /**
     * @brief set simulator end time.
     *
     * @author	Ahmed Aredah
     * @date	2/28/2023
     *
     * @param newEndTime    the new end time of the simulator in seconds.
     *                      Zero means do not stop untill all trains
     *                      reach destination.
     */
    void setEndTime(double newEndTime);

    /**
     * @brief set the plot frequency of trains, this only works in the gui
     * @param newPlotFrequency
     */
    void setPlotFrequency(int newPlotFrequency);

    /**
     * @brief setOutputFolderLocation.
     *
     * @author	Ahmed Aredah
     * @date	2/28/2023
     *
     * @param newOutputFolderLocation   the new output folder location
     *                                  on the disk.
     */
    void setOutputFolderLocation(std::string newOutputFolderLocation);

    /**
     * @brief set summary filename.
     * @param newfilename           the new file name of the summary file.
     */
    void setSummaryFilename(std::string newfilename =
                            DefaultSummaryEmptyFilename);

    /**
     * @brief setExportInstantaneousTrajectory
     *
     * @author	Ahmed Aredah
     * @date	2/28/2023
     *
     * @param exportInstaTraject
     * @param newInstaTrajectFilename
     */
    void setExportInstantaneousTrajectory(
        bool exportInstaTraject,
        std::string newInstaTrajectFilename =
        DefaultInstantaneousTrajectoryEmptyFilename);



signals:
    /**
     * @brief Updates the progress of the simulation.
     *
     * @param progressPercentage The progress of the
     *                              simulation as a percentage.
     */
    void progressUpdated(int progressPercentage);

    /**
     * @brief Updates the plot of trains with their start and end points.
     *
     * @param trainsStartEndPoints A vector containing the names of the trains
     *                            along with their start and end points.
     */
    void plotShipsUpdated(
        std::vector<std::pair<std::string,
                         std::vector<std::pair<double,
                                          double>>>> trainsStartEndPoints);

    /**
     * @brief Signals that the simulation has finished.
     *
     * @param summaryData   A vector containing the summary
     *                      data of the simulation.
     * @param trajectoryFile The file path of the generated
     *                          trajectory file.
     */
    void finishedSimulation(
        const std::vector<std::pair<std::string,
                               std::string>>& summaryData,
        const std::string& trajectoryFile);

public slots:
    /**
     * Executes the 'simulator' operation
     *
     * @author	Ahmed Aredah
     * @date	2/28/2023
     */
    void runSimulation();

    /**
     * @brief pause the simulation
     */
    void pauseSimulation();

    /**
     * @brief resume the simulation
     */
    void resumeSimulation();

private:
    QMutex mutex;
    QWaitCondition pauseCond;
    bool pauseFlag;

};

#endif // SIMULATOR_H
