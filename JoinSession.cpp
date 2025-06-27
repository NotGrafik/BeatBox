#include "JoinSession.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>

JoinSession::JoinSession(const QString &code, QObject *parent) 
    : QObject(parent), sessionCode(code) {
    connect(&socket, &QTcpSocket::readyRead, this, &JoinSession::handleReadyRead);
    connect(&socket, &QTcpSocket::connected, this, &JoinSession::handleConnected);
    connect(&socket, &QTcpSocket::disconnected, this, &JoinSession::handleDisconnected);
    connect(&socket, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::errorOccurred),
            this, &JoinSession::handleError);
}

void JoinSession::start() {
    qDebug() << "Attempting to connect to host with code:" << sessionCode;

    QUrl url("http://172.20.10.5:3000/resolve/" + sessionCode); // IP du serveur Node
    QNetworkRequest request(url);

    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    QNetworkReply *reply = manager->get(request);

    connect(reply, &QNetworkReply::finished, this, [=]() {
        if (reply->error() != QNetworkReply::NoError) {
            qDebug() << "Error resolving IP:" << reply->errorString();
            emit connectionError("Cannot resolve host IP");
            reply->deleteLater();
            return;
        }

        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        QString ip = doc.object()["ip"].toString();
        if (ip.isEmpty()) {
            emit connectionError("Host IP not found");
            reply->deleteLater();
            return;
        }

        qDebug() << "Resolved host IP:" << ip;
        socket.connectToHost(ip, 4242);
        reply->deleteLater();
    });
}

void JoinSession::handleConnected() {
    qDebug() << "Connected to host, sending join request...";
    QJsonObject obj;
    obj["type"] = "join";
    obj["name"] = "Client_" + sessionCode;
    obj["code"] = sessionCode;
    QJsonDocument doc(obj);
    socket.write(doc.toJson(QJsonDocument::Compact) + "\n");
    socket.flush();
}

void JoinSession::handleReadyRead() {
    while (socket.canReadLine()) {
        QByteArray line = socket.readLine().trimmed();
        qDebug() << "Received data:" << line;
        QJsonDocument doc = QJsonDocument::fromJson(line);
        if (!doc.isObject()) continue;
        QJsonObject obj = doc.object();

        QString type = obj["type"].toString();
        if (type == "joined") {
            qDebug() << "Successfully joined session!";
            emit joinedSuccessfully();
        } else if (type == "start") {
            qDebug() << "Session started!";
            emit sessionStarted();
        } else if (type == "error") {
            QString errorMsg = obj["message"].toString();
            qDebug() << "Server error:" << errorMsg;
            emit connectionError(errorMsg);
        }
    }
}

void JoinSession::handleDisconnected() {
    qDebug() << "Disconnected from host";
}

void JoinSession::handleError(QAbstractSocket::SocketError error) {
    QString errorString = socket.errorString();
    qDebug() << "Socket error:" << error << errorString;
    emit connectionError("Connection failed: " + errorString);
}
