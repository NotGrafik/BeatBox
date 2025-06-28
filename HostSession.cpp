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

            // Préparer la réception du fichier
            QByteArray fileData = client->read(fileSize);
            if (fileData.size() == fileSize) {
                // Sauvegarder le fichier localement
                QString saveDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
                QDir dir;
                if (!dir.exists(saveDir)) {
                    dir.mkpath(saveDir);
                }
                QString savePath = saveDir + "/" + fileName;
                QFile file(savePath);
                if (file.open(QIODevice::WriteOnly)) {
                    if (file.write(fileData) == fileData.size()) {
                        file.close();

                        // Ajouter le son au SoundManager
                        soundManager->importSound(savePath);

                        // Synchroniser avec les autres clients
                        syncSoundToClients(savePath, fileName);

                        // Confirmer l'upload
                        QJsonObject response;
                        response["type"] = "uploadComplete";
                        client->write(QJsonDocument(response).toJson() + "\n");

                        qDebug() << "File uploaded successfully:" << savePath;
                    } else {
                        file.close();
                        qWarning() << "Failed to write complete file data:" << savePath;

                        // Envoyer une erreur au client
                        QJsonObject response;
                        response["type"] = "uploadError";
                        response["message"] = "Failed to write file data";
                        client->write(QJsonDocument(response).toJson() + "\n");
                    }
                } else {
                    qWarning() << "Failed to open file for writing:" << savePath << file.errorString();

                    // Envoyer une erreur au client
                    QJsonObject response;
                    response["type"] = "uploadError";
                    response["message"] = "Failed to create file: " + file.errorString();
                    client->write(QJsonDocument(response).toJson() + "\n");
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

void HostSession::syncSoundToClients(const QString &path, const QString &name) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to open file for sync:" << path;
        return;
    }

    QByteArray fileData = file.readAll();
    file.close();

    int index = soundManager->getSoundCount();
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
