#ifndef SIMULATORAPI_H
#define SIMULATORAPI_H

#include "export.h"
#include <QObject>
#include "shiploaderworker.h"
#include "simulator.h"
#include "ship/ship.h"
#include "network/optimizednetwork.h"
#include "simulatorworker.h"


SHIPNETSIM_EXPORT struct APIData {
    ShipNetSimCore::OptimizedNetwork* network = nullptr;
    SimulatorWorker* simulatorWorker = nullptr;
    ShipNetSimCore::Simulator* simulator = nullptr;
    ShipLoaderWorker* shipLoaderWorker = nullptr;
    QThread *workerThread = nullptr;
    QMap<QString, std::shared_ptr<ShipNetSimCore::Ship>> ships;
    bool isBusy = false;  // Flag for activity tracking
};

class SHIPNETSIM_EXPORT SimulatorAPI : public QObject
{
    Q_OBJECT
    friend struct std::default_delete<SimulatorAPI>; // Allow unique_ptr to delete

public:
    enum class Mode {
        Async,
        Sync
    };


signals:
    void simulationCreated(QString networkName);
    void networkLoaded(QString networkName);
    void simulationsPaused(QVector<QString> networkNames);
    void simulationsResumed(QVector<QString> networkNames);
    void simulationsRestarted(QVector<QString> networkNames);
    void simulationsTerminated(QVector<QString> networkNames);
    void simulationFinished(QString networkName);
    void simulationAdvanced(QMap<QString, QPair<units::time::second_t, double>>
                                currentSimulorTimePairs);
    void simulationProgressUpdated(QMap<QString, int> currentSimulationProgress);
    void shipsReachedDestination(const QJsonObject shipsStates);
    void shipsAddedToSimulation(const QString networkName,
                                const QVector<QString> shipIDs);
    void simulationResultsAvailable(QMap<QString, ShipsResults> results);
    void shipCurrentStateAvailable(const QJsonObject shipState);
    void simulationCurrentStateAvailable(const QJsonObject simulatorState);
    void shipCoordinatesUpdated(QString shipID,
                                ShipNetSimCore::GPoint position,
                                units::angle::degree_t heading,
                                QVector<std::shared_ptr<
                                    ShipNetSimCore::GLine>> paths);
    void containersAddedToShip(QString networkName, QString shipID);
    void errorOccurred(QString error);
    // Signal to notify when worker completes its work
    void workersReady(QVector<QString> networkNames);

protected:
    static SimulatorAPI& getInstance();
    static void resetInstance() ;

    ~SimulatorAPI();

    void setConnectionType(Qt::ConnectionType connectionType);

    // Simulator control methods
    void createNewSimulationEnvironment(
        QString networkFilePath,
        QString networkName,
        QVector<std::shared_ptr<ShipNetSimCore::Ship>> shipList =
        QVector<std::shared_ptr<ShipNetSimCore::Ship>>(),
        units::time::second_t timeStep = units::time::second_t(1.0),
        bool isExternallyControlled = false,
        Mode mode = Mode::Async);

    void createNewSimulationEnvironment(
        QString networkName,
        QVector<std::shared_ptr<ShipNetSimCore::Ship>> shipList =
        QVector<std::shared_ptr<ShipNetSimCore::Ship>>(),
        units::time::second_t timeStep = units::time::second_t(1.0),
        bool isExternallyControlled = false,
        Mode mode = Mode::Async);

    ShipNetSimCore::OptimizedNetwork* loadNetwork(QString filePath,
                                                  QString networkName = "*");



    void requestSimulationCurrentResults(
        QVector<QString> networkNames);
    void restartSimulations(
        QVector<QString> networkNames);

