#include "udp_link.h"

#include "endpoint.h"

// Qt
#include <QUdpSocket>
#include <QHostAddress>

using namespace data_source;

UdpLink::UdpLink(const Endpoint& send, const Endpoint& receive, QObject* parent):
    AbstractLink(send, receive, parent),
    m_socket(new QUdpSocket(this)),
    m_sendSocket(new QUdpSocket(this))
{
    QObject::connect(m_socket, &QUdpSocket::readyRead, this, &UdpLink::onReadyRead);
    connect(m_socket, static_cast< void(QAbstractSocket::*)(QAbstractSocket::SocketError) >(&QAbstractSocket::error),
            this, &AbstractLink::onSocketError);
}

UdpLink::~UdpLink()
{
}

bool UdpLink::isConnected() const
{
    return m_socket->state() == QAbstractSocket::BoundState;
}

bool UdpLink::waitData(int timeout)
{
    return m_socket->waitForReadyRead(timeout);
}

AbstractLink* UdpLink::clone(const Endpoint& send, const Endpoint& receive)
{
    return new UdpLink(send, receive, this);
}

void UdpLink::connectLink()
{
    if (this->isConnected() || this->receive().port() == 0) return;

    if (!m_socket->bind(this->receive().address(), this->receive().port(),
                        QAbstractSocket::ShareAddress))
    {
        qCritical() << Q_FUNC_INFO << m_socket->errorString();
        m_socket->close();
    }
    else
    {
        qDebug() << Q_FUNC_INFO << "Binded on" << this->receive().address() << this->receive().port();
        emit connectedChanged(true);
    }
}

void UdpLink::disconnectLink()
{
    if (!this->isConnected()) return;

    m_socket->close();
    emit connectedChanged(false);
}

bool UdpLink::sendDataImpl(const QByteArray& data)
{
    return (m_sendSocket->writeDatagram(data, this->send().address(), this->send().port()) == data.size());
}

void UdpLink::onReadyRead()
{
    while (m_socket->hasPendingDatagrams())
    {
        QByteArray datagram;
        datagram.resize(m_socket->pendingDatagramSize());
        QHostAddress sender;
        quint16 port;
        m_socket->readDatagram(datagram.data(), datagram.size(), &sender, &port);
        m_lastSender.setAddress(sender);
        m_lastSender.setPort(port);
        this->receiveData(datagram);
    }
}

Endpoint UdpLink::lastSender() const
{
    return m_lastSender;
}
