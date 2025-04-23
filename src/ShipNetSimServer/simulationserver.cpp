#include "SimulationServer.h"
#include "./VersionConfig.h"
#include "simulatorapi.h"
#include "utils/serverutils.h"
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <containerLib/container.h>

static const int MAX_RECONNECT_ATTEMPTS = 5;
static const int RECONNECT_DELAY_SECONDS =
    5; // Delay between reconnection attempts

static const std::string GithubLink =
    "https://github.com/VTTI-CSM/ShipNetSim";

static const std::string EXCHANGE_NAME =
    "CargoNetSim.Exchange";
static const std::string COMMAND_QUEUE_NAME =
    "CargoNetSim.CommandQueue.ShipNetSim";
static const std::string RESPONSE_QUEUE_NAME =
    "CargoNetSim.ResponseQueue.ShipNetSim";
static const std::string RECEIVING_ROUTING_KEY =
    "CargoNetSim.Command.ShipNetSim";
static const std::string PUBLISHING_ROUTING_KEY =
    "CargoNetSim.Response.ShipNetSim";

static const int MAX_SEND_COMMAND_RETRIES = 3;

SimulationServer::SimulationServer(QObject *parent)
    : QObject(parent)
    , mWorkerBusy(false)
{
    qRegisterMetaType<ShipParamsMap>("ShipParamsMap");
    setupServer();
}

void SimulationServer::setupServer()
{
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
            &SimulatorAPI::simulationsTerminated, this,
            &SimulationServer::onSimulationTerminated);
    connect(&SimulatorAPI::InteractiveMode::getInstance(),
            &SimulatorAPI::simulationFinished, this,
            &SimulationServer::onSimulationFinished);
    connect(&SimulatorAPI::InteractiveMode::getInstance(),
            &SimulatorAPI::shipsAddedToSimulation, this,
            &SimulationServer::onShipAddedToSimulator);
    connect(&SimulatorAPI::InteractiveMode::getInstance(),
            &SimulatorAPI::shipsReachedDestination, this,
            &SimulationServer::onShipReachedDestination);
    connect(&SimulatorAPI::InteractiveMode::getInstance(),
            &SimulatorAPI::allShipsReachedDestination, this,
            &SimulationServer::onAllShipsReachDestination);
    connect(
        &SimulatorAPI::InteractiveMode::getInstance(),
        &SimulatorAPI::simulationResultsAvailable, this,
        &SimulationServer::onSimulationResultsAvailable);
    connect(&SimulatorAPI::InteractiveMode::getInstance(),
            &SimulatorAPI::shipStateAvailable, this,
            &SimulationServer::onShipStateAvailable);
    connect(&SimulatorAPI::InteractiveMode::getInstance(),
            &SimulatorAPI::simulationCurrentStateAvailable,
            this,
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
    connect(&SimulatorAPI::InteractiveMode::getInstance(),
            &SimulatorAPI::ContainersUnloaded, this,
            &SimulationServer::onContainersUnloaded);
}

SimulationServer::~SimulationServer()
{
    stopRabbitMQServer(); // Ensure server stops cleanly
                          // before destroying
    if (mRabbitMQThread)
    {
        mRabbitMQThread->quit();
        mRabbitMQThread->wait();
    }
}

void SimulationServer::startRabbitMQServer(
    const std::string &hostname, int port)
{
    mHostname = hostname;
    mPort     = port;
    reconnectToRabbitMQ();
}

