/**
 * @mainpage SimulatorAPI Documentation
 * @brief A comprehensive API for maritime simulation with support
 * for interactive and continuous simulation modes
 *
 * @section overview Overview
 * SimulatorAPI provides a thread-safe, event-driven interface for maritime
 * simulation with support for multiple networks, ships, and simulation modes.
 *
 * Key Features:
 * - Multi-threaded simulation support
 * - Interactive and Continuous simulation modes
 * - Network and ship management
 * - Container tracking (when BUILD_SERVER_ENABLED is defined)
 *
 * @section architecture Architecture
 * The API uses a singleton pattern with two primary operation modes:
 * - Interactive Mode: Step-by-step simulation control
 * - Continuous Mode: Continuous simulation with pause/resume capabilities
 *
 * @section examples Usage Examples
 *
 * Interactive Mode Example:
 * @code{.cpp}
 * // Load network and create simulation
 * auto* network = SimulatorAPI::InteractiveMode::loadNetwork("network.json",
 *                                                            "myNetwork");
 * SimulatorAPI::InteractiveMode::createNewSimulationEnvironment("myNetwork");
 *
 * // Run step by step
 * SimulatorAPI::InteractiveMode::runSimulation({"myNetwork"},
 *                                              units::time::second_t(1.0));
 * @endcode
 *
 * Continuous Mode Example:
 * @code{.cpp}
 * // Similar setup but continuous execution
 * SimulatorAPI::ContinuousMode::runSimulation({"myNetwork"});
 * @endcode
 *
 * @section error_handling Error Handling
 * The API uses signal-based error reporting:
 * - Connect to errorOccurred signal for error notifications
 * - All methods that can fail emit detailed error messages
 * - Async operations report completion via specific signals
 *
 */

#ifndef SIMULATORAPI_H
#define SIMULATORAPI_H

// Include necessary headers
#include "export.h"
#include <QObject>
#include "shiploaderworker.h"
#include "simulator.h"
#include "ship/ship.h"
#include "network/optimizednetwork.h"
#include "simulatorworker.h"




/**
* @class NoThousandsSeparator
* @brief Custom locale facet to remove thousand separators from numeric output
*
* @details This class customizes number formatting by removing thousand
* separators (e.g., converts "1,000" to "1000").
* Used internally for consistent numeric string representation across
* different locales.
*
* @note Inherits from std::numpunct<char> to modify numeric punctuation behavior
*/
class NoThousandsSeparator : public std::numpunct<char> {
protected:
    /**
    * @brief Defines the thousand separator character
    * @return '\0' to indicate no separator
    */
    char do_thousands_sep() const override {
        return '\0'; // No separator
    }

    /**
    * @brief Defines the grouping behavior for digits
    * @return Empty string to disable digit grouping
    */
    std::string do_grouping() const override {
        return ""; // No grouping
    }
};

/**
* @struct APIData
* @brief Container for network-specific simulation components
*
* @details Holds all components required for a single network simulation,
* including network, simulator, workers, and ships. Each network in the
* simulation has its own APIData instance.
*/
struct SHIPNETSIM_EXPORT APIData {
    ShipNetSimCore::OptimizedNetwork* network = nullptr;  ///< Network instance for routing and geography
    SimulatorWorker* simulatorWorker = nullptr;           ///< Worker for simulation processing
    ShipNetSimCore::Simulator* simulator = nullptr;       ///< Main simulator instance
    ShipLoaderWorker* shipLoaderWorker = nullptr;        ///< Worker for loading ship data
    QThread *workerThread = nullptr;                      ///< Dedicated thread for this network
    QMap<QString, std::shared_ptr<ShipNetSimCore::Ship>> ships;  ///< Map of ship ID to ship instance
    bool isBusy = false;  ///< Indicates if network is currently processing
};

/**
* @struct RequestData
* @brief Generic container for tracking asynchronous operation progress
*
* @tparam T Type of data being tracked (e.g., ShipsResults, QString)
*
* @details Used to track the progress and store results of asynchronous
* operations across multiple networks. Helps manage synchronization in
* both Async and Sync modes.
*
* Example usage:
* @code{.cpp}
* RequestData<ShipsResults> resultsTracker;
* resultsTracker.dataBuffer["network1"] = someResults;
* resultsTracker.completedRequests++;
* @endcode
*/
template<typename T>
struct RequestData {
    QMap<QString, T> dataBuffer;  ///< Stores operation results for each network
    int completedRequests = 0;    ///< Counter for completed network operations

    /**
    * @brief List of networks involved in the current operation
    * @note Used to track which networks are being processed and
    * determine when all operations are complete
    */
    QVector<QString> requestedNetworkProcess;
};

/**
 * @class SimulatorAPI
 * @brief Main API class providing simulation control and management
 *
 * The SimulatorAPI class is implemented as a singleton and provides
 * two operation modes through SimulatorAPI::InteractiveMode and
 * SimulatorAPI::ContinuousMode.
 */
