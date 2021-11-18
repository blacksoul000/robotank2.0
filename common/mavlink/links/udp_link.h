#ifndef UDP_LINK_H
#define UDP_LINK_H

// Internal
#include "abstract_link.h"

class QUdpSocket;

namespace data_source
{
    class Endpoint;

    class UdpLink: public AbstractLink
    {
        Q_OBJECT

    public:
        UdpLink(const Endpoint& send, const Endpoint& receive, QObject* parent = nullptr);
        ~UdpLink();

        bool isConnected() const override;
        bool waitData(int timeout = 5000) override;

        AbstractLink* clone(const Endpoint& send, const Endpoint& receive) override;
        Endpoint lastSender() const override;

    public slots:
        void connectLink() override;
        void disconnectLink() override;

    protected:
        bool sendDataImpl(const QByteArray& data) override;

    private slots:
        void onReadyRead();

    private:
        QUdpSocket* m_socket = nullptr;
        QUdpSocket* m_sendSocket = nullptr;
        Endpoint m_lastSender;
    };
}

#endif // UDP_LINK_H
