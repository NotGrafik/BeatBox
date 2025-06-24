#include "JoinSession.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>

JoinSession::JoinSession(const QString &code, QObject *parent) : QObject(parent), sessionCode(code) {
    connect(&socket, &QTcpSocket::readyRead, this, &JoinSession::handleReadyRead);
}

void JoinSession::start() {
    socket.connectToHost("127.0.0.1", 4242);
    if (!socket.waitForConnected(3000)) {
        emit connectionError("Could not connect to host.");
        return;
    }
    QJsonObject obj;
    obj["type"] = "join";
    obj["name"] = "Client"; // Optional: make dynamic
    QJsonDocument doc(obj);
    socket.write(doc.toJson(QJsonDocument::Compact) + "\n");
}

void JoinSession::handleReadyRead() {
    while (socket.canReadLine()) {
        QByteArray line = socket.readLine().trimmed();
        QJsonDocument doc = QJsonDocument::fromJson(line);
        if (!doc.isObject()) continue;
        QJsonObject obj = doc.object();

        if (obj["type"] == "start") {
            emit sessionStarted();
        } else if (obj["type"] == "error") {
            emit connectionError(obj["message"].toString());
        }
    }
}
