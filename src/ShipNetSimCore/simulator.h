#ifndef SIMULATOR_H
#define SIMULATOR_H

#include "export.h"
#include "qmutex.h"
#include "qwaitcondition.h"
#include "network/optimizednetwork.h"
// #include <string>
// #include <iostream>
// #include <filesystem>
// #include <fstream>
#include <QObject>
#include <QDir>
#include "utils/data.h"
#include "utils/shipscommon.h"

namespace ShipNetSimCore
{

class SHIPNETSIM_EXPORT Simulator : public QObject
{
    Q_OBJECT
private:
    /** (Immutable) the default time step */
    static constexpr units::time::second_t DefaultTimeStep =
        units::time::second_t(1.0);
    /** (Immutable) the default end time */
    static constexpr units::time::second_t DefaultEndTime =
        units::time::second_t(0.0);
    /** (Immutable) true to default export instantaneous trajectory */
    static constexpr bool
        DefaultExportInstantaneousTrajectory = true;
    /** (Immutable) the default instantaneous trajectory empty filename */
    inline static const QString
        DefaultInstantaneousTrajectoryEmptyFilename = "";
    /** (Immutable) the default summary empty filename */
    inline static const QString DefaultSummaryEmptyFilename = "";
    /** (Immutable) the default instantaneous trajectory filename */
    inline static const QString
        DefaultInstantaneousTrajectoryFilename = "shipTrajectory_";
    /** (Immutable) the default summary filename */
    inline static const QString
        DefaultSummaryFilename =  "shipSummary_";

    struct shipLinksSpeedResults
    {
        QVector<units::velocity::meters_per_second_t> freeFlowSpeeds;
        QVector<std::shared_ptr<Line>> pathLines;
    };

    struct criticalPoints
    {
        QVector<units::length::meter_t> gapToCriticalPoint;
        QVector<units::velocity::meters_per_second_t> speedAtCriticalPoint;
        QVector<bool> isFollowingAnotherShip;
    };

    /** The ships */
    QVector<std::shared_ptr<Ship>> mShips;
    /** The simulation time */
    units::time::second_t mSimulationTime;
    /** The simulation end time */
    units::time::second_t mSimulationEndTime;
    /** The time step */
    units::time::second_t mTimeStep;
    /** The network */
    OptimizedNetwork* mNetwork;
    /** Allow the simulator to be externally controlled */
    bool mIsExternallyControlled;
    /** The frequency of plotting the ships */
    int mPlotFrequency;
    /** The output location */
    QString mOutputLocation;
    /** Filename of the summary file */
    QString mSummaryFileName;
    /** Filename of the trajectory file */
    QString mTrajectoryFilename;
    /** Full path of the trajectory file*/
    QString mTrajectoryFullPath;
    /** The progress */
    int mProgress = -1;
    /** True to run simulation endlessly */
    bool mRunSimulationEndlessly;
    /** True to export trajectory */
    bool mExportTrajectory;
    /** The trajectory file */
    Data::CSV mTrajectoryFile = Data::CSV();
    /** The summary file */
    Data::TXT mSummaryFile = Data::TXT();
    /** export individualized ships summary in the summary file*/
    bool mExportIndividualizedTrainsSummary = false;
    /** The serial number of the current simulation run. */
    long long simulation_serial_number;
    /** Time of when the simulation started. */
    std::time_t mInitTime;

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
     * @param 	ship	The ship.
     */
    void playShipOneTimeStep(std::shared_ptr <Ship> ship);

    /**
     * Progress bar
     *
     * @author	Ahmed Aredah
     * @date	2/28/2023
     *
     * @param 	ships   	All ships in the simulator..
     * @param 	bar_length	(Optional) Length of the bar.
     */
    void ProgressBar(QVector<std::shared_ptr<Ship>> ships,
                     int bar_length = 100);

    void initializeAllShips();

    /**
     * @brief checkNoShipIsOnNetwork
     * @return
     */
    bool checkNoShipIsOnNetwork();

    /**
     * @brief getNotLoadedShipsMinStartTime
     * @return
     */
    units::time::second_t getNotLoadedShipsMinStartTime();

public:
    explicit Simulator(OptimizedNetwork* network,
                       QVector<std::shared_ptr<Ship>> shipList,
                       units::time::second_t simulatorTimeStep =
                       DefaultTimeStep,
                       bool isExternallyControlled = false,
                       QObject *parent = nullptr);


    ~Simulator();

    void moveObjectToThread(QThread* thread);

    /**
     * @brief Study the ships resistance.
     *
     * This function does not run the simulation,
     * it only increments the ship speed and measures the resistance.
     */
    void studyShipsResistance();

