#include "gamepad.h"

//Qt
#include <QScopedPointer>
#include <QDir>
#include <QFileInfo>
#include <QFile>
#include <QDebug>

// linux
#include <unistd.h>
#include <linux/joystick.h>
#include <fcntl.h>
#include <errno.h>

class Gamepad::Impl
{
public:
    quint8 axesCount = 0;
    QVector< short > axes;
    quint16 buttons = 0;

    QString device = "/dev/input/js0";
    int fd = -1;
    bool isOpened = false;

    QFile capacityFd;
    QFile status;

    int capacity = 0;
    bool isConnected = false;
    bool isCharging = false;

    QString gamepadSystemPath() const;
};

Gamepad::Gamepad(const QString& device, QObject* parent) :
    QObject(parent),
    d(new Impl)
{
	d->device = device;
}

Gamepad::~Gamepad()
{
    if (d->capacityFd.isOpen()) d->capacityFd.close();
    if (d->status.isOpen()) d->status.close();

    ::close (d->fd);
    delete d;
}

void Gamepad::execute()
{
    if (!d->isOpened)
    {
        if (this->open())
        {
            qDebug() << Q_FUNC_INFO << "Connected";

            fcntl (d->fd, F_SETFD, FD_CLOEXEC);
            ioctl (d->fd, JSIOCGAXES, &d->axesCount);
            d->axes.resize(d->axesCount);
            d->isConnected = true;
            emit connectedChanged(d->isConnected);

            const QString path = d->gamepadSystemPath();
            if (!path.isEmpty())
            {
                d->capacityFd.setFileName(path + "/capacity");
                d->capacityFd.open(QIODevice::ReadOnly | QIODevice::Text);
                connect(&d->capacityFd, &QIODevice::readyRead, this, &Gamepad::onCapacityChanged);
                this->onCapacityChanged();

                d->status.setFileName(path + "/status");
                d->status.open(QIODevice::ReadOnly | QIODevice::Text);
                connect(&d->status, &QIODevice::readyRead, this, &Gamepad::onStatusChanged);
                this->onStatusChanged();
            }
        }
        else
        {
            return;
        }
    }
    this->readData();
}

QVector< short > Gamepad::axes() const
{
	return d->axes;
}

quint16 Gamepad::buttons() const
{
	return d->buttons;
}

quint8 Gamepad::axesCount() const
{
	return d->axesCount;
}

bool Gamepad::open()
{
    d->fd = ::open(d->device.toStdString().c_str(), O_RDONLY | O_NONBLOCK);
    d->isOpened = (d->fd >= 0);
    return d->isOpened;
}

void Gamepad::close()
{
    qDebug() << Q_FUNC_INFO << "Disconnected";
    d->fd = -1;
    d->isOpened = false;
    d->capacityFd.close();
    d->status.close();
    d->buttons = 0;
    d->axes.fill(0, d->axes.size());

    d->isConnected = false;
    emit connectedChanged(d->isConnected);
}

void Gamepad::readData()
{
	if (fcntl (d->fd, F_GETFD) == 0 || errno == ENODEV)
	{
		this->close();
		return;
	}

    struct js_event event;
    while (::read(d->fd, &event, sizeof(event)) != -1)
    {
        switch (event.type)
        {
        case JS_EVENT_BUTTON:
            d->buttons = (d->buttons & ~(1 << event.number)) | (event.value << event.number);
            break;
        case JS_EVENT_AXIS:
            d->axes[event.number] = event.value;
            break;
        default:
            break;
        }
    }
}

void Gamepad::onCapacityChanged()
{
    d->capacityFd.seek(0);
    QByteArray data = d->capacityFd.readAll().trimmed();
    bool ok;
    const int capacity = data.toInt(&ok);
    if (ok)
	{
    	d->capacity = capacity;
    	emit capacityChanged(capacity);
	}
}

void Gamepad::onStatusChanged()
{
    d->status.seek(0);
    const QString data = d->status.readAll();
    d->isCharging = data.compare("Charging", Qt::CaseInsensitive) == 0;
    emit chargingChanged(d->isCharging);
}

int Gamepad::capacity() const
{
	return d->capacity;
}

bool Gamepad::isConnected() const
{
	return d->isConnected;
}

bool Gamepad::isCharging() const
{
	return d->isCharging;
}

QString Gamepad::Impl::gamepadSystemPath() const
{
    QDir dir("/sys/class/power_supply");
    if (!dir.exists()) return QString();

    const auto& list = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
    if (list.isEmpty()) return QString();
    return list.first().absoluteFilePath();
}
