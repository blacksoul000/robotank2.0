#include "../exchangers/uart.h"

#include <QSerialPort>

class Uart::Impl
{
public:
    QSerialPort* serial = nullptr;
    int packageSize;

    QByteArray buffer;
    QByteArray prefix = QByteArray(2, 0x55);
    bool waitPrefix = true;

    bool readData();
};

Uart::Uart(const QString& device, int packageSize, QObject* parent)
	: IExchanger(parent),
	  d(new Impl)
{
	d->packageSize = packageSize;

    d->serial = new QSerialPort(device, this);
    d->serial->setBaudRate(115200);
    d->serial->setDataBits(QSerialPort::Data8);
    d->serial->setParity(QSerialPort::NoParity);
    d->serial->setStopBits(QSerialPort::OneStop);

    connect(d->serial, &QSerialPort::readyRead, this, &Uart::onReadyRead);
}

Uart::~Uart()
{
	delete d;
}

bool Uart::open()
{
	return d->serial->open(QIODevice::ReadWrite);
}

void Uart::close()
{
	d->serial->close();
}

bool Uart::isOpen() const
{
	return d->serial->isOpen();
}

bool Uart::sendData(const QByteArray& data)
{
    QByteArray package(d->prefix);
    package.append(data);

    return (d->serial->write(package) == package.size());
}

void Uart::onReadyRead()
{
	if (d->readData()) emit dataAvailable(d->buffer.mid(d->prefix.size(), d->packageSize));
	d->buffer.remove(0, d->packageSize + d->prefix.size());
}

bool Uart::Impl::readData()
{
    QByteArray data = serial->readAll();
    if (serial->error() != QSerialPort::NoError) return false;

    buffer.append(data);
    if (waitPrefix)
    {
        if (buffer.size() < prefix.size()) return false;

        auto idx = buffer.lastIndexOf(prefix, buffer.count() - packageSize);
        if (idx == -1)
        {
            buffer.remove(0, buffer.size() - prefix.size());
            return false;
        }

        buffer.remove(0, idx);
        waitPrefix = false;
    }

    if (!waitPrefix)
    {
        const int packetSize = prefix.size() + packageSize;
        if (buffer.size() < packetSize) return false;

		waitPrefix = true;
		return true;
    }

    return false;
}

