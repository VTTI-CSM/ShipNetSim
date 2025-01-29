// Include necessary headers
#include "simulatorapi.h"
#include "network/networkdefaults.h"
#include "network/seaportloader.h"
#include "shiploaderworker.h"
#include "utils/shipscommon.h"
#include <QThread>

// Initialize static members

QBasicMutex SimulatorAPI::s_instanceMutex;


// Default to synchronous mode
SimulatorAPI::Mode SimulatorAPI::mMode = Mode::Sync;

// Create singleton instance
std::unique_ptr<SimulatorAPI> SimulatorAPI::instance(new SimulatorAPI());


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
//                                BASIC FUNCTIONS
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------


void SimulatorAPI::setLocale() {
    try {
        // Set locale to UTF-8 with no thousands separator
        std::locale customLocale(
#ifdef _WIN32
            std::locale("English_United States.1252"), // Windows
#else
            std::locale("en_US.UTF-8"),               // Linux/macOS
#endif
            new NoThousandsSeparator()
            );

        // Apply custom locale globally and to std::cout
        std::locale::global(customLocale);
        std::cout.imbue(customLocale);
    } catch (const std::exception& e) {
        qWarning() << "Failed to set std::locale: " << e.what();

        // Fallback to "C" locale
        std::locale fallbackLocale("C");
        std::locale::global(fallbackLocale);
        std::cout.imbue(fallbackLocale);
    }

    // Configure Qt's QLocale for UTF-8 without thousands separators
    QLocale qtLocale(QLocale::English, QLocale::UnitedStates);
    qtLocale.setNumberOptions(QLocale::OmitGroupSeparator); // Disable thousands separator
    QLocale::setDefault(qtLocale);
}

void SimulatorAPI::
    createNewSimulationEnvironment(
        QString networkFilePath,
        QString networkName,
        QVector<std::shared_ptr<ShipNetSimCore::Ship>> shipList,
        units::time::second_t timeStep,
        bool isExternallyControlled,
        Mode mode)
{
    // Set locale to US format (no comma for thousands separator,
    // dot for decimals)
    setLocale();

    // Verify network exists
    if (apiDataMap.contains(networkName)) {
        emit errorOccurred("A network with name " + networkName
                           + " exist!");
        return;
    }

    // Load network and set up simulation
    loadNetwork(networkFilePath, networkName);
    setupSimulator(networkName, shipList, timeStep,
                   isExternallyControlled, mode);

    // Notify successful creation
    emit simulationCreated(networkName);
}

void SimulatorAPI::registerQMeta() {
    // Register shared pointer types for Qt's meta-system
    qRegisterMetaType<std::shared_ptr<ShipNetSimCore::Ship>>(
        "std::shared_ptr<ShipNetSimCore::Ship>");

    // Register custom data types
    qRegisterMetaType<ShipsResults>("ShipsResults");
    qRegisterMetaType<SimulatorAPI::Mode>("SimulatorAPI::Mode");

    // Register geometric types
    qRegisterMetaType<ShipNetSimCore::GPoint>("ShipNetSimCore::GPoint");
    qRegisterMetaType<QVector<std::pair<QString, ShipNetSimCore::GPoint>>>(
        "QVector<std::pair<QString, ShipNetSimCore::GPoint>>");
    qRegisterMetaType<ShipNetSimCore::GLine>("ShipNetSimCore::GLine");
    qRegisterMetaType<QVector<std::shared_ptr<ShipNetSimCore::GLine>>>(
        "QVector<std::shared_ptr<ShipNetSimCore::GLine>>");

    // Register unit types
    qRegisterMetaType<units::time::second_t>("units::time::second_t");
    qRegisterMetaType<units::length::meter_t>("units::length::meter_t");
    qRegisterMetaType<units::angle::degree_t>("units::angle::degree_t");
    qRegisterMetaType<QVector<units::length::meter_t>>(
        "QVector<units::length::meter_t>");

    // Register ship-related types
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

    // Register JSON and container types
    qRegisterMetaType<QJsonObject>("QJsonObject");
    qRegisterMetaType<QVector<std::shared_ptr<ShipNetSimCore::Ship>>>(
        "QVector<std::shared_ptr<ShipNetSimCore::Ship>>");

    // Register seaport types
    qRegisterMetaType<std::shared_ptr<ShipNetSimCore::SeaPort>>(
        "std::shared_ptr<ShipNetSimCore::SeaPort>");
    qRegisterMetaType<ShipNetSimCore::SeaPort>("ShipNetSimCore::SeaPort");
}

SimulatorAPI& SimulatorAPI::getInstance() {
    QMutexLocker locker(&s_instanceMutex);
    if (!instance) {
        registerQMeta();
        instance.reset(new SimulatorAPI());
    }
    return *instance;
}

void SimulatorAPI::resetInstance() {
    QMutexLocker locker(&s_instanceMutex);
    if (instance) {
        // Get all network names from the thread-safe data map
        QList<QString> networkNames = instance->apiDataMap.getNetworkNames();

        // Iterate over each network and clean up its resources
        for (const QString& networkName : networkNames) {
            // Retrieve the APIData for the current network
            APIData data = instance->apiDataMap.get(networkName);

            // Clean up network
            if (data.network) {
                if (data.network->thread() == data.workerThread) {
                    QMetaObject::invokeMethod(
                        data.network, "deleteLater",
                        Qt::BlockingQueuedConnection);
                } else {
                    delete data.network;
                }
                data.network = nullptr;
            }

            // Clean up simulatorWorker
            if (data.simulatorWorker) {
                if (data.simulatorWorker->thread() == data.workerThread) {
                    QMetaObject::invokeMethod(
                        data.simulatorWorker, "deleteLater",
                        Qt::BlockingQueuedConnection);
                } else {
                    delete data.simulatorWorker;
                }
                data.simulatorWorker = nullptr;
            }

            // Clean up simulator
            if (data.simulator) {
                if (data.simulator->thread() == data.workerThread) {
                    QMetaObject::invokeMethod(
                        data.simulator, "deleteLater",
                        Qt::BlockingQueuedConnection);
                } else {
                    delete data.simulator;
                }
                data.simulator = nullptr;
            }

            // Clean up shipLoaderWorker
            if (data.shipLoaderWorker) {
                if (data.shipLoaderWorker->thread() == data.workerThread) {
                    QMetaObject::invokeMethod(
                        data.shipLoaderWorker, "deleteLater",
                        Qt::BlockingQueuedConnection);
                } else {
                    delete data.shipLoaderWorker;
                }
                data.shipLoaderWorker = nullptr;
            }

            // Clean up workerThread
            if (data.workerThread) {
                data.workerThread->quit(); // Request thread to stop
                data.workerThread->wait(); // Wait for thread to finish
                delete data.workerThread;  // Delete the thread
                data.workerThread = nullptr;
            }

            // Clear ships
            data.ships.clear();

            // Update the APIData in the thread-safe map
            instance->apiDataMap.addOrUpdate(networkName, data);
        }

        // Clear all APIData from the thread-safe map
        instance->apiDataMap.clear();

        // Destroy the singleton instance
        instance.reset();
    }

    // Re-register meta types and create a new instance
    instance.reset();
    registerQMeta();
    instance.reset(new SimulatorAPI());
}


