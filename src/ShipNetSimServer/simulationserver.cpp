#include "simulatorapi.h"
#include "SimulationServer.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>
#include <container.h>
#include "./VersionConfig.h"
#include "utils/serverutils.h"

static const int MAX_RECONNECT_ATTEMPTS = 5;
static const int RECONNECT_DELAY_SECONDS = 5;  // Delay between reconnection attempts

static const std::string GithubLink = "https://github.com/VTTI-CSM/ShipNetSim";

static const std::string EXCHANGE_NAME = "CargoNetSim.Exchange";
static const std::string COMMAND_QUEUE_NAME = "CargoNetSim.CommandQueue.ShipNetSim";
static const std::string RESPONSE_QUEUE_NAME = "CargoNetSim.ResponseQueue.ShipNetSim";
static const std::string RECEIVING_ROUTING_KEY = "CargoNetSim.Command.ShipNetSim";
static const std::string PUBLISHING_ROUTING_KEY = "CargoNetSim.Response.ShipNetSim";

SimulationServer::SimulationServer(QObject *parent) :
    QObject(parent), mWorkerBusy(false)
{
    qRegisterMetaType<ShipParamsMap>("ShipParamsMap");
    setupServer();
}

void SimulationServer::setupServer() {

    // Connect the worker ready signal to resume RabbitMQ consumption
    // connect(&SimulatorAPI::InteractiveMode::getInstance(),
    //         &SimulatorAPI::workersReady, this,
    //         &SimulationServer::onWorkerReady);
    // Connect worker signals to slots
    connect(&SimulatorAPI::InteractiveMode::getInstance(),
            &SimulatorAPI::networkLoaded, this,
            &SimulationServer::onSimulationNetworkLoaded);
    connect(&SimulatorAPI::InteractiveMode::getInstance(),
            &SimulatorAPI::simulationCreated, this,
            &SimulationServer::onSimulationCreated);
    connect(&SimulatorAPI::InteractiveMode::getInstance(),
            &SimulatorAPI::simulationAdvanced, this,
            &SimulationServer::onSimulationAdvanced);
    connect(&SimulatorAPI::InteractiveMode::getInstance(),
            &SimulatorAPI::simulationsRestarted, this,
            &SimulationServer::onSimulationRestarted);
    connect(&SimulatorAPI::InteractiveMode::getInstance(),
            &::SimulatorAPI::simulationsTerminated, this,
            &SimulationServer::onSimulationTerminated);
    connect(&SimulatorAPI::InteractiveMode::getInstance(),
            &::SimulatorAPI::shipsAddedToSimulation, this,
            &SimulationServer::onShipAddedToSimulator);
    connect(&SimulatorAPI::InteractiveMode::getInstance(),
            &::SimulatorAPI::shipsReachedDestination, this,
            &SimulationServer::onShipReachedDestination);
    connect(&SimulatorAPI::InteractiveMode::getInstance(),
            &SimulatorAPI::simulationResultsAvailable, this,
            &SimulationServer::onSimulationResultsAvailable);
    connect(&SimulatorAPI::InteractiveMode::getInstance(),
            &SimulatorAPI::shipCurrentStateAvailable, this,
            &SimulationServer::onShipStateAvailable);
    connect(&SimulatorAPI::InteractiveMode::getInstance(),
            &SimulatorAPI::simulationCurrentStateAvailable, this,
            &SimulationServer::onSimulatorStateAvailable);
    connect(&SimulatorAPI::InteractiveMode::getInstance(),
            &SimulatorAPI::errorOccurred, this,
            &SimulationServer::onErrorOccurred);
    connect(&SimulatorAPI::InteractiveMode::getInstance(),
            &SimulatorAPI::containersAddedToShip, this,
            &SimulationServer::onContainersAddedToShip);
    connect(&SimulatorAPI::InteractiveMode::getInstance(),
            &SimulatorAPI::shipReachedSeaPort, this,
            &SimulationServer::onShipReachedSeaPort);
    connect(&SimulatorAPI::InteractiveMode::getInstance(),
            &SimulatorAPI::availablePorts, this,
            &SimulationServer::onPortsAvailable);
}

SimulationServer::~SimulationServer() {
    stopRabbitMQServer();  // Ensure server stops cleanly before destroying
    if (mRabbitMQThread) {
        mRabbitMQThread->quit();
        mRabbitMQThread->wait();
    }
}

