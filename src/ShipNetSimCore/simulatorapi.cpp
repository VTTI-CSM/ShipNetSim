#include "simulatorapi.h"
#include "./ship/shipsList.h"
#include "shiploaderworker.h"
#include "utils/shipscommon.h"
#include <QThread>

SimulatorAPI::Mode SimulatorAPI::mMode = Mode::Sync;
std::unique_ptr<SimulatorAPI> SimulatorAPI::instance(new SimulatorAPI());

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

    if (!mData.contains(networkName)) {
        emit errorOccurred("A network with name " + networkName
                           + " does not exist!"
                           + "\nUse loadNetwork() first!");
        return;
    }

    loadNetwork(networkFilePath, networkName);

    setupSimulator(networkName, shipList, timeStep,
                   isExternallyControlled, mode);

    emit simulationCreated(networkName);
}

void SimulatorAPI::registerQMeta() {
    // Register std::shared_ptr types
    qRegisterMetaType<std::shared_ptr<ShipNetSimCore::Ship>>(
        "std::shared_ptr<ShipNetSimCore::Ship>");

    // Register custom structs/classes
    qRegisterMetaType<ShipsResults>("ShipsResults");

    // Register enums
    qRegisterMetaType<SimulatorAPI::Mode>("SimulatorAPI::Mode");

    qRegisterMetaType<ShipNetSimCore::GPoint>("ShipNetSimCore::GPoint");
    qRegisterMetaType<QVector<std::pair<QString, ShipNetSimCore::GPoint>>>(
        "QVector<std::pair<QString, ShipNetSimCore::GPoint>>");
    qRegisterMetaType<ShipNetSimCore::GLine>("ShipNetSimCore::GLine");
    qRegisterMetaType<QVector<std::shared_ptr<ShipNetSimCore::GLine>>>(
        "QVector<std::shared_ptr<ShipNetSimCore::GLine>>");

    qRegisterMetaType<units::time::second_t>("units::time::second_t");
    qRegisterMetaType<units::length::meter_t>("units::length::meter_t");
    qRegisterMetaType<units::angle::degree_t>("units::angle::degree_t");
    qRegisterMetaType<QVector<units::length::meter_t>>(
        "QVector<units::length::meter_t>");
    qRegisterMetaType<ShipNetSimCore::Ship::stopPointDefinition>(
        "ShipNetSimCore::Ship::stopPointDefinition");
    qRegisterMetaType<QMap<ShipNetSimCore::Ship::ShipAppendage,
                           units::area::square_meter_t>>(
        "QMap<ShipNetSimCore::Ship::ShipAppendage, "
        "units::area::square_meter_t>");
    qRegisterMetaType<units::velocity::meters_per_second_t>(
        "units::velocity::meters_per_second_t");
    qRegisterMetaType<ShipNetSimCore::Ship::NavigationStatus>(
        "ShipNetSimCore::Ship::NavigationStatus");

    qRegisterMetaType<QJsonObject>("QJsonObject");

    qRegisterMetaType<QVector<std::shared_ptr<ShipNetSimCore::Ship>>>(
        "QVector<std::shared_ptr<ShipNetSimCore::Ship>>");

}

SimulatorAPI& SimulatorAPI::getInstance() {
    if (!instance) {
        registerQMeta();
        instance.reset(new SimulatorAPI());
    }
    return *instance;
}

void SimulatorAPI::resetInstance() {
    if (instance) {
        // Clean up existing resources
        for (auto& data : instance->mData) {
            if (data.simulator) {
                data.simulator->deleteLater();
            }
            if (data.network) {
                delete data.network;
            }
            if (data.workerThread) {
                data.workerThread->quit();
                data.workerThread->wait();
                delete data.workerThread;
            }
        }
        instance->mData.clear();

        // Destroy the current instance
        instance.reset();
    }

    registerQMeta();
    // Create a new instance
    instance.reset(new SimulatorAPI());
}

void SimulatorAPI::createNewSimulationEnvironment(
    QString networkName,
    QVector<std::shared_ptr<ShipNetSimCore::Ship> > shipList,
    units::time::second_t timeStep,
    bool isExternallyControlled,
    Mode mode)
{
    // Set locale to US format (comma for thousands separator, dot for decimals)
    QLocale::setDefault(QLocale(QLocale::English, QLocale::UnitedStates));
    std::locale::global(std::locale("en_US.UTF-8"));
    std::cout.imbue(std::locale());

    if (!mData.contains(networkName)) {
        emit errorOccurred("A network with name " + networkName
                           + " does not exist!"
                           + "\nUse loadNetwork() first!");
        return;
    }

    setupSimulator(networkName, shipList, timeStep,
                   isExternallyControlled, mode);

    emit simulationCreated(networkName);
}