void SimulationServer::reconnectToRabbitMQ()
{
    int     retryCount = 0;
    QString yearRange =
        QString("(C) %1-%2 ")
            .arg(QDate::currentDate().year() - 1)
            .arg(QDate::currentDate().year());

    // Show app details
    QString     appDetails;
    QTextStream detailsStream(&appDetails);
    detailsStream << ShipNetSim_NAME << " [Version "
                  << ShipNetSim_VERSION << "]" << Qt::endl;
    detailsStream << yearRange << ShipNetSim_VENDOR
                  << Qt::endl;
    detailsStream << QString::fromStdString(GithubLink)
                  << Qt::endl;

    qInfo().noquote() << appDetails;

    while (retryCount < MAX_RECONNECT_ATTEMPTS)
    {
        // Initialize RabbitMQ connection
        mRabbitMQConnection = amqp_new_connection();
        amqp_socket_t *socket =
            amqp_tcp_socket_new(mRabbitMQConnection);
        if (!socket)
        {
            qCritical() << "Error: Unable to create "
                           "RabbitMQ socket. "
                           "Retrying...";
            retryCount++;
            std::this_thread::sleep_for(
                std::chrono::seconds(
                    RECONNECT_DELAY_SECONDS));

            // Reset pointers before retry
            mRabbitMQConnection = nullptr;
            socket              = nullptr;

            continue; // Retry
        }

        int status = amqp_socket_open(
            socket, mHostname.c_str(), mPort);
        if (status != AMQP_STATUS_OK)
        {
            qCritical() << "Error: Failed to open RabbitMQ "
                           "socket on"
                        << mHostname.c_str() << ":" << mPort
                        << ". Retrying...";
            retryCount++;
            std::this_thread::sleep_for(
                std::chrono::seconds(
                    RECONNECT_DELAY_SECONDS));
            continue; // Retry
        }

        amqp_rpc_reply_t loginRes = amqp_login(
            mRabbitMQConnection, "/", 0, 131072, 0,
            AMQP_SASL_METHOD_PLAIN, "guest", "guest");
        if (loginRes.reply_type != AMQP_RESPONSE_NORMAL)
        {
            qCritical() << "Error: RabbitMQ login failed. "
                           "Retrying...";
            retryCount++;
            std::this_thread::sleep_for(
                std::chrono::seconds(
                    RECONNECT_DELAY_SECONDS));
            continue; // Retry
        }

        amqp_channel_open(mRabbitMQConnection, 1);
        amqp_rpc_reply_t channelRes =
            amqp_get_rpc_reply(mRabbitMQConnection);
        if (channelRes.reply_type != AMQP_RESPONSE_NORMAL)
        {
            qCritical() << "Error: Unable to open RabbitMQ "
                           "channel. "
                           "Retrying...";
            retryCount++;
            std::this_thread::sleep_for(
                std::chrono::seconds(
                    RECONNECT_DELAY_SECONDS));

            // Reset pointers before retry
            mRabbitMQConnection = nullptr;
            socket              = nullptr;

            continue; // Retry
        }

        // Declare the exchange for simulation
        amqp_exchange_declare_ok_t *exchange_declare_res =
            amqp_exchange_declare(
                mRabbitMQConnection, 1,
                amqp_cstring_bytes(
                    EXCHANGE_NAME.c_str()), // Exchange name
                amqp_cstring_bytes(
                    "topic"),    // Exchange type
                0,               // passive (false)
                1,               // durable (true)
                0,               // auto-delete (false)
                0,               // internal (false)
                amqp_empty_table // no additional arguments
            );

        if (!exchange_declare_res)
        {
            qCritical()
                << "Error: Unable to declare exchange "
                << EXCHANGE_NAME.c_str() << ". Retrying...";
            retryCount++;
            std::this_thread::sleep_for(
                std::chrono::seconds(
                    RECONNECT_DELAY_SECONDS));
            continue;
        }

        // Declare the command queue to listen to commands
        amqp_queue_declare(
            mRabbitMQConnection, 1,
            amqp_cstring_bytes(COMMAND_QUEUE_NAME.c_str()),
            0, 1, 0, 0, amqp_empty_table);

        if (amqp_get_rpc_reply(mRabbitMQConnection)
                .reply_type
            != AMQP_RESPONSE_NORMAL)
        {
            qCritical() << "Error: Unable to declare "
                           "RabbitMQ command queue. "
                           "Retrying...";
            retryCount++;
            std::this_thread::sleep_for(
                std::chrono::seconds(
                    RECONNECT_DELAY_SECONDS));
            continue; // Retry
        }

        // Bind the command queue to the exchange with a
        // routing key
        amqp_queue_bind(
            mRabbitMQConnection, 1,
            amqp_cstring_bytes(
                COMMAND_QUEUE_NAME.c_str()), // Queue name
            amqp_cstring_bytes(
                EXCHANGE_NAME.c_str()), // Exchange name
            amqp_cstring_bytes(RECEIVING_ROUTING_KEY
                                   .c_str()), // Routing key
            amqp_empty_table                  // Arguments
        );

        amqp_rpc_reply_t comQueueRes =
            amqp_get_rpc_reply(mRabbitMQConnection);
        if (comQueueRes.reply_type != AMQP_RESPONSE_NORMAL)
        {
            qCritical() << "Error: Unable to bind queue to "
                           "exchange.  "
                           "Retrying...";
            retryCount++;
            std::this_thread::sleep_for(
                std::chrono::seconds(
                    RECONNECT_DELAY_SECONDS));
            continue; // Retry
        }

        // Declare the response queue to send commands
        amqp_queue_declare(
            mRabbitMQConnection, 1,
            amqp_cstring_bytes(RESPONSE_QUEUE_NAME.c_str()),
            0, 1, 0, 0, amqp_empty_table);

        if (amqp_get_rpc_reply(mRabbitMQConnection)
                .reply_type
            != AMQP_RESPONSE_NORMAL)
        {
            qCritical() << "Error: Unable to declare "
                           "RabbitMQ response queue. "
                           "Retrying...";
            retryCount++;
            std::this_thread::sleep_for(
                std::chrono::seconds(
                    RECONNECT_DELAY_SECONDS));
            continue; // Retry
        }

        // Bind the response queue to the exchange with a
        // routing key
        amqp_queue_bind(
            mRabbitMQConnection, 1,
            amqp_cstring_bytes(
                RESPONSE_QUEUE_NAME.c_str()), // Queue name
            amqp_cstring_bytes(
                EXCHANGE_NAME.c_str()), // Exchange name
            amqp_cstring_bytes(PUBLISHING_ROUTING_KEY
                                   .c_str()), // Routing key
            amqp_empty_table                  // Arguments
        );

        amqp_rpc_reply_t resQueueRes =
            amqp_get_rpc_reply(mRabbitMQConnection);
        if (resQueueRes.reply_type != AMQP_RESPONSE_NORMAL)
        {
            qCritical() << "Error: Unable to bind queue to "
                           "exchange.  "
                           "Retrying...";
            retryCount++;
            std::this_thread::sleep_for(
                std::chrono::seconds(
                    RECONNECT_DELAY_SECONDS));
            continue; // Retry
        }

        // Listen for messages
        amqp_basic_consume(
            mRabbitMQConnection, 1,
            amqp_cstring_bytes(COMMAND_QUEUE_NAME.c_str()),
            amqp_empty_bytes, 0, 0, 0, amqp_empty_table);

        if (amqp_get_rpc_reply(mRabbitMQConnection)
                .reply_type
            != AMQP_RESPONSE_NORMAL)
        {
            qCritical()
                << "Error: Failed to start consuming "
                   "from the queue. Retrying...";
            retryCount++;
            std::this_thread::sleep_for(
                std::chrono::seconds(
                    RECONNECT_DELAY_SECONDS));
            continue; // Retry
        }

        qInfo() << "Simulator initialized successfully. "
                   "Awaiting commands from "
                << mHostname.c_str() << ":" << mPort
                << ". The system is now fully operational.";

        startConsumingMessages(); // Start consuming

        return; // Successful connection
    }

    qCritical()
        << "Error: Failed to establish a connection to "
           "RabbitMQ after"
        << MAX_RECONNECT_ATTEMPTS
        << "attempts. Server initialization aborted.";
}

