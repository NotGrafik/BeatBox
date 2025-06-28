#include "HostSession.h"
#include <QRandomGenerator>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>
#include <QNetworkInterface>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QStandardPaths>
#include <QFile>
#include <QDir>

HostSession::HostSession(SoundManager* soundManager, QObject *parent) : QObject(parent), soundManager(soundManager) {
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
    QUrl url("http://192.168.1.27:3000/register");  // Remplacer par l'IP du serveur Node
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
        else if (obj["type"] == "upload") {
            // Nouveau code pour gérer l'upload
            QString fileName = obj["name"].toString();
            qint64 fileSize = obj["size"].toInt();
            int index = obj["index"].toInt(); // Get the pad index from client

            // Préparer la réception du fichier - handle chunked data properly
            QByteArray fileData;
            while (fileData.size() < fileSize) {
                if (!client->waitForReadyRead(5000)) {
                    qDebug() << "Timeout while waiting for file data from client";
                    return;
                }
                fileData.append(client->read(fileSize - fileData.size()));
            }

            if (fileData.size() == fileSize) {
                // Sauvegarder le fichier localement
                QString savePath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/" + fileName;
                QFile file(savePath);
                if (file.open(QIODevice::WriteOnly)) {
                    if (file.write(fileData) == fileData.size()) {
                        file.close();
                        qDebug() << "File uploaded successfully from client:" << fileName;

                        // Import sound to host's sound manager at the correct index
                        soundManager->importSound(savePath);

                        // Synchroniser avec les autres clients
                        syncSoundToClients(index, savePath, fileName);

                        // Confirmer l'upload
                        QJsonObject response;
                        response["type"] = "uploadComplete";
                        client->write(QJsonDocument(response).toJson() + "\n");
                        client->flush();
                        qDebug() << "Upload confirmation sent to client";
                    } else {
                        qDebug() << "Failed to write uploaded file:" << file.errorString();
                    }
                } else {
                    qDebug() << "Failed to open file for writing:" << file.errorString();
                }
            } else {
                qDebug() << "File size mismatch: expected" << fileSize << "got" << fileData.size();
            }
        }
        else if (obj["type"] == "play") {
            // Handle play command from client
            int index = obj["index"].toInt();
            qDebug() << "Received play command for pad" << index;

            // Play sound locally on host
            soundManager->playSound(index);

            // Forward play command to all other clients
            QJsonObject playMsg;
            playMsg["type"] = "play";
            playMsg["index"] = index;
            QByteArray playData = QJsonDocument(playMsg).toJson(QJsonDocument::Compact) + "\n";

            for (QTcpSocket* c : clients) {
                if (c != client && c->state() == QAbstractSocket::ConnectedState) {
                    c->write(playData);
                }
            }
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
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to open file for sync:" << path;
        return;
    }

    QByteArray fileData = file.readAll();
    file.close();

    QJsonObject obj;
    obj["type"] = "syncSound";
    obj["index"] = index;
    obj["name"] = name;
    obj["size"] = fileData.size();

    QByteArray header = QJsonDocument(obj).toJson(QJsonDocument::Compact) + "\n";

    for (QTcpSocket* c : clients) {
        if (c->state() == QAbstractSocket::ConnectedState) {
            c->write(header);
            c->write(fileData);
        }
    }
}

void HostSession::broadcastPlayCommand(int index) {
    QJsonObject obj;
    obj["type"] = "play";
    obj["index"] = index;
    QByteArray msg = QJsonDocument(obj).toJson(QJsonDocument::Compact) + "\n";

    for (QTcpSocket* c : clients) {
        if (c->state() == QAbstractSocket::ConnectedState) {
            c->write(msg);
        }
    }
}