class SHIPNETSIM_EXPORT SimulatorAPI : public QObject
{
    Q_OBJECT
    friend struct std::default_delete<SimulatorAPI>; // Allow unique_ptr to delete

public:

    /**
     * @enum Mode
     * @brief Defines the operation mode for signal emission
     *
     * @var Mode::Async
     * Signals are emitted only when all simulators/networks reach
     * the same point
     *
     * @var Mode::Sync
     * Signals are emitted immediately for each simulator/network
     */
    enum class Mode {
        Async,  ///< Synchronize all networks before emitting signals
        Sync    ///< Emit signals immediately for each network
    };


signals:

    /**
    * @brief Emitted when a new simulation environment is successfully created.
    * @param networkName The name of the network for which simulation was
    * created
    */
    void simulationCreated(QString networkName);

    /**
    * @brief Emitted when a network is successfully loaded into the simulator.
    * @param networkName The name of the loaded network
    */
    void networkLoaded(QString networkName);

    /**
    * @brief Emitted when simulations are paused (Continuous Mode only).
    * @param networkNames List of network names whose simulations were paused
    * @note In Async mode, signal is emitted only when all specified networks
    * are paused
    */
    void simulationsPaused(QVector<QString> networkNames);

    /**
    * @brief Emitted when paused simulations are resumed (Continuous Mode only).
    * @param networkNames List of network names whose simulations were resumed
    * @note In Async mode, signal is emitted only when all specified networks
    * are resumed
    */
    void simulationsResumed(QVector<QString> networkNames);

    /**
    * @brief Emitted when simulations are restarted from their initial state.
    * @param networkNames List of network names whose simulations were
    * restarted
    * @note In Async mode, signal is emitted only when all specified networks
    * are restarted
    */
    void simulationsRestarted(QVector<QString> networkNames);

    /**
    * @brief Emitted when simulations are forcefully terminated.
    * @param networkNames List of network names whose simulations were
    * terminated
    * @note In Async mode, signal is emitted only when all specified networks
    * are terminated
    */
    void simulationsTerminated(QVector<QString> networkNames);

    /**
    * @brief Emitted when simulations complete their execution naturally.
    * @param networkName List of network names whose simulations finished
    * @note Differs from simulationsTerminated which is for forced termination
    */
    void simulationFinished(QVector<QString> networkName);

    /**
    * @brief Emitted when simulation time advances by one step.
    * @param currentSimulorTimePairs Map of network names to their current
    * time and progress
    *        The QPair contains <current simulation time, progress percentage>
    */
    void simulationAdvanced(QMap<QString, QPair<units::time::second_t, double>>
                                currentSimulorTimePairs);

    /**
    * @brief Emitted to update the overall simulation progress.
    * @param currentSimulationProgress Map of network names to their progress
    * percentage (0-100)
    */
    void simulationProgressUpdated(
        QMap<QString, int> currentSimulationProgress);

    /**
    * @brief Emitted when ships reach their designated destinations.
    * @param shipsStates JSON object containing states of ships that reached
    * destination
    * @note Format: {"networkName": {"shipStates":
    *                                       [ship1State, ship2State, ...]}}
    */
    void shipsReachedDestination(const QJsonObject shipsStates);

    /**
    * @brief Emitted when new ships are successfully added to a simulation.
    * @param networkName Name of the network where ships were added
    * @param shipIDs List of IDs of the newly added ships
    */
    void shipsAddedToSimulation(const QString networkName,
                                const QVector<QString> shipIDs);

    /**
    * @brief Emitted when simulation results are ready for retrieval.
    * @param results Map of network names to their respective simulation
    * results
    */
    void simulationResultsAvailable(QMap<QString, ShipsResults> results);

    /**
    * @brief Emitted when a ship's current state information is available.
    * @param shipState JSON object containing the ship's current state
    * information
    */
    void shipCurrentStateAvailable(const QJsonObject shipState);

    /**
    * @brief Emitted when simulation's current state information is available.
    * @param simulatorState JSON object containing the simulator's current
    * state
    */
    void simulationCurrentStateAvailable(const QJsonObject simulatorState);

    /**
    * @brief Emitted when a ship's position is updated during simulation.
    * @param shipID The unique identifier of the ship
    * @param position Current geographical position of the ship
    * @param heading Current heading angle of the ship
    * @param paths Vector of line segments representing the ship's planned path
    */
    void shipCoordinatesUpdated(QString shipID,
                                ShipNetSimCore::GPoint position,
                                units::angle::degree_t heading,
                                QVector<std::shared_ptr<
                                    ShipNetSimCore::GLine>> paths);

    /**
    * @brief Emitted when information about available ports is requested.
    * @param NetworkPortIDs Map of network names to their available port IDs
    */
    void availablePorts(QMap<QString, QVector<QString>> NetworkPortIDs);


#ifdef BUILD_SERVER_ENABLED

