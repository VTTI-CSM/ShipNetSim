#include "simulatorapi.h"
#include "./ship/shipsList.h"
#include "utils/shipscommon.h"
#include <QThread>

SimulatorAPI::Mode SimulatorAPI::mMode = Mode::Sync;

void SimulatorAPI::
    createNewSimulationEnvironment(
        QString networkFilePath,
        QString networkName,
        QVector<std::shared_ptr<ShipNetSimCore::Ship>> shipList,
        units::time::second_t timeStep,
        bool isExternallyControlled,
        Mode mode)
{
    // Set locale to US format (comma for thousands separator, dot for decimals)
    QLocale::setDefault(QLocale(QLocale::English, QLocale::UnitedStates));
    std::locale::global(std::locale("en_US.UTF-8"));
    std::cout.imbue(std::locale());

    if (mData.contains(networkName)) {
        emit errorOccurred("A network with name " + networkName + " exists!");
        return;
    }

    auto net = loadNetwork(networkFilePath, networkName);

    setupSimulator(net, shipList, timeStep, isExternallyControlled, mode);

    emit simulationCreated(networkName);
}

void SimulatorAPI::createNewSimulationEnvironment(
    ShipNetSimCore::OptimizedNetwork *network,
    QVector<std::shared_ptr<ShipNetSimCore::Ship> > shipList,
    units::time::second_t timeStep,
    bool isExternallyControlled,
    Mode mode)
{
    // Set locale to US format (comma for thousands separator, dot for decimals)
    QLocale::setDefault(QLocale(QLocale::English, QLocale::UnitedStates));
    std::locale::global(std::locale("en_US.UTF-8"));
    std::cout.imbue(std::locale());

    if (network && mData.contains(network->getRegionName())) {
        emit errorOccurred("A network with name " +
                           network->getRegionName() + " exists!");
        return;
    }

    setupSimulator(network, shipList, timeStep, isExternallyControlled, mode);

    emit simulationCreated(network->getRegionName());
}

ShipNetSimCore::OptimizedNetwork *SimulatorAPI::loadNetwork(QString filePath,
                                                            QString networkName)
{
    std::cout << "Reading Network: " << networkName.toStdString()
              << "!                    \r";
    auto net = new ShipNetSimCore::OptimizedNetwork(filePath);
    net->setRegionName(networkName);
    emit networkLoaded(networkName);
    return net;
}

QVector<std::shared_ptr<ShipNetSimCore::Ship> >
SimulatorAPI::loadShips(QJsonObject &ships,
                        QString networkName)
{
    auto net = getInstance().getNetwork(networkName);

    return ShipNetSimCore::ShipsList::loadShipsFromJson(ships,
                                                        net,
                                                        false);
}

QVector<std::shared_ptr<ShipNetSimCore::Ship> >
SimulatorAPI::loadShips(QJsonObject &ships,
                        ShipNetSimCore::OptimizedNetwork* network)
{
    return ShipNetSimCore::ShipsList::loadShipsFromJson(ships,
                                                        network,
                                                        false);
}

QVector<std::shared_ptr<ShipNetSimCore::Ship> >
SimulatorAPI::loadShips(QVector<QMap<QString, QString> > ships,
                        QString networkName)
{
    auto net = getInstance().getNetwork(networkName);

    return ShipNetSimCore::ShipsList::loadShipsFromParameters(ships,
                                                              net,
                                                              false);
}

QVector<std::shared_ptr<ShipNetSimCore::Ship> >
SimulatorAPI::loadShips(QVector<QMap<QString, std::any> > ships,
                        QString networkName)
{
    auto net = getInstance().getNetwork(networkName);

    return ShipNetSimCore::ShipsList::loadShipsFromParameters(ships,
                                                              net,
                                                              false);
}

