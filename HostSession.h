#pragma once
#include <QTcpServer>
#include <QTcpSocket>
#include <QObject>
#include <QList>

class HostSession : public QObject {
    Q_OBJECT
public:
    HostSession(QObject *parent = nullptr);
    QString getSessionCode() const;
    void start();
    void startSession();

signals:
    void clientJoined(QString name);
    void sessionReady();

private slots:
    void handleNewConnection();
    void readClientData();

private:
    QTcpServer server;
    QList<QTcpSocket*> clients;
    QString sessionCode;

    QString generateCode();
};