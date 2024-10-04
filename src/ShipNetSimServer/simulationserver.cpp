#include "SimulationServer.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>
#include "./VersionConfig.h"
#include "./utils/updatechecker.h"

static const int MAX_RECONNECT_ATTEMPTS = 5;
static const int RECONNECT_DELAY_SECONDS = 5;  // Delay between reconnection attempts
static const std::string GithubLink = "https://github.com/VTTI-CSM/ShipNetSim";

SimulationServer::SimulationServer(QObject *parent) :
    QObject(parent), mSimulationWorker(new ContinuousSimulationWorker(this)),
    mWorkerBusy(false)
{
    qRegisterMetaType<ShipParamsMap>("ShipParamsMap");

    // Connect the worker ready signal to resume RabbitMQ consumption
    connect(mSimulationWorker.get(), &SimulationWorker::workerReady,
            this, &SimulationServer::onWorkerReady);
    // Connect worker signals to slots
    connect(mSimulationWorker.get(),
            &ContinuousSimulationWorker::simulatorNetworkLoaded, this,
            &SimulationServer::onSimulationNetworkLoaded);
    connect(mSimulationWorker.get(),
            &ContinuousSimulationWorker::simulatorDefined, this,
            &SimulationServer::onSimulationInitialized);
    connect(mSimulationWorker.get(),
            &ContinuousSimulationWorker::simulationPaused, this,
            &SimulationServer::onSimulationPaused);
    connect(mSimulationWorker.get(),
            &ContinuousSimulationWorker::simulationResumed, this,
            &SimulationServer::onSimulationResumed);
    connect(mSimulationWorker.get(),
            &ContinuousSimulationWorker::simulatorRestarted, this,
            &SimulationServer::onSimulationRestarted);
    connect(mSimulationWorker.get(),
            &ContinuousSimulationWorker::simulatorEnded, this,
            &SimulationServer::onSimulationEnded);
    connect(mSimulationWorker.get(),
            &ContinuousSimulationWorker::progressUpdated, this,
            &SimulationServer::onSimulationProgressUpdate);
    connect(mSimulationWorker.get(),
            &ContinuousSimulationWorker::shipAddedToSimulator, this,
            &SimulationServer::onShipAddedToSimulator);
    connect(mSimulationWorker.get(),
            &ContinuousSimulationWorker::shipReachedDestination, this,
            &SimulationServer::onShipReachedDestination);
    connect(mSimulationWorker.get(),
            &ContinuousSimulationWorker::simulationResultsAvailable, this,
            &SimulationServer::onSimulationResultsAvailable);
    connect(mSimulationWorker.get(),
            &ContinuousSimulationWorker::shipCurrentStateRequested, this,
            &SimulationServer::onShipStateAvailable);
    connect(mSimulationWorker.get(),
            &ContinuousSimulationWorker::simulatorCurrentStateRequested, this,
            &SimulationServer::onSimulatorStateAvailable);
    connect(mSimulationWorker.get(),
            &ContinuousSimulationWorker::errorOccurred, this,
            &SimulationServer::onErrorOccurred);
}

SimulationServer::~SimulationServer() {
    stopRabbitMQServer();  // Ensure server stops cleanly before destroying
}

void SimulationServer::startRabbitMQServer(const std::string &hostname,
                                           int port) {
    mHostname = hostname;
    mPort = port;


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
            continue;  // Retry
        }

        int status = amqp_socket_open(socket, hostname.c_str(), port);
        if (status != AMQP_STATUS_OK) {
            qCritical() << "Error: Failed to open RabbitMQ socket on"
                        << hostname.c_str() << ":" << port << ". Retrying...";
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
            continue;  // Retry
        }

        // Declare the queue to listen to commands
        amqp_queue_declare(mRabbitMQConnection, 1,
                           amqp_cstring_bytes("shipnetsim_commands"), 0, 0, 0,
                           1, amqp_empty_table);
        amqp_rpc_reply_t queueRes = amqp_get_rpc_reply(mRabbitMQConnection);
        if (queueRes.reply_type != AMQP_RESPONSE_NORMAL) {
            qCritical() << "Error: Unable to declare RabbitMQ queue. "
                           "Retrying...";
            retryCount++;
            std::this_thread::sleep_for(
                std::chrono::seconds(RECONNECT_DELAY_SECONDS));
            continue;  // Retry
        }

        // Listen for messages
        amqp_basic_consume(mRabbitMQConnection, 1,
                           amqp_cstring_bytes("shipnetsim_commands"),
                           amqp_empty_bytes, 0, 0, 0, amqp_empty_table);

        qInfo() << "Simulator initialized successfully. Awaiting commands from "
                << hostname.c_str() << ":" << port
                << ". The system is now fully operational.";
        break;  // Successful connection

    }

    if (retryCount == MAX_RECONNECT_ATTEMPTS) {
        qCritical() << "Error: Failed to establish a connection to "
                       "RabbitMQ after"
                    << MAX_RECONNECT_ATTEMPTS
                    << "attempts. Server initialization aborted.";
    }
}