void SimulationServer::stopRabbitMQServer()
{
    emit stopConsuming();

    // If the connection is already closed, just return
    if (mRabbitMQConnection == nullptr)
    {
        qDebug() << "RabbitMQ connection already closed.";
        return;
    }

    // Gracefully close the RabbitMQ channel
    amqp_channel_close(mRabbitMQConnection, 1,
                       AMQP_REPLY_SUCCESS);

    // Gracefully close the RabbitMQ connection
    amqp_connection_close(mRabbitMQConnection,
                          AMQP_REPLY_SUCCESS);

    // Destroy the RabbitMQ connection
    if (mRabbitMQConnection)
        amqp_destroy_connection(mRabbitMQConnection);

    mRabbitMQConnection =
        nullptr; // Set the connection to null to indicate
                 // it's closed

    qDebug() << "RabbitMQ server stopped cleanly.";
}

void SimulationServer::startConsumingMessages()
{
    if (mRabbitMQThread)
    {
        mRabbitMQThread->quit();
        mRabbitMQThread->wait();
        mRabbitMQThread
            ->deleteLater(); // Ensure proper deletion
    }

    // Move the consuming logic to a separate thread
    mRabbitMQThread = new QThread(this);
    connect(mRabbitMQThread, &QThread::started, this,
            &SimulationServer::consumeFromRabbitMQ);
    connect(this, &SimulationServer::stopConsuming,
            mRabbitMQThread, &QThread::quit);
    connect(mRabbitMQThread, &QThread::finished,
            mRabbitMQThread, &QThread::deleteLater);
    mRabbitMQThread->start();
}

void SimulationServer::consumeFromRabbitMQ()
{
    while (true)
    {
        // Ensure the RabbitMQ connection is valid
        if (!mRabbitMQConnection)
        {
            qCritical()
                << "RabbitMQ connection is not valid. "
                   "Exiting consume loop.";
            break;
        }

        // Check if worker is busy before attempting to
        // consume messages
        bool workerIsBusy = false;
        {
            QMutexLocker locker(&mMutex);
            workerIsBusy = mWorkerBusy;
        }

        if (workerIsBusy)
        {
            // Process events while waiting for worker to be
            // ready
            QCoreApplication::processEvents();
            QThread::msleep(
                100); // Small sleep to prevent CPU hogging
            continue; // Skip message consumption and check
                      // again
        }

        amqp_rpc_reply_t res;
        amqp_envelope_t  envelope;

        // Wait for a new message from RabbitMQ (with a
        // short timeout
        //                                       to avoid
        //                                       blocking)
        amqp_maybe_release_buffers(mRabbitMQConnection);
        struct timeval timeout;
        timeout.tv_sec  = 0;      // No blocking
        timeout.tv_usec = 100000; // 100ms timeout

        res = amqp_consume_message(mRabbitMQConnection,
                                   &envelope, &timeout, 0);

        if (res.reply_type == AMQP_RESPONSE_NORMAL)
        {
            // Acknowledge the message regardless of
            // validity
            amqp_basic_ack(mRabbitMQConnection, 1,
                           envelope.delivery_tag, 0);

            QByteArray messageData(
                static_cast<char *>(
                    envelope.message.body.bytes),
                envelope.message.body.len);
            QJsonDocument doc =
                QJsonDocument::fromJson(messageData);
            QJsonObject jsonMessage = doc.object();

            emit dataReceived(jsonMessage);
            onDataReceivedFromRabbitMQ(jsonMessage,
                                       envelope);

            // Clean up the envelope
            amqp_destroy_envelope(&envelope);
        }
        else if (res.reply_type
                     == AMQP_RESPONSE_LIBRARY_EXCEPTION
                 && res.library_error
                        == AMQP_STATUS_TIMEOUT)
        {
            // Timeout reached but no message available,
            // continue to next iteration Sleep for 100ms
            // before checking again to avoid a busy loop
            QThread::msleep(50);
        }
        else
        {
            qCritical() << "Error receiving message from "
                           "RabbitMQ. Type:"
                        << res.reply_type;
            stopRabbitMQServer();
            qDebug() << "Attempting to reconnect...";
            reconnectToRabbitMQ();
            break;
        }

        // Process events to keep the application responsive
        QCoreApplication::processEvents();
    }
}

void SimulationServer::onDataReceivedFromRabbitMQ(
    QJsonObject &message, const amqp_envelope_t &envelope)
{
    {
        QMutexLocker locker(&mMutex);
        if (mWorkerBusy)
        {
            qInfo() << "Simulator is busy, not consuming "
                       "new messages.";
            return;
        }
        mWorkerBusy = true;
    }

    // Ensure mWorkerBusy is reset even if an exception
    // occurs
    struct WorkerGuard
    {
        SimulationServer *server;
        ~WorkerGuard()
        {
            server->onWorkerReady();
        }
    };
    WorkerGuard guard{this};

    try
    {
        processCommand(message);
    }
    catch (const std::exception &e)
    {
        qCritical()
            << "Unhandled exception in processCommand: "
            << e.what();
        onErrorOccurred("Internal server error: "
                        + QString(e.what()));
    }
    catch (...)
    {
        qCritical() << "Unknown error in processCommand";
        onErrorOccurred("Internal server error");
    }
}

QPair<bool, QString> SimulationServer::checkJsonField(
    const QJsonObject &json, const QString &fieldName,
    const QString &command, bool checkParamsObject)
{
    // Check directly in the root object
    if (json.contains(fieldName))
    {
        return {true, ""};
    }

    // If instructed, also check in the params object
    if (checkParamsObject && json.contains("params")
        && json["params"].isObject())
    {
        QJsonObject params = json["params"].toObject();
        if (params.contains(fieldName))
        {
            return {true, ""};
        }
    }

    QString error = "Missing parameter: " + fieldName
                    + " in command: " + command;
    qWarning() << error;
    return {false, error};
}

QJsonValue
SimulationServer::getJsonValue(const QJsonObject &json,
                               const QString     &fieldName)
{
    // Check directly in the root object
    if (json.contains(fieldName))
    {
        return json[fieldName];
    }

    // Also check in the params object
    if (json.contains("params")
        && json["params"].isObject())
    {
        QJsonObject params = json["params"].toObject();
        if (params.contains(fieldName))
        {
            return params[fieldName];
        }
    }

    return QJsonValue();
}