ShipNetSimCore::OptimizedNetwork* SimulatorAPI::loadNetwork(
    QString filePath, QString networkName)
{
    std::cout << "Reading Network: " << networkName.toStdString() << "!\r";

    // Create a new thread for network loading
    QThread* networkThread = new QThread(this);

    // Store the network thread in APIData before network creation
    APIData apiData;
    apiData.workerThread = networkThread;
    mData.insert(networkName, std::move(apiData));

    // Use an event loop to wait for network initialization
    QEventLoop loop;

    // Create the network instance **inside the main thread first**
    ShipNetSimCore::OptimizedNetwork* net =
        new ShipNetSimCore::OptimizedNetwork();

    // Move it to the worker thread`
    net->moveToThread(networkThread);

    // Create the workers for later setup
    SimulatorWorker* simWorker = new SimulatorWorker();
    ShipLoaderWorker* shipLoaderWorker = new ShipLoaderWorker();

    // Move them to the worker thread
    simWorker->moveToThread(networkThread);
    shipLoaderWorker->moveToThread(networkThread);

    // Store the network, simulatorWorker, and shipLoaderWorker in APIData
    mData[networkName].network = net;
    mData[networkName].simulatorWorker = simWorker;
    mData[networkName].shipLoaderWorker = shipLoaderWorker;

    connect(networkThread, &QThread::started, mData[networkName].network,
            [networkName, filePath, this](){
        mData[networkName].network->initializeNetwork(filePath);
    });

    // Connect the network loaded signal to exit the event loop
    connect(net, &ShipNetSimCore::OptimizedNetwork::NetworkLoaded,
            &loop, &QEventLoop::quit);

    // Start the worker thread
    networkThread->start();

    // Block execution until the network is fully loaded
    loop.exec();  // This will wait for `NetworkLoaded` signal before proceeding

    qInfo() << "Network " << networkName.toStdString()
              << " loaded successfully!\n";

    // Emit signal when network is fully loaded
    emit networkLoaded(networkName);

    return net;  // The network is now fully initialized and accessible
}

APIData& SimulatorAPI::getApiDataAndEnsureThread(const QString &networkName)
{
    if (!mData.contains(networkName)) {
        throw std::runtime_error(
            QString("Network not found in APIData: %1").arg(networkName)
                .toStdString());
    }

    APIData &apiData = mData[networkName];

    if (!apiData.workerThread) {
        throw std::runtime_error(
            QString("Worker thread for %1 is null!").arg(networkName)
                .toStdString());
    }

    if (!apiData.workerThread->isRunning()) {
        apiData.workerThread->start();
    }

    if (!apiData.shipLoaderWorker) {
        throw std::runtime_error(
            QString("shipLoaderWorker for %1 is null!").arg(networkName)
                .toStdString());
    }

    return apiData;
}

void SimulatorAPI::setupShipsConnection(
    QVector<std::shared_ptr<ShipNetSimCore::Ship>> ships,
    QString networkName, Mode mode)
{
    for (const auto& ship : ships) {
        connect(ship.get(), &ShipNetSimCore::Ship::reachedDestination, this,
                [this, networkName, ship, mode](const QJsonObject shipState) {
                    qDebug() << "Current thread 9:" << QThread::currentThread();
                    handleShipReachedDestination(
                        networkName,
                        shipState,
                        mode);
                }, mConnectionType);

        connect(ship.get(), &ShipNetSimCore::Ship::positionUpdated, this,
                [this, ship](ShipNetSimCore::GPoint currentPos,
                             units::angle::degree_t heading,
                             QVector<std::shared_ptr<
                                 ShipNetSimCore::GLine>> path)
                {
                    // qDebug() << "Current thread 10:" <<
                    //     QThread::currentThread();
                    emit shipCoordinatesUpdated(
                        ship->getUserID(),
                        currentPos, heading, path);
                }, mConnectionType);
    }
}