void SimulationServer::stopRabbitMQServer() {
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


void SimulationServer::onDataReceivedFromRabbitMQ(
    const QJsonObject &message,
    const amqp_envelope_t &envelope) {

    QString command = message["command"].toString();

    if (command == "pauseSimulator") {
        // Immediately process the pause command
        qDebug() << "Received 'pauseSimulator' command, executing immediately.";

        // Acknowledge the message only after it is
        // successfully processed or queued
        amqp_basic_ack(mRabbitMQConnection, 1, envelope.delivery_tag, 0);

        processCommand(message);  // Prioritize pauseSimulator
    }

    if (mWorkerBusy) {
        if (mCommandQueue.size() < 1) {
            // Only queue one extra command while processing the current one
            mCommandQueue.enqueue(message);

            // Acknowledge the message only after it is
            // successfully processed or queued
            amqp_basic_ack(mRabbitMQConnection, 1, envelope.delivery_tag, 0);
        } else {
            qDebug() << "Command queue is full. Ignoring additional commands.";
            return;
        }
    } else {
        // Acknowledge the message only after it is
        // successfully processed or queued
        amqp_basic_ack(mRabbitMQConnection, 1, envelope.delivery_tag, 0);

        // If the worker is not busy, process the command directly
        processCommand(message);
        mWorkerBusy = true;  // Mark worker as busy
    }
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
        QMetaObject::invokeMethod(mSimulationWorker.get(), "runSimulation");
    } else if (command == "pauseSimulator") {
        QMetaObject::invokeMethod(
            mSimulationWorker.get(), "pauseSimulator", Qt::QueuedConnection);
    } else if (command == "resumeSimulator") {
        QMetaObject::invokeMethod(
            mSimulationWorker.get(), "resumeSimulator", Qt::QueuedConnection);
    } else {
        qWarning() << "Unrecognized command:" << command;
    }

}

void SimulationServer::onWorkerReady() {
    // Worker is now ready for the next command
    mWorkerBusy = false;

    if (!mCommandQueue.isEmpty()) {
        // If there's a command in the queue, process it
        QJsonObject nextCommand = mCommandQueue.dequeue();
        processCommand(nextCommand);
        mWorkerBusy = true;  // Mark worker as busy again
    } else {
        // Continue consuming messages from RabbitMQ
        amqp_rpc_reply_t res;
        amqp_envelope_t envelope;

        // Wait for a new message from RabbitMQ
        amqp_maybe_release_buffers(mRabbitMQConnection);
        res = amqp_consume_message(mRabbitMQConnection, &envelope, nullptr, 0);

        if (res.reply_type == AMQP_RESPONSE_NORMAL) {
            QByteArray messageData(
                static_cast<char *>(envelope.message.body.bytes),
                envelope.message.body.len);
            QJsonDocument doc = QJsonDocument::fromJson(messageData);
            QJsonObject jsonMessage = doc.object();

            emit dataReceived(jsonMessage);
            onDataReceivedFromRabbitMQ(jsonMessage, envelope);  // Process the new message
        }
        else {
            qCritical() << "Error receiving message from RabbitMQ. Type:"
                        << res.reply_type;
            stopRabbitMQServer();  // Close the connection first
            qDebug() << "Attempting to reconnect...";
            startRabbitMQServer(mHostname, mPort);  // Retry connection
        }

        amqp_destroy_envelope(&envelope);
    }
}

void SimulationServer::sendRabbitMQMessage(const QString &queue,
                                           const QJsonObject &message) {
    QByteArray messageData = QJsonDocument(message).toJson();

    amqp_bytes_t messageBytes;
    messageBytes.len = messageData.size();
    messageBytes.bytes = messageData.data();

    int publishStatus =
        amqp_basic_publish(mRabbitMQConnection, 1, amqp_empty_bytes,
                           amqp_cstring_bytes(queue.toUtf8().constData()),
                           0, 0, nullptr, messageBytes);

    if (publishStatus != AMQP_STATUS_OK) {
        qCritical() << "Failed to publish message to RabbitMQ queue:" << queue;
    }
}