QPair<bool, QString> SimulationServer::validateArray(
    const QJsonObject &json, const QString &fieldName,
    const QString &commandName, bool allowEmpty,
    bool checkElementsAreStrings)
{
    QJsonValue value = getJsonValue(json, fieldName);

    if (value.isUndefined())
    {
        QString error = "Missing parameter: " + fieldName
                        + " in command: " + commandName;
        qWarning() << error;
        return {false, error};
    }

    if (!value.isArray())
    {
        QString error = "'" + fieldName
                        + "' must be an array for command: "
                        + commandName;
        qWarning() << error;
        return {false, error};
    }

    QJsonArray arr = value.toArray();
    if (!allowEmpty && arr.isEmpty())
    {
        QString error =
            "'" + fieldName
            + "' array cannot be empty for command: "
            + commandName;
        qWarning() << error;
        return {false, error};
    }

    if (checkElementsAreStrings)
    {
        for (const QJsonValue &val : arr)
        {
            if (!val.isString())
            {
                QString error = "'" + fieldName
                                + "' array contains "
                                  "non-string elements "
                                  "for command: "
                                + commandName;
                qWarning() << error;
                return {false, error};
            }
        }
    }
    return {true, ""};
}

void SimulationServer::processCommand(
    QJsonObject &jsonMessage)
{
    // Extract the command
    if (!jsonMessage.contains("command"))
    {
        onErrorOccurred(
            "Missing 'command' field in the message");
        return;
    }
    QString command = jsonMessage["command"].toString();

    // Extract commandId if present
    if (jsonMessage.contains("commandId"))
    {
        commandID = jsonMessage["commandId"].toString();
    }

    // Handle each command with improved error checking
    if (command == "checkConnection")
    {
        // Log the event for debugging (optional but
        // recommended)
        qInfo() << "[Server] Received command: "
                   "checkConnection. "
                   "Responding with 'connected'.";

        // Create the response JSON object
        QJsonObject response;
        response["event"] =
            "connectionStatus"; // Identifies the response
                                // type
        response["status"] =
            "connected"; // Confirms the server is connected
        response["host"] =
            "ShipNetSim";           // Identifies the server
        response["success"] = true; // Add success flag

        // Only include commandId if it was in the original
        // request
        if (!commandID.isEmpty())
        {
            response["commandId"] = commandID;
        }

        // Send the response via RabbitMQ
        sendRabbitMQMessage(PUBLISHING_ROUTING_KEY.c_str(),
                            response);

        // Signal that the worker is ready for the next
        // command
        onWorkerReady();
        return;
    }
    else if (command == "defineSimulator")
    {
        qInfo() << "[Server] Received command: "
                   "Initializing a new "
                   "simulation environment.";

        QString networkPath =
            ServerUtils::getOptionalString(
                jsonMessage, "networkFilePath");

        // Validate required fields
        QList<QPair<bool, QString>> checks;
        checks << checkJsonField(jsonMessage, "networkName",
                                 command);
        checks << checkJsonField(jsonMessage, "timeStep",
                                 command);

        // Collect errors
        QStringList errors;
        for (const auto &check : checks)
        {
            if (!check.first)
            {
                errors << check.second;
            }
        }
        if (!errors.isEmpty())
        {
            onErrorOccurred(errors.join("; "));
            return;
        }

        // Extract fields using getJsonValue
        QString networkName =
            getJsonValue(jsonMessage, "networkName")
                .toString();
        double timeStepValue =
            getJsonValue(jsonMessage, "timeStep")
                .toDouble(-100);

        if (timeStepValue == -100)
        {
            QString error =
                QString(
                    "Simulator time step must be a double"
                    "Value passed is: %1")
                    .arg(timeStepValue);
            qWarning() << error;
            onErrorOccurred(error);
        }

        if (timeStepValue <= 0)
        {
            onErrorOccurred("Invalid time step value");
            return;
        }

        qDebug() << "[Server] Loading network: "
                 << networkName
                 << " with time step: " << timeStepValue
                 << "s.";

        // load the network
        SimulatorAPI::InteractiveMode::loadNetwork(
            networkPath, networkName);

        // load the ships
        QVector<std::shared_ptr<ShipNetSimCore::Ship>>
            shipsList;

        // Check if ships is in the root object or in params
        if (jsonMessage.contains("ships"))
        {
            // Ships is in the root object, pass jsonMessage
            shipsList = SimulatorAPI::loadShips(
                jsonMessage, networkName);
        }
        else if (jsonMessage.contains("params")
                 && jsonMessage["params"].isObject())
        {
            // Ships is in params, pass the params object
            // instead
            QJsonObject params =
                jsonMessage["params"].toObject();
            if (params.contains("ships"))
            {
                shipsList = SimulatorAPI::loadShips(
                    params, networkName);
            }
            else
            {
                // Neither contains ships
                shipsList = QVector<std::shared_ptr<
                    ShipNetSimCore::Ship>>(); // empty list
            }
        }
        else
        {
            // Neither contains ships
            auto shipsList = QVector<std::shared_ptr<
                ShipNetSimCore::Ship>>(); // empty
                                          // list
        }

        qDebug() << "[Server] Creating new simulation "
                    "environment "
                    "for network: "
                 << networkName;

        SimulatorAPI::InteractiveMode::
            createNewSimulationEnvironment(
                networkName, shipsList,
                units::time::second_t(timeStepValue), true,
                SimulatorAPI::Mode::Async);
    }
    else if (command == "runSimulator")
    {
        qInfo() << "[Server] Received command: Running "
                   "simulation.";

        // Validate networkNames array
        auto arrayCheck = validateArray(
            jsonMessage, "networkNames", command);
        if (!arrayCheck.first)
        {
            onErrorOccurred(arrayCheck.second);
            return;
        }

        // Validate byTimeSteps using getJsonValue
        QJsonValue byTimeStepsValue =
            getJsonValue(jsonMessage, "byTimeSteps");
        if (byTimeStepsValue.isUndefined()
            || !byTimeStepsValue.isDouble())
        {
            onErrorOccurred(
                "'byTimeSteps' must be a numeric value");
            return;
        }

        // Process networks
        QVector<QString> nets;
        QJsonArray       networkNamesArray =
            getJsonValue(jsonMessage, "networkNames")
                .toArray();
        for (const QJsonValue &value : networkNamesArray)
        {
            nets.append(value.toString());
        }

        double runBy = byTimeStepsValue.toDouble(-100);

        if (runBy == -100)
        {
            QString error =
                QString(
                    "Simulator time step must be a double"
                    "Value passed is: %1")
                    .arg(runBy);
            qWarning() << error;
            onErrorOccurred(error);
        }

        // Connect simulationProgressUpdated if runBy is any
        // value in [0, -inf]
        if (runBy <= 0)
        {
            qDebug() << "[Server] Executing simulation for "
                        "networks: ["
                     << nets.join(", ") << "] till end.";
            if (m_progressConnection)
            {
                disconnect(m_progressConnection);
                m_progressConnection =
                    QMetaObject::Connection();
            }
            m_progressConnection = connect(
                &SimulatorAPI::InteractiveMode::
                    getInstance(),
                &SimulatorAPI::simulationProgressUpdated,
                this,
                &SimulationServer::
                    onSimulationProgressUpdate);
        }
        else
        {
            qDebug() << "[Server] Executing simulation for "
                        "networks: ["
                     << nets.join(", ")
                     << "] with number of steps: " << runBy
                     << "s.";
        }

        try
        {
            SimulatorAPI::InteractiveMode::runSimulation(
                nets, units::time::second_t(runBy), true);
        }
        catch (std::exception &e)
        {
            onErrorOccurred(e.what());
        }
    }
    else if (command == "terminateSimulator")
    {
        qInfo() << "[Server] Received command: Terminating "
                   "simulation.";

        // Validate networkNames array
        auto arrayCheck = validateArray(
            jsonMessage, "networkNames", command);
        if (!arrayCheck.first)
        {
            onErrorOccurred(arrayCheck.second);
            return;
        }

        QVector<QString> nets;
        QJsonArray       networkNamesArray =
            getJsonValue(jsonMessage, "networkNames")
                .toArray();
        for (const QJsonValue &value : networkNamesArray)
        {
            nets.append(value.toString());
        }

        qDebug() << "[Server] Terminating simulation for "
                    "networks: "
                 << nets;

        SimulatorAPI::InteractiveMode::terminateSimulation(
            nets);
    }
    else if (command == "endSimulator")
    {
        qInfo() << "[Server] Received command: Ending "
                   "simulation.";

        // Validate networkNames array
        auto arrayCheck = validateArray(
            jsonMessage, "networkNames", command);
        if (!arrayCheck.first)
        {
            onErrorOccurred(arrayCheck.second);
            return;
        }

        QVector<QString> nets;
        QJsonArray       networkNamesArray =
            getJsonValue(jsonMessage, "networkNames")
                .toArray();
        for (const QJsonValue &value : networkNamesArray)
        {
            nets.append(value.toString());
        }

        qDebug()
            << "[Server] Ending simulation for networks: "
            << nets;

        SimulatorAPI::InteractiveMode::finalizeSimulation(
            nets);
    }
    else if (command == "addShipsToSimulator")
    {
        qInfo() << "[Server] Received command: Adding "
                   "ships to the simulation.";

        // Validate required fields
        QList<QPair<bool, QString>> checks;
        checks << checkJsonField(jsonMessage, "networkName",
                                 command);
        checks << checkJsonField(jsonMessage, "ships",
                                 command);

        // Collect errors
        QStringList errors;
        for (const auto &check : checks)
        {
            if (!check.first)
            {
                errors << check.second;
            }
        }
        if (!errors.isEmpty())
        {
            onErrorOccurred(errors.join("; "));
            return;
        }

        QString networkName =
            getJsonValue(jsonMessage, "networkName")
                .toString();

        // load the ships
        QVector<std::shared_ptr<ShipNetSimCore::Ship>>
            shipsList;

        // Check if ships is in the root object or in params
        if (jsonMessage.contains("ships"))
        {
            // Additional validation for ship objects
            QJsonArray shipsArray =
                jsonMessage["ships"].toArray();
            for (const QJsonValue &shipVal : shipsArray)
            {
                if (!shipVal.isObject())
                {
                    onErrorOccurred(
                        "'ships' array contains "
                        "invalid ship definitions");
                    mWorkerBusy = false;
                    return;
                }
            }
            // Ships is in the root object, pass jsonMessage
            shipsList = SimulatorAPI::loadShips(
                jsonMessage, networkName);
        }
        else if (jsonMessage.contains("params")
                 && jsonMessage["params"].isObject())
        {
            // Ships is in params, pass the params object
            // instead
            QJsonObject params =
                jsonMessage["params"].toObject();
            // Additional validation for ship objects
            QJsonArray shipsArray =
                params["ships"].toArray();
            for (const QJsonValue &shipVal : shipsArray)
            {
                if (!shipVal.isObject())
                {
                    onErrorOccurred(
                        "'ships' array contains "
                        "invalid ship definitions");
                    mWorkerBusy = false;
                    return;
                }
            }
            if (params.contains("ships"))
            {
                shipsList = SimulatorAPI::loadShips(
                    params, networkName);
            }
            else
            {
                // Neither contains ships
                shipsList = QVector<std::shared_ptr<
                    ShipNetSimCore::Ship>>(); // empty list
            }
        }
        else
        {
            // Neither contains ships
            auto shipsList = QVector<std::shared_ptr<
                ShipNetSimCore::Ship>>(); // empty
                                          // list
        }

        auto ships = SimulatorAPI::loadShips(jsonMessage,
                                             networkName);
        SimulatorAPI::InteractiveMode::addShipToSimulation(
            networkName, ships);
    }
    else if (command == "addContainersToShip")
    {
        qInfo() << "[Server] Received command: Adding "
                   "containers to a ship.";

        // Validate required fields
        QList<QPair<bool, QString>> checks;
        checks << checkJsonField(jsonMessage, "networkName",
                                 command);
        checks << checkJsonField(jsonMessage, "shipID",
                                 command);
        checks << checkJsonField(jsonMessage, "containers",
                                 command);

        // Collect errors
        QStringList errors;
        for (const auto &check : checks)
        {
            if (!check.first)
            {
                errors << check.second;
            }
        }
        if (!errors.isEmpty())
        {
            onErrorOccurred(errors.join("; "));
            return;
        }

        QString net =
            getJsonValue(jsonMessage, "networkName")
                .toString();
        QString shipID =
            getJsonValue(jsonMessage, "shipID").toString();

        qDebug() << "[Server] Adding containers to ship "
                 << shipID << " in network: " << net;

        // Check if "containers" is in the root object or in
        // params
        if (jsonMessage.contains("containers"))
        {
            // Containers is in the root object, pass
            // jsonMessage
            SimulatorAPI::InteractiveMode::
                addContainersToShip(net, shipID,
                                    jsonMessage);
        }
        else if (jsonMessage.contains("params")
                 && jsonMessage["params"].isObject())
        {
            // Check if "containers" is in the params object
            QJsonObject params =
                jsonMessage["params"].toObject();
            if (params.contains("containers"))
            {
                // Containers is in params, pass the params
                // object instead
                SimulatorAPI::InteractiveMode::
                    addContainersToShip(net, shipID,
                                        params);
            }
            else
            {
                // Neither contains containers
                onErrorOccurred("No containers specified "
                                "to add to ship");
            }
        }
        else
        {
            // Neither contains containers
            onErrorOccurred(
                "No containers specified to add to ship");
        }
    }
    else if (command == "getNetworkSeaPorts")
    {
        qInfo() << "[Server] Received command: Getting a "
                   "list of sea ports.";

        // Validate required fields
        QList<QPair<bool, QString>> checks;
        checks << checkJsonField(jsonMessage, "networkName",
                                 command);

        // Collect errors
        QStringList errors;
        for (const auto &check : checks)
        {
            if (!check.first)
            {
                errors << check.second;
            }
        }
        if (!errors.isEmpty())
        {
            onErrorOccurred(errors.join("; "));
            return;
        }

        QString net =
            getJsonValue(jsonMessage, "networkName")
                .toString();

        // Try to get considerShipsPathOnly from either
        // location
        QJsonValue considerPathValue = getJsonValue(
            jsonMessage, "considerShipsPathOnly");
        bool considerShipsPathOnly =
            considerPathValue.toBool(
                false); // false is the default value

        SimulatorAPI::InteractiveMode::
            requestAvailablePorts({net},
                                  considerShipsPathOnly);
    }
    else if (command
             == "unloadContainersFromShipAtTerminal")
    {
        qInfo() << "[Server] Received command: Unloading "
                   "containers to a ship.";
        // Validate required fields
        QList<QPair<bool, QString>> checks;
        checks << checkJsonField(jsonMessage, "networkName",
                                 command);
        checks << checkJsonField(jsonMessage, "shipID",
                                 command);
        checks << checkJsonField(jsonMessage,
                                 "terminalNames", command);

        // Collect errors
        QStringList errors;
        for (const auto &check : checks)
        {
            if (!check.first)
            {
                errors << check.second;
            }
        }
        if (!errors.isEmpty())
        {
            onErrorOccurred(errors.join("; "));
            return;
        }

        QString net =
            getJsonValue(jsonMessage, "networkName")
                .toString();
        QString shipID =
            getJsonValue(jsonMessage, "shipID").toString();

        QVector<QString> portNames = QVector<QString>();
        QJsonArray       portNamesArray =
            getJsonValue(jsonMessage, "terminalNames")
                .toArray();
        for (const QJsonValue &value : portNamesArray)
        {
            portNames.append(value.toString());
        }

        SimulatorAPI::InteractiveMode::
            requestUnloadContainersAtPort(net, shipID,
                                          portNames);
    }
    else if (command == "resetServer")
    {
        qInfo() << "[Server] Received command: Resetting "
                   "the server.";

        SimulatorAPI::InteractiveMode::resetAPI();
        onServerReset();
    }
    else
    {
        QString error = "Unrecognized command:" + command;
        onErrorOccurred(error);
        qWarning() << error;
        onWorkerReady();
    }
}