QVector<std::shared_ptr<ShipNetSimCore::Ship> >
SimulatorAPI::loadShips(QString shipsFilePath,
                        QString networkName)
{
    auto net = getInstance().getNetwork(networkName);

    auto ships = ShipNetSimCore::ShipsList::readShipsFile(shipsFilePath,
                                                          net,
                                                          false);

    return ShipNetSimCore::ShipsList::loadShipsFromParameters(ships,
                                                              net,
                                                              false);
}

void SimulatorAPI::
    setupSimulator(ShipNetSimCore::OptimizedNetwork *network,
                   QVector<std::shared_ptr<ShipNetSimCore::Ship>> &shipList,
                   units::time::second_t timeStep, bool isExternallyControlled,
                   Mode mode) {

    qRegisterMetaType<ShipsResults>("ShipsResults");
    APIData apiData;

    QString netName = (network)? network->getRegionName() : "Not Defined";

    std::cout << "Define Simulator Space for network: "
              << netName.toStdString()
              << "!                          \r";
    for (const auto& ship : shipList) {
        apiData.ships[ship->getUserID()] = ship;
    }
    apiData.network = network;
    apiData.simulator =
        new ShipNetSimCore::Simulator(apiData.network, shipList,
                                      timeStep, isExternallyControlled);
    apiData.workerThread = new QThread(this);

    mData.insert(netName, std::move(apiData));

    // Set up permanent connections for this network
    setupConnections(netName, mode);

    connect(mData[netName].workerThread, &QThread::finished,
            this, [this, netName, mode]() {
        checkAndEmitSignal(m_completedSimulatorWorkerFinished,
                           m_totalSimulatorWorkerFinished.size(),
                           m_totalSimulatorWorkerFinished,
                           &SimulatorAPI::workersReady,
                           mode);
    });
    mData[netName].simulator->moveObjectToThread(
        mData[netName].workerThread);
    mData[netName].network->moveObjectToThread(
        mData[netName].workerThread);
    mData[netName].workerThread->start();
}

void SimulatorAPI::setupConnections(const QString& networkName, Mode mode)
{
    auto& simulator = mData[networkName].simulator;

    connect(simulator, &ShipNetSimCore::Simulator::simulationResultsAvailable, this,
            [this, networkName, &mode](ShipsResults results) {
                handleResultsAvailable(networkName, results, mode);
            });

    connect(simulator, &ShipNetSimCore::Simulator::simulationFinished, this,
            [this, networkName]() {
                emit simulationFinished(networkName);
            });

    connect(simulator, &ShipNetSimCore::Simulator::simulationReachedReportingTime, this,
            [this, networkName, &mode]
            (units::time::second_t currentSimulatorTime) {
                handleOneTimeStepCompleted(networkName,
                                           currentSimulatorTime, mode);
            });

    connect(simulator, &ShipNetSimCore::Simulator::simulationPaused, this,
            [this, networkName, &mode]() {
                m_completedSimulatorsPaused++;
                checkAndEmitSignal(m_completedSimulatorsPaused,
                                   m_totalSimulatorPauseRequested.size(),
                                   m_totalSimulatorPauseRequested,
                                   &SimulatorAPI::simulationsPaused,
                                   mode);
            });

    connect(simulator, &ShipNetSimCore::Simulator::simulationResumed, this,
            [this, networkName, &mode]() {
                m_completedSimulatorsResumed++;
                checkAndEmitSignal(m_completedSimulatorsResumed,
                                   m_totalSimulatorResumeRequested.size(),
                                   m_totalSimulatorResumeRequested,
                                   &SimulatorAPI::simulationsResumed,
                                   mode);
            });

    connect(simulator, &ShipNetSimCore::Simulator::simulationStopped, this,
            [this, networkName, &mode]() {
                m_completedSimulatorsEnded++;
                checkAndEmitSignal(m_completedSimulatorsEnded,
                                   m_totalSimulatorEndRequested.size(),
                                   m_totalSimulatorEndRequested,
                                   &SimulatorAPI::simulationsEnded,
                                   mode);
            });

    connect(simulator, &ShipNetSimCore::Simulator::simulationRestarted, this,
            [this, networkName, &mode]() {
                m_completedSimulatorRestarted++;
                checkAndEmitSignal(m_completedSimulatorRestarted,
                                   m_totalSimulatorRestartedRequested.size(),
                                   m_totalSimulatorRestartedRequested,
                                   &SimulatorAPI::simulationsRestarted,
                                   mode);
            });

    for (const auto& ship : mData[networkName].ships) {
        connect(ship.get(), &ShipNetSimCore::Ship::reachedDestination, this,
                [this, networkName, ship, &mode](const QJsonObject shipState) {
                    handleShipReachedDestination(
                        networkName,
                        shipState,
                        mode);
                });
    }
}

