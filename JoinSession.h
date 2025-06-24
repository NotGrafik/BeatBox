#pragma once
#include <QTcpSocket>
#include <QObject>

class JoinSession : public QObject {
    Q_OBJECT
public:
    JoinSession(const QString &code, QObject *parent = nullptr);
    void start();

signals:
    void sessionStarted();
    void connectionError(QString reason);

private slots:
    void handleReadyRead();

private:
    QTcpSocket socket;
    QString sessionCode;
};