    // Ship control methods
    void addShipToSimulation(
        QString networkName,
        QVector<std::shared_ptr<ShipNetSimCore::Ship>> ships);
    // GETTERS
    ShipNetSimCore::Simulator* getSimulator(QString networkName);
    ShipNetSimCore::OptimizedNetwork* getNetwork(QString networkName);
    std::shared_ptr<ShipNetSimCore::Ship> getShipByID(
        QString networkName, QString& shipID);
    QVector<std::shared_ptr<ShipNetSimCore::Ship>>
    getAllShips(QString networkName);
    QJsonObject requestShipCurrentStateByID(QString networkName, QString ID);
    QJsonObject requestSimulatorCurrentState(QString networkName);
    void addContainersToShip(QString networkName,
                              QString shipID, QJsonObject json);

private:
    static std::unique_ptr<SimulatorAPI> instance;
    static void registerQMeta();
    APIData& getApiDataAndEnsureThread(const QString &networkName);
    void setupShipsConnection(
        QVector<std::shared_ptr<ShipNetSimCore::Ship>> ships,
        QString networkName, Mode mode);
protected slots:
    void handleShipReachedDestination(QString networkName,
                                      QJsonObject shipState,
                                      Mode mode);
    void handleOneTimeStepCompleted(QString networkName,
                                    units::time::second_t currentSimulatorTime,
                                    double progress, Mode mode);
    void handleProgressUpdate(QString networkName, int progress, Mode mode);
    void handleResultsAvailable(QString networkName,
                                ShipsResults result,
                                Mode mode);

    void emitShipsReachedDestination();
    void emitSimulationResults(QMap<QString, ShipsResults> results);
    // void handleSimulationResults(
    //     const QVector<SimulationResult>& results);

protected:
    SimulatorAPI() = default;
    SimulatorAPI(const SimulatorAPI&) = delete;
    SimulatorAPI& operator=(const SimulatorAPI&) = delete;

    void setupConnections(const QString& networkName, Mode mode);

    // Network name and its related data
    QMap<QString, APIData> mData;

    int m_completedSummaryResults = 0;
    QMap<QString, ShipsResults> m_simulationResultBuffer;

    Qt::ConnectionType mConnectionType = Qt::QueuedConnection;

    int m_completedSimulatorsTimeStep = 0;  // Track the number of completed simulators
    int m_completedSimulatorsProgress = 0;  // Track the number of updated simulators
    QMap<QString, QPair<units::time::second_t, double>> m_timeStepData;
    QMap<QString, int> m_progressData;
    QMap<QString, QJsonObject> m_shipsReachedBuffer;

    QVector<QString> m_totalSimulatorsRunRequested;
    int m_completedSimulatorsRun = 0;
    QVector<QString> m_totalSimulatorsInitializationRequested;
    int m_completedSimulatorsInitialization = 0;
    QVector<QString> m_totalSimulatorPauseRequested;
    int m_completedSimulatorsPaused = 0;
    QVector<QString> m_totalSimulatorResumeRequested;
    int m_completedSimulatorsResumed = 0;
    QVector<QString> m_totalSimulatorEndRequested;
    int m_completedSimulatorsEnded = 0;
    QVector<QString> m_totalSimulatorRestartedRequested;
    int m_completedSimulatorRestarted = 0;
    QVector<QString> m_totalSimulatorWorkerFinished;
    int m_completedSimulatorWorkerFinished = 0;

    void checkAndEmitSignal(int& counter,
                            int total,
                            const QVector<QString>& networkNames,
                            void(SimulatorAPI::*signal)(QVector<QString>),
                            Mode mode);

    void setupSimulator(QString networkName,
                        QVector<std::shared_ptr<ShipNetSimCore::Ship>> &shipList,
                        units::time::second_t timeStep,
                        bool isExternallyControlled,
                        Mode mode);

    QVector<std::shared_ptr<ShipNetSimCore::Ship>>
    mLoadShips(QJsonObject& ships,
              QString networkName = "");
    QVector<std::shared_ptr<ShipNetSimCore::Ship> >
    mLoadShips(QJsonObject &ships,
              ShipNetSimCore::OptimizedNetwork* network);
    QVector<std::shared_ptr<ShipNetSimCore::Ship>>
    mLoadShips(QVector<QMap<QString, QString>> ships,
              QString networkName = "");
    QVector<std::shared_ptr<ShipNetSimCore::Ship>>
    mLoadShips(QVector<QMap<QString, std::any>> ships,
              QString networkName = "");
    QVector<std::shared_ptr<ShipNetSimCore::Ship>>
    mLoadShips(QString shipsFilePath,
              QString networkName = "");

    std::any convertQVariantToAny(const QVariant& variant);
    std::map<std::string, std::any>
    convertQMapToStdMap(const QMap<QString, QVariant>& qMap);
    bool isWorkerBusy(QString networkName);

public:

    static Mode mMode;