    /**
    * @brief Emitted when containers are successfully added to a ship.
    * @param networkName The name of the network containing the ship
    * @param shipID The unique identifier of the ship to which containers
    *               were added
    * @note This signal is only available when BUILD_SERVER_ENABLED is defined
    * @see addContainersToShip()
    */
    void containersAddedToShip(QString networkName, QString shipID);

    /**
    * @brief Emitted when a ship carrying containers reaches a seaport.
    * @param networkName The name of the network
    * @param shipID The unique identifier of the ship
    * @param seaPortCode The LOCODE of the seaport reached
    * @param containers JSON array containing information about the containers
    *                   being carried
    * @note This signal is only available when BUILD_SERVER_ENABLED is defined
    * @note The containers JSON array contains the state and details of all
    *                      containers at arrival
    */
    void shipReachedSeaPort(QString networkName, QString shipID,
                            QString seaPortCode, QJsonArray containers);
#endif

    /**
    * @brief Emitted when an error occurs during any operation.
    * @param error Description of the error that occurred
    * @note This signal provides error information for error handling and
    *       debugging
    * @note Common error scenarios include:
    *       - Network not found
    *       - File loading failures
    *       - Invalid ship configurations
    *       - Simulation setup errors
    */
    void errorOccurred(QString error);


    /**
    * @brief Emitted when all worker threads complete their assigned tasks.
    * @param networkNames List of networks whose workers have completed
    * @note In Async mode, this signal is emitted only when all workers are done
    * @note In Sync mode, this signal is emitted for each worker completion
    * @see Mode
    */
    void workersReady(QVector<QString> networkNames);

protected:
    /**
    * @brief Get the singleton instance of SimulatorAPI
    * @return Reference to the singleton instance
    * @note Thread-safe implementation ensures single instance across threads
    */
    static SimulatorAPI& getInstance();

    /**
    * @brief Reset the API instance to its initial state
    * @details Cleans up all resources including:
    * - Networks and simulators
    * - Worker threads
    * - Ships and their data
    * @note Creates a new clean instance after reset
    */
    static void resetInstance() ;

    /**
    * @brief Destructor
    * @details Performs cleanup of all simulation resources
    */
    ~SimulatorAPI();

    // Simulator control methods

    /**
    * @brief Create a new simulation environment using network file
    * @param networkFilePath Path to the network configuration file
    * @param networkName Unique identifier for the network
    * @param shipList List of ships to initialize with simulation
    * @param timeStep Time increment for simulation steps
    * @param isExternallyControlled Whether external control is enabled
    * @param mode Synchronization mode for operation
    * @throws Emits errorOccurred if network creation fails
    */
    void createNewSimulationEnvironment(
        QString networkFilePath,
        QString networkName,
        QVector<std::shared_ptr<ShipNetSimCore::Ship>> shipList =
        QVector<std::shared_ptr<ShipNetSimCore::Ship>>(),
        units::time::second_t timeStep = units::time::second_t(1.0),
        bool isExternallyControlled = false,
        Mode mode = Mode::Async);

    /**
    * @brief Create simulation environment from existing network
    * @param networkName Name of previously loaded network
    * @param shipList List of ships to initialize with simulation
    * @param timeStep Time increment for simulation steps
    * @param isExternallyControlled Whether external control is enabled
    * @param mode Synchronization mode for operation
    * @throws Emits errorOccurred if network doesn't exist
    */
    void createNewSimulationEnvironment(
        QString networkName,
        QVector<std::shared_ptr<ShipNetSimCore::Ship>> shipList =
        QVector<std::shared_ptr<ShipNetSimCore::Ship>>(),
        units::time::second_t timeStep = units::time::second_t(1.0),
        bool isExternallyControlled = false,
        Mode mode = Mode::Async);

    /**
    * @brief Load a maritime network from file
    * @param filePath Path to network configuration file
    * @param networkName Identifier for the network (use "*" for default)
    * @return Pointer to loaded network or nullptr on failure
    * @throws Emits errorOccurred if file cannot be loaded
    */
    ShipNetSimCore::OptimizedNetwork* loadNetwork(QString filePath,
                                                  QString networkName = "*");

    /**
    * @brief Request current results from active simulations
    * @param networkNames List of networks to query
    * @note Results are provided via simulationResultsAvailable signal
    */
    void requestSimulationCurrentResults(
        QVector<QString> networkNames);

    /**
    * @brief Restart specified simulations from initial state
    * @param networkNames List of networks to restart
    * @note Emits simulationsRestarted signal when complete
    */
    void restartSimulations(
        QVector<QString> networkNames);

    // Ship control methods

    /**
    * @brief Add ships to an existing simulation
    * @param networkName Target network identifier
    * @param ships List of ships to add
    * @throws Emits errorOccurred if network doesn't exist
    */
    void addShipToSimulation(
        QString networkName,
        QVector<std::shared_ptr<ShipNetSimCore::Ship>> ships);


    // GETTERS

