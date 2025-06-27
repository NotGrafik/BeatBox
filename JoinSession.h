#ifndef JOINSESSION_H
#define JOINSESSION_H

#include <QObject>
#include <QTcpSocket>

class JoinSession : public QObject {
    Q_OBJECT

public:
    explicit JoinSession(const QString &code, QObject *parent = nullptr);
    void start();

signals:
    void sessionStarted();
    void connectionError(const QString &error);
    void joinedSuccessfully();

private slots:
    void handleReadyRead();
    void handleConnected();
    void handleDisconnected();
    void handleError(QAbstractSocket::SocketError error);

private:
    QTcpSocket socket;
    QString sessionCode;
};

#endif 