QVector<std::shared_ptr<ShipNetSimCore::Ship>>
SimulatorAPI::mLoadShips(QJsonObject &ships, QString networkName)
{
    // 1) Prepare a default empty result in case of errors
    QVector<std::shared_ptr<ShipNetSimCore::Ship>> loadedShips;

    try {

        // 2) Ensure we have APIData for this network and
        //    retrieve the existing APIData for the network
        APIData &apiData = getApiDataAndEnsureThread(networkName);

        // --------------------------------------------------------------------
        // 3) Because 'loadShips' is async and emits a signal, we use an
        //    event loop here to wait for the result or an error.
        // --------------------------------------------------------------------
        QEventLoop loop;

        // -- Connect to the success signal
        QMetaObject::Connection connSuccess = connect(
            apiData.shipLoaderWorker,
            &ShipLoaderWorker::shipsLoaded,
            [&](QVector<std::shared_ptr<ShipNetSimCore::Ship>> shipsVec) {
                // Store the result in our local variable, then quit the loop
                loadedShips = shipsVec;
                loop.quit();
            }
            );

        // -- Connect to the error signal
        QMetaObject::Connection connError = connect(
            apiData.shipLoaderWorker,
            &ShipLoaderWorker::errorOccured,
            [&](const QString &errMsg) {
                qWarning() << "Error loading ships in mLoadShips:" << errMsg;
                // We simply quit the loop; 'loadedShips' stays empty
                loop.quit();
            }
            );

        // --------------------------------------------------------------------
        // 4) Invoke the worker’s loadShips(...) in the worker thread.
        //    We do NOT use BlockingQueuedConnection because we want the worker
        //    to emit a signal and let this thread’s event loop catch it.
        // --------------------------------------------------------------------
        QMetaObject::invokeMethod(
            apiData.shipLoaderWorker,
            [&, networkName]() {
                // This runs in the worker thread
                apiData.shipLoaderWorker->loadShips(apiData,
                                                    ships,
                                                    networkName);
            },
            Qt::QueuedConnection
            );

        // 5) Start a local event loop which will block until:
        //    - shipsLoaded(...) is emitted, or
        //    - errorOccured(...) is emitted.
        loop.exec();

        // 6) Once we exit the loop, disconnect our temporary connections
        disconnect(connSuccess);
        disconnect(connError);

        // 7) Now 'loadedShips' contains our result (if any), so return
        return loadedShips;
    } catch (const std::runtime_error &err) {
        qWarning() << err.what();
        return loadedShips;  // empty
    }
}


QVector<std::shared_ptr<ShipNetSimCore::Ship>>
SimulatorAPI::mLoadShips(QJsonObject &ships,
                         ShipNetSimCore::OptimizedNetwork* network)
{
    // 1) Prepare a default empty result in case of errors
    QVector<std::shared_ptr<ShipNetSimCore::Ship>> loadedShips;

    // 2) Ensure we have APIData for this network and
    //    retrieve the existing APIData for the network
    APIData &apiData = getApiDataAndEnsureThread(network->getRegionName());

    // validate the network is not nullptr.
    if (!network) {
        qWarning() << "Error: Network cannot be a nullptr!";
    }

    // 3) Ensure the network is in the same thread as the worker so that
    //    the connections work properlly.
    if (network->thread() != apiData.workerThread) {
        qWarning() << "Error: Network is not in the expected worker thread!"
                   << "Expected thread:" << apiData.workerThread
                   << "Actual thread:" << network->thread();
        return loadedShips;
    }


    try {
        // --------------------------------------------------------------------
        // 4) We want to call loadShips(...) in the worker thread
        //    asynchronously, but still return the result synchronously from
        //    this function. So we'll use a local QEventLoop to block until
        //    we get a signal.
        // --------------------------------------------------------------------
        QEventLoop loop;

        // 5) Connect the success signal
        QMetaObject::Connection connSuccess = connect(
            apiData.shipLoaderWorker,
            &ShipLoaderWorker::shipsLoaded,
            [&](QVector<std::shared_ptr<ShipNetSimCore::Ship>> shipsVec) {
                // Store the result in our local variable and quit the loop
                loadedShips = shipsVec;
                loop.quit();
            }
            );

        // 6) Connect the error signal
        QMetaObject::Connection connError = connect(
            apiData.shipLoaderWorker,
            &ShipLoaderWorker::errorOccured,
            [&](const QString &errorMsg) {
                qWarning() << "Error loading ships in mLoadShips:" << errorMsg;
                // We'll quit the loop, loadedShips stays empty
                loop.quit();
            }
            );

        // --------------------------------------------------------------------
        // 7) Actually call 'loadShips' via QueuedConnection
        // in the worker thread
        // --------------------------------------------------------------------
        QMetaObject::invokeMethod(
            apiData.shipLoaderWorker,
            [&, network]() {
                // This lambda runs in the worker thread
                apiData.shipLoaderWorker->loadShips(ships, network);
            },
            Qt::QueuedConnection
            );

        // 8) Block this function until either shipsLoaded
        // or errorOccured is emitted
        loop.exec();

        // 9) Disconnect the temporary connections
        disconnect(connSuccess);
        disconnect(connError);

        // 10) Return the ships we loaded (or empty if an error occurred)
        return loadedShips;

    } catch (const std::runtime_error &err) {
        qWarning() << err.what();
        return loadedShips;  // empty
    }
}