void SimulationServer::startRabbitMQServer(const std::string &hostname,
                                           int port) {
    mHostname = hostname;
    mPort = port;
    reconnectToRabbitMQ();
}

void SimulationServer::reconnectToRabbitMQ() {
    int retryCount = 0;
    QString yearRange = QString("(C) %1-%2 ")
                            .arg(QDate::currentDate().year() - 1)
                            .arg(QDate::currentDate().year());

    // Show app details
    QString appDetails;
    QTextStream detailsStream(&appDetails);
    detailsStream << ShipNetSim_NAME
                  << " [Version " << ShipNetSim_VERSION << "]" << Qt::endl;
    detailsStream << yearRange << ShipNetSim_VENDOR << Qt::endl;
    detailsStream << QString::fromStdString(GithubLink) << Qt::endl;

    qInfo().noquote() << appDetails;

    while (retryCount < MAX_RECONNECT_ATTEMPTS) {

        // Initialize RabbitMQ connection
        mRabbitMQConnection = amqp_new_connection();
        amqp_socket_t *socket = amqp_tcp_socket_new(mRabbitMQConnection);
        if (!socket) {
            qCritical() << "Error: Unable to create RabbitMQ socket. "
                           "Retrying...";
            retryCount++;
            std::this_thread::sleep_for(
                std::chrono::seconds(RECONNECT_DELAY_SECONDS));

            // Reset pointers before retry
            mRabbitMQConnection = nullptr;
            socket = nullptr;

            continue;  // Retry
        }

        int status = amqp_socket_open(socket, mHostname.c_str(), mPort);
        if (status != AMQP_STATUS_OK) {
            qCritical() << "Error: Failed to open RabbitMQ socket on"
                        << mHostname.c_str() << ":" << mPort << ". Retrying...";
            retryCount++;
            std::this_thread::sleep_for(
                std::chrono::seconds(RECONNECT_DELAY_SECONDS));
            continue;  // Retry
        }

        amqp_rpc_reply_t loginRes =
            amqp_login(mRabbitMQConnection, "/", 0, 131072, 0,
                       AMQP_SASL_METHOD_PLAIN, "guest", "guest");
        if (loginRes.reply_type != AMQP_RESPONSE_NORMAL) {
            qCritical() << "Error: RabbitMQ login failed. Retrying...";
            retryCount++;
            std::this_thread::sleep_for(
                std::chrono::seconds(RECONNECT_DELAY_SECONDS));
            continue;  // Retry
        }

        amqp_channel_open(mRabbitMQConnection, 1);
        amqp_rpc_reply_t channelRes = amqp_get_rpc_reply(mRabbitMQConnection);
        if (channelRes.reply_type != AMQP_RESPONSE_NORMAL) {
            qCritical() << "Error: Unable to open RabbitMQ channel. "
                           "Retrying...";
            retryCount++;
            std::this_thread::sleep_for(
                std::chrono::seconds(RECONNECT_DELAY_SECONDS));

            // Reset pointers before retry
            mRabbitMQConnection = nullptr;
            socket = nullptr;

            continue;  // Retry
        }

        // Declare the exchange for simulation
        amqp_exchange_declare_ok_t *exchange_declare_res =
            amqp_exchange_declare(
                mRabbitMQConnection,
                1,
                amqp_cstring_bytes(EXCHANGE_NAME.c_str()), // Exchange name
                amqp_cstring_bytes("topic"),         // Exchange type
                0,                                   // passive (false)
                1,                                   // durable (true)
                0,                                   // auto-delete (false)
                0,                                   // internal (false)
                amqp_empty_table                     // no additional arguments
                );

        if (!exchange_declare_res) {
            qCritical() << "Error: Unable to declare exchange "
                        << EXCHANGE_NAME.c_str() << ". Retrying...";
            retryCount++;
            std::this_thread::sleep_for(
                std::chrono::seconds(RECONNECT_DELAY_SECONDS));
            continue;
        }

        // Declare the command queue to listen to commands
        amqp_queue_declare(
            mRabbitMQConnection, 1,
            amqp_cstring_bytes(COMMAND_QUEUE_NAME.c_str()),
            0, 1, 0, 0, amqp_empty_table);

        if (amqp_get_rpc_reply(mRabbitMQConnection).reply_type
            != AMQP_RESPONSE_NORMAL)
        {
            qCritical() << "Error: Unable to declare RabbitMQ command queue. "
                           "Retrying...";
            retryCount++;
            std::this_thread::sleep_for(
                std::chrono::seconds(RECONNECT_DELAY_SECONDS));
            continue;  // Retry
        }

        // Bind the command queue to the exchange with a routing key
        amqp_queue_bind(
            mRabbitMQConnection,
            1,
            amqp_cstring_bytes(COMMAND_QUEUE_NAME.c_str()), // Queue name
            amqp_cstring_bytes(EXCHANGE_NAME.c_str()),        // Exchange name
            amqp_cstring_bytes(RECEIVING_ROUTING_KEY.c_str()),  // Routing key
            amqp_empty_table                                 // Arguments
            );

        amqp_rpc_reply_t comQueueRes = amqp_get_rpc_reply(mRabbitMQConnection);
        if (comQueueRes.reply_type != AMQP_RESPONSE_NORMAL) {
            qCritical() << "Error: Unable to bind queue to exchange.  "
                           "Retrying...";
            retryCount++;
            std::this_thread::sleep_for(
                std::chrono::seconds(RECONNECT_DELAY_SECONDS));
            continue;  // Retry
        }

        // Declare the response queue to send commands
        amqp_queue_declare(
            mRabbitMQConnection, 1,
            amqp_cstring_bytes(RESPONSE_QUEUE_NAME.c_str()),
            0, 1, 0, 0, amqp_empty_table);

        if (amqp_get_rpc_reply(mRabbitMQConnection).reply_type
            != AMQP_RESPONSE_NORMAL)
        {
            qCritical() << "Error: Unable to declare RabbitMQ response queue. "
                           "Retrying...";
            retryCount++;
            std::this_thread::sleep_for(
                std::chrono::seconds(RECONNECT_DELAY_SECONDS));
            continue;  // Retry
        }

        // Bind the response queue to the exchange with a routing key
        amqp_queue_bind(
            mRabbitMQConnection,
            1,
            amqp_cstring_bytes(RESPONSE_QUEUE_NAME.c_str()), // Queue name
            amqp_cstring_bytes(EXCHANGE_NAME.c_str()),        // Exchange name
            amqp_cstring_bytes(PUBLISHING_ROUTING_KEY.c_str()),  // Routing key
            amqp_empty_table                                 // Arguments
            );

        amqp_rpc_reply_t resQueueRes = amqp_get_rpc_reply(mRabbitMQConnection);
        if (resQueueRes.reply_type != AMQP_RESPONSE_NORMAL) {
            qCritical() << "Error: Unable to bind queue to exchange.  "
                           "Retrying...";
            retryCount++;
            std::this_thread::sleep_for(
                std::chrono::seconds(RECONNECT_DELAY_SECONDS));
            continue;  // Retry
        }

        // Listen for messages
        amqp_basic_consume(
            mRabbitMQConnection, 1,
            amqp_cstring_bytes(COMMAND_QUEUE_NAME.c_str()),
            amqp_empty_bytes, 0, 0, 0, amqp_empty_table);

        if (amqp_get_rpc_reply(mRabbitMQConnection).reply_type
            != AMQP_RESPONSE_NORMAL)
        {
            qCritical() << "Error: Failed to start consuming "
                           "from the queue. Retrying...";
            retryCount++;
            std::this_thread::sleep_for(
                std::chrono::seconds(RECONNECT_DELAY_SECONDS));
            continue;  // Retry
        }

        qInfo() << "Simulator initialized successfully. Awaiting commands from "
                << mHostname.c_str() << ":" << mPort
                << ". The system is now fully operational.";

        startConsumingMessages(); // Start consuming


        return;  // Successful connection

    }

    qCritical() << "Error: Failed to establish a connection to "
                   "RabbitMQ after"
                << MAX_RECONNECT_ATTEMPTS
                << "attempts. Server initialization aborted.";
}