SimulatorAPI::~SimulatorAPI() {
    for (auto& data : mData) {
        data.workerThread->quit();
        data.workerThread->wait();
        delete data.workerThread;

        if (data.simulator) {
            delete data.simulator;
        }

        if (data.network) {
            delete data.network;
        }
    }
}

ShipNetSimCore::Simulator* SimulatorAPI::getSimulator(QString networkName)
{
    if (!mData.contains(networkName)) {
        emit errorOccurred("A network with name " + networkName +
                           " does not exist!");
        return nullptr;
    }
    return mData[networkName].simulator;
}

ShipNetSimCore::OptimizedNetwork* SimulatorAPI::getNetwork(QString networkName)
{
    if (!mData.contains(networkName)) {
        emit errorOccurred("A network with name " + networkName +
                           " does not exist!");
        return nullptr;
    }
    return mData[networkName].network;
}

void SimulatorAPI::requestSimulationCurrentResults(QVector<QString> networkNames)
{
    if (networkNames.contains("*")) {
        networkNames = mData.keys();
    }
    for (const auto &networkName: networkNames) {
        if (!mData.contains(networkName)) {
            emit errorOccurred("A network with name " + networkName +
                               " does not exist!");
            return;
        }
        if (mData[networkName].simulator) {
            QMetaObject::invokeMethod(mData[networkName].simulator,
                                      "generateSummaryData", Qt::QueuedConnection);
        }
    }
}

void SimulatorAPI::restartSimulations(QVector<QString> networkNames)
{
    if (networkNames.contains("*")) {
        networkNames = mData.keys();
    }
    for (const auto &networkName: networkNames) {
        if (!mData.contains(networkName)) {
            emit errorOccurred("A network with name " + networkName +
                               " does not exist!");
            return;
        }
        if (mData[networkName].simulator) {
            QMetaObject::invokeMethod(mData[networkName].simulator,
                                      "restartSimulation",
                                      Qt::QueuedConnection);
        }
    }

}

void SimulatorAPI::addShipToSimulation(
    QString networkName,
    QVector<std::shared_ptr<ShipNetSimCore::Ship>> ships)
{
    if (!mData.contains(networkName)) {
        emit errorOccurred("A network with name " + networkName +
                           " does not exist!");
        return;
    }

    QVector<QString> IDs;
    for (auto ship: ships) {
        connect(ship.get(), &ShipNetSimCore::Ship::reachedDestination,
                [this, ship, &networkName](const QJsonObject shipDetails) {
                    QString ID = ship->getUserID();

                    handleShipReachedDestination(networkName,
                                                 shipDetails,
                                                 mMode);
                });

        mData[networkName].ships.insert(ship->getUserID(), ship);

        QMetaObject::invokeMethod(mData[networkName].simulator,
                                  "addShipToSimulation", Qt::QueuedConnection,
                                  Q_ARG(std::shared_ptr<ShipNetSimCore::Ship>, ship));

        IDs.push_back(ship->getUserID());
    }

    emit shipsAddedToSimulation(networkName, IDs);
}