QVector<std::shared_ptr<ShipNetSimCore::Ship> >
SimulatorAPI::mLoadShips(QVector<QMap<QString, QString>> ships,
                         QString networkName)
{
    // 1) Prepare a default empty result in case of errors
    QVector<std::shared_ptr<ShipNetSimCore::Ship>> loadedShips;

    try {

        // 2) Ensure we have APIData for this network and
        //    retrieve the existing APIData for the network
        APIData &apiData = getApiDataAndEnsureThread(networkName);

        // ------------------------------------------------------------------
        // 3) Setup a local QEventLoop to wait for the result or an error
        // ------------------------------------------------------------------
        QEventLoop loop;

        // 4) Connect success signal
        QMetaObject::Connection connSuccess = connect(
            apiData.shipLoaderWorker,
            &ShipLoaderWorker::shipsLoaded,
            [&](QVector<std::shared_ptr<ShipNetSimCore::Ship>> shipsVec) {
                // Store the result, then quit the loop
                loadedShips = shipsVec;
                loop.quit();
            }
            );

        // 5) Connect error signal
        QMetaObject::Connection connError = connect(
            apiData.shipLoaderWorker,
            &ShipLoaderWorker::errorOccured,
            [&](const QString &errMsg) {
                // Log the error, then quit the loop
                qWarning() << "Error loading ships in mLoadShips:" << errMsg;
                loop.quit();
            }
            );

        // ------------------------------------------------------------------
        // 6) Invoke the worker's loadShips(...) in the worker thread
        // ------------------------------------------------------------------
        QMetaObject::invokeMethod(
            apiData.shipLoaderWorker,
            [&, networkName]() {
                // This lambda executes in the worker thread
                apiData.shipLoaderWorker->loadShips(apiData,
                                                    ships,
                                                    networkName);
            },
            Qt::QueuedConnection
            );

        // 7) Exec the local event loop (blocking until
        // shipsLoaded or errorOccured)
        loop.exec();

        // 8) Disconnect temporary connections
        disconnect(connSuccess);
        disconnect(connError);

        // 9) Return the result (either populated or empty if error)
        return loadedShips;

    } catch (const std::runtime_error &err) {
        qWarning() << err.what();
        return loadedShips;  // empty
    }
}