void SimulationServer::stopRabbitMQServer() {
    emit stopConsuming();

    // If the connection is already closed, just return
    if (mRabbitMQConnection == nullptr) {
        qDebug() << "RabbitMQ connection already closed.";
        return;
    }

    // Gracefully close the RabbitMQ channel
    amqp_channel_close(mRabbitMQConnection, 1, AMQP_REPLY_SUCCESS);

    // Gracefully close the RabbitMQ connection
    amqp_connection_close(mRabbitMQConnection, AMQP_REPLY_SUCCESS);

    // Destroy the RabbitMQ connection
    amqp_destroy_connection(mRabbitMQConnection);

    mRabbitMQConnection = nullptr;  // Set the connection to null to indicate it's closed

    qDebug() << "RabbitMQ server stopped cleanly.";
}

void SimulationServer::startConsumingMessages() {
    if (mRabbitMQThread) {
        mRabbitMQThread->quit();
        mRabbitMQThread->wait();
        mRabbitMQThread->deleteLater();  // Ensure proper deletion
    }

    // Move the consuming logic to a separate thread
    mRabbitMQThread = new QThread(this);
    connect(mRabbitMQThread , &QThread::started, this,
            &SimulationServer::consumeFromRabbitMQ);
    connect(this, &SimulationServer::stopConsuming,
            mRabbitMQThread , &QThread::quit);
    connect(mRabbitMQThread , &QThread::finished,
            mRabbitMQThread , &QThread::deleteLater);
    mRabbitMQThread->start();
}