std::any SimulatorAPI::convertQVariantToAny(const QVariant& variant) {
    switch (variant.typeId()) {
    case QMetaType::Int:
        return std::any(variant.toInt());
    case QMetaType::Double:
        return std::any(variant.toDouble());
    case QMetaType::Bool:
        return std::any(variant.toBool());
    case QMetaType::QString:
        return std::any(variant.toString().toStdString());
    case QMetaType::QVariantList: {
        QVariantList qList = variant.toList();
        std::vector<std::any> anyList;
        for (const QVariant& qVar : qList) {
            anyList.push_back(convertQVariantToAny(qVar));
        }
        return std::any(anyList);
    }
    case QMetaType::QVariantMap: {
        QVariantMap qMap = variant.toMap();
        std::map<std::string, std::any> anyMap;
        for (auto it = qMap.begin(); it != qMap.end(); ++it) {
            anyMap[it.key().toStdString()] = convertQVariantToAny(it.value());
        }
        return std::any(anyMap);
    }
    default:
        return std::any();
    }
}

std::map<std::string, std::any>
SimulatorAPI::convertQMapToStdMap(const QMap<QString, QVariant>& qMap) {
    std::map<std::string, std::any> stdMap;

    for (auto it = qMap.begin(); it != qMap.end(); ++it) {
        std::string key = it.key().toStdString();
        std::any value = convertQVariantToAny(it.value());
        stdMap[key] = value;
    }

    return stdMap;
}

bool SimulatorAPI::isWorkerBusy(QString networkName)
{
    if (mData.contains(networkName)) {
        return mData[networkName].workerThread->isRunning();
    } else {
        emit errorOccurred("Network with name " + networkName
                           + " does not exist!");
        return false;
    }
}

std::shared_ptr<ShipNetSimCore::Ship> SimulatorAPI::getShipByID(
    QString networkName, QString& shipID)
{
    if (!mData.contains(networkName)) {
        emit errorOccurred("A network with name " + networkName +
                           " does not exist!");
        return nullptr;
    }
    if (mData[networkName].ships.contains(shipID)) {
        return mData[networkName].ships.value(shipID);
    }
    return nullptr;
}

QVector<std::shared_ptr<ShipNetSimCore::Ship>>
SimulatorAPI::getAllShips(QString networkName)
{
    if (!mData.contains(networkName)) {
        emit errorOccurred("A network with name " + networkName +
                           " does not exist!");
        return QVector<std::shared_ptr<ShipNetSimCore::Ship>>();
    }
    return QVector<std::shared_ptr<ShipNetSimCore::Ship>>(
        mData[networkName].ships.values().begin(),
        mData[networkName].ships.values().end());
}

QJsonObject SimulatorAPI::requestShipCurrentStateByID(QString networkName,
                                                      QString ID)
{
    if (!mData.contains(networkName)) {
        emit errorOccurred("A network with name " + networkName +
                           " does not exist!");
        return QJsonObject();
    }
    if (mData[networkName].ships.contains(ID)) {
        auto out = mData[networkName].ships.value(ID)->getCurrentStateAsJson();
        shipCurrentStateAvailable(out);
        return out;
    }
    emit errorOccurred("A ship with ID " + ID +
                       " does not exist!");
    return QJsonObject();
}

QJsonObject SimulatorAPI::requestSimulatorCurrentState(QString networkName)
{
    if (!mData.contains(networkName)) {
        emit errorOccurred("A network with name " + networkName +
                           " does not exist!");
        return QJsonObject();
    }

    auto out = mData[networkName].simulator->getCurrentStateAsJson();
    emit simulationCurrentStateAvailable(out);
    return out;
}

void SimulatorAPI::addContainersToShip(QString networkName, QString shipID,
                                       QJsonObject json)
{
    getShipByID(networkName, shipID)->addContainers(json);
    emit containersAddedToShip(networkName, shipID);
}