void SimulationServer::onWorkerReady()
{
    // Worker is now ready for the next command
    QMutexLocker locker(&mMutex);

    // Worker is now ready for the next command
    mWorkerBusy = false;

    mWaitCondition.wakeAll(); // Notify waiting consumers
}

void SimulationServer::sendRabbitMQMessage(
    const QString &routingKey, const QJsonObject &message)
{
    QByteArray messageData =
        QJsonDocument(message).toJson();

    amqp_bytes_t messageBytes;
    messageBytes.len   = messageData.size();
    messageBytes.bytes = messageData.data();

    int retries = MAX_SEND_COMMAND_RETRIES;
    while (retries > 0)
    {
        int publishStatus = amqp_basic_publish(
            mRabbitMQConnection, 1,
            amqp_cstring_bytes(EXCHANGE_NAME.c_str()),
            amqp_cstring_bytes(
                routingKey.toUtf8().constData()),
            0, 0, nullptr, messageBytes);

        if (publishStatus == AMQP_STATUS_OK)
        {
            return; // Success
        }
        qWarning()
            << "Failed to publish message to RabbitMQ "
               "with routing key:"
            << routingKey << ". Retrying...";
        retries--;
        QThread::msleep(
            1000); // Wait 1 second before retrying
    }
    qCritical() << "Failed to publish message to RabbitMQ "
                   "after retries with routing key:"
                << routingKey;
}