    /**
    * @brief Get simulator instance for a network
    * @param networkName Network identifier
    * @return Pointer to simulator or nullptr if not found
    */
    ShipNetSimCore::Simulator* getSimulator(QString networkName);

    /**
    * @brief Get network instance
    * @param networkName Network identifier
    * @return Pointer to network or nullptr if not found
    */
    ShipNetSimCore::OptimizedNetwork* getNetwork(QString networkName);

    /**
    * @brief Get ship by its ID
    * @param networkName Network containing the ship
    * @param shipID Unique identifier of the ship
    * @return Shared pointer to ship or nullptr if not found
    */
    std::shared_ptr<ShipNetSimCore::Ship> getShipByID(
        QString networkName, QString& shipID);

    /**
    * @brief Get all ships in a network
    * @param networkName Network identifier
    * @return Vector of all ships in the network
    */
    QVector<std::shared_ptr<ShipNetSimCore::Ship>>
    getAllShips(QString networkName);

    /**
    * @brief Get current state of a specific ship
    * @param networkName Network containing the ship
    * @param ID Ship identifier
    * @return JSON object containing ship state
    */
    QJsonObject requestShipCurrentStateByID(QString networkName,
                                            QString ID);

    /**
    * @brief Get current state of a simulator
    * @param networkName Network identifier
    * @return JSON object containing simulator state
    */
    QJsonObject requestSimulatorCurrentState(QString networkName);

    /**
    * @brief Request available ports information
    * @param networkNames List of networks to query
    * @param getOnlyPortsOnShipsPaths If true, returns only ports
    *                                   on ship routes
    * @note Results provided via availablePorts signal
    */
    void requestAvailablePorts(QVector<QString> networkNames,
                               bool getOnlyPortsOnShipsPaths);

#ifdef BUILD_SERVER_ENABLED

    /**
    * @brief Add containers to a ship
    * @param networkName Network containing the ship
    * @param shipID Ship identifier
    * @param json Container configuration in JSON format
    * @note Only available when BUILD_SERVER_ENABLED is defined
    */
    void addContainersToShip(QString networkName,
                             QString shipID, QJsonObject json);
#endif

    /**
    * @brief Check if a network is loaded
    * @param networkName Network identifier
    * @return true if network exists and is loaded
    */
    bool isNetworkLoaded(QString networkName);



protected slots:

    /**
    * @brief Handler for when a ship reaches its destination
    * @param networkName Network where the event occurred
    * @param shipState Current state of the ship as JSON object
    * @param mode Current operation mode (Async/Sync)
    *
    * @details In Async mode, accumulates results until all tracked ships reach
    * destination. In Sync mode, emits signal immediately.
    * Emits shipsReachedDestination signal with accumulated results.
    */
    void handleShipReachedDestination(QString networkName,
                                      QJsonObject shipState,
                                      Mode mode);

    /**
    * @brief Handler for completion of a single simulation time step
    * @param networkName Network that completed the step
    * @param currentSimulatorTime Current simulation time
    * @param progress Progress percentage of overall simulation
    * @param mode Current operation mode (Async/Sync)
    *
    * @details Updates simulation progress tracking and emits
    * simulationAdvanced signal based on mode:
    * - Async: Waits for all networks to complete step
    * - Sync: Emits immediately for each network
    */
    void handleOneTimeStepCompleted(QString networkName,
                                    units::time::second_t currentSimulatorTime,
                                    double progress,
                                    Mode mode);

    /**
    * @brief Handler for simulation progress updates
    * @param networkName Network reporting progress
    * @param progress Progress percentage (0-100)
    * @param mode Current operation mode (Async/Sync)
    *
    * @details Updates progress tracking and emits simulationProgressUpdated
    * signal based on mode:
    * - Async: Waits for all networks to report
    * - Sync: Emits immediately
    */
    void handleProgressUpdate(QString networkName,
                              int progress,
                              Mode mode);

    /**
    * @brief Handler for simulation results
    * @param networkName Network providing results
    * @param result Simulation results data
    * @param mode Current operation mode (Async/Sync)
    *
    * @details Processes simulation results and emits
    * simulationResultsAvailable
    * signal based on mode:
    * - Async: Accumulates results until all networks report
    * - Sync: Emits results immediately
    */
    void handleResultsAvailable(QString networkName,
                                ShipsResults result,
                                Mode mode);

    /**
    * @brief Emits consolidated ship destination data
    *
    * @details Collects all accumulated ship destination data from
    * m_shipsReachedBuffer and emits as a single shipsReachedDestination signal.
    * Clears the buffer after emission.
    */
    void emitShipsReachedDestination();

    /**
    * @brief Emits consolidated simulation results
    * @param results Map of network names to their simulation results
    *
    * @details Emits simulationResultsAvailable signal with provided results
    * if the results map is not empty. Clears results after emission.
    */
    void emitSimulationResults(QMap<QString, ShipsResults> results);