QVector<std::shared_ptr<ShipNetSimCore::Ship>>
SimulatorAPI::mLoadShips(QVector<QMap<QString, std::any>> ships,
                         QString networkName)
{
    // 1) Prepare a default empty result in case of errors
    QVector<std::shared_ptr<ShipNetSimCore::Ship>> loadedShips;

    try {

        // 2) Ensure we have APIData for this network and
        //    retrieve the existing APIData for the network
        APIData &apiData = getApiDataAndEnsureThread(networkName);

        // -------------------------------------------------------------------
        // 3) We'll use a local QEventLoop to block until we get
        //    shipsLoaded or errorOccured
        // -------------------------------------------------------------------
        QEventLoop loop;

        // 4) Connect the success signal
        QMetaObject::Connection connSuccess = connect(
            apiData.shipLoaderWorker,
            &ShipLoaderWorker::shipsLoaded,
            [&](QVector<std::shared_ptr<ShipNetSimCore::Ship>> shipsVec) {
                // Capture the result, then quit the loop
                loadedShips = shipsVec;
                loop.quit();
            }
            );

        // 5) Connect the error signal
        QMetaObject::Connection connError = connect(
            apiData.shipLoaderWorker,
            &ShipLoaderWorker::errorOccured,
            [&](const QString &errMsg) {
                qWarning() << "Error loading ships in mLoadShips:" << errMsg;
                // We'll still quit the loop; 'loadedShips' stays empty
                loop.quit();
            }
            );

        // -------------------------------------------------------------------
        // 6) Invoke the worker's loadShips(...) in the worker thread
        // -------------------------------------------------------------------
        QMetaObject::invokeMethod(
            apiData.shipLoaderWorker,
            [&, networkName]() {
                // This lambda runs in the worker thread
                apiData.shipLoaderWorker->loadShips(apiData, ships, networkName);
            },
            Qt::QueuedConnection
            );

        // 7) Block until shipsLoaded or errorOccured
        loop.exec();

        // 8) Disconnect temporary connections
        disconnect(connSuccess);
        disconnect(connError);

        // 9) Return the final result
        return loadedShips;

    } catch (const std::runtime_error &err) {
        qWarning() << err.what();
        return loadedShips;  // empty
    }
}

QVector<std::shared_ptr<ShipNetSimCore::Ship>>
SimulatorAPI::mLoadShips(QString shipsFilePath, QString networkName)
{
    // 1) Prepare a default empty result in case of errors
    QVector<std::shared_ptr<ShipNetSimCore::Ship>> loadedShips;

    try {

        // 2) Ensure we have APIData for this network and
        //    retrieve the existing APIData for the network
        APIData &apiData = getApiDataAndEnsureThread(networkName);

        // -------------------------------------------------------------------
        // 7) We'll use a local QEventLoop to block until
        //    shipsLoaded or errorOccured
        // -------------------------------------------------------------------
        QEventLoop loop;

        // 8) Connect the success signal
        QMetaObject::Connection connSuccess = connect(
            apiData.shipLoaderWorker,
            &ShipLoaderWorker::shipsLoaded,
            [&](QVector<std::shared_ptr<ShipNetSimCore::Ship>> shipsVec) {
                // Capture the result, then quit the loop
                loadedShips = shipsVec;
                loop.quit();
            }
            );

        // 9) Connect the error signal
        QMetaObject::Connection connError = connect(
            apiData.shipLoaderWorker,
            &ShipLoaderWorker::errorOccured,
            [&](const QString &errMsg) {
                qWarning() << "Error loading ships in mLoadShips:" << errMsg;
                // We'll still quit the loop; 'loadedShips' remains empty
                loop.quit();
            }
            );

        // -------------------------------------------------------------------
        // 10) Invoke the worker's loadShips(...) in the worker thread
        // -------------------------------------------------------------------
        QMetaObject::invokeMethod(
            apiData.shipLoaderWorker,
            [&, networkName]() {
                // This lambda runs in the worker thread
                apiData.shipLoaderWorker->loadShips(apiData,
                                                    shipsFilePath,
                                                    networkName);
            },
            Qt::QueuedConnection
            );

        // 11) Block until shipsLoaded or errorOccured
        loop.exec();

        // 12) Disconnect the temporary connections
        disconnect(connSuccess);
        disconnect(connError);

        // 13) Return the final result
        return loadedShips;

    } catch (const std::runtime_error &err) {
        qWarning() << err.what();
        return loadedShips;  // empty
    }
}