void SimulationServer::consumeFromRabbitMQ() {
    while (true) {
        // Ensure the RabbitMQ connection is valid
        if (!mRabbitMQConnection) {
            qCritical() << "RabbitMQ connection is not valid. "
                           "Exiting consume loop.";
            break;
        }

        {
            QMutexLocker locker(&mMutex);
            if (mWorkerBusy) {
                mWaitCondition.wait(&mMutex, 100);  // Wait for 100ms or until notified
                continue;
            }
        }

        amqp_rpc_reply_t res;
        amqp_envelope_t envelope;

        // Wait for a new message from RabbitMQ (with a short timeout
        //                                       to avoid blocking)
        amqp_maybe_release_buffers(mRabbitMQConnection);
        struct timeval timeout;
        timeout.tv_sec = 0;          // No blocking
        timeout.tv_usec = 100000;    // 100ms timeout

        res = amqp_consume_message(mRabbitMQConnection, &envelope, &timeout, 0);

        if (res.reply_type == AMQP_RESPONSE_NORMAL) {
            // Acknowledge the message regardless of validity
            amqp_basic_ack(mRabbitMQConnection, 1, envelope.delivery_tag, 0);

            QByteArray messageData(
                static_cast<char *>(envelope.message.body.bytes),
                envelope.message.body.len);
            QJsonDocument doc = QJsonDocument::fromJson(messageData);
            QJsonObject jsonMessage = doc.object();

            emit dataReceived(jsonMessage);
            onDataReceivedFromRabbitMQ(jsonMessage, envelope);

            // Clean up the envelope
            amqp_destroy_envelope(&envelope);
        } else if (res.reply_type == AMQP_RESPONSE_LIBRARY_EXCEPTION &&
                   res.library_error == AMQP_STATUS_TIMEOUT) {
            // Timeout reached but no message available, continue to next iteration
            // Sleep for 100ms before checking again to avoid a busy loop
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        } else {
            qCritical() << "Error receiving message from RabbitMQ. Type:"
                        << res.reply_type;
            stopRabbitMQServer();
            qDebug() << "Attempting to reconnect...";
            reconnectToRabbitMQ();
            break;
        }

        // Sleep for 100ms between each iteration to avoid busy-looping
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void SimulationServer::onDataReceivedFromRabbitMQ(
    QJsonObject &message,
    const amqp_envelope_t &envelope) {

    {
        QMutexLocker locker(&mMutex);
        if (mWorkerBusy) {
            qDebug() << "Simulator is busy, not consuming new messages.";
            return;
        }
        mWorkerBusy = true;
    }

    processCommand(message);
}


void SimulationServer::processCommand(QJsonObject &jsonMessage) {

    auto checkJsonField = [this](const QJsonObject &json,
                                 const QString &fieldName,
                                 const QString &command) -> bool {
        if (!json.contains(fieldName)) {
            qWarning() << "Missing parameter:" << fieldName
                       << "in command:" << command;
            mWorkerBusy = false;
            return false;
        }
        return true;
    };

    QString command = jsonMessage["command"].toString();

    if (command == "defineSimulator") {
        qInfo() << "[Server] Received command: Initializing a new "
                   "simulation environment.";

        QString networkPath =
            ServerUtils::getOptionalString(jsonMessage, "networkFilePath");

        if (!checkJsonField(jsonMessage, "networkName", command) ||
            !checkJsonField(jsonMessage, "timeStep", command)) {
            qWarning() << "[Server] Missing required fields "
                          "for 'defineSimulator'.";
            return; // Skip processing this command
        }

        QString networkName =
            jsonMessage["networkName"].toString();
        double timeStepValue = jsonMessage["timeStep"].toDouble();

        qDebug() << "[Server] Loading network: " << networkName
                 << " with time step: " << timeStepValue << "s.";
        auto net =
            SimulatorAPI::InteractiveMode::loadNetwork(networkPath,
                                                       networkName);

        auto shipsList = SimulatorAPI::loadShips(jsonMessage, net);

        qDebug() << "[Server] Creating new simulation environment "
                    "for network: " << networkName;

        SimulatorAPI::InteractiveMode::createNewSimulationEnvironment(
            networkName, shipsList, units::time::second_t(timeStepValue), true,
            SimulatorAPI::Mode::Async);

    } else if (command == "runSimulator") {
        qInfo() << "[Server] Received command: Running simulation.";

        if (!checkJsonField(jsonMessage, "networkNames", command) ||
            !checkJsonField(jsonMessage, "byTimeSteps", command)) {
            qWarning() << "[Server] Missing required fields "
                          "for 'runSimulator'.";
            return; // Skip processing this command
        }
        QVector<QString> nets;
        QJsonArray networkNamesArray = jsonMessage["networkNames"].toArray();
        // Iterate over the QJsonArray and populate the QVector<QString>
        for (const QJsonValue &value : networkNamesArray) {
            if (value.isString()) {
                nets.append(value.toString());
            }
        }
        double runBy = jsonMessage["byTimeSteps"].toDouble(60);

        qDebug() << "[Server] Executing simulation for networks: " << nets
                 << " with time step duration: " << runBy << "s.";

        SimulatorAPI::InteractiveMode::runSimulation(
            nets, units::time::second_t(runBy));

    } else if (command == "terminateSimulator") {
        qInfo() << "[Server] Received command: Terminating simulation.";

        if (!checkJsonField(jsonMessage, "networkNames", command)) {
            qWarning() << "[Server] Missing required fields "
                          "for 'terminateSimulator'.";

            return; // Skip processing this command
        }

        QVector<QString> nets;
        QJsonArray networkNamesArray = jsonMessage["networkNames"].toArray();
        // Iterate over the QJsonArray and populate the QVector<QString>
        for (const QJsonValue &value : networkNamesArray) {
            if (value.isString()) {
                nets.append(value.toString());
            }
        }

        qDebug() << "[Server] Terminating simulation for networks: " << nets;

        SimulatorAPI::InteractiveMode::terminateSimulation(nets);

    } else if (command == "endSimulator") {
        qInfo() << "[Server] Received command: Ending simulation.";

        if (!checkJsonField(jsonMessage, "networkNames", command)) {
            qWarning() << "[Server] Missing required fields "
                          "for 'endSimulator'.";
            return; // Skip processing this command
        }
        QVector<QString> nets;
        QJsonArray networkNamesArray = jsonMessage["networkNames"].toArray();
        // Iterate over the QJsonArray and populate the QVector<QString>
        for (const QJsonValue &value : networkNamesArray) {
            if (value.isString()) {
                nets.append(value.toString());
            }
        }

        qDebug() << "[Server] Ending simulation for networks: " << nets;

        SimulatorAPI::InteractiveMode::endSimulation(nets);

    } else if (command == "addShipsToSimulator") {
        qInfo() << "[Server] Received command: Adding ships to the simulation.";


        if (!checkJsonField(jsonMessage, "networkName", command) ||
            !checkJsonField(jsonMessage, "ships", command)) {
            return; // Skip processing this command
        }
        QString net =
            jsonMessage["networkName"].toString();
        auto ships = SimulatorAPI::loadShips(jsonMessage, net);
        SimulatorAPI::InteractiveMode::addShipToSimulation(net, ships);

    } else if (command == "addContainersToShip") {
        qInfo() << "[Server] Received command: Adding containers to a ship.";

        if (!checkJsonField(jsonMessage, "networkName", command) ||
            !checkJsonField(jsonMessage, "shipID", command) ||
            !checkJsonField(jsonMessage, "containers", command)) {
            qWarning() << "[Server] Missing required fields "
                          "for 'addContainersToShip'.";
            return; // Skip processing this command
        }
        QString net =
            jsonMessage["networkName"].toString();
        QString shipID =
            jsonMessage["shipID"].toString();

        qDebug() << "[Server] Adding containers to ship " << shipID
                 << " in network: " << net;

        SimulatorAPI::InteractiveMode::addContainersToShip(net, shipID,
                                                           jsonMessage);

    } else if (command == "getNetworkSeaPorts") {
        qInfo() << "[Server] Received command: Getting a list of sea ports.";

        if (!checkJsonField(jsonMessage, "networkName", command)) {
            qWarning() << "[Server] Missing required fields "
                          "for 'getNetworkSeaPorts'.";
            return; // Skip processing this command
        }

        QString net =
            jsonMessage["networkName"].toString();

        bool considerShipsPathOnly =
            jsonMessage["considerShipsPathOnly"].toBool(false);  // false is the default value

        SimulatorAPI::InteractiveMode::
            requestAvailablePorts({net}, considerShipsPathOnly);


    } else if (command == "restServer") {
        qInfo() << "[Server] Received command: Resetting the server.";

        SimulatorAPI::InteractiveMode::resetAPI();
        onServerReset();

    } else {
        qWarning() << "Unrecognized command:" << command;
        onWorkerReady();
    }

}

void SimulationServer::onWorkerReady() {
    // Worker is now ready for the next command
    QMutexLocker locker(&mMutex);

    // Worker is now ready for the next command
    mWorkerBusy = false;

    mWaitCondition.wakeAll();  // Notify waiting consumers

}

void SimulationServer::sendRabbitMQMessage(const QString &routingKey,
                                           const QJsonObject &message) {
    QByteArray messageData = QJsonDocument(message).toJson();

    amqp_bytes_t messageBytes;
    messageBytes.len = messageData.size();
    messageBytes.bytes = messageData.data();

    int publishStatus =
        amqp_basic_publish(
            mRabbitMQConnection, 1, amqp_cstring_bytes(EXCHANGE_NAME.c_str()),
            amqp_cstring_bytes(routingKey.toUtf8().constData()),
            0, 0, nullptr, messageBytes);

    if (publishStatus != AMQP_STATUS_OK) {
        qCritical() << "Failed to publish message to "
                       "RabbitMQ with routing key:"
                    << routingKey;
    }
}


// simulation events handling
void SimulationServer::onSimulationNetworkLoaded(QString networkName) {
    QJsonObject jsonMessage;
    jsonMessage["event"] = "simulationNetworkLoaded";
    jsonMessage["networkName"] = networkName;
    jsonMessage["host"] = ShipNetSim_NAME;
    sendRabbitMQMessage(PUBLISHING_ROUTING_KEY.c_str(),
                        jsonMessage);

    onWorkerReady();
}

void SimulationServer::onSimulationCreated(QString networkName) {
    QJsonObject jsonMessage;
    jsonMessage["event"] = "simulationCreated";
    jsonMessage["networkName"] = networkName;
    jsonMessage["host"] = ShipNetSim_NAME;
    sendRabbitMQMessage(PUBLISHING_ROUTING_KEY.c_str(),
                        jsonMessage);

    onWorkerReady();
}

void SimulationServer::onSimulationPaused(QVector<QString> networkNames) {
    QJsonObject jsonMessage;
    jsonMessage["event"] = "simulationPaused";
    QJsonArray networkNamesArray;
    for (const QString &name : networkNames) {
        networkNamesArray.append(name);
    }
    jsonMessage["networkNames"] = networkNamesArray;
    jsonMessage["host"] = ShipNetSim_NAME;
    sendRabbitMQMessage(PUBLISHING_ROUTING_KEY.c_str(),
                        jsonMessage);

    onWorkerReady();
}

void SimulationServer::onSimulationResumed(QVector<QString> networkNames) {
    QJsonObject jsonMessage;
    jsonMessage["event"] = "simulationResumed";
    QJsonArray networkNamesArray;
    for (const QString &name : networkNames) {
        networkNamesArray.append(name);
    }
    jsonMessage["networkNames"] = networkNamesArray;
    jsonMessage["host"] = ShipNetSim_NAME;
    sendRabbitMQMessage(PUBLISHING_ROUTING_KEY.c_str(),
                        jsonMessage);

    onWorkerReady();
}

void SimulationServer::onSimulationRestarted(QVector<QString> networkNames) {
    QJsonObject jsonMessage;
    jsonMessage["event"] = "simulationRestarted";
    QJsonArray networkNamesArray;
    for (const QString &name : networkNames) {
        networkNamesArray.append(name);
    }
    jsonMessage["networkNames"] = networkNamesArray;
    jsonMessage["host"] = ShipNetSim_NAME;
    sendRabbitMQMessage(PUBLISHING_ROUTING_KEY.c_str(),
                        jsonMessage);

    onWorkerReady();
}

void SimulationServer::onSimulationTerminated(QVector<QString> networkNames) {
    QJsonObject jsonMessage;
    jsonMessage["event"] = "simulationEnded";
    QJsonArray networkNamesArray;
    for (const QString &name : networkNames) {
        networkNamesArray.append(name);
    }
    jsonMessage["networkNames"] = networkNamesArray;
    jsonMessage["host"] = ShipNetSim_NAME;
    sendRabbitMQMessage(PUBLISHING_ROUTING_KEY.c_str(),
                        jsonMessage);

    onWorkerReady();
}

void SimulationServer::onSimulationAdvanced(
    QMap<QString, QPair<units::time::second_t, double>> newSimulationTime) {
    QJsonObject jsonMessage;
    jsonMessage["event"] = "simulationAdvanced";
    jsonMessage["host"] = ShipNetSim_NAME;

    QJsonObject jsonNetworkTimes;
    for (auto it = newSimulationTime.constBegin();
         it != newSimulationTime.constEnd(); ++it) {
        // Add each network name (key) and its corresponding
        // simulation time (value) to the jsonNetworkTimes object
        jsonNetworkTimes[it.key()] = it.value().first.value();
    }
    // Add the network names to the JSON message
    jsonMessage["networkNamesTimes"] = jsonNetworkTimes;

    QJsonObject jsonNetworkProgress;
    for (auto it = newSimulationTime.constBegin();
         it != newSimulationTime.constEnd(); ++it) {
        // Add each network name (key) and its corresponding
        // simulation time (value) to the jsonNetworkProgress object
        jsonNetworkProgress[it.key()] = it.value().second;
    }
    // Add the network names to the JSON message
    jsonMessage["networkNamesProgress"] = jsonNetworkProgress;

    sendRabbitMQMessage(PUBLISHING_ROUTING_KEY.c_str(),
                        jsonMessage);

    onWorkerReady();
}

void SimulationServer::onSimulationProgressUpdate(int progressPercentage)
{
    QJsonObject jsonMessage;
    jsonMessage["event"] = "simulationProgressUpdate";
    jsonMessage["newProgress"] = progressPercentage;
    jsonMessage["host"] = ShipNetSim_NAME;
    sendRabbitMQMessage(PUBLISHING_ROUTING_KEY.c_str(),
                        jsonMessage);

    onWorkerReady();
}

void SimulationServer::onShipAddedToSimulator(const QString networkName,
                                              const QVector<QString> shipIDs) {
    QJsonObject jsonMessage;
    jsonMessage["event"] = "shipAddedToSimulator";
    QJsonArray shipsJson;
    for (const auto& shipID : shipIDs) {
        shipsJson.append(shipID);
    }
    jsonMessage["shipIDs"] = shipsJson;
    jsonMessage["host"] = ShipNetSim_NAME;
    sendRabbitMQMessage(PUBLISHING_ROUTING_KEY.c_str(),
                        jsonMessage);

    onWorkerReady();
}

void SimulationServer::onShipReachedDestination(const QJsonObject shipStatus) {
    QJsonObject jsonMessage;
    jsonMessage["event"] = "shipReachedDestination";
    jsonMessage["state"] = shipStatus;
    jsonMessage["host"] = ShipNetSim_NAME;
    sendRabbitMQMessage(PUBLISHING_ROUTING_KEY.c_str(),
                        jsonMessage);

    onWorkerReady();
}

void SimulationServer::onShipStateAvailable(const QJsonObject shipState)  {
    QJsonObject jsonMessage;
    jsonMessage["event"] = "ShipState";
    jsonMessage["state"] = shipState;
    jsonMessage["host"] = ShipNetSim_NAME;
    sendRabbitMQMessage(PUBLISHING_ROUTING_KEY.c_str(),
                        jsonMessage);

    onWorkerReady();
}

void SimulationServer::
    onSimulatorStateAvailable(const QJsonObject simulatorState)
{
    QJsonObject jsonMessage;
    jsonMessage["event"] = "simulatorState";
    jsonMessage["state"] = simulatorState;
    jsonMessage["host"] = ShipNetSim_NAME;
    sendRabbitMQMessage(PUBLISHING_ROUTING_KEY.c_str(),
                        jsonMessage);

    onWorkerReady();
}

void SimulationServer::onSimulationResultsAvailable(
    QMap<QString, ShipsResults> results)
{
    QJsonObject jsonMessage;
    jsonMessage["event"] = "simulationResultsAvailable";

    QJsonObject resultData;
    for (auto it = results.constBegin(); it != results.constEnd(); ++it) {
        resultData.insert(it.key(), it.value().toJson());
    }
    jsonMessage["results"] = resultData;

    jsonMessage["host"] = ShipNetSim_NAME;
    sendRabbitMQMessage(PUBLISHING_ROUTING_KEY.c_str(),
                        jsonMessage);

    onWorkerReady();
}

void SimulationServer::onContainersAddedToShip(QString networkName,
                                                QString shipID)
{
    QJsonObject jsonMessage;
    jsonMessage["event"] = "containersAddedToShip";
    jsonMessage["networkName"] = networkName;
    jsonMessage["shipID"] = shipID;
    jsonMessage["host"] = ShipNetSim_NAME;

    sendRabbitMQMessage(PUBLISHING_ROUTING_KEY.c_str(),
                        jsonMessage);
    onWorkerReady();
}

void SimulationServer::onShipReachedSeaPort(
    QString networkName, QString shipID,
    QString seaPortCode, QJsonArray containers)
{
    QJsonObject jsonMessage;
    jsonMessage["event"] = "shipReachedSeaPort";
    jsonMessage["networkName"] = networkName;
    jsonMessage["shipID"] = shipID;
    jsonMessage["SeaPortCode"] = seaPortCode;
    jsonMessage["host"] = ShipNetSim_NAME;
    jsonMessage["containers"] = containers;

    sendRabbitMQMessage(PUBLISHING_ROUTING_KEY.c_str(),
                        jsonMessage);

    onWorkerReady();
}

void SimulationServer::onPortsAvailable(QMap<QString,
                                             QVector<QString>> networkPorts)
{
    QJsonObject jsonMessage;
    jsonMessage["event"] = "containersAddedToShip";
    jsonMessage["host"] = ShipNetSim_NAME;

    // Convert QMap<QString, QVector<QString>> to QJsonObject
    QJsonObject portsJson;
    for (auto it = networkPorts.constBegin();
         it != networkPorts.constEnd(); ++it)
    {
        QJsonArray portArray;
        for (const auto &port : it.value())
        {
            portArray.append(port);
        }
        portsJson[it.key()] = portArray;
    }

    jsonMessage["network_ports"] = portsJson;

    sendRabbitMQMessage(PUBLISHING_ROUTING_KEY.c_str(),
                        jsonMessage);
    onWorkerReady();
}

void SimulationServer::onErrorOccurred(const QString &errorMessage) {
    QJsonObject jsonMessage;
    jsonMessage["event"] = "errorOccurred";
    jsonMessage["errorMessage"] = errorMessage;
    jsonMessage["host"] = ShipNetSim_NAME;
    sendRabbitMQMessage(PUBLISHING_ROUTING_KEY.c_str(),
                        jsonMessage);

    onWorkerReady();
}

void SimulationServer::onServerReset() {
    setupServer();
    QJsonObject jsonMessage;
    jsonMessage["event"] = "serverReset";
    jsonMessage["host"] = ShipNetSim_NAME;
    sendRabbitMQMessage(PUBLISHING_ROUTING_KEY.c_str(),
                        jsonMessage);
    onWorkerReady();
    qInfo() << "Server reset Successfully!";
}