    /**
    * @brief Handler for available ports information
    * @param networkName Network reporting ports
    * @param portIDs List of available port identifiers
    * @param mode Current operation mode (Async/Sync)
    *
    * @details Processes port availability data and emits availablePorts
    * signal based on mode:
    * - Async: Accumulates data until all networks report
    * - Sync: Emits port data immediately
    */
    void handleAvailablePorts(QString networkName,
                              QVector<QString> portIDs,
                              Mode mode);

protected:
    /** @brief Default constructor for singleton pattern */
    SimulatorAPI() = default;

    /** @brief Deleted copy constructor to prevent copying */
    SimulatorAPI(const SimulatorAPI&) = delete;

    /** @brief Deleted assignment operator to prevent copying */
    SimulatorAPI& operator=(const SimulatorAPI&) = delete;

    /**
    * @brief Set up signal-slot connections for a network
    * @param networkName Network to set up connections for
    * @param mode Operation mode for signal handling
    * @details Establishes all necessary connections between simulator,
    * ships, and API components
    */
    void setupConnections(const QString& networkName, Mode mode);

    // Protected Member Variables

    /** @brief Map of network names to their simulation data */
    QMap<QString, APIData> mData;

    /** @brief Tracks simulation results across networks */
    RequestData<ShipsResults> mSimulationResultsTracker;

    /** @brief Tracks available ports information */
    RequestData<QVector<QString>> mAvailablePortTracker;

    /** @brief Tracks simulation time steps and progress */
    RequestData<QPair<units::time::second_t, double>> mTimeStepTracker;

    /** @brief Tracks overall simulation progress */
    RequestData<int> mProgressTracker;

    /** @brief Tracks pause operations */
    RequestData<QString> mPauseTracker;

    /** @brief Tracks resume operations */
    RequestData<QString> mResumeTracker;

    /** @brief Tracks termination operations */
    RequestData<QString> mTerminateTracker;

    /** @brief Tracks simulation completion */
    RequestData<QString> mFinishedTracker;

    /** @brief Tracks worker thread status */
    RequestData<QString> mWorkerTracker;

    /** @brief Tracks simulation run requests */
    RequestData<QString> mRunTracker;

    /** @brief Tracks simulation restart operations */
    RequestData<QString> mRestartTracker;

    /** @brief Connection type for Qt signals/slots */
    Qt::ConnectionType mConnectionType = Qt::QueuedConnection;

    /** @brief Buffer for ships that have reached their destination */
    QMap<QString, QJsonObject> m_shipsReachedBuffer;


    /**
    * @brief Check conditions and emit signal based on mode
    * @param counter Current count of completed operations
    * @param total Total expected operations
    * @param networkNames List of networks involved
    * @param signal Pointer to member signal to emit
    * @param mode Operation mode
    * @return true if signal was emitted, false otherwise
    */
    bool checkAndEmitSignal(int& counter,
                            int total,
                            const QVector<QString>& networkNames,
                            void(SimulatorAPI::*signal)(QVector<QString>),
                            Mode mode);

    /**
    * @brief Set up simulator instance for a network
    * @param networkName Target network
    * @param shipList Ships to initialize with
    * @param timeStep Simulation time step
    * @param isExternallyControlled External control flag
    * @param mode Operation mode
    */
    void setupSimulator(
        QString networkName,
        QVector<std::shared_ptr<ShipNetSimCore::Ship>> &shipList,
        units::time::second_t timeStep,
        bool isExternallyControlled,
        Mode mode);

    /**
    * @brief Load ships from various data sources
    * These overloaded methods handle different input formats
    */

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

    /**
    * @brief Convert QVariant to std::any
    * @param variant Source QVariant
    * @return Equivalent std::any value
    */
    std::any convertQVariantToAny(const QVariant& variant);

    /**
    * @brief Convert Qt map to STL map
    * @param qMap Source Qt map
    * @return Equivalent STL map
    */
    std::map<std::string, std::any>
    convertQMapToStdMap(const QMap<QString, QVariant>& qMap);

    /**
    * @brief Check if network's worker is busy
    * @param networkName Network to check
    * @return true if worker is busy
    */
    bool isWorkerBusy(QString networkName);

public:

    static Mode mMode;

    /**
    * @brief Load ships from a JSON object configuration
    * @param ships JSON object containing ship configurations
    * @param networkName Name of the network to associate ships with.
    *                    If empty, uses default network
    * @return Vector of loaded ship objects
    *
    * @details The JSON object should follow this structure:
    * @code{.json}
    * {
    *   "ships": [
    *     {
    *       "id": "ship1",
    *       "type": "container",
    *       "origin": "USNYC",
    *       "destination": "NLRTM",
    *       // other ship properties...
    *     }
    *     // more ships...
    *   ]
    * }
    * @endcode
    *
    * @throws Emits errorOccurred signal if network doesn't exist or JSON
    *               is invalid
    * @note This method is thread-safe and runs in the worker thread
    */
    static QVector<std::shared_ptr<ShipNetSimCore::Ship>>
    loadShips(QJsonObject& ships,
              QString networkName = "");