SimulatorAPI::~SimulatorAPI() {
    // Get all network names from the thread-safe data map
    QList<QString> networkNames = apiDataMap.getNetworkNames();

    // Iterate over each network and clean up its resources
    for (const QString& networkName : networkNames) {
        // Retrieve the APIData for the current network
        APIData apiData = apiDataMap.get(networkName);

        // Clean up the worker thread
        if (apiData.workerThread) {
            apiData.workerThread->quit(); // Request thread to stop
            apiData.workerThread->wait(); // Wait for thread to finish
            delete apiData.workerThread;  // Delete the thread
            apiData.workerThread = nullptr;
        }

        // Clean up the simulator
        if (apiData.simulator) {
            apiData.simulator->terminateSimulation(false);
            delete apiData.simulator;
            apiData.simulator = nullptr;
        }

        // Clean up the network
        if (apiData.network) {
            delete apiData.network;
            apiData.network = nullptr;
        }

        // Update the APIData in the thread-safe map
        apiDataMap.addOrUpdate(networkName, apiData);
    }

    // Clear all APIData from the thread-safe map
    apiDataMap.clear();
}


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
//                     CREATION, LOADING & SETTING UPFUNCTIONS
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------


void SimulatorAPI::createNewSimulationEnvironment(
    QString networkName,
    QVector<std::shared_ptr<ShipNetSimCore::Ship> > shipList,
    units::time::second_t timeStep,
    bool isExternallyControlled,
    Mode mode)
{
    // Set locale to US format (no comma for thousands separator,
    // dot for decimals)
    setLocale();

    if (!apiDataMap.contains(networkName)) {
        emit errorOccurred("A network with name " + networkName
                           + " does not exist!"
                           + "\nUse loadNetwork() first!");
        return;
    }

    // Set up simulation
    setupSimulator(networkName, shipList, timeStep,
                   isExternallyControlled, mode);

    // Notify successful creation
    emit simulationCreated(networkName);
}

ShipNetSimCore::OptimizedNetwork* SimulatorAPI::loadNetwork(
    QString filePath, QString networkName)
{
    std::cout << "Reading Network: " << networkName.toStdString() << "!\r";

    // Check if the file exists
    if (filePath.trimmed().toLower() != "default") {
        if (!QFile::exists(filePath)) {
            emit errorOccurred("Network file does not exist: " + filePath);
            return nullptr;
        }
    }

    // Cleanup existing network data if present
    if (apiDataMap.contains(networkName)) {
        APIData oldData = apiDataMap.get(networkName);
        // Properly delete old resources
        if (oldData.workerThread) {
            oldData.workerThread->quit();
            oldData.workerThread->wait();
            delete oldData.workerThread;
        }
        delete oldData.network;
        delete oldData.simulator;
        delete oldData.simulatorWorker;
        delete oldData.shipLoaderWorker;
        apiDataMap.remove(networkName);
    }

    // Create a new thread for network loading
    QThread* networkThread = new QThread(this);

    // Create and initialize APIData
    APIData apiData;
    apiData.workerThread = networkThread;

    // Store the APIData in the thread-safe map
    apiDataMap.addOrUpdate(networkName, apiData);

    // Use an event loop to wait for network initialization
    QEventLoop loop;

    // Create the network instance **inside the main thread first**
    ShipNetSimCore::OptimizedNetwork* net =
        new ShipNetSimCore::OptimizedNetwork();

    // Move the network instance to the worker thread
    net->moveToThread(networkThread);

    // Create the workers for later setup
    SimulatorWorker* simWorker = new SimulatorWorker();
    ShipLoaderWorker* shipLoaderWorker = new ShipLoaderWorker();

    // Move the workers to the worker thread
    simWorker->moveToThread(networkThread);
    shipLoaderWorker->moveToThread(networkThread);

    // Update the APIData with the network and workers
    apiData.network = net;
    apiData.simulatorWorker = simWorker;
    apiData.shipLoaderWorker = shipLoaderWorker;
    apiDataMap.addOrUpdate(networkName, apiData);

    // Connect the network thread's started signal to initialize the network
    connect(networkThread, &QThread::started, net,
            [this, networkName, filePath]() {
                QString fileLoc;
                if (filePath.trimmed().toLower() == "default") {
                    QString baseDataDir =
                        ShipNetSimCore::Utils::getDataDirectory();

                    auto filePaths =
                        ShipNetSimCore::NetworkDefaults::
                        worldNetworkLocation(baseDataDir);

                    fileLoc = ShipNetSimCore::Utils::
                        getFirstExistingPathFromList(filePaths);
                } else {
                    fileLoc = filePath;
                }

                // Retrieve the APIData for the network
                APIData apiData = apiDataMap.get(networkName);
                apiData.network->initializeNetwork(fileLoc, networkName);
            });

    // Keep track of the network initialization, loading, and errors
    bool loadSuccess = false;
    QString errorMsg;

    // Handle errors from the network
    connect(net, &ShipNetSimCore::OptimizedNetwork::errorOccurred, &loop,
            [&](QString error) {
        errorMsg = error;
        emit errorOccurred(error);
        loadSuccess = false;
        loop.quit();
    });

    // Connect the network loaded signal to exit the event loop
    connect(net, &ShipNetSimCore::OptimizedNetwork::NetworkLoaded, &loop,
            [&]() {
                loadSuccess = true;
                loop.quit();
            });

    // Start the worker thread
    networkThread->start();

    // Block execution until the network is fully loaded
    loop.exec();  // This will wait for the `NetworkLoaded`
                  // signal before proceeding

    if (!loadSuccess) {
        emit errorOccurred(errorMsg);
        // Cleanup resources
        net->deleteLater();
        simWorker->deleteLater();
        shipLoaderWorker->deleteLater();
        networkThread->quit();
        networkThread->wait();
        delete networkThread;
        apiDataMap.remove(networkName);
        return nullptr;
    }


    qInfo() << "Network " << networkName.toStdString()
            << " loaded successfully!\n";

    // Emit signal when the network is fully loaded
    emit networkLoaded(networkName);

    return net;  // The network is now fully initialized and accessible
}

