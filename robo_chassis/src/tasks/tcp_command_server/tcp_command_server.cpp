#include "tcp_command_server.h"
#include <QDebug>
#include "robo_core.h"
#include "pub_sub.h"

class TcpCommandServer::Impl
{
public:
    QTcpServer* server = nullptr;
    QList<QTcpSocket*> clients;
    quint16 port = 5555;
    RoboCore* roboCore = nullptr;
};

TcpCommandServer::TcpCommandServer(quint16 port) : d(new Impl())
{
    d->port = port;
    d->server = new QTcpServer(this);
    
    connect(d->server, &QTcpServer::newConnection, this, &TcpCommandServer::onNewConnection);
}

TcpCommandServer::~TcpCommandServer()
{
    delete d;
}

void TcpCommandServer::execute()
{
    if (!d->server->listen(QHostAddress::Any, d->port)) {
        qCritical() << "TCP Server failed to start:" << d->server->errorString();
        return;
    }
    qDebug() << "TCP Command Server listening on port" << d->port;
    
    // Подписка на команды и пересылка в RoboCore через PubSub
    PubSub::instance()->subscribe("core/command", &RoboCore::onCommandReceived, 
                                  PubSub::instance()->getSubscriber<RoboCore>("robo_core_instance"));
}

void TcpCommandServer::onNewConnection()
{
    QTcpSocket* socket = d->server->nextPendingConnection();
    connect(socket, &QTcpSocket::readyRead, this, &TcpCommandServer::onReadyRead);
    connect(socket, &QTcpSocket::disconnected, this, &TcpCommandServer::onDisconnected);
    d->clients.append(socket);
    qDebug() << "New client connected. Total clients:" << d->clients.size();
}

void TcpCommandServer::onReadyRead()
{
    QTcpSocket* socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket) return;

    while (socket->canReadLine()) {
        QByteArray line = socket->readLine().trimmed();
        if (line.isEmpty()) continue;

        QJsonDocument doc = QJsonDocument::fromJson(line);
        if (doc.isObject()) {
            emit commandReceived(doc.object());
        } else {
            qWarning() << "Invalid JSON received:" << line;
        }
    }
}

void TcpCommandServer::onDisconnected()
{
    QTcpSocket* socket = qobject_cast<QTcpSocket*>(sender());
    if (socket) {
        d->clients.removeAll(socket);
        socket->deleteLater();
        qDebug() << "Client disconnected. Total clients:" << d->clients.size();
    }
}