    /**
    * @brief Load ships from JSON object using a specific network instance
    * @param ships JSON object containing ship configurations
    * @param network Pointer to an initialized OptimizedNetwork instance
    * @return Vector of loaded ship objects
    *
    * @details Similar to the networkName version but uses a direct network
    *       reference
    * Useful when working with a specific network instance
    *
    * @throws Emits errorOccurred signal if network is nullptr or JSON is
    *           invalid
    * @note The network must be in the same thread as the worker
    */
    static QVector<std::shared_ptr<ShipNetSimCore::Ship> >
    loadShips(QJsonObject &ships,
              ShipNetSimCore::OptimizedNetwork* network);

    /**
    * @brief Load ships from a vector of string-based property maps
    * @param ships Vector of maps containing ship properties as strings
    * @param networkName Name of the network to associate ships with
    * @return Vector of loaded ship objects
    *
    * @details Each map in the vector should contain ship properties:
    * @code{.cpp}
    * QMap<QString, QString> shipProps;
    * shipProps["id"] = "ship1";
    * shipProps["type"] = "container";
    * shipProps["origin"] = "USNYC";
    * // etc...
    * @endcode
    *
    * @throws Emits errorOccurred signal if network doesn't exist or
    *           properties are invalid
    */
    static QVector<std::shared_ptr<ShipNetSimCore::Ship>>
    loadShips(QVector<QMap<QString, QString>> ships,
              QString networkName = "");

    /**
    * @brief Load ships from a vector of type-flexible property maps
    * @param ships Vector of maps containing ship properties with varied types
    * @param networkName Name of the network to associate ships with
    * @return Vector of loaded ship objects
    *
    * @details Supports properties of different types using std::any:
    * @code{.cpp}
    * QMap<QString, std::any> shipProps;
    * shipProps["id"] = "ship1";
    * shipProps["speed"] = 25.5;
    * shipProps["hasContainers"] = true;
    * // etc...
    * @endcode
    *
    * @throws Emits errorOccurred signal if network doesn't exist or
    *               properties are invalid
    * @note Uses type conversion for numeric and boolean properties
    */
    static QVector<std::shared_ptr<ShipNetSimCore::Ship>>
    loadShips(QVector<QMap<QString, std::any>> ships,
              QString networkName = "");

    /**
    * @brief Load ships from a configuration file
    * @param shipsFilePath Path to the ships configuration file
    * @param networkName Name of the network to associate ships with
    * @return Vector of loaded ship objects
    *
    * @details The configuration file should be in JSON format following
    * the same structure as described in the JSON-based loadShips method
    *
    * @throws Emits errorOccurred signal if:
    *         - File cannot be opened or read
    *         - File format is invalid
    *         - Network doesn't exist
    * @note This method is thread-safe and uses the worker thread for file
    * operations
    */
    static QVector<std::shared_ptr<ShipNetSimCore::Ship>>
    loadShips(QString shipsFilePath,
              QString networkName = "");

    /**
    * @class SimulatorAPI::InteractiveMode
    * @brief Provides step-by-step control over maritime simulations
    *
    * @details The InteractiveMode class provides a static interface for
    * running simulations
    * in a step-by-step manner, allowing fine-grained control over simulation
    *  execution.
    * This mode is ideal for:
    * - Debugging and testing
    * - Visualizing simulation steps
    * - Custom simulation control flows
    */
    class SHIPNETSIM_EXPORT InteractiveMode {
    public:

        /**
        * @brief Get the singleton instance of SimulatorAPI
        * @return Reference to the SimulatorAPI singleton
        */

        static SimulatorAPI& getInstance();


        // Simulator control methods

        /**
        * @brief Create a new simulation environment with explicit network file
        * @param networkFilePath Path to the network configuration file
        * @param networkName Unique identifier for the network
        * @param shipList Optional list of ships to initialize with the
        *                   simulation
        * @param timeStep Simulation time step duration (default: 1.0 second)
        * @param isExternallyControlled Whether simulation is controlled
        *                               externally
        * @param mode Synchronization mode for signal emission
        * @throws Emits errorOccurred if network file cannot be loaded or
        *               name conflicts
        */
        static void createNewSimulationEnvironment(
            QString networkFilePath,
            QString networkName,
            QVector<std::shared_ptr<ShipNetSimCore::Ship>> shipList =
            QVector<std::shared_ptr<ShipNetSimCore::Ship>>(),
            units::time::second_t timeStep = units::time::second_t(1.0),
            bool isExternallyControlled = false,
            Mode mode = Mode::Async);

