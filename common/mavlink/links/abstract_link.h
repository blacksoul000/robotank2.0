#ifndef ABSTRACT_LINK_H
#define ABSTRACT_LINK_H

#include "endpoint.h"

// Qt
#include <QObject>
#include <QSharedPointer>

namespace data_source
{
    class AbstractLink: public QObject
    {
        Q_OBJECT

    public:
        explicit AbstractLink(const Endpoint& send, const Endpoint& receive);
        virtual ~AbstractLink() = default;

        virtual bool isConnected() const = 0;
        virtual bool waitData(int timeout = 5000) = 0;

        virtual AbstractLink* clone(const Endpoint& send, const Endpoint& receive) = 0;

        Endpoint send() const;
        Endpoint receive() const;
        virtual Endpoint local() const;

        QByteArray lastReceivedData() const;
        int takeBytesReceived();
        int takeBytesSent();

    public slots:
        void setConnected(bool connected);
        virtual void connectLink() = 0;
        virtual void disconnectLink() = 0;

        void sendData(const QByteArray& data);

    signals:
        void connectedChanged(bool connected);
        void errored(QString error);
        void dataReceived(QByteArray data);
        void dataSent();

    protected:
        virtual bool sendDataImpl(const QByteArray& data) = 0;

    protected slots:
        void receiveData(const QByteArray& data);

    public slots: // QOverload require public
        void onSocketError(int error);

    private:
        QByteArray m_lastReceivedData;
        Endpoint m_send;
        Endpoint m_receive;
        int m_bytesReceived = 0;
        int m_bytesSent = 0;
    };
}

#endif // ABSTRACT_LINK_H
