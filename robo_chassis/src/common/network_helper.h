#include <QHostAddress>
#include <QNetworkInterface>

namespace common
{
    static QHostAddress localIp()
    {
        for (const QHostAddress& address: QNetworkInterface::allAddresses())
        {
            if (address.protocol() == QAbstractSocket::IPv4Protocol
                && address != QHostAddress(QHostAddress::LocalHost))
            {
                 return address;
            }
        }
        return QHostAddress();
    }

    static QHostAddress localNetwork()
    {
        QHostAddress localIp = common::localIp();
        if (localIp.isNull()) return QHostAddress();

        QStringList address = localIp.toString().split('.');
        address.last() = "255";
        return QHostAddress(address.join('.'));
    }
} // namespace common