        /**
        * @brief Create a new simulation environment using existing network
        * @param networkName Identifier of an already loaded network
        * @param shipList Optional list of ships to initialize with the
        *                   simulation
        * @param timeStep Simulation time step duration (default: 1.0 second)
        * @param isExternallyControlled Whether simulation is controlled
        *                               externally
        * @param mode Synchronization mode for signal emission
        * @throws Emits errorOccurred if network name doesn't exist
        */
        static void createNewSimulationEnvironment(
            QString networkName,
            QVector<std::shared_ptr<ShipNetSimCore::Ship>> shipList =
            QVector<std::shared_ptr<ShipNetSimCore::Ship>>(),
            units::time::second_t timeStep = units::time::second_t(1.0),
            bool isExternallyControlled = false,
            Mode mode = Mode::Async);

        /**
        * @brief Load a network configuration
        * @param filePath Path to network configuration file
        * @param networkName Unique identifier for the network
        * @return Pointer to the loaded network instance
        * @throws Emits errorOccurred if file cannot be loaded
        */
        static ShipNetSimCore::OptimizedNetwork*
        loadNetwork(QString filePath, QString networkName);

        /**
        * @brief Add ships to an existing simulation
        * @param networkName Target network identifier
        * @param ships List of ships to add
        * @throws Emits errorOccurred if network doesn't exist
        */
        static void addShipToSimulation(
            QString networkName,
            QVector<std::shared_ptr<ShipNetSimCore::Ship>> ships);

        /**
        * @brief Run simulation for specified number of time steps
        * @param networkNames List of networks to run
        * @param timeSteps Duration to run (use negative value for infinite)
        */
        static void runSimulation(
            QVector<QString> networkNames,
            units::time::second_t timeSteps);

        /**
        * @brief end the simulation and produce the summary file
        * @param networkNames List of networks to end
        */
        static void endSimulation(
            QVector<QString> networkNames);

        /**
        * @brief Forcefully terminate the simulation
        * @param networkNames List of networks to terminate
        */
        static void terminateSimulation(
            QVector<QString> networkNames);

        /**
        * @brief Get simulator instance for a network
        * @param networkName Network identifier
        * @return Pointer to simulator or nullptr if not found
        */
        static ShipNetSimCore::Simulator* getSimulator(QString networkName);

        /**
        * @brief Get network instance
        * @param networkName Network identifier
        * @return Pointer to network or nullptr if not found
        */
        static ShipNetSimCore::OptimizedNetwork*
        getNetwork(QString networkName);

        /**
        * @brief Get ship by its ID
        * @param networkName Network containing the ship
        * @param shipID Unique identifier of the ship
        * @return Shared pointer to ship or nullptr if not found
        */
        static std::shared_ptr<ShipNetSimCore::Ship> getShipByID(
            QString networkName, QString& shipID);

        /**
        * @brief Get all ships in a network
        * @param networkName Network identifier
        * @return Vector of all ships in the network
        */
        static QVector<std::shared_ptr<ShipNetSimCore::Ship>>
        getAllShips(QString networkName);

        /**
        * @brief Request available ports information
        * @param networkNames List of networks to query
        * @param getOnlyPortsOnShipsPaths If true, returns only ports
        *                                   on ship routes
        * @note Results provided via availablePorts signal
        */
        static void requestAvailablePorts(QVector<QString> networkNames,
                                          bool getOnlyPortsOnShipsPaths);

        /**
        * @brief Check if network's worker thread is busy
        * @param networkName Network identifier
        * @return true if worker is busy, false otherwise
        */
        static bool isWorkerBusy(QString networkName);

#ifdef BUILD_SERVER_ENABLED

        /**
        * @brief Add containers to a ship
        * @param networkName Network containing the ship
        * @param shipID Ship identifier
        * @param json Container configuration in JSON format
        * @note Only available when BUILD_SERVER_ENABLED is defined
        */
        static void addContainersToShip(QString networkName,
                                        QString shipID,
                                        QJsonObject json);
#endif

        /**
        * @brief Check if a network is loaded
        * @param networkName Network identifier
        * @return true if network exists and is loaded
        */
        static bool isNetworkLoaded(QString networkName);


        /**
        * @brief Reset the API to initial state
        * @note Cleans up all resources and creates new instance
        */
        static void resetAPI();
    };

    /**
    * @class SimulatorAPI::ContinuousMode
    * @brief Provides continuous maritime simulation control with pause/resume
    * capabilities
    *
    * @details The ContinuousMode class offers a static interface for running
    * simulations continuously until completion or manual intervention.
    * This mode is ideal for:
    * - Production simulations
    * - Long-running scenarios
    * - Automated simulation execution
    */
    class SHIPNETSIM_EXPORT ContinuousMode {
    public:
        /**
        * @brief Get the singleton instance of SimulatorAPI
        * @return Reference to the SimulatorAPI singleton
        */
        static SimulatorAPI& getInstance();