APIData SimulatorAPI::getApiDataAndEnsureThread(const QString &networkName)
{
    // Check if the network exists in the thread-safe map
    if (!apiDataMap.contains(networkName)) {
        throw std::runtime_error(
            QString("Network not found in APIData: %1").arg(networkName)
                .toStdString());
    }

    // Retrieve a copy of the APIData for the specified network
    APIData apiData = apiDataMap.get(networkName);

    // Ensure the worker thread is valid and running
    if (!apiData.workerThread) {
        throw std::runtime_error(
            QString("Worker thread for %1 is null!").arg(networkName)
                .toStdString());
    }

    if (!apiData.workerThread->isRunning()) {
        apiData.workerThread->start();
    }

    // Ensure the shipLoaderWorker is valid
    if (!apiData.shipLoaderWorker) {
        throw std::runtime_error(
            QString("shipLoaderWorker for %1 is null!").arg(networkName)
                .toStdString());
    }

    // Update the APIData in the thread-safe map (if any changes were made)
    apiDataMap.addOrUpdate(networkName, apiData);

    // Return a copy of the APIData
    return apiData;
}

void SimulatorAPI::setupShipsConnection(
    QVector<std::shared_ptr<ShipNetSimCore::Ship>> ships,
    QString networkName, Mode mode)
{
    for (const auto& ship : ships)
    {
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

        connect(ship.get(), &ShipNetSimCore::Ship::containersAdded, this,
                [this, ship, networkName]() {
                    emit SimulatorAPI::containersAddedToShip(networkName,
                                                             ship->getUserID());
                });

        connect(ship.get(), &ShipNetSimCore::Ship::reachedSeaPort, this,
                [this, ship, networkName]
                (QString shipID, QString seaPortCode,
                 qsizetype containersCount)
                {
                    emit SimulatorAPI::shipReachedSeaPort(networkName,
                                                          shipID,
                                                          seaPortCode,
                                                          containersCount);
                });

        connect(ship.get(), &ShipNetSimCore::Ship::containersUnloaded, this,
                [this, ship, networkName]
                (QString shipID, QString seaPortCode, QJsonArray containers)
                {
                    emit SimulatorAPI::ContainersUnloaded(networkName,
                                                          shipID,
                                                          seaPortCode,
                                                          containers);
                });

        connect(ship.get(), &ShipNetSimCore::Ship::shipStateAvailable, this,
                [this, ship, networkName] (QJsonObject state) {
                    emit SimulatorAPI::shipStateAvailable(networkName,
                                                          ship->getUserID(),
                                                          state);
                });
    }
}

