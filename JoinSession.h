#ifndef JOINSESSION_H
#define JOINSESSION_H

#include <QObject>
#include <QTcpSocket>
#include <QTimer>

class JoinSession : public QObject
{
    Q_OBJECT
public:
    explicit JoinSession(const QString &code, QObject *parent = nullptr);
    void start();
    void uploadSound(int index, const QString &filePath);
    void sendPlayCommand(int index);

signals:
    void joinedSuccessfully();
    void sessionStarted();
    void connectionError(const QString &error);
    void uploadComplete();
    void uploadFailed(const QString &error);
    void syncSound(int index, const QString &path, const QString &name);
    void remotePlay(int index);

private slots:
    void handleReadyRead();
    void handleConnected();
    void handleDisconnected();
    void handleError(QAbstractSocket::SocketError error);
    void handleUploadTimeout();

private:
    QTcpSocket socket;
    QString sessionCode;
    QTimer* uploadTimer;
    bool uploadConfirmed;
};

#endif // JOINSESSION_H