void SimulatorAPI::handleShipReachedDestination(QString networkName,
                                                QJsonObject shipState,
                                                Mode mode)
{
    // Retrieve the current QJsonObject for networkName,
    // or create a new one if it doesn't exist
    QJsonObject& networkObject = m_shipsReachedBuffer[networkName];

    // Check if "ships" array exists in the networkObject; if not, create it
    QJsonArray shipsArray = networkObject.value("shipStates").toArray();

    // Append the new shipState to the ships array
    shipsArray.append(shipState);

    // Update the "ships" array in the networkObject
    networkObject["shipStates"] = shipsArray;

    // Reassign the updated networkObject to m_shipsReachedBuffer
    m_shipsReachedBuffer[networkName] = networkObject;

    if (mode == Mode::Sync) {
        QJsonObject concatenatedObject;

        // Iterate over each entry in m_shipsReachedBuffer
        for (auto it = m_shipsReachedBuffer.constBegin();
             it != m_shipsReachedBuffer.constEnd(); ++it) {
            // Insert each key-value pair into the concatenatedObject
            concatenatedObject.insert(it.key(), it.value());
        }
        emit shipsReachedDestination(concatenatedObject);
        m_shipsReachedBuffer.clear();
    }
}

void SimulatorAPI::handleOneTimeStepCompleted(
    QString networkName, units::time::second_t currentSimulatorTime,
    Mode mode)
{
    m_timeStepData.insert(networkName, currentSimulatorTime);

    switch (mode) {
    case Mode::Async:
        // Increment the number of completed simulators
        m_completedSimulatorsTimeStep++;

        // Check if all simulators have completed their time step
        if (m_completedSimulatorsTimeStep == mData.size()) {
            // Emit the aggregated signal
            emit simulationAdvanced(m_timeStepData);

            // Emit the aggregated signal
            emitShipsReachedDestination();

            m_completedSimulatorsTimeStep = 0;  // Reset the counter for the next time step
        }
        break;

    case Mode::Sync:
        // In sync mode, emit results immediately for each network
        emit simulationAdvanced(m_timeStepData);
        break;

    default:
        // Handle unexpected mode
        qWarning() << "Unexpected mode in handleResultsAvailable";
        break;
    }
}

void SimulatorAPI::handleResultsAvailable(QString networkName,
                                          ShipsResults result,
                                          Mode mode)
{

    // Always store the result in the buffer
    m_simulationResultBuffer.insert(networkName, result);

    switch (mode) {
    case Mode::Async:
        // Increment the number of completed simulators
        m_completedSummaryResults++;

        // Check if all simulators have completed their time step
        if (m_completedSummaryResults == mData.size()) {
            // Emit the aggregated signal
            emitSimulationResults(m_simulationResultBuffer);
            m_completedSummaryResults = 0;  // Reset the counter for the next time step
        }
        break;

    case Mode::Sync:
        // In sync mode, emit results immediately for each network
        emitSimulationResults(m_simulationResultBuffer);
        break;

    default:
        // Handle unexpected mode
        qWarning() << "Unexpected mode in handleResultsAvailable";
        break;
    }

}

void SimulatorAPI::emitShipsReachedDestination()
{
    if (!m_shipsReachedBuffer.isEmpty()) {
        QJsonObject concatenatedObject;

        // Iterate over each entry in m_shipsReachedBuffer
        for (auto it = m_shipsReachedBuffer.constBegin();
             it != m_shipsReachedBuffer.constEnd(); ++it) {
            // Insert each key-value pair into the concatenatedObject
            concatenatedObject.insert(it.key(), it.value());
        }

        emit shipsReachedDestination(concatenatedObject);
        m_shipsReachedBuffer.clear();
    }
}

void SimulatorAPI::emitSimulationResults(QMap<QString, ShipsResults> results)
{
    if (!results.isEmpty()) {
        emit simulationResultsAvailable(results);
        results.clear();
    }
}

