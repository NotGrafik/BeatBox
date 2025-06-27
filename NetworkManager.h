#ifndef NETWORKMANAGER_H
#define NETWORKMANAGER_H
#include "SoundManager.h"

#include <QObject>

// Forward declarations pour éviter les inclusions circulaires
class HostSession;
class JoinSession;

class NetworkManager : public QObject {
    Q_OBJECT

public:
    explicit NetworkManager(QObject *parent = nullptr);
    ~NetworkManager();
    
    // Mode host
    void startHosting();
    QString getSessionCode() const;
    void startGameSession();
    
    // Mode client
    void joinSession(const QString &code);
    
    bool isHost() const { return hostSession != nullptr; }
    bool isConnected() const;

signals:
    void sessionCreated(const QString &code);
    void clientJoined(const QString &name);
    void sessionStarted();
    void joinedSession();
    void connectionError(const QString &error);
    void soundReady(int index, const QString &name);
    void remotePlay(int index);

private slots:
    void onClientJoined(const QString &name);
    void onSessionReady();
    void onJoinedSuccessfully();
    void onSessionStarted();
    void onConnectionError(const QString &error);
    void onSyncSound(int index, const QString& path, const QString& name);
    void onRemotePlay(int index);

private:
    HostSession *hostSession;
    JoinSession *joinSessionClient;
    SoundManager* soundManager;
};

#endif