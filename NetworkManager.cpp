#include "NetworkManager.h"
#include "HostSession.h"
#include "JoinSession.h"
#include <QDebug>

NetworkManager::NetworkManager(QObject *parent)
    : QObject(parent), hostSession(nullptr), joinSessionClient(nullptr) {
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
    
    hostSession = new HostSession(this);
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