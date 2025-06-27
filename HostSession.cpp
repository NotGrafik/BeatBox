#include "HostSession.h"
#include <QRandomGenerator>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>

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
    } else {
        qDebug() << "Session started on port" << server.serverPort() << "with code:" << sessionCode;
    }
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
            
            // Confirmer la connexion au client
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