// simulation events handling
void SimulationServer::onSimulationNetworkLoaded(
    QString networkName)
{
    QJsonObject jsonMessage;
    jsonMessage["event"]       = "simulationNetworkLoaded";
    jsonMessage["networkName"] = networkName;
    jsonMessage["host"]        = ShipNetSim_NAME;
    sendRabbitMQMessage(PUBLISHING_ROUTING_KEY.c_str(),
                        jsonMessage);

    // onWorkerReady();
}

void SimulationServer::onSimulationCreated(
    QString networkName)
{
    QJsonObject jsonMessage;
    jsonMessage["event"]       = "simulationCreated";
    jsonMessage["networkName"] = networkName;
    jsonMessage["host"]        = ShipNetSim_NAME;
    sendRabbitMQMessage(PUBLISHING_ROUTING_KEY.c_str(),
                        jsonMessage);

    onWorkerReady();
}

void SimulationServer::onSimulationPaused(
    QVector<QString> networkNames)
{
    QJsonObject jsonMessage;
    jsonMessage["event"] = "simulationPaused";
    QJsonArray networkNamesArray;
    for (const QString &name : networkNames)
    {
        networkNamesArray.append(name);
    }
    jsonMessage["networkNames"] = networkNamesArray;
    jsonMessage["host"]         = ShipNetSim_NAME;
    sendRabbitMQMessage(PUBLISHING_ROUTING_KEY.c_str(),
                        jsonMessage);

    onWorkerReady();
}