    static QVector<std::shared_ptr<ShipNetSimCore::Ship>>
    loadShips(QJsonObject& ships,
              QString networkName = "");
    static QVector<std::shared_ptr<ShipNetSimCore::Ship> >
    loadShips(QJsonObject &ships,
              ShipNetSimCore::OptimizedNetwork* network);
    static QVector<std::shared_ptr<ShipNetSimCore::Ship>>
    loadShips(QVector<QMap<QString, QString>> ships,
              QString networkName = "");
    static QVector<std::shared_ptr<ShipNetSimCore::Ship>>
    loadShips(QVector<QMap<QString, std::any>> ships,
              QString networkName = "");
    static QVector<std::shared_ptr<ShipNetSimCore::Ship>>
    loadShips(QString shipsFilePath,
              QString networkName = "");

    class SHIPNETSIM_EXPORT InteractiveMode {
    public:
        static SimulatorAPI& getInstance();
        // Simulator control methods
        static void createNewSimulationEnvironment(
            QString networkFilePath,
            QString networkName,
            QVector<std::shared_ptr<ShipNetSimCore::Ship>> shipList =
            QVector<std::shared_ptr<ShipNetSimCore::Ship>>(),
            units::time::second_t timeStep = units::time::second_t(1.0),
            bool isExternallyControlled = false,
            Mode mode = Mode::Async);

        static void createNewSimulationEnvironment(
            QString networkName,
            QVector<std::shared_ptr<ShipNetSimCore::Ship>> shipList =
            QVector<std::shared_ptr<ShipNetSimCore::Ship>>(),
            units::time::second_t timeStep = units::time::second_t(1.0),
            bool isExternallyControlled = false,
            Mode mode = Mode::Async);

        static ShipNetSimCore::OptimizedNetwork*
        loadNetwork(QString filePath, QString networkName);

        static void addShipToSimulation(
            QString networkName,
            QVector<std::shared_ptr<ShipNetSimCore::Ship>> ships);
        static void initSimulation(
            QVector<QString> networkNames);
        static void runOneTimeStep(
            QVector<QString> networkNames);
        static void runSimulation(
            QVector<QString> networkNames,
            units::time::second_t timeSteps);
        static void endSimulation(
            QVector<QString> networkNames);

        static ShipNetSimCore::Simulator* getSimulator(QString networkName);
        static ShipNetSimCore::OptimizedNetwork*
        getNetwork(QString networkName);
        static std::shared_ptr<ShipNetSimCore::Ship> getShipByID(
            QString networkName, QString& shipID);
        static QVector<std::shared_ptr<ShipNetSimCore::Ship>>
        getAllShips(QString networkName);

        static bool isWorkerBusy(QString networkName);
        static void addContainersToShip(QString networkName,
                                        QString shipID, QJsonObject json);
        static void setConnectionType(Qt::ConnectionType connectionType);
    };

    class SHIPNETSIM_EXPORT ContinuousMode {
    public:
        static SimulatorAPI& getInstance();
        // Simulator control methods
        static void createNewSimulationEnvironment(
            QString networkFilePath,
            QString networkName,
            QVector<std::shared_ptr<ShipNetSimCore::Ship>> shipList,
            units::time::second_t timeStep = units::time::second_t(1.0),
            bool isExternallyControlled = false,
            Mode mode = Mode::Async);

        static void createNewSimulationEnvironment(
            QString networkName,
            QVector<std::shared_ptr<ShipNetSimCore::Ship>> shipList,
            units::time::second_t timeStep = units::time::second_t(1.0),
            bool isExternallyControlled = false,
            Mode mode = Mode::Async);

        static ShipNetSimCore::OptimizedNetwork*
        loadNetwork(QString filePath, QString networkName);
        static void addShipToSimulation(
            QString networkName,
            QVector<std::shared_ptr<ShipNetSimCore::Ship>> ships);

        static void runSimulation(
            QVector<QString> networkNames);
        static void pauseSimulation(
            QVector<QString> networkNames);
        static void resumeSimulation(
            QVector<QString> networkNames);
        static void terminateSimulation(
            QVector<QString> networkNames);
        static ShipNetSimCore::Simulator* getSimulator(QString networkName);
        static ShipNetSimCore::OptimizedNetwork*
        getNetwork(QString networkName);
        static bool isWorkerBusy(QString networkName);
        static void addContainersToShip(QString networkName,
                                        QString shipID, QJsonObject json);
        static void setConnectionType(Qt::ConnectionType connectionType);
    };

};

#endif // SIMULATORAPI_H