    /**
     * Play all ships one time step and advance the simulator clock
     *
     * @author	Ahmed Aredah
     * @date	2/28/2023
     *
     * @param 	ship	The ship.
     */
    void playShipsOneTimeStep();

    /**
     * @brief add a ship to the simulator
     * @param ship the new ship pointer to be simulated
     */
    void addShipToSimulation(std::shared_ptr<Ship> ship);

    /**
     * @brief add ships to the simulator
     * @param ships the new ships' pointers to be simulated
     */
    void addShipsToSimulation(QVector<std::shared_ptr<Ship>> ships);

    /**
     * @brief set simulator time step
     *
     * @author	Ahmed Aredah
     * @date	2/28/2023
     *
     * @param newTimeStep
     */
    void setTimeStep(units::time::second_t newTimeStep);

    /**
     * @brief Gets the time step of the simulator.
     * @return time step in seconds.
     */
    units::time::second_t getSimulatorTimeStep();

    /**
     * @brief Gets the current simulator time in seconds.
     * @return simulator time (sec)
     */
    units::time::second_t getCurrentSimulatorTime();

    /**
     * @brief Get the output folder directory.
     *
     * @author	Ahmed Aredah
     * @date	2/28/2023
     *
     * @return the directory.
     */
    QString getOutputFolder();
    /**
     * @brief set simulator end time.
     *
     * @author	Ahmed Aredah
     * @date	2/28/2023
     *
     * @param newEndTime    the new end time of the simulator in seconds.
     *                      Zero means do not stop untill all ships
     *                      reach destination.
     */
    void setEndTime(units::time::second_t newEndTime);

    /**
     * @brief set the plot frequency of ships, this only works in the gui
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
    void setOutputFolderLocation(QString newOutputFolderLocation);

    /**
     * @brief set summary filename.
     * @param newfilename           the new file name of the summary file.
     */
    void setSummaryFilename(QString newfilename =
                            DefaultSummaryEmptyFilename);

    /**
     * @brief set export instantaneous ship trajectory for all ships in the
     * simulator at every simulation time step.
     *
     * @author	Ahmed Aredah
     * @date	2/28/2023
     *
     * @param exportInstaTraject        a boolean to enable or disable
     *                                  the export trajectory
     * @param newInstaTrajectFilename   a path to the file in case the
     *                                  exportInstaTraject is set to true.
     */
    void setExportInstantaneousTrajectory(
        bool exportInstaTraject,
        QString newInstaTrajectFilename =
        DefaultInstantaneousTrajectoryEmptyFilename);


    /**
     * @brief set the export individualized ships summary for all ships
     * in the simulator.
     *
     *@param exportAllTrainsSummary a boolean to enable or disable exporting
     *individualized ships summary.
     */
    void setExportIndividualizedShipsSummary(bool exportAllTrainsSummary);

    QJsonObject getCurrentStateAsJson();

signals:
    /**
     * @brief Updates the progress of the simulation.
     *
     * @param progressPercentage The progress of the
     *                              simulation as a percentage.
     */
    void progressUpdated(int progressPercentage);

    /**
     * @brief Updates the plot of ships with their start and end points.
     *
     * @param shipsStartEndPoints A vector containing the names of the ships
     *                            along with their start and end points.
     */
    void plotShipsUpdated(
        QVector<std::pair<QString, GPoint>> shipsPoints);

    /**
     * @brief Signals that a new simulation results is available.
     *
     * @param results   The simulation results struct
     */
    void simulationResultsAvailable(
        ShipsResults& results);

    /**
     * @brief Signals that all ships in the simulator reached their destination.
     */
    void allShipsReachedDestination();

    /**
     * @brief Signals that the simulation has finished.
     */
    void simulationFinished();

public slots:

    /**
     * @brief initialize the simulator.
     */
    void initSimulation();

    /**
     * Executes the 'simulator' operation
     *
     * @author	Ahmed Aredah
     * @date	2/28/2023
     */
    void runSimulation();

    /**
     * @brief Close the simulator, open streams, and write summary file.
     */
    void exportSimulationResults(bool writeSummaryFile = true);

    /**
     * @brief pause the simulation
     */
    void pauseSimulation();

    /**
     * @brief resume the simulation
     */
    void resumeSimulation();

    /**
     * @brief stop the simulation completely
     */
    void stopSimulation();

private:
    QMutex mutex;
    QWaitCondition pauseCond;
    bool mIsSimulatorPaused = false;
    bool mIsSimulatorRunning = true;

};
};
#endif // SIMULATOR_H
