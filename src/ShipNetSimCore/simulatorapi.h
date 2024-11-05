#ifndef SIMULATORAPI_H
#define SIMULATORAPI_H

#include "export.h"
#include <QObject>
#include "simulator.h"
#include "ship/ship.h"
#include "network/optimizednetwork.h"

class SHIPNETSIM_EXPORT SimulatorAPI : public QObject
{
    Q_OBJECT

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
    void simulationsEnded(QVector<QString> networkNames);
    void simulationFinished(QString networkName);
    void simulationAdvanced(QMap<QString, units::time::second_t>
                                currentSimulorTimePairs);
    void shipsReachedDestination(const QJsonObject shipsStates);
    void shipsAddedToSimulation(const QString networkName,
                                const QVector<QString> shipIDs);
    void simulationResultsAvailable(QMap<QString, ShipsResults>& results);
    void shipCurrentStateAvailable(const QJsonObject shipState);
    void simulationCurrentStateAvailable(const QJsonObject simulatorState);
    void containersAddedToShip(QString networkName, QString shipID);
    void errorOccurred(QString error);
    // Signal to notify when worker completes its work
    void workersReady(QVector<QString> networkNames);

protected:
    struct APIData {
        ShipNetSimCore::OptimizedNetwork* network = nullptr;
        ShipNetSimCore::Simulator* simulator = nullptr;
        QThread *workerThread = nullptr;
        QMap<QString, std::shared_ptr<ShipNetSimCore::Ship>> ships;
    };

    static SimulatorAPI& getInstance() {
        static SimulatorAPI instance;
        return instance;
    }

    ~SimulatorAPI();

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
        ShipNetSimCore::OptimizedNetwork* network,
        QVector<std::shared_ptr<ShipNetSimCore::Ship>> shipList =
        QVector<std::shared_ptr<ShipNetSimCore::Ship>>(),
        units::time::second_t timeStep = units::time::second_t(1.0),
        bool isExternallyControlled = false,
        Mode mode = Mode::Async);

    ShipNetSimCore::OptimizedNetwork* loadNetwork(QString filePath,
                                                  QString networkName = "*");



    void requestSimulationCurrentResults(QVector<QString> networkNames);
    void restartSimulations(QVector<QString> networkNames);

    // Ship control methods
    void addShipToSimulation(QString networkName,
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

protected slots:
    void handleShipReachedDestination(QString networkName,
                                      QJsonObject shipState,
                                      Mode mode);
    void handleOneTimeStepCompleted(QString networkName,
                                    units::time::second_t currentSimulatorTime,
                                    Mode mode);
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

    int m_completedSimulatorsTimeStep = 0;  // Track the number of completed simulators
    QMap<QString, units::time::second_t> m_timeStepData;
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

    void setupSimulator(ShipNetSimCore::OptimizedNetwork* network,
                        QVector<std::shared_ptr<ShipNetSimCore::Ship>> &shipList,
                        units::time::second_t timeStep,
                        bool isExternallyControlled,
                        Mode mode);

    std::any convertQVariantToAny(const QVariant& variant);
    std::map<std::string, std::any>
    convertQMapToStdMap(const QMap<QString, QVariant>& qMap);
    bool isWorkerBusy(QString networkName);

public:

    static Mode mMode;

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
            ShipNetSimCore::OptimizedNetwork* network,
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
        static void initSimulation(QVector<QString> networkNames);
        static void runOneTimeStep(QVector<QString> networkNames);
        static void runSimulation(QVector<QString> networkNames,
                                  units::time::second_t timeSteps);
        static void endSimulation(QVector<QString> networkNames);
        static ShipNetSimCore::Simulator* getSimulator(QString networkName);

        static ShipNetSimCore::OptimizedNetwork*
        getNetwork(QString networkName);
        static std::shared_ptr<ShipNetSimCore::Ship> getShipByID(
            QString networkName, QString& shipID);
        static QVector<std::shared_ptr<ShipNetSimCore::Ship>>
        getAllTrains(QString networkName);
        static bool isWorkerBusy(QString networkName);
        static void addContainersToShip(QString networkName,
                                        QString shipID, QJsonObject json);
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
            ShipNetSimCore::OptimizedNetwork* network,
            QVector<std::shared_ptr<ShipNetSimCore::Ship>> shipList,
            units::time::second_t timeStep = units::time::second_t(1.0),
            bool isExternallyControlled = false,
            Mode mode = Mode::Async);

        static ShipNetSimCore::OptimizedNetwork*
        loadNetwork(QString filePath, QString networkName);

        static void runSimulation(QVector<QString> networkNames);
        static void pauseSimulation(QVector<QString> networkNames);
        static void resumeSimulation(QVector<QString> networkNames);
        static void endSimulation(QVector<QString> networkNames);
        static ShipNetSimCore::Simulator* getSimulator(QString networkName);
        static ShipNetSimCore::OptimizedNetwork*
        getNetwork(QString networkName);
        static bool isWorkerBusy(QString networkName);
        static void addContainersToShip(QString networkName,
                                        QString shipID, QJsonObject json);
    };


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
};

#endif // SIMULATORAPI_H
