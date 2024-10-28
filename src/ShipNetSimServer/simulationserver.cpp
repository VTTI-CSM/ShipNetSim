#include "SimulationServer.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>
#include "./VersionConfig.h"

static const int MAX_RECONNECT_ATTEMPTS = 5;
static const int RECONNECT_DELAY_SECONDS = 5;  // Delay between reconnection attempts

static const std::string GithubLink = "https://github.com/VTTI-CSM/ShipNetSim";

static const std::string EXCHANGE_NAME = "CargoNetSim.Exchange";
static const std::string COMMAND_QUEUE_NAME = "CargoNetSim.CommandQueue.ShipNetSim";
static const std::string RESPONSE_QUEUE_NAME = "CargoNetSim.ResponseQueue.ShipNetSim";
static const std::string RECEIVING_ROUTING_KEY = "CargoNetSim.Command.ShipNetSim";
static const std::string PUBLISHING_ROUTING_KEY = "CargoNetSim.Response.ShipNetSim";

SimulationServer::SimulationServer(QObject *parent) :
    QObject(parent), mWorkerBusy(false),
    mSimulationWorker(new StepSimulationWorker(this))
{
    qRegisterMetaType<ShipParamsMap>("ShipParamsMap");

    // Connect the worker ready signal to resume RabbitMQ consumption
    connect(mSimulationWorker.get(), &SimulationWorker::workerReady,
            this, &SimulationServer::onWorkerReady);
    // Connect worker signals to slots
    connect(mSimulationWorker.get(),
            &StepSimulationWorker::simulatorNetworkLoaded, this,
            &SimulationServer::onSimulationNetworkLoaded);
    connect(mSimulationWorker.get(),
            &StepSimulationWorker::simulatorDefined, this,
            &SimulationServer::onSimulationInitialized);
    connect(mSimulationWorker.get(),
            &StepSimulationWorker::simulatorRestarted, this,
            &SimulationServer::onSimulationRestarted);
    connect(mSimulationWorker.get(),
            &StepSimulationWorker::simulatorEnded, this,
            &SimulationServer::onSimulationEnded);
    connect(mSimulationWorker.get(),
            &StepSimulationWorker::shipAddedToSimulator, this,
            &SimulationServer::onShipAddedToSimulator);
    connect(mSimulationWorker.get(),
            &StepSimulationWorker::shipReachedDestination, this,
            &SimulationServer::onShipReachedDestination);
    connect(mSimulationWorker.get(),
            &StepSimulationWorker::simulationResultsAvailable, this,
            &SimulationServer::onSimulationResultsAvailable);
    connect(mSimulationWorker.get(),
            &StepSimulationWorker::shipCurrentStateRequested, this,
            &SimulationServer::onShipStateAvailable);
    connect(mSimulationWorker.get(),
            &StepSimulationWorker::simulatorCurrentStateRequested, this,
            &SimulationServer::onSimulatorStateAvailable);
    connect(mSimulationWorker.get(),
            &StepSimulationWorker::errorOccurred, this,
            &SimulationServer::onErrorOccurred);
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

    // Show app details
    QString appDetails;
    QTextStream detailsStream(&appDetails);
    detailsStream << ShipNetSim_NAME
                  << " [Version " << ShipNetSim_VERSION << "]" << Qt::endl;
    detailsStream << ShipNetSim_VENDOR << Qt::endl;
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
    const QJsonObject &message,
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
    // Acknowledge the message only after it is
    // successfully processed
    amqp_basic_ack(mRabbitMQConnection, 1, envelope.delivery_tag, 0);
}


