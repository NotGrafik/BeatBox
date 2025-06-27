#include "HostSession.h"
#include <QRandomGenerator>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>
#include <QNetworkInterface>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>

HostSession::HostSession(QObject *parent) : QObject(parent) {
    connect(&server, &QTcpServer::newConnection, this, &HostSession::handleNewConnection);
    sessionCode = generateCode();
}

QString HostSession::getSessionCode() const {
    return sessionCode;
}

QString HostSession::generateCode() {
    const QString chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    QString code;
    for (int i = 0; i < 6; ++i)
        code += chars[QRandomGenerator::global()->bounded(chars.size())];
    return code;
}

void HostSession::start() {
    if (!server.listen(QHostAddress::Any, 4242)) {
        qWarning() << "Server failed to start:" << server.errorString();
        return;
    }

    qDebug() << "Session started on port" << server.serverPort() << "with code:" << sessionCode;

    // Récupérer l'IP locale
    QString localIp;
    foreach (const QHostAddress &address, QNetworkInterface::allAddresses()) {
        if (address.protocol() == QAbstractSocket::IPv4Protocol && !address.isLoopback()) {
            localIp = address.toString();
            break;
        }
    }

    if (localIp.isEmpty()) {
        qWarning() << "Could not determine local IP.";
        return;
    }

    // Envoyer IP + code au serveur Node
    QUrl url("http://172.20.10.5:3000/register");  // Remplacer par l'IP du serveur Node
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject obj;
    obj["code"] = sessionCode;
    obj["ip"] = localIp;

    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    QNetworkReply *reply = manager->post(request, QJsonDocument(obj).toJson());

    connect(reply, &QNetworkReply::finished, this, [reply]() {
        if (reply->error() != QNetworkReply::NoError) {
            qWarning() << "Failed to register session:" << reply->errorString();
        } else {
            qDebug() << "Session successfully registered with server.";
        }
        reply->deleteLater();
    });
}

void HostSession::handleNewConnection() {
    if (clients.size() >= 4) {
        QTcpSocket *excess = server.nextPendingConnection();
        excess->write("{\"type\":\"error\",\"message\":\"Session full\"}\n");
        excess->disconnectFromHost();
        excess->deleteLater();
        return;
    }

    QTcpSocket *client = server.nextPendingConnection();
    connect(client, &QTcpSocket::readyRead, this, &HostSession::readClientData);
    connect(client, &QTcpSocket::disconnected, this, &HostSession::handleClientDisconnected);
    clients.append(client);
    qDebug() << "Client connected. Total:" << clients.size();
}

void HostSession::readClientData() {
    QTcpSocket *client = qobject_cast<QTcpSocket*>(sender());
    if (!client) return;

    while (client->canReadLine()) {
        QByteArray line = client->readLine().trimmed();
        QJsonDocument doc = QJsonDocument::fromJson(line);
        if (!doc.isObject()) continue;

        QJsonObject obj = doc.object();
        if (obj["type"] == "join") {
            QString clientName = obj["name"].toString();
            qDebug() << "Client joined:" << clientName;
            emit clientJoined(clientName);

            QJsonObject response;
            response["type"] = "joined";
            response["message"] = "Successfully joined session";
            QJsonDocument responseDoc(response);
            client->write(responseDoc.toJson(QJsonDocument::Compact) + "\n");
        }
    }
}

void HostSession::handleClientDisconnected() {
    QTcpSocket *client = qobject_cast<QTcpSocket*>(sender());
    if (client) {
        clients.removeAll(client);
        client->deleteLater();
        qDebug() << "Client disconnected. Total:" << clients.size();
    }
}

void HostSession::startSession() {
    QJsonObject obj;
    obj["type"] = "start";
    QJsonDocument doc(obj);
    QByteArray msg = doc.toJson(QJsonDocument::Compact) + "\n";

    for (QTcpSocket* c : clients) {
        if (c->state() == QTcpSocket::ConnectedState) {
            c->write(msg);
        }
    }
    emit sessionReady();
}

void HostSession::syncSoundToClients(int index, const QString &path, const QString &name) {
    QJsonObject obj;
    obj["type"] = "syncSound";
    obj["index"] = index;
    obj["path"] = path;
    obj["name"] = name;

    QJsonDocument doc(obj);
    QByteArray msg = doc.toJson(QJsonDocument::Compact) + "\n";

    for (QTcpSocket* c : clients) {
        if (c->state() == QAbstractSocket::ConnectedState) {
            c->write(msg);
        }
    }
}