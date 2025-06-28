#include "JoinSession.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QFile>           // Pour QFile
#include <QFileInfo>
#include <QTimer>
#include <QStandardPaths>  // Pour QStandardPaths
#include <QDir>            // Pour QDir
#include <QCoreApplication> 

JoinSession::JoinSession(const QString &code, QObject *parent) 
    : QObject(parent), sessionCode(code), uploadConfirmed(false) {
    connect(&socket, &QTcpSocket::readyRead, this, &JoinSession::handleReadyRead);
    connect(&socket, &QTcpSocket::connected, this, &JoinSession::handleConnected);
    connect(&socket, &QTcpSocket::disconnected, this, &JoinSession::handleDisconnected);
    connect(&socket, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::errorOccurred),
            this, &JoinSession::handleError);

    uploadTimer = new QTimer(this);
    uploadTimer->setSingleShot(true);
    connect(uploadTimer, &QTimer::timeout, this, &JoinSession::handleUploadTimeout);
}

void JoinSession::start() {
    qDebug() << "Attempting to connect to host with code:" << sessionCode;

    QUrl url("http://192.168.1.27:3000/resolve/" + sessionCode); // IP du serveur Node
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
        } else if (type == "uploadComplete") {
            uploadTimer->stop();
            uploadConfirmed = true;
            emit uploadComplete();
            qDebug() << "Upload completed successfully!";
        } else if (type == "syncSound") {
            int index = obj["index"].toInt();
            QString name = obj["name"].toString();
            qint64 fileSize = obj["size"].toInt();

            // Créer un répertoire pour stocker les sons
            QString saveDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/sounds/";
            QDir dir;
            if (!dir.exists(saveDir)) {
                dir.mkpath(saveDir);
            }

            // Lire les données du fichier
            QByteArray fileData;
            while (fileData.size() < fileSize) {
                if (!socket.waitForReadyRead(5000)) {
                    qDebug() << "Timeout while waiting for file data";
                    return;
                }
                fileData.append(socket.read(fileSize - fileData.size()));
            }

            // Sauvegarder le fichier
            QString filePath = saveDir + name;
            QFile file(filePath);
            if (file.open(QIODevice::WriteOnly)) {
                if (file.write(fileData) == fileData.size()) {
                    file.close();
                    qDebug() << "Sound file saved successfully:" << filePath;
                    emit syncSound(index, filePath, name);
                } else {
                    qDebug() << "Failed to write sound file:" << file.errorString();
                }
            } else {
                qDebug() << "Failed to open file for writing:" << file.errorString();
            }
        } else if (type == "play") {
            int index = obj["index"].toInt();
            qDebug() << "Received play command for pad" << index;
            emit remotePlay(index);
        }
    }
}

void JoinSession::handleDisconnected() {
    qDebug() << "Disconnected from host";
    if (uploadTimer->isActive()) {
        uploadTimer->stop();
        emit uploadFailed("Disconnected during upload");
    }
}

void JoinSession::handleError(QAbstractSocket::SocketError error) {
    QString errorString = socket.errorString();
    qDebug() << "Socket error:" << error << errorString;
    if (uploadTimer->isActive()) {
        uploadTimer->stop();
        emit uploadFailed("Connection error: " + errorString);
    }
    emit connectionError("Connection failed: " + errorString);
}

void JoinSession::uploadSound(int index, const QString &filePath) {
    uploadTimer->stop(); // Arrêter le timer s'il était actif
    uploadConfirmed = false;

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        emit uploadFailed("Cannot open file");
        return;
    }

    QByteArray fileData = file.readAll();
    file.close();

    QFileInfo info(filePath);
    QString fileName = info.fileName();

    QJsonObject obj;
    obj["type"] = "upload";
    obj["index"] = index;  // Include the pad index
    obj["name"] = fileName;
    obj["size"] = fileData.size();

    QByteArray header = QJsonDocument(obj).toJson(QJsonDocument::Compact) + "\n";

    if (socket.state() == QTcpSocket::ConnectedState) {
        if (socket.write(header) == -1 || socket.write(fileData) == -1) {
            emit uploadFailed("Failed to send data");
            return;
        }
        if (!socket.flush()) {
            emit uploadFailed("Failed to flush data");
            return;
        }

        uploadTimer->start(30000); // 30 seconds timeout for large audio files
        qDebug() << "Upload started, timeout timer running...";
    } else {
        emit uploadFailed("Not connected to host");
    }
}

void JoinSession::sendPlayCommand(int index) {
    QJsonObject obj;
    obj["type"] = "play";
    obj["index"] = index;
    QByteArray msg = QJsonDocument(obj).toJson(QJsonDocument::Compact) + "\n";

    if (socket.state() == QTcpSocket::ConnectedState) {
        socket.write(msg);
        socket.flush();
        qDebug() << "Sent play command for pad" << index;
    } else {
        qDebug() << "Cannot send play command - not connected to host";
    }
}

void JoinSession::handleUploadTimeout() {
    if (!uploadConfirmed) {
        qDebug() << "Upload timed out!";
        emit uploadFailed("Upload timed out");
        // Vous pourriez vouloir déconnecter ou réinitialiser la connexion ici
    }
}