void SimulatorAPI::
    setupSimulator(QString networkName,
                   QVector<std::shared_ptr<ShipNetSimCore::Ship>> &shipList,
                   units::time::second_t timeStep, bool isExternallyControlled,
                   Mode mode) {

    qRegisterMetaType<ShipsResults>("ShipsResults");

    // Ensure the network exists in APIData
    if (!mData.contains(networkName)) {
        qWarning() << "Network not found in APIData: " << networkName;
        return;
    }

    std::cout << "Define Simulator Space for network: "
              << networkName.toStdString()
              << "!                          \r";

    // Retrieve the existing APIData for the specified network
    APIData& apiData = mData[networkName];

    // Retrieve the worker thread associated with this network
    QThread* workerThread = apiData.workerThread;
    if (!workerThread) {
        qFatal("Worker thread for network is null! This should never happen.");
        return;
    }

    // 1. Ensure the worker thread is running before using it
    if (!workerThread->isRunning()) {
        workerThread->start();
    }

    // 2. Create SimulatorWorker INSIDE the worker thread using invokeMethod()
    if (!apiData.simulatorWorker) {
        qFatal("simulator Worker for network is null! "
               "This should never happen.");
        return;
    }

    // 3. Setup the simulator safely inside the worker thread
    QMetaObject::invokeMethod(
        apiData.simulatorWorker,
        [&apiData, &shipList,
         timeStep, isExternallyControlled]()
        {
            apiData.simulatorWorker->setupSimulator(apiData, shipList,
                                                    timeStep,
                                                    isExternallyControlled);
        }, Qt::BlockingQueuedConnection);


    // 4. Set up permanent connections for this network
    setupConnections(networkName, mode);

    // 5. Ensure proper cleanup and signal handling
    //    when the worker thread finishes
    connect(workerThread, &QThread::finished, this,
            [this, networkName, mode]() {
                qDebug() << "Current thread 1:" << QThread::currentThread();

                // Check and emit workersReady signal when all simulator
                // workers are done
                checkAndEmitSignal(m_completedSimulatorWorkerFinished,
                                   m_totalSimulatorWorkerFinished.size(),
                                   m_totalSimulatorWorkerFinished,
                                   &SimulatorAPI::workersReady,
                                   mode);
            }, mConnectionType);

    // 6. Set the thread priority to LowPriority to
    //    prevent it from blocking other operations
    apiData.workerThread->setPriority(QThread::LowPriority);
}

void SimulatorAPI::setupConnections(const QString& networkName, Mode mode)
{
    if (!mData[networkName].simulator) {
        qFatal("Simulator instance is nullptr! "
               "Initialization failed for network: %s",
               qPrintable(networkName));
    }

    auto& simulator = mData[networkName].simulator;

    connect(simulator,
            &ShipNetSimCore::Simulator::simulationResultsAvailable, this,
            [this, networkName, mode](ShipsResults results) {
                qDebug() << "Current thread 2:" << QThread::currentThread();
                handleResultsAvailable(networkName, results, mode);
            }, mConnectionType);

    connect(simulator, &ShipNetSimCore::Simulator::simulationFinished, this,
            [this, networkName]() {
                qDebug() << "Current thread 3:" << QThread::currentThread();
                emit simulationFinished(networkName);
            }, mConnectionType);

    connect(simulator,
            &ShipNetSimCore::Simulator::simulationReachedReportingTime, this,
            [this, networkName, mode]
            (units::time::second_t currentSimulatorTime,
             double progressPercent)
            {
                qDebug() << "Current thread 4:" << QThread::currentThread();

                handleOneTimeStepCompleted(networkName, currentSimulatorTime,
                                           progressPercent, mode);
            }, mConnectionType);

    connect(simulator,
            &ShipNetSimCore::Simulator::progressUpdated, this,
            [this, networkName, mode]
            (int progress)
            {
                handleProgressUpdate(networkName, progress, mode);
            }, mConnectionType);

    connect(simulator, &ShipNetSimCore::Simulator::simulationPaused, this,
            [this, networkName, mode]() {
                m_completedSimulatorsPaused++;
                qDebug() << "Current thread 5:" << QThread::currentThread();
                checkAndEmitSignal(m_completedSimulatorsPaused,
                                   m_totalSimulatorPauseRequested.size(),
                                   m_totalSimulatorPauseRequested,
                                   &SimulatorAPI::simulationsPaused,
                                   mode);
            }, mConnectionType);

    connect(simulator, &ShipNetSimCore::Simulator::simulationResumed, this,
            [this, networkName, mode]() {
                m_completedSimulatorsResumed++;
                qDebug() << "Current thread 6:" << QThread::currentThread();
                checkAndEmitSignal(m_completedSimulatorsResumed,
                                   m_totalSimulatorResumeRequested.size(),
                                   m_totalSimulatorResumeRequested,
                                   &SimulatorAPI::simulationsResumed,
                                   mode);
            }, mConnectionType);

    connect(simulator, &ShipNetSimCore::Simulator::simulationTerminated, this,
            [this, networkName, mode]() {
                m_completedSimulatorsEnded++;
                qDebug() << "Current thread 7:" << QThread::currentThread();
                checkAndEmitSignal(m_completedSimulatorsEnded,
                                   m_totalSimulatorEndRequested.size(),
                                   m_totalSimulatorEndRequested,
                                   &SimulatorAPI::simulationsTerminated,
                                   mode);
            }, mConnectionType);

    connect(simulator, &ShipNetSimCore::Simulator::simulationRestarted, this,
            [this, networkName, mode]() {
                m_completedSimulatorRestarted++;
                qDebug() << "Current thread 8:" << QThread::currentThread();
                checkAndEmitSignal(m_completedSimulatorRestarted,
                                   m_totalSimulatorRestartedRequested.size(),
                                   m_totalSimulatorRestartedRequested,
                                   &SimulatorAPI::simulationsRestarted,
                                   mode);
            }, mConnectionType);

    setupShipsConnection(mData[networkName].ships.values(), networkName, mode);
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

void SimulatorAPI::setConnectionType(Qt::ConnectionType connectionType)
{
    mConnectionType = connectionType;
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

void SimulatorAPI::requestSimulationCurrentResults(
    QVector<QString> networkNames)
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
            bool success =
                QMetaObject::invokeMethod(mData[networkName].simulator,
                                          "generateSummaryData",
                                          mConnectionType);
            if (!success) {
                qWarning() << "Failed to invoke generateSummaryData";
            }
        }
    }
}

