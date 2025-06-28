#include "NetworkManager.h"
#include "HostSession.h"
#include "JoinSession.h"
#include <QDebug>
#include <QFileInfo>

NetworkManager::NetworkManager(QObject *parent)
    : QObject(parent), hostSession(nullptr), joinSessionClient(nullptr), soundManager(new SoundManager(this)), currentUploadIndex(-1) {
}


NetworkManager::~NetworkManager() {
    if (hostSession) {
        delete hostSession;
        hostSession = nullptr;
    }
    if (joinSessionClient) {
        delete joinSessionClient;
        joinSessionClient = nullptr;
    }
}

void NetworkManager::startHosting() {
    if (hostSession) {
        delete hostSession;
    }

    hostSession = new HostSession(soundManager, this);
    connect(hostSession, &HostSession::clientJoined, this, &NetworkManager::onClientJoined);
    connect(hostSession, &HostSession::sessionReady, this, &NetworkManager::onSessionReady);

    hostSession->start();
    emit sessionCreated(hostSession->getSessionCode());
}

QString NetworkManager::getSessionCode() const {
    return hostSession ? hostSession->getSessionCode() : QString();
}

void NetworkManager::startGameSession() {
    if (hostSession) {
        hostSession->startSession();
    }
}

void NetworkManager::joinSession(const QString &code) {
    if (joinSessionClient) {
        delete joinSessionClient;
    }

    joinSessionClient = new JoinSession(code, this);
    connect(joinSessionClient, &JoinSession::joinedSuccessfully, this, &NetworkManager::onJoinedSuccessfully);
    connect(joinSessionClient, &JoinSession::sessionStarted, this, &NetworkManager::onSessionStarted);
    connect(joinSessionClient, &JoinSession::connectionError, this, &NetworkManager::onConnectionError);
    connect(joinSessionClient, &JoinSession::syncSound, this, &NetworkManager::onSyncSound);
    connect(joinSessionClient, &JoinSession::uploadComplete, this, &NetworkManager::onUploadComplete);

    joinSessionClient->start();
}

bool NetworkManager::isConnected() const {
    if (hostSession) return true;
    if (joinSessionClient) return true; // Vous pourriez vouloir vérifier l'état de la socket
    return false;
}

void NetworkManager::onClientJoined(const QString &name) {
    qDebug() << "NetworkManager: Client joined -" << name;
    emit clientJoined(name);
}

void NetworkManager::onSessionReady() {
    qDebug() << "NetworkManager: Session ready";
    emit sessionStarted();
}

void NetworkManager::onJoinedSuccessfully() {
    qDebug() << "NetworkManager: Successfully joined session";
    emit joinedSession();
}

void NetworkManager::onSessionStarted() {
    qDebug() << "NetworkManager: Session started";
    emit sessionStarted();
}

void NetworkManager::onConnectionError(const QString &error) {
    qDebug() << "NetworkManager: Connection error -" << error;
    emit connectionError(error);
}

void NetworkManager::onSyncSound(int index, const QString& path, const QString& name) {
    soundManager->importSound(path);
    if (hostSession) {
        int soundIndex = soundManager->getSoundCount() - 1;
        QFileInfo info(path);
        QString fileName = info.fileName();
        hostSession->syncSoundToClients(path, fileName);
    }
    emit soundReady(index, name);
}

void NetworkManager::uploadSound(int index, const QString &filePath) {
    if (isHost()) {
        // Host mode: import sound locally and sync to clients
        soundManager->importSound(filePath);
        QFileInfo info(filePath);
        QString fileName = info.fileName();

        if (hostSession) {
            hostSession->syncSoundToClients(filePath, fileName);
        }

        emit soundReady(index, fileName);
    } else if (joinSessionClient) {
        // Client mode: upload to host
        currentUploadIndex = index;
        QFileInfo info(filePath);
        currentUploadFileName = info.fileName();
        joinSessionClient->uploadSound(filePath);
    }
}

void NetworkManager::onUploadComplete() {
    if (currentUploadIndex >= 0) {
        emit soundReady(currentUploadIndex, currentUploadFileName);
        currentUploadIndex = -1; // Reset
        currentUploadFileName.clear(); // Reset
    }
}

void NetworkManager::onRemotePlay(int index) {
    soundManager->playSound(index);
}