void SimulationServer::onSimulationResumed(
    QVector<QString> networkNames)
{
    QJsonObject jsonMessage;
    jsonMessage["event"] = "simulationResumed";
    QJsonArray networkNamesArray;
    for (const QString &name : networkNames)
    {
        networkNamesArray.append(name);
    }
    jsonMessage["networkNames"] = networkNamesArray;
    jsonMessage["host"]         = ShipNetSim_NAME;
    sendRabbitMQMessage(PUBLISHING_ROUTING_KEY.c_str(),
                        jsonMessage);

    onWorkerReady();
}

void SimulationServer::onSimulationRestarted(
    QVector<QString> networkNames)
{
    QJsonObject jsonMessage;
    jsonMessage["event"] = "simulationRestarted";
    QJsonArray networkNamesArray;
    for (const QString &name : networkNames)
    {
        networkNamesArray.append(name);
    }
    jsonMessage["networkNames"] = networkNamesArray;
    jsonMessage["host"]         = ShipNetSim_NAME;
    sendRabbitMQMessage(PUBLISHING_ROUTING_KEY.c_str(),
                        jsonMessage);

    onWorkerReady();
}

void SimulationServer::onSimulationTerminated(
    QVector<QString> networkNames)
{
    if (m_progressConnection)
    {
        disconnect(m_progressConnection);
        m_progressConnection = QMetaObject::Connection();
    }

    QJsonObject jsonMessage;
    jsonMessage["event"] = "simulationTerminated";
    QJsonArray networkNamesArray;
    for (const QString &name : networkNames)
    {
        networkNamesArray.append(name);
    }
    jsonMessage["networkNames"] = networkNamesArray;
    jsonMessage["host"]         = ShipNetSim_NAME;
    sendRabbitMQMessage(PUBLISHING_ROUTING_KEY.c_str(),
                        jsonMessage);

    onWorkerReady();
}

void SimulationServer::onSimulationFinished(
    QString networkName)
{
    if (m_progressConnection)
    {
        disconnect(m_progressConnection);
        m_progressConnection = QMetaObject::Connection();
    }

    QJsonObject jsonMessage;
    jsonMessage["event"]       = "simulationFinished";
    jsonMessage["networkName"] = networkName;
    jsonMessage["host"]        = ShipNetSim_NAME;
    sendRabbitMQMessage(PUBLISHING_ROUTING_KEY.c_str(),
                        jsonMessage);

    onWorkerReady();
}

void SimulationServer::onSimulationAdvanced(
    QMap<QString, QPair<units::time::second_t, double>>
        newSimulationTime)
{
    QJsonObject jsonMessage;
    jsonMessage["event"] = "simulationAdvanced";
    jsonMessage["host"]  = ShipNetSim_NAME;

    QJsonObject jsonNetworkTimes;
    for (auto it = newSimulationTime.constBegin();
         it != newSimulationTime.constEnd(); ++it)
    {
        // Add each network name (key) and its corresponding
        // simulation time (value) to the jsonNetworkTimes
        // object
        jsonNetworkTimes[it.key()] =
            it.value().first.value();
    }
    // Add the network names to the JSON message
    jsonMessage["networkNamesTimes"] = jsonNetworkTimes;

    QJsonObject jsonNetworkProgress;
    for (auto it = newSimulationTime.constBegin();
         it != newSimulationTime.constEnd(); ++it)
    {
        // Add each network name (key) and its corresponding
        // simulation time (value) to the
        // jsonNetworkProgress object
        jsonNetworkProgress[it.key()] = it.value().second;
    }
    // Add the network names to the JSON message
    jsonMessage["networkNamesProgress"] =
        jsonNetworkProgress;

    sendRabbitMQMessage(PUBLISHING_ROUTING_KEY.c_str(),
                        jsonMessage);

    onWorkerReady();
}

void SimulationServer::onSimulationProgressUpdate(
    QString networkName, int progressPercentage)
{
    // Check if progressPercentage is a multiple of 5
    if (progressPercentage % 5 == 0)
    {
        QJsonObject jsonMessage;
        jsonMessage["event"] = "simulationProgressUpdate";
        jsonMessage["networkName"] = networkName;
        jsonMessage["newProgress"] = progressPercentage;
        jsonMessage["host"]        = ShipNetSim_NAME;

        // Send the message
        sendRabbitMQMessage(PUBLISHING_ROUTING_KEY.c_str(),
                            jsonMessage);
    }
}