void SimulatorAPI::restartSimulations(
    QVector<QString> networkNames)
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
            bool success =
                QMetaObject::invokeMethod(mData[networkName].simulator,
                                          "restartSimulation",
                                          mConnectionType);

            if (!success) {
                qWarning() << "Failed to invoke restartSimulation";
            }
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

    setupShipsConnection(ships, networkName, mMode);


    QVector<QString> IDs;
    for (auto &ship: ships) {
        mData[networkName].ships.insert(ship->getUserID(), ship);

        bool success =
            QMetaObject::invokeMethod(
                mData[networkName].simulator,
                "addShipToSimulation",
                mConnectionType,
                Q_ARG(std::shared_ptr<ShipNetSimCore::Ship>, ship));
        if (!success) {
            qWarning() << "Failed to invoke addShipToSimulation";
        }

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
        return mData[networkName].isBusy;
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

    // flag the simulator as not busy
    mData[networkName].isBusy = false;

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
    double progress, Mode mode)
{
    m_timeStepData.insert(networkName, {currentSimulatorTime, progress});

    // flag the simulator as not busy
    mData[networkName].isBusy = false;

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

void SimulatorAPI::handleProgressUpdate(QString networkName,
                                        int progress,
                                        Mode mode)
{
    m_progressData.insert(networkName, progress);

    // flag the simulator as not busy
    mData[networkName].isBusy = false;

    switch (mode) {
    case Mode::Async:
        // Increment the number of completed simulators
        m_completedSimulatorsProgress++;

        // Check if all simulators have completed their time step
        if (m_completedSimulatorsProgress == mData.size()) {
            // Emit the aggregated signal
            emit simulationProgressUpdated(m_progressData);

            // Emit the aggregated signal
            emitShipsReachedDestination();

            m_completedSimulatorsProgress = 0;  // Reset the counter for the next time step
        }
        break;

    case Mode::Sync:
        // In sync mode, emit results immediately for each network
        emit simulationProgressUpdated(m_progressData);
        break;

    default:
        // Handle unexpected mode
        qWarning() << "Unexpected mode in handleProgressUpdate";
        break;
    }
}

void SimulatorAPI::handleResultsAvailable(QString networkName,
                                          ShipsResults result,
                                          Mode mode)
{

    // Always store the result in the buffer
    m_simulationResultBuffer.insert(networkName, result);

    // flag the simulator as not busy
    mData[networkName].isBusy = false;

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
    QString networkName,
    QVector<std::shared_ptr<ShipNetSimCore::Ship> > shipList,
    units::time::second_t timeStep,
    bool isExternallyControlled,
    Mode mode)
{
    mMode = mode;
    getInstance().createNewSimulationEnvironment(networkName,
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
    QString networkName,
    QVector<std::shared_ptr<ShipNetSimCore::Ship>> ships)
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
            // flag the simulator as not busy
            getInstance().mData[networkName].isBusy = true;

            bool success =
                QMetaObject::invokeMethod(
                    getInstance().mData[networkName].simulator,
                    "runOneTimeStep",
                    getInstance().mConnectionType);

            if (!success) {
                qWarning() << "Failed to invoke runOneTimeStep";
            }
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

                // flag the simulator as not busy
                getInstance().mData[networkName].isBusy = true;

                bool success =
                    QMetaObject::invokeMethod(
                        getInstance().mData[networkName].simulator,
                        "runSimulation",
                        getInstance().mConnectionType);

                if (!success) {
                    qWarning() << "Failed to invoke runSimulation";
                }
            }
        }
        else {
            if (getInstance().mData[networkName].simulator) {

                // flag the simulator as not busy
                getInstance().mData[networkName].isBusy = true;

                bool success =
                    QMetaObject::invokeMethod(
                        getInstance().mData[networkName].simulator,
                        "runBy",
                        getInstance().mConnectionType,
                        Q_ARG(units::time::second_t, timeSteps));

                if (!success) {
                    qWarning() << "Failed to invoke runBy";
                }
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
            bool success =
                QMetaObject::invokeMethod(
                    getInstance().mData[networkName].simulator,
                    "initializeSimulation",
                    getInstance().mConnectionType);

            if (!success) {
                qWarning() << "Failed to invoke initializeSimulation";
            }
        }
    }
}