        /**
        * @brief Create a new simulation environment with explicit network file
        * @param networkFilePath Path to the network configuration file
        * @param networkName Unique identifier for the network
        * @param shipList List of ships to initialize with the simulation
        * @param timeStep Simulation time step duration (default: 1.0 second)
        * @param isExternallyControlled Whether simulation is controlled
        *                               externally
        * @param mode Synchronization mode for signal emission
        * @throws Emits errorOccurred if network file cannot be loaded or
        *               name conflicts
        * @note Unlike InteractiveMode, shipList is required in ContinuousMode
        */
        static void createNewSimulationEnvironment(
            QString networkFilePath,
            QString networkName,
            QVector<std::shared_ptr<ShipNetSimCore::Ship>> shipList,
            units::time::second_t timeStep = units::time::second_t(1.0),
            bool isExternallyControlled = false,
            Mode mode = Mode::Async);

        /**
        * @brief Create a new simulation environment using existing network
        * @param networkName Identifier of an already loaded network
        * @param shipList List of ships to initialize with the simulation
        * @param timeStep Simulation time step duration (default: 1.0 second)
        * @param isExternallyControlled Whether simulation is controlled
        *                               externally
        * @param mode Synchronization mode for signal emission
        * @throws Emits errorOccurred if network name doesn't exist
        */
        static void createNewSimulationEnvironment(
            QString networkName,
            QVector<std::shared_ptr<ShipNetSimCore::Ship>> shipList,
            units::time::second_t timeStep = units::time::second_t(1.0),
            bool isExternallyControlled = false,
            Mode mode = Mode::Async);

        /**
        * @brief Load a network configuration
        * @param filePath Path to network configuration file
        * @param networkName Unique identifier for the network
        * @return Pointer to the loaded network instance
        * @throws Emits errorOccurred if file cannot be loaded
        */
        static ShipNetSimCore::OptimizedNetwork*
        loadNetwork(QString filePath, QString networkName);

        /**
        * @brief Add ships to an existing simulation
        * @param networkName Target network identifier
        * @param ships List of ships to add
        * @throws Emits errorOccurred if network doesn't exist
        */
        static void addShipToSimulation(
            QString networkName,
            QVector<std::shared_ptr<ShipNetSimCore::Ship>> ships);

        /**
        * @brief Start continuous simulation execution
        * @param networkNames List of networks to run
        * @note Simulation will run until completion or manual intervention
        * @note Use "*" in networkNames to run all loaded networks
        */
        static void runSimulation(QVector<QString> networkNames);

        /**
        * @brief Pause running simulations
        * @param networkNames List of networks to pause
        * @note Simulation state is preserved and can be resumed
        */
        static void pauseSimulation(QVector<QString> networkNames);

        /**
        * @brief Resume paused simulations
        * @param networkNames List of networks to resume
        * @note Continues simulation from last paused state
        */
        static void resumeSimulation(QVector<QString> networkNames);

        /**
        * @brief Forcefully terminate running simulations
        * @param networkNames List of networks to terminate
        * @note Cannot be resumed after termination
        */
        static void terminateSimulation(QVector<QString> networkNames);

        /**
        * @brief Get simulator instance for a network
        * @param networkName Network identifier
        * @return Pointer to simulator or nullptr if not found
        */
        static ShipNetSimCore::Simulator* getSimulator(QString networkName);

        /**
        * @brief Get network instance
        * @param networkName Network identifier
        * @return Pointer to network or nullptr if not found
        */
        static ShipNetSimCore::OptimizedNetwork*
        getNetwork(QString networkName);

        /**
        * @brief Request available ports information
        * @param networkNames List of networks to query
        * @param getOnlyPortsOnShipsPaths If true, returns only ports
        *                                   on ship routes
        * @note Results provided via availablePorts signal
        */
        static void requestAvailablePorts(QVector<QString> networkNames,
                                          bool getOnlyPortsOnShipsPaths);

        /**
        * @brief Check if network's worker thread is busy
        * @param networkName Network identifier
        * @return true if worker is busy, false otherwise
        */
        static bool isWorkerBusy(QString networkName);

#ifdef BUILD_SERVER_ENABLED
        /**
        * @brief Add containers to a ship
        * @param networkName Network containing the ship
        * @param shipID Ship identifier
        * @param json Container configuration in JSON format
        * @note Only available when BUILD_SERVER_ENABLED is defined
        */
        static void addContainersToShip(QString networkName,
                                        QString shipID,
                                        QJsonObject json);
#endif

        /**
        * @brief Check if a network is loaded
        * @param networkName Network identifier
        * @return true if network exists and is loaded
        */
        static bool isNetworkLoaded(QString networkName);

        /**
        * @brief Reset the API to initial state
        * @note Cleans up all resources and creates new instance
        */
        static void resetAPI();
    };

private:
    static QBasicMutex s_instanceMutex;
    static std::unique_ptr<SimulatorAPI> instance;
    static void registerQMeta();
    APIData& getApiDataAndEnsureThread(const QString &networkName);
    static void setLocale();
    void setupShipsConnection(
        QVector<std::shared_ptr<ShipNetSimCore::Ship>> ships,
        QString networkName, Mode mode);
};

#endif // SIMULATORAPI_H
