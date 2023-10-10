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
        DefaultInstantaneousTrajectoryFilename = "trainTrajectory_";
    /** (Immutable) the default summary filename */
    inline static const QString
        DefaultSummaryFilename =  "trainSummary_";

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

    /** The trains */
    QVector<std::shared_ptr<Ship>> mShips;
    /** The simulation time */
    units::time::second_t mSimulationTime;
    /** The simulation end time */
    units::time::second_t mSimulationEndTime;
    /** The time step */
    units::time::second_t mTimeStep;
    /** The frequency of plotting the trains */
    int mPlotFrequency;
    /** The output location */
    QString mOutputLocation;
    /** Filename of the summary file */
    QString mSummaryFileName;
    /** Filename of the trajectory file */
    QString mTrajectoryFilename;
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
    explicit Simulator(std::shared_ptr<Network> network,
                       QVector<std::shared_ptr<Ship>> shipList,
                       units::time::second_t simulatorTimeStep =
                       DefaultTimeStep,
                       QObject *parent = nullptr);


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
     *                      Zero means do not stop untill all trains
     *                      reach destination.
     */
    void setEndTime(units::time::second_t newEndTime);

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
    void setOutputFolderLocation(QString newOutputFolderLocation);

    /**
     * @brief set summary filename.
     * @param newfilename           the new file name of the summary file.
     */
    void setSummaryFilename(QString newfilename =
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
        QString newInstaTrajectFilename =
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
        QVector<std::pair<QString, Point>> shipsPoints);

    /**
     * @brief Signals that the simulation has finished.
     *
     * @param summaryData   A vector containing the summary
     *                      data of the simulation.
     * @param trajectoryFile The file path of the generated
     *                          trajectory file.
     */
    void finishedSimulation(
        const QVector<std::pair<QString,
                                QString>>& summaryData,
        const QString& trajectoryFile);

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
    bool mPauseFlag;

};

#endif // SIMULATOR_H
