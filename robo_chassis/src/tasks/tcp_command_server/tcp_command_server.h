#ifndef TCP_COMMAND_SERVER_H
#define TCP_COMMAND_SERVER_H

#include "i_task.h"
#include <QTcpServer>
#include <QTcpSocket>
#include <QJsonObject>

class TcpCommandServer : public ITask
{
    Q_OBJECT
public:
    explicit TcpCommandServer(quint16 port = 5555);
    ~TcpCommandServer();

    void execute() override;

signals:
    void commandReceived(const QJsonObject& command);

private slots:
    void onNewConnection();
    void onReadyRead();
    void onDisconnected();

private:
    class Impl;
    Impl* d;
};

#endif // TCP_COMMAND_SERVER_H