void SimulationServer::processCommand(const QJsonObject &jsonMessage) {
    QString command = jsonMessage["command"].toString();

    if (command == "defineSimulator") {
        double timeStepValue = jsonMessage["timeStep"].toDouble();

        QMetaObject::invokeMethod(
            mSimulationWorker.get(), "defineSimulator",
            Qt::QueuedConnection,
            Q_ARG(double, timeStepValue),
            Q_ARG(QJsonObject, jsonMessage));
    } else if (command == "restartSimulator") {
        double timeStepValue = jsonMessage["timeStep"].toDouble();
        QMetaObject::invokeMethod(
            mSimulationWorker.get(), "restartSimulator",
            Qt::QueuedConnection, Q_ARG(double, timeStepValue));
    } else if (command == "runSimulator") {
        double runBy = jsonMessage["byTimeSteps"].toDouble(60);
        QMetaObject::invokeMethod(mSimulationWorker.get(), "runSimulation",
                                  Qt::QueuedConnection, Q_ARG(double, runBy));
    } else if (command == "pauseSimulator") {
        QMetaObject::invokeMethod(
            mSimulationWorker.get(), "pauseSimulator", Qt::QueuedConnection);
    } else if (command == "resumeSimulator") {
        QMetaObject::invokeMethod(
            mSimulationWorker.get(), "resumeSimulator", Qt::QueuedConnection);
    } else if (command == "endSimulator") {
        QMetaObject::invokeMethod(
            mSimulationWorker.get(), "endSimulator", Qt::QueuedConnection);
    } else if (command == "addShips") {
        QMetaObject::invokeMethod(
            mSimulationWorker.get(), "addShipsToSimulation",
            Qt::QueuedConnection, Q_ARG(QJsonObject, jsonMessage));
    }else {
        qWarning() << "Unrecognized command:" << command;
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
void SimulationServer::onSimulationNetworkLoaded() {
    QJsonObject jsonMessage;
    jsonMessage["event"] = "simulationNetworkLoaded";
    jsonMessage["host"] = ShipNetSim_NAME;
    sendRabbitMQMessage(PUBLISHING_ROUTING_KEY.c_str(),
                        jsonMessage);
}

void SimulationServer::onSimulationInitialized() {
    QJsonObject jsonMessage;
    jsonMessage["event"] = "simulationInitialized";
    jsonMessage["host"] = ShipNetSim_NAME;
    sendRabbitMQMessage(PUBLISHING_ROUTING_KEY.c_str(),
                        jsonMessage);
}

void SimulationServer::onSimulationPaused() {
    QJsonObject jsonMessage;
    jsonMessage["event"] = "simulationPaused";
    jsonMessage["host"] = ShipNetSim_NAME;
    sendRabbitMQMessage(PUBLISHING_ROUTING_KEY.c_str(),
                        jsonMessage);
}

void SimulationServer::onSimulationResumed() {
    QJsonObject jsonMessage;
    jsonMessage["event"] = "simulationResumed";
    jsonMessage["host"] = ShipNetSim_NAME;
    sendRabbitMQMessage(PUBLISHING_ROUTING_KEY.c_str(),
                        jsonMessage);
}

void SimulationServer::onSimulationRestarted() {
    QJsonObject jsonMessage;
    jsonMessage["event"] = "simulationRestarted";
    jsonMessage["host"] = ShipNetSim_NAME;
    sendRabbitMQMessage(PUBLISHING_ROUTING_KEY.c_str(),
                        jsonMessage);
}

void SimulationServer::onSimulationEnded() {
    QJsonObject jsonMessage;
    jsonMessage["event"] = "simulationEnded";
    jsonMessage["host"] = ShipNetSim_NAME;
    sendRabbitMQMessage(PUBLISHING_ROUTING_KEY.c_str(),
                        jsonMessage);
}

void SimulationServer::onSimulationAdvanced(double newSimulationTime) {
    QJsonObject jsonMessage;
    jsonMessage["event"] = "simulationAdvanced";
    jsonMessage["newSimulationTime"] = newSimulationTime;
    jsonMessage["host"] = ShipNetSim_NAME;
    sendRabbitMQMessage(PUBLISHING_ROUTING_KEY.c_str(),
                        jsonMessage);
}

void SimulationServer::onSimulationProgressUpdate(int progressPercentage)
{
    QJsonObject jsonMessage;
    jsonMessage["event"] = "simulationProgressUpdate";
    jsonMessage["newProgress"] = progressPercentage;
    jsonMessage["host"] = ShipNetSim_NAME;
    sendRabbitMQMessage(PUBLISHING_ROUTING_KEY.c_str(),
                        jsonMessage);
}

void SimulationServer::onShipAddedToSimulator(const QString shipID) {
    QJsonObject jsonMessage;
    jsonMessage["event"] = "shipAddedToSimulator";
    jsonMessage["shipID"] = shipID;
    jsonMessage["host"] = ShipNetSim_NAME;
    sendRabbitMQMessage(PUBLISHING_ROUTING_KEY.c_str(),
                        jsonMessage);
}

void SimulationServer::onShipReachedDestination(const QJsonObject shipStatus) {
    QJsonObject jsonMessage;
    jsonMessage["event"] = "shipReachedDestination";
    jsonMessage["state"] = shipStatus;
    jsonMessage["host"] = ShipNetSim_NAME;
    sendRabbitMQMessage(PUBLISHING_ROUTING_KEY.c_str(),
                        jsonMessage);
}

void SimulationServer::onShipStateAvailable(const QJsonObject shipState)  {
    QJsonObject jsonMessage;
    jsonMessage["event"] = "ShipState";
    jsonMessage["state"] = shipState;
    jsonMessage["host"] = ShipNetSim_NAME;
    sendRabbitMQMessage(PUBLISHING_ROUTING_KEY.c_str(),
                        jsonMessage);
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
}

void SimulationServer::onSimulationResultsAvailable(ShipsResults& results) {
    QJsonObject jsonMessage;
    jsonMessage["event"] = "simulationResultsAvailable";

    // Add the results to the JSON
    QJsonObject resultData;
    resultData = results.toJson();

    jsonMessage["results"] = resultData;
    jsonMessage["host"] = ShipNetSim_NAME;
    sendRabbitMQMessage(PUBLISHING_ROUTING_KEY.c_str(),
                        jsonMessage);
}

void SimulationServer::onErrorOccurred(const QString &errorMessage) {
    QJsonObject jsonMessage;
    jsonMessage["event"] = "errorOccurred";
    jsonMessage["errorMessage"] = errorMessage;
    jsonMessage["host"] = ShipNetSim_NAME;
    sendRabbitMQMessage(PUBLISHING_ROUTING_KEY.c_str(),
                        jsonMessage);
}