void SimulatorAPI::InteractiveMode::endSimulation(
    QVector<QString> networkNames)
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

            bool success =
                QMetaObject::invokeMethod(
                    getInstance().mData[networkName].simulator,
                    "finalizeSimulation",
                    getInstance().mConnectionType);

            if (!success) {
                qWarning() << "Failed to invoke finalizeSimulation";
            }
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
SimulatorAPI::InteractiveMode::getAllShips(QString networkName)
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

void SimulatorAPI::InteractiveMode::
    setConnectionType(Qt::ConnectionType connectionType)
{
    getInstance().setConnectionType(connectionType);
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
    QString networkName,
    QVector<std::shared_ptr<ShipNetSimCore::Ship> > shipList,
    units::time::second_t timeStep,
    bool isExternallyControlled, Mode mode)
{
    mMode = mode;
    getInstance().createNewSimulationEnvironment(networkName,
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

void SimulatorAPI::ContinuousMode::addShipToSimulation(
    QString networkName,
    QVector<std::shared_ptr<ShipNetSimCore::Ship> > ships)
{
    getInstance().addShipToSimulation(networkName, ships);
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

            bool success =
                QMetaObject::invokeMethod(
                    getInstance().mData[networkName].simulator,
                    "runSimulation",
                    getInstance().mConnectionType);

            if (!success) {
                qWarning() << "Failed to invoke runSimulation";
            }
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

            getInstance().mData[networkName].simulator->pauseSimulation(true);
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

            getInstance().mData[networkName].simulator->resumeSimulation(true);
        }
    }
}

void SimulatorAPI::ContinuousMode::terminateSimulation(
    QVector<QString> networkNames)
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

            getInstance().mData[networkName].simulator->terminateSimulation();
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

void SimulatorAPI::ContinuousMode::
    setConnectionType(Qt::ConnectionType connectionType)
{
    getInstance().setConnectionType(connectionType);
}

QVector<std::shared_ptr<ShipNetSimCore::Ship> >
SimulatorAPI::loadShips(QJsonObject &ships, QString networkName)
{
    return SimulatorAPI::getInstance().mLoadShips(ships, networkName);
}

QVector<std::shared_ptr<ShipNetSimCore::Ship> >
SimulatorAPI::loadShips(QJsonObject &ships,
                        ShipNetSimCore::OptimizedNetwork *network)
{
    return SimulatorAPI::getInstance().mLoadShips(ships, network);

}

QVector<std::shared_ptr<ShipNetSimCore::Ship> >
SimulatorAPI::loadShips(QVector<QMap<QString, QString> > ships,
                        QString networkName)
{
    return SimulatorAPI::getInstance().mLoadShips(ships, networkName);
}

QVector<std::shared_ptr<ShipNetSimCore::Ship> >
SimulatorAPI::loadShips(QVector<QMap<QString, std::any> > ships,
                        QString networkName)
{
    return SimulatorAPI::getInstance().mLoadShips(ships, networkName);
}

QVector<std::shared_ptr<ShipNetSimCore::Ship> >
SimulatorAPI::loadShips(QString shipsFilePath, QString networkName)
{
    return SimulatorAPI::getInstance().mLoadShips(shipsFilePath, networkName);
}
