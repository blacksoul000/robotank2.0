#include "i2c_master.h"

 //linux
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>

#include <QTimer>
#include <QDebug>

namespace
{
    const int arduinoAddress = 0x4;
    const int requestDataInterval = 1000; // ms
}

class I2CMaster::Impl
{
public:
    int fd = -1;
    QString device;
    int packageSize;
    QTimer* timer = nullptr;

    bool readData();
};

I2CMaster::I2CMaster(const QString& device, int packageSize, QObject* parent) :
	IExchanger(parent),
	d(new Impl)
{
	d->device = device;
	d->packageSize = packageSize;
	d->timer = new QTimer(this);
	d->timer->setInterval(::requestDataInterval);

	connect(d->timer, &QTimer::timeout, this, &I2CMaster::readData);
}

I2CMaster::~I2CMaster()
{
	delete d;
}

bool I2CMaster::open()
{
	d->fd = ::open(d->device.toStdString().c_str(), O_RDWR | O_NONBLOCK);
	if (d->fd < 0)
	{
		qDebug() << Q_FUNC_INFO << "Failed to open";
		return false;
	}

	if (ioctl(d->fd, I2C_SLAVE, ::arduinoAddress) < 0)
	{
		qDebug() << Q_FUNC_INFO << "Failed to acquire bus access and/or talk to slave";
		return false;
	}
	d->timer->start();
	this->readData();
	return true;
}

void I2CMaster::close()
{
	d->timer->stop();
	::close(d->fd);
}

bool I2CMaster::isOpen() const
{
	return d->fd >= 0;
}

bool I2CMaster::sendData(const QByteArray& data)
{
	return (::write(d->fd, data.toStdString().c_str(), data.size()) == data.size());
}

bool I2CMaster::readData()
{
	QByteArray data;
	data.resize(d->packageSize);

    if (::read(d->fd, data.data(), d->packageSize) != d->packageSize)
    {
    	qDebug() << Q_FUNC_INFO << "Failed to read from the bus.";
        return false;
    }

    emit dataAvailable(data);
    return true;
}

