/**
 * @file SimulationWorker.h
 * @brief This file contains the declaration of the SimulationWorker class.
 *        The SimulationWorker class is a QObject subclass that performs simulation work in a separate thread.
 *        It receives input data and parameters, performs the simulation using the Simulator class, and emits signals to update the UI.
 *        The SimulationWorker class is intended to be used for simulation tasks in a multi-threaded application.
 *        It is designed to work with the Simulator class and communicates with the UI through signals and slots.
 *        The SimulationWorker class takes nodeRecords, linkRecords, shipRecords, networkName, endTime, timeStep, plotFrequency,
 *        exportDir, summaryFilename, exportInsta, instaFilename, and exportAllShipsSummary as input data for simulation.
 *        It performs the simulation, updates the progress, coordinates of ships, and emits signals to inform the UI about the progress and results.
 *        The SimulationWorker class can be used in a QWidget-based application.
 * @author Ahmed Aredah
 * @date 6/7/2023
 */

#ifndef SIMULATIONWORKER_H
#define SIMULATIONWORKER_H

#include <QObject>
#include "../ShipNetSim/simulator.h"
#include "../ShipNetSim/network/gpoint.h"

/**
 * @class SimulationWorker
 * @brief The SimulationWorker class performs simulation work in
 * a separate thread.
 */
class SimulationWorker : public QObject {
    Q_OBJECT

public:
    /**
     * @brief Constructs a SimulationWorker object with the specified
     * input data and parameters.
     * @param nodeRecords The node records for the simulation.
     * @param linkRecords The link records for the simulation.
     * @param shipRecords The ship records for the simulation.
     * @param networkName The network name for the simulation.
     * @param endTime The end time for the simulation.
     * @param timeStep The time step for the simulation.
     * @param plotFrequency The plot frequency for the simulation.
     * @param exportDir The export directory for the simulation.
     * @param summaryFilename The summary filename for the simulation.
     * @param exportInsta Indicates whether to export instant data in the
     * simulation.
     * @param instaFilename The instant data filename for the simulation.
     * @param exportAllShipsSummary Indicates whether to export summary
     * data for all ships in the simulation.
     */
    template<typename T>
    SimulationWorker(QString waterBoundariesFile,
                     QVector<QMap<QString, T>> shipsRecords,
                     QString networkName,
                     units::time::second_t endTime,
                     units::time::second_t timeStep, double plotFrequency,
                     QString exportDir,
                     QString summaryFilename, bool exportInsta,
                     QString instaFilename, bool exportAllShipsSummary);

    /**
     * @brief Destroys the SimulationWorker object.
     */
    ~SimulationWorker();

signals:
    /**
     * @brief Signal emitted when the simulation is finished.
     * @param summaryData The summary data of the simulation.
     * @param trajectoryFile The trajectory file path of the simulation.
     */
    void simulationFinished(
        const QVector<std::pair<QString, QString>>& summaryData,
        const QString& trajectoryFile);

    /**
     * @brief Signal emitted when the coordinates of ships are updated.
     * @param shipsStartEndPoints The start and end points of ships'
     * coordinates.
     */
    void shipsCoordinatesUpdated(
        QVector<std::pair<QString,
                          ShipNetSimCore::GPoint>> shipsStartEndPoints);

    /**
     * @brief Signal emitted when the simulation progress is updated.
     * @param progressPercentage The progress percentage of the simulation.
     */
    void simulaionProgressUpdated(int progressPercentage);

    /**
     * @brief Signal emitted when an error occurs during the simulation.
     * @param error The error message.
     */
    void errorOccurred(QString error);

    void shipSuddenAcceleration(QString msg);

    void shipSlowSpeed(QString msg);

    void shipsCollided(QString& msg);

public slots:
    /**
     * @brief Slot called when the progress is updated.
     * @param progressPercentage The progress percentage of the simulation.
     */
    void onProgressUpdated(int progressPercentage);

    /**
     * @brief Slot called when the coordinates of ships are updated.
     * @param shipsStartEndPoints The start and end points of ships'
     * coordinates.
     */
    void onShipsCoordinatesUpdated(
        QVector<std::pair<QString,
                          ShipNetSimCore::GPoint> > shipsStartEndPoints);

    /**
     * @brief Slot called when the simulation is finished.
     * @param summaryData The summary data of the simulation.
     * @param trajectoryFile The trajectory file path of the simulation.
     */
    void onSimulationFinished(
        const QVector<std::pair<QString,
                                QString>> &summaryData,
        const QString &trajectoryFile);

    /**
     * @brief Slot called to start the simulation work.
     */
    void doWork();

public:
    /**< Pointer to the Simulator object for performing the simulation. */
    ShipNetSimCore::Simulator* sim;
    /**< Pointer to the Network object used in the simulation. */
    ShipNetSimCore::OptimizedNetwork* net;
};

#endif // SIMULATIONWORKER_H