void SimulatorAPI::checkAndEmitSignal(
    int& counter, int total,
    const QVector<QString>& networkNames,
    void(SimulatorAPI::*signal)(QVector<QString>),
    Mode mode) {

    switch (mode) {
    case Mode::Async:
        // In async mode, emit signal when counter reaches total
        if (counter == total) {
            (this->*signal)(networkNames);
        }
        break;

    case Mode::Sync:
        // In sync mode, emit signal immediately
        (this->*signal)(networkNames);
        break;

    default:
        // Handle unexpected mode
        qWarning() << "Unexpected mode in checkAndEmitSignal";
        break;
    }
}

// -----------------------------------------------------------------------------
// ---------------------------- Interactive Mode -------------------------------
// -----------------------------------------------------------------------------

SimulatorAPI& SimulatorAPI::InteractiveMode::getInstance() {
    return SimulatorAPI::getInstance();
}

void SimulatorAPI::InteractiveMode::createNewSimulationEnvironment(
    QString networkFilePath, QString networkName,
    QVector<std::shared_ptr<ShipNetSimCore::Ship>> shipList,
    units::time::second_t timeStep,
    bool isExternallyControlled,
     Mode mode)
{
    mMode = mode;

    getInstance().createNewSimulationEnvironment(networkFilePath,
                                                 networkName,
                                                 shipList,
                                                 timeStep,
                                                 isExternallyControlled,
                                                 mode);
}

void SimulatorAPI::InteractiveMode::createNewSimulationEnvironment(
    ShipNetSimCore::OptimizedNetwork *network,
    QVector<std::shared_ptr<ShipNetSimCore::Ship> > shipList,
    units::time::second_t timeStep,
    bool isExternallyControlled,
    Mode mode)
{
    mMode = mode;
    getInstance().createNewSimulationEnvironment(network,
                                                 shipList,
                                                 timeStep,
                                                 isExternallyControlled,
                                                 mode);
}

ShipNetSimCore::OptimizedNetwork*
SimulatorAPI::InteractiveMode::loadNetwork(
    QString filePath, QString networkName)
{
    return getInstance().loadNetwork(filePath, networkName);
}

void SimulatorAPI::InteractiveMode::addShipToSimulation(
    QString networkName, QVector<std::shared_ptr<ShipNetSimCore::Ship>> ships)
{
    getInstance().addShipToSimulation(networkName, ships);
}

void SimulatorAPI::InteractiveMode::
    runOneTimeStep(QVector<QString> networkNames)
{
    if (networkNames.contains("*")) {
        networkNames = getInstance().mData.keys();
    }
    for (const auto &networkName: networkNames) {
        if (!getInstance().mData.contains(networkName)) {
            emit getInstance().errorOccurred("A network with name "
                                             + networkName +
                                             " does not exist!");
            return;
        }
        if (getInstance().mData[networkName].simulator) {
            QMetaObject::invokeMethod(
                getInstance().mData[networkName].simulator,
                "runOneTimeStep", Qt::QueuedConnection);
        }
    }
}

void SimulatorAPI::InteractiveMode::
    runSimulation(QVector<QString> networkNames,
                  units::time::second_t timeSteps) {
    if (networkNames.contains("*")) {
        networkNames = getInstance().mData.keys();
    }
    for (const auto &networkName: networkNames) {
        if (!getInstance().mData.contains(networkName)) {
            emit getInstance().errorOccurred("A network with name "
                                             + networkName +
                                             " does not exist!");
            return;
        }
        if (timeSteps.value() < 0) {
            if (getInstance().mData[networkName].simulator) {
                QMetaObject::invokeMethod(
                    getInstance().mData[networkName].simulator,
                    "runSimulation", Qt::QueuedConnection);
            }
        }
        else {
            if (getInstance().mData[networkName].simulator) {
                QMetaObject::invokeMethod(
                    getInstance().mData[networkName].simulator,
                    "runBy", Qt::QueuedConnection,
                    Q_ARG(units::time::second_t, timeSteps));
            }
        }

    }
}