void SimulationServer::onShipAddedToSimulator(
    const QString          networkName,
    const QVector<QString> shipIDs)
{
    QJsonObject jsonMessage;
    jsonMessage["event"] = "shipAddedToSimulator";
    QJsonArray shipsJson;
    for (const auto &shipID : shipIDs)
    {
        shipsJson.append(shipID);
    }
    jsonMessage["shipIDs"] = shipsJson;
    jsonMessage["host"]    = ShipNetSim_NAME;
    sendRabbitMQMessage(PUBLISHING_ROUTING_KEY.c_str(),
                        jsonMessage);

    onWorkerReady();
}

void SimulationServer::onAllShipsReachDestination(
    const QString networkName)
{
    QJsonObject jsonMessage;
    jsonMessage["event"] = "allShipsReachedDestination";
    jsonMessage["host"]  = ShipNetSim_NAME;
    sendRabbitMQMessage(PUBLISHING_ROUTING_KEY.c_str(),
                        jsonMessage);
    onWorkerReady();
}

void SimulationServer::onShipReachedDestination(
    const QJsonObject shipStatus)
{
    QJsonObject jsonMessage;
    jsonMessage["event"] = "shipReachedDestination";
    jsonMessage["state"] = shipStatus;
    jsonMessage["host"]  = ShipNetSim_NAME;
    sendRabbitMQMessage(PUBLISHING_ROUTING_KEY.c_str(),
                        jsonMessage);
}

void SimulationServer::onShipStateAvailable(
    QString networkName, QString shipID,
    const QJsonObject shipState)
{
    QJsonObject jsonMessage;
    jsonMessage["event"]       = "ShipState";
    jsonMessage["networkName"] = networkName;
    jsonMessage["shipID"]      = shipID;
    jsonMessage["state"]       = shipState;
    jsonMessage["host"]        = ShipNetSim_NAME;
    sendRabbitMQMessage(PUBLISHING_ROUTING_KEY.c_str(),
                        jsonMessage);

    // onWorkerReady();
}

void SimulationServer::onSimulatorStateAvailable(
    const QJsonObject simulatorState)
{
    QJsonObject jsonMessage;
    jsonMessage["event"] = "simulatorState";
    jsonMessage["state"] = simulatorState;
    jsonMessage["host"]  = ShipNetSim_NAME;
    sendRabbitMQMessage(PUBLISHING_ROUTING_KEY.c_str(),
                        jsonMessage);

    // onWorkerReady();
}

void SimulationServer::onSimulationResultsAvailable(
    QPair<QString, ShipsResults> results)
{
    QJsonObject jsonMessage;
    jsonMessage["event"] = "simulationResultsAvailable";
    jsonMessage["networkName"] = results.first;
    jsonMessage["shipResults"] = results.second.toJson();
    jsonMessage["host"]        = ShipNetSim_NAME;
    sendRabbitMQMessage(PUBLISHING_ROUTING_KEY.c_str(),
                        jsonMessage);

    // onWorkerReady();
}

void SimulationServer::onContainersAddedToShip(
    QString networkName, QString shipID)
{
    QJsonObject jsonMessage;
    jsonMessage["event"]       = "containersAddedToShip";
    jsonMessage["networkName"] = networkName;
    jsonMessage["shipID"]      = shipID;
    jsonMessage["host"]        = ShipNetSim_NAME;

    sendRabbitMQMessage(PUBLISHING_ROUTING_KEY.c_str(),
                        jsonMessage);
    onWorkerReady();
}

void SimulationServer::onShipReachedSeaPort(
    QString networkName, QString shipID,
    QString seaPortCode, qsizetype containersCount)
{
    QJsonObject jsonMessage;
    jsonMessage["event"]           = "shipReachedSeaPort";
    jsonMessage["networkName"]     = networkName;
    jsonMessage["shipID"]          = shipID;
    jsonMessage["seaPortCode"]     = seaPortCode;
    jsonMessage["host"]            = ShipNetSim_NAME;
    jsonMessage["containersCount"] = containersCount;

    sendRabbitMQMessage(PUBLISHING_ROUTING_KEY.c_str(),
                        jsonMessage);

    // onWorkerReady();
}

void SimulationServer::onPortsAvailable(
    QMap<QString, QVector<QString>> networkPorts)
{
    QJsonObject jsonMessage;
    jsonMessage["event"] = "containersAddedToShip";
    jsonMessage["host"]  = ShipNetSim_NAME;

    // Convert QMap<QString, QVector<QString>> to
    // QJsonObject
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

    jsonMessage["portNames"] = portsJson;

    sendRabbitMQMessage(PUBLISHING_ROUTING_KEY.c_str(),
                        jsonMessage);
    onWorkerReady();
}

void SimulationServer::onContainersUnloaded(
    QString networkName, QString shipID,
    QString seaPortName, QJsonArray containers)
{
    std::cout << "containers unloaded\n";
    QJsonObject jsonMessage;
    jsonMessage["event"]      = "containersUnloaded";
    jsonMessage["host"]       = ShipNetSim_NAME;
    jsonMessage["portName"]   = seaPortName;
    jsonMessage["shipID"]     = shipID;
    jsonMessage["containers"] = containers;

    sendRabbitMQMessage(PUBLISHING_ROUTING_KEY.c_str(),
                        jsonMessage);
    onWorkerReady();
}

void SimulationServer::onErrorOccurred(
    const QString &errorMessage)
{
    if (m_progressConnection)
    {
        disconnect(m_progressConnection);
        m_progressConnection = QMetaObject::Connection();
    }

    QJsonObject jsonMessage;
    jsonMessage["event"]        = "errorOccurred";
    jsonMessage["errorMessage"] = errorMessage;
    jsonMessage["host"]         = ShipNetSim_NAME;
    sendRabbitMQMessage(PUBLISHING_ROUTING_KEY.c_str(),
                        jsonMessage);

    onWorkerReady();
}

void SimulationServer::onServerReset()
{
    setupServer();
    QJsonObject jsonMessage;
    jsonMessage["event"] = "serverReset";
    jsonMessage["host"]  = ShipNetSim_NAME;
    sendRabbitMQMessage(PUBLISHING_ROUTING_KEY.c_str(),
                        jsonMessage);
    onWorkerReady();
    qInfo() << "Server reset Successfully!";
}