// simulation events handling
void SimulationServer::onSimulationNetworkLoaded() {
    QJsonObject jsonMessage;
    jsonMessage["event"] = "simulationNetworkLoaded";
    jsonMessage["host"] = "ShipNetSim";
    sendRabbitMQMessage("client_queue", jsonMessage);
}

void SimulationServer::onSimulationInitialized() {
    QJsonObject jsonMessage;
    jsonMessage["event"] = "simulationInitialized";
    jsonMessage["host"] = "ShipNetSim";
    sendRabbitMQMessage("client_queue", jsonMessage);
}

void SimulationServer::onSimulationPaused() {
    QJsonObject jsonMessage;
    jsonMessage["event"] = "simulationPaused";
    jsonMessage["host"] = "ShipNetSim";
    sendRabbitMQMessage("client_queue", jsonMessage);
}

void SimulationServer::onSimulationResumed() {
    QJsonObject jsonMessage;
    jsonMessage["event"] = "simulationResumed";
    jsonMessage["host"] = "ShipNetSim";
    sendRabbitMQMessage("client_queue", jsonMessage);
}

void SimulationServer::onSimulationRestarted() {
    QJsonObject jsonMessage;
    jsonMessage["event"] = "simulationRestarted";
    jsonMessage["host"] = "ShipNetSim";
    sendRabbitMQMessage("client_queue", jsonMessage);
}

void SimulationServer::onSimulationEnded() {
    QJsonObject jsonMessage;
    jsonMessage["event"] = "simulationEnded";
    jsonMessage["host"] = "ShipNetSim";
    sendRabbitMQMessage("client_queue", jsonMessage);
}

void SimulationServer::onSimulationAdvanced(double newSimulationTime) {
    QJsonObject jsonMessage;
    jsonMessage["event"] = "simulationAdvanced";
    jsonMessage["newSimulationTime"] = newSimulationTime;
    jsonMessage["host"] = "ShipNetSim";
    sendRabbitMQMessage("client_queue", jsonMessage);
}

void SimulationServer::onSimulationProgressUpdate(int progressPercentage)
{
    QJsonObject jsonMessage;
    jsonMessage["event"] = "simulationProgressUpdate";
    jsonMessage["newProgress"] = progressPercentage;
    jsonMessage["host"] = "ShipNetSim";
    sendRabbitMQMessage("client_queue", jsonMessage);
}

void SimulationServer::onShipAddedToSimulator(const QString shipID) {
    QJsonObject jsonMessage;
    jsonMessage["event"] = "shipAddedToSimulator";
    jsonMessage["shipID"] = shipID;
    jsonMessage["host"] = "ShipNetSim";
    sendRabbitMQMessage("client_queue", jsonMessage);
}

void SimulationServer::onShipReachedDestination(const QJsonObject shipStatus) {
    QJsonObject jsonMessage;
    jsonMessage["event"] = "shipReachedDestination";
    jsonMessage["state"] = shipStatus;
    jsonMessage["host"] = "ShipNetSim";
    sendRabbitMQMessage("client_queue", jsonMessage);
}

void SimulationServer::onShipStateAvailable(const QJsonObject shipState)  {
    QJsonObject jsonMessage;
    jsonMessage["event"] = "ShipState";
    jsonMessage["state"] = shipState;
    jsonMessage["host"] = "ShipNetSim";
    sendRabbitMQMessage("client_queue", jsonMessage);
}

void SimulationServer::onSimulatorStateAvailable(const QJsonObject simulatorState)  {
    QJsonObject jsonMessage;
    jsonMessage["event"] = "simulatorState";
    jsonMessage["state"] = simulatorState;
    jsonMessage["host"] = "ShipNetSim";
    sendRabbitMQMessage("client_queue", jsonMessage);
}

void SimulationServer::onSimulationResultsAvailable(ShipsResults& results) {
    QJsonObject jsonMessage;
    jsonMessage["event"] = "simulationResultsAvailable";

    // Add the results to the JSON
    QJsonObject resultData;
    resultData = results.toJson();

    jsonMessage["results"] = resultData;
    jsonMessage["host"] = "ShipNetSim";
    sendRabbitMQMessage("client_queue", jsonMessage);
}

void SimulationServer::onErrorOccurred(const QString &errorMessage) {
    QJsonObject jsonMessage;
    jsonMessage["event"] = "errorOccurred";
    jsonMessage["errorMessage"] = errorMessage;
    jsonMessage["host"] = "ShipNetSim";
    sendRabbitMQMessage("client_queue", jsonMessage);
}