void SimulatorAPI::InteractiveMode::
    initSimulation(QVector<QString> networkNames)
{
    if (networkNames.contains("*")) {
        networkNames = getInstance().mData.keys();
        getInstance().m_totalSimulatorsInitializationRequested = networkNames;
    }

    getInstance().m_completedSimulatorsInitialization = 0;

    for (const auto &networkName: networkNames) {
        if (!getInstance().mData.contains(networkName)) {
            emit getInstance().errorOccurred("A network with name "
                                             + networkName +
                                             " does not exist!");
            return;
        }
        if (getInstance().mData[networkName].simulator) {
            QMetaObject::invokeMethod(
                getInstance().mData[networkName].simulator,
                "initializeSimulation",
                Qt::QueuedConnection);
        }
    }
}

void SimulatorAPI::InteractiveMode::endSimulation(QVector<QString> networkNames)
{
    if (networkNames.contains("*")) {
        networkNames = getInstance().mData.keys();
    }

    getInstance().m_completedSimulatorsEnded = 0;
    getInstance().m_totalSimulatorEndRequested = networkNames;

    for (const auto &networkName: networkNames) {
        if (!getInstance().mData.contains(networkName)) {
            emit getInstance().errorOccurred("A network with name "
                                             + networkName +
                                             " does not exist!");
            return;
        }
        if (getInstance().mData[networkName].simulator) {
            QMetaObject::invokeMethod(
                getInstance().mData[networkName].simulator,
                "finalizeSimulation",
                Qt::QueuedConnection);
        }
    }
}

ShipNetSimCore::Simulator*
SimulatorAPI::InteractiveMode::getSimulator(QString networkName) {
    return getInstance().getSimulator(networkName);
}

ShipNetSimCore::OptimizedNetwork*
SimulatorAPI::InteractiveMode::getNetwork(QString networkName) {
    return getInstance().getNetwork(networkName);
}

std::shared_ptr<ShipNetSimCore::Ship>
SimulatorAPI::InteractiveMode::getShipByID(
    QString networkName, QString& shipID)
{
    return getInstance().getShipByID(networkName, shipID);
}

QVector<std::shared_ptr<ShipNetSimCore::Ship>>
SimulatorAPI::InteractiveMode::getAllTrains(QString networkName)
{
    return getInstance().getAllShips(networkName);
}

bool SimulatorAPI::InteractiveMode::isWorkerBusy(QString networkName)
{
    return getInstance().isWorkerBusy(networkName);
}

void SimulatorAPI::InteractiveMode::addContainersToShip(QString networkName,
                                                        QString shipID,
                                                        QJsonObject json)
{
    getInstance().addContainersToShip(networkName, shipID, json);
}

// -----------------------------------------------------------------------------
// ----------------------------- Continuous Mode -------------------------------
// -----------------------------------------------------------------------------

SimulatorAPI& SimulatorAPI::ContinuousMode::getInstance() {
    return SimulatorAPI::getInstance();
}

void SimulatorAPI::ContinuousMode::createNewSimulationEnvironment(
    QString networkFilePath,
    QString networkName,
    QVector<std::shared_ptr<ShipNetSimCore::Ship>> shipList,
    units::time::second_t timeStep, bool isExternallyControlled,
    Mode mode)
{
    mMode = mode;
    getInstance().createNewSimulationEnvironment(networkFilePath,
                                                 networkName,
                                                 shipList,
                                                 timeStep,
                                                 isExternallyControlled,
                                                 mode);
}

void SimulatorAPI::ContinuousMode::createNewSimulationEnvironment(
    ShipNetSimCore::OptimizedNetwork *network,
    QVector<std::shared_ptr<ShipNetSimCore::Ship> > shipList,
    units::time::second_t timeStep,
    bool isExternallyControlled, Mode mode)
{
    mMode = mode;
    getInstance().createNewSimulationEnvironment(network,
                                                 shipList,
                                                 timeStep,
                                                 isExternallyControlled,
                                                 mode);
}

