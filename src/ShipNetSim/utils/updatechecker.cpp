#include "UpdateChecker.h"
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QDateTime>
#include <QDebug>
#include "src/ShipNetSim/VersionConfig.h"

namespace ShipNetSimCore
{

UpdateChecker::UpdateChecker(QObject *parent)
    : QObject(parent), manager(new QNetworkAccessManager(this)) {
    connect(manager, &QNetworkAccessManager::finished,
            this, &UpdateChecker::replyFinished);
    // get latest version from qmake
    currentVersion = QString("v") + ShipNetSim_VERSION;

    connect(manager, &QNetworkAccessManager::sslErrors,
            this, &UpdateChecker::handleSslErrors);
}

void UpdateChecker::handleSslErrors(QNetworkReply *reply,
                                    const QList<QSslError> &errors) {
#ifndef NDebug
    qDebug() << "SSL Errors:";
    for (const QSslError &error : errors) {
        qDebug() << error.errorString();
    }
#else
    (void) reply;
    (void) errors;
    return;
#endif
}

void UpdateChecker::checkForUpdates() {
    QUrl url("https://api.github.com/repos/VTTI-CSM/ShipNetSim/releases");
    QNetworkRequest request(url);
    // Force HTTP/1.1
    request.setAttribute(QNetworkRequest::Http2AllowedAttribute, false);

    manager->get(request);
}

void UpdateChecker::replyFinished(QNetworkReply *reply) {
    if (reply->error()) {
        emit updateAvailable(false);
        return;
    }

    QJsonDocument document = QJsonDocument::fromJson(reply->readAll());
    QJsonArray releases = document.array();

    QString mostRecentVersion;
    QDateTime mostRecentDate;
    for (const QJsonValue &value : releases) {
        QJsonObject releaseObj = value.toObject();
        QString version = releaseObj.value("tag_name").toString();
        QDateTime publishedDate =
            QDateTime::fromString(releaseObj.value("published_at").toString(),
                                  Qt::ISODate);

        if (publishedDate > mostRecentDate) {
            mostRecentDate = publishedDate;
            mostRecentVersion = version;
        }
    }

    // Compare mostRecentVersion with your app's current version
    bool isUpdateRequired = mostRecentVersion != currentVersion;
    emit updateAvailable(isUpdateRequired);
}
}
