// HostSession.h
#ifndef HOSTSESSION_H
#define HOSTSESSION_H

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QList>

class HostSession : public QObject {
    Q_OBJECT

public:
    explicit HostSession(QObject *parent = nullptr);
    QString getSessionCode() const;
    void start();
    void startSession();

    void syncSoundToClients(int index, const QString &path, const QString &name);

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
};

#endif