ShipNetSimCore::OptimizedNetwork*
SimulatorAPI::ContinuousMode::loadNetwork(
    QString filePath, QString networkName)
{
    return getInstance().loadNetwork(filePath, networkName);
}

void SimulatorAPI::ContinuousMode::runSimulation(
    QVector<QString> networkNames)
{
    if (networkNames.contains("*")) {
        networkNames = getInstance().mData.keys();
    }

    getInstance().m_completedSimulatorsRun = 0;
    getInstance().m_totalSimulatorsRunRequested = networkNames;

    for (const auto &networkName: networkNames) {
        if (!getInstance().mData.contains(networkName)) {
            emit getInstance().errorOccurred("A network with name "
                                             + networkName +
                                             " does not exist!");
            return;
        }
        if (getInstance().mData[networkName].simulator) {
            QMetaObject::invokeMethod(
                getInstance().mData[networkName].simulator,
                "runSimulation", Qt::QueuedConnection);
        }
    }

}

void SimulatorAPI::ContinuousMode::
    pauseSimulation(QVector<QString> networkNames)
{
    if (networkNames.contains("*")) {
        networkNames = getInstance().mData.keys();
    }

    getInstance().m_completedSimulatorsPaused = 0;
    getInstance().m_totalSimulatorPauseRequested = networkNames;

    for (const auto &networkName: networkNames) {
        if (!getInstance().mData.contains(networkName)) {
            emit getInstance().errorOccurred("A network with name "
                                             + networkName +
                                             " does not exist!");
            return;
        }
        if (getInstance().mData[networkName].simulator) {
            QMetaObject::invokeMethod(
                getInstance().mData[networkName].simulator,
                "pauseSimulation", Qt::QueuedConnection);
        }
    }
}

void SimulatorAPI::ContinuousMode::
    resumeSimulation(QVector<QString> networkNames)
{
    if (networkNames.contains("*")) {
        networkNames = getInstance().mData.keys();
    }

    getInstance().m_completedSimulatorsResumed = 0;
    getInstance().m_totalSimulatorResumeRequested = networkNames;

    for (const auto &networkName: networkNames) {
        if (!getInstance().mData.contains(networkName)) {
            emit getInstance().errorOccurred("A network with name "
                                             + networkName +
                                             " does not exist!");
            return;
        }
        if (getInstance().mData[networkName].simulator) {
            QMetaObject::invokeMethod(
                getInstance().mData[networkName].simulator,
                "resumeSimulation", Qt::QueuedConnection);
        }
    }
}

void SimulatorAPI::ContinuousMode::endSimulation(QVector<QString> networkNames)
{
    if (networkNames.contains("*")) {
        networkNames = getInstance().mData.keys();
    }

    getInstance().m_completedSimulatorsEnded = 0;
    getInstance().m_totalSimulatorEndRequested = networkNames;

    for (const auto &networkName: networkNames) {
        if (!getInstance().mData.contains(networkName)) {
            emit getInstance().errorOccurred("A network with name "
                                             + networkName +
                                             " does not exist!");
            return;
        }
        if (getInstance().mData[networkName].simulator) {
            QMetaObject::invokeMethod(
                getInstance().mData[networkName].simulator,
                "finalizeSimulation",
                Qt::QueuedConnection);
        }
    }
}

ShipNetSimCore::Simulator*
SimulatorAPI::ContinuousMode::getSimulator(QString networkName) {
    return getInstance().getSimulator(networkName);
}

ShipNetSimCore::OptimizedNetwork*
SimulatorAPI::ContinuousMode::getNetwork(QString networkName) {
    return getInstance().getNetwork(networkName);
}

bool SimulatorAPI::ContinuousMode::isWorkerBusy(QString networkName)
{
    return getInstance().isWorkerBusy(networkName);
}

void SimulatorAPI::ContinuousMode::addContainersToShip(QString networkName,
                                                       QString shipID,
                                                       QJsonObject json)
{
    getInstance().addContainersToShip(networkName, shipID, json);
}
