// HostSession.h
#ifndef HOSTSESSION_H
#define HOSTSESSION_H

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QList>
#include "SoundManager.h"

class HostSession : public QObject {
    Q_OBJECT

public:
    explicit HostSession(SoundManager* soundManager, QObject *parent = nullptr);
    QString getSessionCode() const;
    void start();
    void startSession();

    void syncSoundToClients(int index, const QString &path, const QString &name);
    void broadcastPlayCommand(int index);
    void handleFileUpload(QTcpSocket* client, const QByteArray& data);

signals:
    void clientJoined(const QString &name);
    void sessionReady();

private slots:
    void handleNewConnection();
    void readClientData();
    void handleClientDisconnected();

private:
    QString generateCode();
    QTcpServer server;
    QList<QTcpSocket*> clients;
    QString sessionCode;
    SoundManager* soundManager;
};

#endif