QVector<std::shared_ptr<ShipNetSimCore::Ship>>
SimulatorAPI::mLoadShips(QJsonObject &ships, QString networkName)
{
    if (!ships.contains("ships") || !ships["ships"].isArray()) {
        emit errorOccurred("Invalid ship configuration: missing 'ships' array");
        return QVector<std::shared_ptr<ShipNetSimCore::Ship>>();
    }

    // 1) Prepare a default empty result in case of errors
    QVector<std::shared_ptr<ShipNetSimCore::Ship>> loadedShips;

    try {

        // 2) Ensure we have APIData for this network and
        //    retrieve the existing APIData for the network
        APIData apiData = getApiDataAndEnsureThread(networkName);

        // --------------------------------------------------------------------
        // 3) Because 'loadShips' is async and emits a signal, we use an
        //    event loop here to wait for the result or an error.
        // --------------------------------------------------------------------
        QEventLoop loop;

        // -- Connect to the success signal
        QMetaObject::Connection connSuccess = connect(
            apiData.shipLoaderWorker,
            &ShipLoaderWorker::shipsLoaded,
            this,
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
            this,
            [&](const QString &errMsg) {
                emit errorOccurred("Error loading ships in mLoadShips:" +
                                   errMsg);
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
                apiData.shipLoaderWorker->loadShips(apiData.network,
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
    APIData apiData = getApiDataAndEnsureThread(network->getRegionName());

    // validate the network is not nullptr.
    if (!network) {
        emit errorOccurred("Error: Network cannot be a nullptr!");
        qWarning() << "Error: Network cannot be a nullptr!";
    }

    // 3) Ensure the network is in the same thread as the worker so that
    //    the connections work properlly.
    if (network->thread() != apiData.workerThread) {
        QString error =
            QString("Error: Network is not in the expected worker thread! "
                    "Expected thread: %1, Actual thread: %2")
                .arg(reinterpret_cast<quintptr>(apiData.workerThread), 0, 16)
                .arg(reinterpret_cast<quintptr>(network->thread()), 0, 16);

        emit errorOccurred( error );
        qWarning() << error;
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
            this,
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
            this,
            [&](const QString &errorMsg) {
                QString error =
                    QString("Error loading ships in mLoadShips: %1")
                                    .arg(errorMsg);
                emit errorOccurred(error);
                qWarning() << error;
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
        APIData apiData = getApiDataAndEnsureThread(networkName);

        // ------------------------------------------------------------------
        // 3) Setup a local QEventLoop to wait for the result or an error
        // ------------------------------------------------------------------
        QEventLoop loop;

        // 4) Connect success signal
        QMetaObject::Connection connSuccess = connect(
            apiData.shipLoaderWorker,
            &ShipLoaderWorker::shipsLoaded,
            this,
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
            this,
            [&](const QString &errMsg) {
                // Log the error, then quit the loop
                QString error =
                    QString("Error loading ships in mLoadShips: %1")
                        .arg(errMsg);
                emit errorOccurred(error);
                qWarning() << error;
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
                apiData.shipLoaderWorker->loadShips(apiData.network,
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
        APIData apiData = getApiDataAndEnsureThread(networkName);

        // -------------------------------------------------------------------
        // 3) We'll use a local QEventLoop to block until we get
        //    shipsLoaded or errorOccured
        // -------------------------------------------------------------------
        QEventLoop loop;

        // 4) Connect the success signal
        QMetaObject::Connection connSuccess = connect(
            apiData.shipLoaderWorker,
            &ShipLoaderWorker::shipsLoaded,
            this,
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
            this,
            [&](const QString &errMsg) {
                QString error =
                    QString("Error loading ships in mLoadShips: %1")
                        .arg(errMsg);
                emit errorOccurred(error);
                qWarning() << error;
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
                apiData.shipLoaderWorker->loadShips(apiData.network,
                                                    ships,
                                                    networkName);
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
        APIData apiData = getApiDataAndEnsureThread(networkName);

        // -------------------------------------------------------------------
        // 7) We'll use a local QEventLoop to block until
        //    shipsLoaded or errorOccured
        // -------------------------------------------------------------------
        QEventLoop loop;

        // 8) Connect the success signal
        QMetaObject::Connection connSuccess = connect(
            apiData.shipLoaderWorker,
            &ShipLoaderWorker::shipsLoaded,
            this,
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
            this,
            [&](const QString &errMsg) {
                QString error =
                    QString("Error loading ships in mLoadShips: %1")
                        .arg(errMsg);
                emit errorOccurred(error);
                qWarning() << error;
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
                apiData.shipLoaderWorker->loadShips(apiData.network,
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
                   units::time::second_t timeStep,
                   bool isExternallyControlled,
                   Mode mode)
{
    qRegisterMetaType<ShipsResults>("ShipsResults");

    // Ensure the network exists in APIData
    if (!apiDataMap.contains(networkName)) {
        qWarning() << "Network not found in APIData: " << networkName;
        return;
    }

    std::cout << "Define Simulator Space for network: "
              << networkName.toStdString()
              << "!                          \r";

    // Retrieve the existing APIData for the specified network
    APIData apiData = apiDataMap.get(networkName);

    // Retrieve the worker thread associated with this network
    QThread* workerThread = apiData.workerThread;
    if (!workerThread) {
        emit errorOccurred("Worker thread for network is null! "
                           "This should never happen.");
        qFatal("Worker thread for network is null! "
               "This should never happen.");
        return;
    }

    // 1. Ensure the worker thread is running before using it
    if (!workerThread->isRunning()) {
        workerThread->start();
    }

    // 2. Create SimulatorWorker INSIDE the worker thread using invokeMethod()
    if (!apiData.simulatorWorker) {
        emit errorOccurred("simulator worker for network is null! "
                           "This should never happen.");
        qFatal("simulator Worker for network is null! "
               "This should never happen.");
        return;
    }

    // Connect the simulator worker's error signal
    connect(apiData.simulatorWorker,
            &SimulatorWorker::errorOccurred,
            this,
            [this](QString error) {
                emit errorOccurred(error);
            });

    // 3. Setup the simulator safely inside the worker thread
    QMetaObject::invokeMethod(
        apiData.simulatorWorker,
        [&apiData, &shipList, timeStep, isExternallyControlled,
         this, networkName]() {
            apiData.simulatorWorker->setupSimulator(apiData, shipList,
                                                    timeStep,
                                                    isExternallyControlled);

            // update the api data after adding the simulator
            apiDataMap.addOrUpdate(networkName, apiData);
        }, Qt::BlockingQueuedConnection);

    // 4. Set up permanent connections for this network
    setupConnections(networkName, mode);

    // 5. Ensure proper cleanup and signal handling
    //    when the worker thread finishes
    connect(workerThread, &QThread::finished, this,
            [this, networkName]() {
                qDebug() << "Current thread 1:" << QThread::currentThread();
                handleWorkersReady(networkName);
            }, mConnectionType);

    // 6. Set the thread priority to LowPriority to
    //    prevent it from blocking other operations
    apiData.workerThread->setPriority(QThread::LowPriority);

    // Update the APIData in the thread-safe map
    apiDataMap.addOrUpdate(networkName, apiData);
}

void SimulatorAPI::setupConnections(const QString& networkName, Mode mode)
{
    // Retrieve the APIData for the specified network
    APIData apiData = apiDataMap.get(networkName);

    // Ensure the simulator instance is valid
    if (!apiData.simulator) {
        emit errorOccurred(QString("Simulator instance is nullptr! "
                                   "Initialization failed for network: %1")
                               .arg(networkName));
        qFatal("Simulator instance is nullptr! "
               "Initialization failed for network: %s",
               qPrintable(networkName));
    }

    auto& simulator = apiData.simulator;

    // Connect simulation results available signal
    connect(simulator,
            &ShipNetSimCore::Simulator::simulationResultsAvailable, this,
            [this, networkName](ShipsResults results) {
                qDebug() << "Current thread 2:" << QThread::currentThread();
                handleResultsAvailable(networkName, results);
            }, mConnectionType);

    // Connect simulation finished signal
    connect(simulator, &ShipNetSimCore::Simulator::simulationFinished, this,
            [this, networkName]() {
                qDebug() << "Current thread 3:" << QThread::currentThread();
                handleSimulationFinished(networkName);
            }, mConnectionType);

    // Connect simulation reached reporting time signal
    connect(simulator,
            &ShipNetSimCore::Simulator::simulationReachedReportingTime, this,
            [this, networkName, mode]
            (units::time::second_t currentSimulatorTime,
             double progressPercent) {
                handleOneTimeStepCompleted(networkName, currentSimulatorTime,
                                           progressPercent, mode);
            }, mConnectionType);

    // Connect progress updated signal
    connect(simulator,
            &ShipNetSimCore::Simulator::progressUpdated, this,
            [this, networkName](int progress) {
                handleProgressUpdate(networkName, progress);
            }, mConnectionType);

    // Connect simulation paused signal
    connect(simulator, &ShipNetSimCore::Simulator::simulationPaused, this,
            [this, networkName, mode]() {
                mPauseTracker.incrementCompletedRequests();
                checkAndEmitSignal(
                    mPauseTracker.getCompletedRequests(),
                    mPauseTracker.getRequestedNetworks().size(),
                    mPauseTracker.getRequestedNetworks(),
                    &SimulatorAPI::simulationsPaused,
                    mode);
            }, mConnectionType);

    // Connect simulation resumed signal
    connect(simulator, &ShipNetSimCore::Simulator::simulationResumed, this,
            [this, networkName, mode]() {
                mResumeTracker.incrementCompletedRequests();
                checkAndEmitSignal(
                    mResumeTracker.getCompletedRequests(),
                    mResumeTracker.getRequestedNetworks().size(),
                    mResumeTracker.getRequestedNetworks(),
                    &SimulatorAPI::simulationsResumed,
                    mode);
            }, mConnectionType);

    // Connect simulation terminated signal
    connect(simulator, &ShipNetSimCore::Simulator::simulationTerminated, this,
            [this, networkName, mode]() {
                mTerminateTracker.incrementCompletedRequests();
                checkAndEmitSignal(
                    mTerminateTracker.getCompletedRequests(),
                    mTerminateTracker.getRequestedNetworks().size(),
                    mTerminateTracker.getRequestedNetworks(),
                    &SimulatorAPI::simulationsTerminated,
                    mode);
            }, mConnectionType);

    // Connect simulation restarted signal
    connect(simulator, &ShipNetSimCore::Simulator::simulationRestarted, this,
            [this, networkName, mode]() {
                mRestartTracker.incrementCompletedRequests();
                checkAndEmitSignal(
                    mRestartTracker.getCompletedRequests(),
                    mRestartTracker.getRequestedNetworks().size(),
                    mRestartTracker.getRequestedNetworks(),
                    &SimulatorAPI::simulationsRestarted,
                    mode);
            }, mConnectionType);

    // Connect available ports signal
    connect(simulator, &ShipNetSimCore::Simulator::availablePorts, this,
            [this, networkName, mode](QVector<QString> ports) {
                handleAvailablePorts(networkName, ports, mode);
            });

    // Connect error occurred signal
    connect(simulator, &ShipNetSimCore::Simulator::errorOccured, this,
            [this, networkName](QString error) {
                emit errorOccurred(error);
            });

    // Set up connections for ships
    setupShipsConnection(apiData.ships.values(), networkName, mode);

    // Update the APIData in the thread-safe map
    apiDataMap.addOrUpdate(networkName, apiData);
}


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
//                            RECEIVED SIGNALS HANDLERS
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

void SimulatorAPI::handleShipReachedDestination(QString networkName,
                                                QJsonObject shipState,
                                                Mode mode)
{

    QJsonObject response;
    QJsonObject networkData;
    QJsonArray shipsArray;

    shipsArray.append(shipState);
    networkData["shipStates"] = shipsArray;
    response[networkName] = networkData;

    emit shipsReachedDestination(response);
}

void SimulatorAPI::handleResultsAvailable(QString networkName,
                                          ShipsResults result)
{
    // Emit single result immediately
    emit simulationResultsAvailable({networkName, result});

}

void SimulatorAPI::handleProgressUpdate(QString networkName,
                                        int progress)
{
    // Emit and clean up
    emit simulationProgressUpdated(networkName, progress);
}

void SimulatorAPI::handleAvailablePorts(QString networkName,
                                        QVector<QString> portIDs,
                                        Mode mode)
{
    // 1. Store port IDs atomically
    mAvailablePortTracker.addUpdateData(networkName, portIDs);

    // 2. Mark network as available
    apiDataMap.setBusy(networkName, false);

    switch (mode) {
    case Mode::Async: {
        // 3. Atomic increment and check
        const int completed = mAvailablePortTracker.incrementAndGetCompleted();
        const int total = mAvailablePortTracker.getRequestedCount();

        if (completed == total) {
            // 4. Get copy of port data
            QMap<QString, QVector<QString>> portData =
                mAvailablePortTracker.getDataBuffer();

            // 5. Emit aggregated ports
            emit availablePorts(portData);

            // 6. Reset tracking
            mAvailablePortTracker.clearAll();
        }
        break;
    }

    case Mode::Sync: {
        // 7. Create immediate response for single network
        QMap<QString, QVector<QString>> immediatePorts;
        immediatePorts[networkName] = portIDs;

        // 8. Emit and clean up
        emit availablePorts(immediatePorts);
        mAvailablePortTracker.removeData(networkName);
        break;
    }

    default:
        emit errorOccurred("Unexpected mode in handleAvailablePorts");
        qWarning() << "Unexpected mode in handleAvailablePorts";
        break;
    }
}

void SimulatorAPI::handleOneTimeStepCompleted(
    QString networkName, units::time::second_t currentSimulatorTime,
    double progress, Mode mode)
{
    // 1. Store time step data atomically
    mTimeStepTracker.addUpdateData(networkName,
                                   {currentSimulatorTime, progress});

    // 2. Mark network as available
    apiDataMap.setBusy(networkName, false);

    switch (mode) {
    case Mode::Async: {
        // 3. Atomic increment and check
        const int completed = mTimeStepTracker.incrementAndGetCompleted();
        const int total = mTimeStepTracker.getRequestedCount();

        if (completed == total) {
            // 4. Get copy of time step data
            QMap<QString, QPair<units::time::second_t, double>> timeData =
                mTimeStepTracker.getDataBuffer();

            // 5. Emit aggregated time steps
            emit simulationAdvanced(timeData);

            // 6. Reset tracking
            mTimeStepTracker.clearAll();
        }
        break;
    }

    case Mode::Sync: {
        // 8. Create immediate response for single network
        QMap<QString, QPair<units::time::second_t, double>> immediateTime;
        immediateTime[networkName] = {currentSimulatorTime, progress};

        // 9. Emit and clean up
        emit simulationAdvanced(immediateTime);
        mTimeStepTracker.removeData(networkName);
        break;
    }

    default:
        emit errorOccurred("Unexpected mode in handleOneTimeStepCompleted");
        qWarning() << "Unexpected mode in handleOneTimeStepCompleted";
        break;
    }
}

void SimulatorAPI::handleSimulationFinished(QString networkName)
{

    // Immediate emission for single network
    emit simulationFinished(networkName);

    apiDataMap.setBusy(networkName, false);

}

void SimulatorAPI::handleWorkersReady(QString networkName)
{
    switch (mMode) {
    case Mode::Async: {
        // Atomic increment and check
        const int completed = mWorkerTracker.incrementAndGetCompleted();
        const int total = mWorkerTracker.getRequestedCount();

        if (completed == total) {
            // Get copy of ready networks
            QVector<QString> readyNetworks =
                mWorkerTracker.getRequestedNetworks();

            // Emit signal with prepared list
            emit workersReady(readyNetworks);

            // Reset tracking state
            mWorkerTracker.clearAll();
        }
        break;
    }

    case Mode::Sync:
        // Immediate emission for single network
        emit workersReady({networkName});
        mWorkerTracker.removeData(networkName);
        break;

    default:
        emit errorOccurred("Unexpected mode in handleWorkersReady");
        qWarning() << "Unexpected mode in handleWorkersReady";
        break;
    }
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
//                               GETTERS AND REQUESTS
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------


ShipNetSimCore::Simulator* SimulatorAPI::getSimulator(QString networkName)
{
    // Check if the network exists in the thread-safe map
    if (!apiDataMap.contains(networkName)) {
        emit errorOccurred("A network with name " +
                           networkName +
                           " does not exist!");
        return nullptr;
    }

    // Retrieve the APIData for the specified network
    APIData apiData = apiDataMap.get(networkName);

    // Return the simulator instance
    return apiData.simulator;
}

ShipNetSimCore::OptimizedNetwork* SimulatorAPI::getNetwork(QString networkName)
{
    // Check if the network exists in the thread-safe map
    if (!apiDataMap.contains(networkName)) {
        emit errorOccurred("A network with name " +
                           networkName +
                           " does not exist!");
        return nullptr;
    }

    // Retrieve the APIData for the specified network
    APIData apiData = apiDataMap.get(networkName);

    // Return the network instance
    return apiData.network;
}

void SimulatorAPI::
    requestSimulationCurrentResults(QVector<QString> networkNames)
{
    // If "*" is specified, process all networks
    if (networkNames.contains("*")) {
        networkNames = apiDataMap.getNetworkNames();
    }

    // Iterate over each network name
    for (const auto& networkName : networkNames) {
        // Check if the network exists in the thread-safe map
        if (!apiDataMap.contains(networkName)) {
            emit errorOccurred("A network with name " +
                               networkName +
                               " does not exist!");
            return;
        }

        // Retrieve the APIData for the specified network
        APIData apiData = apiDataMap.get(networkName);

        // Check if the simulator exists
        if (apiData.simulator) {
            // Invoke the generateSummaryData method on the simulator
            bool success = QMetaObject::invokeMethod(apiData.simulator,
                                                     "generateSummaryData",
                                                     mConnectionType);
            if (!success) {
                emit errorOccurred("Failed to invoke generateSummaryData");
                qWarning() << "Failed to invoke generateSummaryData";
            }
        }
    }
}

void SimulatorAPI::requestRestartSimulations(QVector<QString> networkNames)
{
    // If "*" is specified, process all networks
    if (networkNames.contains("*")) {
        networkNames = apiDataMap.getNetworkNames();
    }

    // Reset the restart tracker
    mRestartTracker.resetCompletedRequests();
    mRestartTracker.setRequestedNetworks(networkNames);

    // Iterate over each network name
    for (const auto& networkName : networkNames) {
        // Check if the network exists in the thread-safe map
        if (!apiDataMap.contains(networkName)) {
            emit errorOccurred("A network with name " +
                               networkName +
                               " does not exist!");
            return;
        }

        // Retrieve the APIData for the specified network
        APIData apiData = apiDataMap.get(networkName);

        // Check if the simulator exists
        if (apiData.simulator) {
            // Invoke the restartSimulation method on the simulator
            bool success = QMetaObject::invokeMethod(apiData.simulator,
                                                     "restartSimulation",
                                                     mConnectionType);
            if (!success) {
                emit errorOccurred("Failed to invoke restartSimulation");
                qWarning() << "Failed to invoke restartSimulation";
            }
        }
    }
}

void SimulatorAPI::addShipToSimulation(
    QString networkName,
    QVector<std::shared_ptr<ShipNetSimCore::Ship>> ships)
{
    // Check if the network exists in the thread-safe map
    if (!apiDataMap.contains(networkName)) {
        emit errorOccurred("A network with name " +
                           networkName +
                           " does not exist!");
        return;
    }

    // Set up connections for the ships
    setupShipsConnection(ships, networkName, mMode);

    // Retrieve the APIData for the specified network
    APIData apiData = apiDataMap.get(networkName);

    QVector<QString> IDs;
    for (auto& ship : ships) {
        // Add the ship to the ships map in APIData
        apiData.ships.insert(ship->getUserID(), ship);

        // Invoke the addShipToSimulation method on the simulator
        bool success = QMetaObject::invokeMethod(
            apiData.simulator,
            "addShipToSimulation",
            mConnectionType,
            Q_ARG(std::shared_ptr<ShipNetSimCore::Ship>, ship)
            );

        if (!success) {
            emit errorOccurred("Failed to invoke addShipToSimulation");
            qWarning() << "Failed to invoke addShipToSimulation";
        }

        // Store the ship ID
        IDs.push_back(ship->getUserID());
    }

    // Update the APIData in the thread-safe map
    apiDataMap.addOrUpdate(networkName, apiData);

    // Emit the shipsAddedToSimulation signal
    emit shipsAddedToSimulation(networkName, IDs);
}

bool SimulatorAPI::isWorkerBusy(QString networkName)
{
    // Check if the network exists in the thread-safe map
    if (apiDataMap.contains(networkName)) {
        // Retrieve the APIData for the specified network
        APIData apiData = apiDataMap.get(networkName);
        return apiData.isBusy;
    } else {
        emit errorOccurred("Network with name " +
                           networkName +
                           " does not exist!");
        return false;
    }
}

std::shared_ptr<ShipNetSimCore::Ship> SimulatorAPI::getShipByID(
    QString networkName, QString& shipID)
{
    // Check if the network exists in the thread-safe map
    if (!apiDataMap.contains(networkName)) {
        emit errorOccurred("A network with name " +
                           networkName +
                           " does not exist!");
        return nullptr;
    }

    // Retrieve the APIData for the specified network
    APIData apiData = apiDataMap.get(networkName);

    // Check if the ship exists in the ships map
    if (apiData.ships.contains(shipID)) {
        return apiData.ships.value(shipID);
    }

    return nullptr;
}

QVector<std::shared_ptr<ShipNetSimCore::Ship>>
SimulatorAPI::getAllShips(QString networkName)
{
    // Check if the network exists in the thread-safe map
    if (!apiDataMap.contains(networkName)) {
        emit errorOccurred("A network with name " +
                           networkName +
                           " does not exist!");
        return QVector<std::shared_ptr<ShipNetSimCore::Ship>>();
    }

    // Retrieve the APIData for the specified network
    APIData apiData = apiDataMap.get(networkName);

    // Return all ships as a QVector
    return apiData.ships.values().toVector();
}

void SimulatorAPI::requestShipCurrentStateByID(QString networkName,
                                                      QString ID)
{
    // Check if the network exists in the thread-safe map
    if (!apiDataMap.contains(networkName)) {
        emit errorOccurred("A network with name " +
                           networkName +
                           " does not exist!");
        return;
    }

    // Retrieve the APIData for the specified network
    APIData apiData = apiDataMap.get(networkName);

    // Check if the ship exists in the ships map
    if (apiData.ships.contains(ID)) {
        // Get the current state of the ship as a JSON object
        apiData.ships.value(ID)->requestCurrentStateAsJson();
        return;
    }

    // Emit an error if the ship does not exist
    emit errorOccurred("A ship with ID " + ID + " does not exist!");
}

QJsonObject SimulatorAPI::requestSimulatorCurrentState(QString networkName)
{
    // Check if the network exists in the thread-safe map
    if (!apiDataMap.contains(networkName)) {
        emit errorOccurred("A network with name " +
                           networkName +
                           " does not exist!");
        return QJsonObject();
    }

    // Retrieve the APIData for the specified network
    APIData apiData = apiDataMap.get(networkName);

    // Ensure the simulator exists
    if (!apiData.simulator) {
        emit errorOccurred("Simulator for network " +
                           networkName +
                           " is not initialized!");
        return QJsonObject();
    }

    // Get the current state of the simulator as a JSON object
    QJsonObject out = apiData.simulator->getCurrentStateAsJson();
    emit simulationCurrentStateAvailable(out);
    return out;
}

void SimulatorAPI::addContainersToShip(QString networkName, QString shipID,
                                       QJsonObject json)
{
    getShipByID(networkName, shipID)->addContainers(json);
}

bool SimulatorAPI::isNetworkLoaded(QString networkName)
{
    // Check if the network exists in the thread-safe map
    if (!apiDataMap.contains(networkName)) {
        return false;
    }

    // Retrieve the APIData for the specified network
    APIData apiData = apiDataMap.get(networkName);

    // Check if the network is loaded
    return (apiData.network != nullptr);
}



void SimulatorAPI::requestPauseSimulation(QVector<QString> networkNames)
{
    // If "*" is specified, process all networks
    if (networkNames.contains("*")) {
        networkNames = apiDataMap.getNetworkNames();
    }

    // Reset the pause tracker
    mPauseTracker.resetCompletedRequests();
    mPauseTracker.setRequestedNetworks(networkNames);

    // Iterate over each network name
    for (const auto& networkName : networkNames) {
        // Check if the network exists in the thread-safe map
        if (!apiDataMap.contains(networkName)) {
            emit errorOccurred("A network with name " +
                               networkName +
                               " does not exist!");
            return;
        }

        // Retrieve the APIData for the specified network
        APIData apiData = apiDataMap.get(networkName);

        // Check if the simulator exists
        if (apiData.simulator) {
            // Pause the simulation
            apiData.simulator->pauseSimulation(true);
        }
    }
}

void SimulatorAPI::requestResumeSimulation(QVector<QString> networkNames)
{
    // If "*" is specified, process all networks
    if (networkNames.contains("*")) {
        networkNames = apiDataMap.getNetworkNames();
    }

    // Reset the resume tracker
    mResumeTracker.resetCompletedRequests();
    mResumeTracker.setRequestedNetworks(networkNames);

    // Iterate over each network name
    for (const auto& networkName : networkNames) {
        // Check if the network exists in the thread-safe map
        if (!apiDataMap.contains(networkName)) {
            emit errorOccurred("A network with name " +
                               networkName +
                               " does not exist!");
            return;
        }

        // Retrieve the APIData for the specified network
        APIData apiData = apiDataMap.get(networkName);

        // Check if the simulator exists
        if (apiData.simulator) {
            // Resume the simulation
            apiData.simulator->resumeSimulation(true);
        }
    }
}

void SimulatorAPI::requestTerminateSimulation(QVector<QString> networkNames)
{

    if (networkNames.contains("*")) {
        networkNames = apiDataMap.getNetworkNames();
    }

    mTerminateTracker.resetCompletedRequests();
    mTerminateTracker.setRequestedNetworks(networkNames);

    // Iterate over each network name
    for (const auto &networkName: networkNames) {
        // Check if the network exists in the thread-safe map
        if (!apiDataMap.contains(networkName)) {
            emit errorOccurred("A network with name " +
                               networkName +
                               " does not exist!");
            return;
        }

        // Retrieve the APIData for the specified network
        APIData apiData = apiDataMap.get(networkName);

        // Check if the simulator exists
        if (apiData.simulator) {

            apiData.simulator->terminateSimulation();
        }
    }
}

void SimulatorAPI::requestRunSimulation(QVector<QString> networkNames,
                                        units::time::second_t timeSteps,
                                        bool endSimulationAfterRun,
                                        bool getStepEndSignal)
{
    // If "*" is specified, process all networks
    if (networkNames.contains("*")) {
        networkNames = apiDataMap.getNetworkNames();
    }

    // Iterate over each network name
    for (const auto& networkName : networkNames) {
        // Check if the network exists in the thread-safe map
        if (!apiDataMap.contains(networkName)) {
            emit errorOccurred("A network with name " +
                               networkName +
                               " does not exist!");
            return;
        }

        mReachedDesTracker.setRequestedNetworks(networkNames);

        // Retrieve the APIData for the specified network
        APIData apiData = apiDataMap.get(networkName);

        // Check if the simulator exists
        if (apiData.simulator) {
            // Flag the simulator as busy
            apiDataMap.setBusy(networkName, true);

            // Invoke the runSimulation method on the simulator
            bool success = QMetaObject::invokeMethod(
                apiData.simulator,
                "runSimulation",
                mConnectionType,
                Q_ARG(units::time::second_t, timeSteps),
                Q_ARG(bool, endSimulationAfterRun),
                Q_ARG(bool, getStepEndSignal)
                );

            if (!success) {
                qWarning() << "Failed to invoke runSimulation";
            }
        }
    }

    mReachedDesTracker.resetCompletedRequests();
    mReachedDesTracker.setRequestedNetworks(networkNames);
}

void SimulatorAPI::finalizeSimulation(QVector<QString> networkNames)
{
    // If "*" is specified, process all networks
    if (networkNames.contains("*")) {
        networkNames = apiDataMap.getNetworkNames();
    }

    // Iterate over each network name
    for (const auto& networkName : networkNames) {
        // Check if the network exists in the thread-safe map
        if (!apiDataMap.contains(networkName)) {
            emit errorOccurred("A network with name " +
                               networkName +
                               " does not exist!");
            return;
        }

        // Retrieve the APIData for the specified network
        APIData apiData = apiDataMap.get(networkName);

        // Check if the simulator exists
        if (apiData.simulator) {
            // Invoke the finalizeSimulation method on the simulator
            bool success = QMetaObject::invokeMethod(
                apiData.simulator,
                "finalizeSimulation",
                mConnectionType
                );

            if (!success) {
                qWarning() << "Failed to invoke finalizeSimulation";
            }
        }
    }
}

void SimulatorAPI::requestAvailablePorts(QVector<QString> networkNames,
                                         bool getOnlyPortsOnShipsPaths)
{
    // If "*" is specified, process all networks
    if (networkNames.contains("*")) {
        networkNames = apiDataMap.getNetworkNames();
    }

    // Update the requested network process in the tracker
    mAvailablePortTracker.setRequestedNetworks(networkNames);

    // Iterate over each network name
    for (const auto& networkName : networkNames) {
        // Check if the network exists in the thread-safe map
        if (!apiDataMap.contains(networkName)) {
            emit errorOccurred("A network with name " +
                               networkName +
                               " does not exist!");
            return;
        }

        // Retrieve the APIData for the specified network
        APIData apiData = apiDataMap.get(networkName);

        // Check if the simulator exists
        if (apiData.simulator) {
            // Flag the simulator as busy
            apiDataMap.setBusy(networkName, true);

            // Invoke the getAvailablePorts method on the simulator
            bool success = QMetaObject::invokeMethod(
                apiData.simulator,
                "getAvailablePorts",
                mConnectionType,
                Q_ARG(bool, getOnlyPortsOnShipsPaths)
                );

            if (!success) {
                qWarning() << "Failed to invoke getAvailablePorts";
            }
        }
    }
}

void SimulatorAPI::requestUnloadContainersAtPort(QString networkName,
                                                 QString shipID,
                                                 QVector<QString> portNames)
{
    // Check if the network exists in the thread-safe map
    if (!apiDataMap.contains(networkName)) {
        emit errorOccurred("A network with name " +
                           networkName +
                           " does not exist!");
        return;
    }

    // Retrieve the APIData for the specified network
    APIData apiData = apiDataMap.get(networkName);

    // Check if the ship exists in the ships map
    if (apiData.ships.contains(shipID)) {
        // Request ship to unload containers
        apiData.ships.value(shipID)->requestUnloadContainersAtPort(portNames);
    }

    // Emit an error if the ship does not exist
    emit errorOccurred("A ship with ID " + shipID + " does not exist!");


}


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
//                              HELPERS & UTILS
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

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

void SimulatorAPI::emitShipsReachedDestination()
{
    if (!mReachedDesTracker.isDataBufferEmpty()) {
        QJsonObject concatenatedObject;

        // Iterate over each entry in mReachedDesTracker
        auto networks = mReachedDesTracker.getDataBufferKeys();
        for (auto &network : networks) {
            // Insert each key-value pair into the concatenatedObject
            concatenatedObject.insert(network,
                                      mReachedDesTracker.getData(network));
        }

        emit shipsReachedDestination(concatenatedObject);
        mReachedDesTracker.clearDataBuffer();
    }
}

bool SimulatorAPI::checkAndEmitSignal(
    const int& counter, const int total,
    const QVector<QString>& networkNames,
    void(SimulatorAPI::*signal)(QVector<QString>),
    Mode mode) {

    switch (mode) {
    case Mode::Async:
        // In async mode, emit signal when counter reaches total
        if (counter == total) {
            (this->*signal)(networkNames);
            return true;
        }
        return false;

    case Mode::Sync:
        // In sync mode, emit signal immediately
        (this->*signal)(networkNames);
        return true;

    default:
        // Handle unexpected mode
        qWarning() << "Unexpected mode in checkAndEmitSignal";
        return false;
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
    runSimulation(QVector<QString> networkNames,
                  units::time::second_t timeSteps,
                  bool getProgressSignal)
{
    bool endSimulationAfterRun = false;

    if (timeSteps.value() < 0) {
        timeSteps =
            units::time::second_t(std::numeric_limits<double>::infinity());
    }

    getInstance().requestRunSimulation(networkNames, timeSteps,
                                       endSimulationAfterRun,
                                       getProgressSignal);
}

void SimulatorAPI::InteractiveMode::finalizeSimulation(
    QVector<QString> networkNames)
{
    getInstance().finalizeSimulation(networkNames);
}

void SimulatorAPI::InteractiveMode::
    terminateSimulation(QVector<QString> networkNames)
{
    getInstance().requestTerminateSimulation(networkNames);
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

void SimulatorAPI::InteractiveMode::requestAvailablePorts(
    QVector<QString> networkNames, bool getOnlyPortsOnShipsPaths)
{
    getInstance().requestAvailablePorts(networkNames,
                                        getOnlyPortsOnShipsPaths);
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


bool SimulatorAPI::InteractiveMode::isNetworkLoaded(QString networkName)
{
    return getInstance().isNetworkLoaded(networkName);
}

void SimulatorAPI::InteractiveMode::
    requestUnloadContainersAtPort(QString networkName,
                                  QString shipID, QVector<QString> portNames)
{
    getInstance().requestUnloadContainersAtPort(networkName,
                                                shipID,
                                                portNames);
}

void SimulatorAPI::InteractiveMode::resetAPI()
{
    getInstance().resetInstance();
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
    QVector<QString> networkNames, bool getProgressSignal)
{
    units::time::second_t timeSteps =
            units::time::second_t(std::numeric_limits<double>::infinity());
    bool endSimulationAfterRun = true;

    getInstance().requestRunSimulation(networkNames, timeSteps,
                                       endSimulationAfterRun,
                                       getProgressSignal);
}

void SimulatorAPI::ContinuousMode::
    pauseSimulation(QVector<QString> networkNames)
{
    getInstance().requestPauseSimulation(networkNames);
}

void SimulatorAPI::ContinuousMode::
    resumeSimulation(QVector<QString> networkNames)
{
    getInstance().requestResumeSimulation(networkNames);
}

void SimulatorAPI::ContinuousMode::terminateSimulation(
    QVector<QString> networkNames)
{
    getInstance().requestTerminateSimulation(networkNames);
}

ShipNetSimCore::Simulator*
SimulatorAPI::ContinuousMode::getSimulator(QString networkName) {
    return getInstance().getSimulator(networkName);
}

ShipNetSimCore::OptimizedNetwork*
SimulatorAPI::ContinuousMode::getNetwork(QString networkName) {
    return getInstance().getNetwork(networkName);
}

void SimulatorAPI::ContinuousMode::requestAvailablePorts(
    QVector<QString> networkNames, bool getOnlyPortsOnShipsPaths)
{
    getInstance().requestAvailablePorts(networkNames,
                                        getOnlyPortsOnShipsPaths);
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


bool SimulatorAPI::ContinuousMode::isNetworkLoaded(QString networkName)
{
    return getInstance().isNetworkLoaded(networkName);
}

void SimulatorAPI::ContinuousMode::
    requestUnloadContainersAtPort(QString networkName,
                                  QString shipID,
                                  QVector<QString> portNames)
{
    getInstance().requestUnloadContainersAtPort(networkName,
                                                shipID,
                                                portNames);
}

void SimulatorAPI::ContinuousMode::resetAPI()
{
    getInstance().resetInstance();
}

// --------------------------------------------------------------------------
// --------------- General Functions ----------------------------------------
// --------------------------------------------------------------------------

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
