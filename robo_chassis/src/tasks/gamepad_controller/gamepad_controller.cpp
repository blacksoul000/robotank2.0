#include "gamepad_controller.h"

//msgs
#include "joy_axes.h"

#include "pub_sub.h"

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

class GamepadController::Impl
{
public:
    bool axesChanged = false;
    bool buttonsChanged = false;

    quint8 axesCount; // FIXME
    JoyAxes axes;
    quint16 buttons = 0;

    Publisher< bool >* joyStatusP = nullptr;
    Publisher< JoyAxes >* axesP = nullptr;
    Publisher< quint16 >* buttonsP = nullptr;
    Publisher< quint8 >* capacityP = nullptr;
    Publisher< bool >* chargingP = nullptr;

    const QString gamepad = "/dev/input/js0";
    int fd = -1;
    bool isOpened = false;

    QFile capacity;
    QFile status;

    QString gamepadSystemPath() const;
    bool open();
    void close();
};

GamepadController::GamepadController() :
    ITask(),
    d(new Impl)
{
    d->joyStatusP = PubSub::instance()->advertise< bool >("joy/status");
    d->axesP = PubSub::instance()->advertise< JoyAxes >("joy/axes");
    d->buttonsP = PubSub::instance()->advertise< quint16 >("joy/buttons");
    d->capacityP = PubSub::instance()->advertise< quint8 >("joy/capacity");
    d->chargingP = PubSub::instance()->advertise< bool >("joy/charging");
}

GamepadController::~GamepadController()
{
    if (d->capacity.isOpen()) d->capacity.close();
    if (d->status.isOpen()) d->status.close();
    close (d->fd);
    delete d->joyStatusP;
    delete d->axesP;
    delete d->buttonsP;
    delete d->capacityP;
    delete d->chargingP;
    delete d;
}

void GamepadController::execute()
{
    if (!d->isOpened)
    {
        if (d->open())
        {
            qDebug() << Q_FUNC_INFO << "Connected";

            fcntl (d->fd, F_SETFD, FD_CLOEXEC);
            ioctl (d->fd, JSIOCGAXES, &d->axesCount);
            d->joyStatusP->publish(true);

            const QString path = d->gamepadSystemPath();
            if (!path.isEmpty())
            {
                d->capacity.setFileName(path + "/capacity");
                d->capacity.open(QIODevice::ReadOnly | QIODevice::Text);
                connect(&d->capacity, &QIODevice::readyRead, this, &GamepadController::onCapacityChanged);
                this->onCapacityChanged();

                d->status.setFileName(path + "/status");
                d->status.open(QIODevice::ReadOnly | QIODevice::Text);
                connect(&d->status, &QIODevice::readyRead, this, &GamepadController::onStatusChanged);
                this->onStatusChanged();
            }
        }
        else
        {
            return;
        }
    }
    this->readData();

    if (d->axesChanged)
    {
        d->axesP->publish(d->axes);
        d->axesChanged = false;
    }
    if (d->buttonsChanged)
    {
        d->buttonsP->publish(d->buttons);
        d->buttonsChanged = false;
    }
}

bool GamepadController::Impl::open()
{
    fd = ::open(gamepad.toStdString().c_str(), O_RDONLY | O_NONBLOCK);
    isOpened = (fd >= 0);
    return isOpened;
}

void GamepadController::Impl::close()
{
    qDebug() << Q_FUNC_INFO << "Disconnected";
    fd = -1;
    isOpened = false;
    capacity.close();
    status.close();
    joyStatusP->publish(false);
}

void GamepadController::readData()
{
	if (fcntl (d->fd, F_GETFD) == 0 || errno == ENODEV)
	{
		d->close();
		return;
	}

    struct js_event event;
    while (read(d->fd, &event, sizeof(event)) != -1)
    {
        switch (event.type)
        {
        case JS_EVENT_BUTTON:
            d->buttons = (d->buttons & ~(1 << event.number)) | (event.value << event.number);
            d->buttonsChanged = true;
            break;
        case JS_EVENT_AXIS:
            d->axes.axes[event.number] = event.value;
            d->axesChanged = true;
            break;
        default:
            break;
        }
    }
}

void GamepadController::onCapacityChanged()
{
    d->capacity.seek(0);
    QByteArray data = d->capacity.readAll().trimmed();
    bool ok;
    const int capacity = data.toInt(&ok);
    if (ok) d->capacityP->publish(capacity);
}

void GamepadController::onStatusChanged()
{
    d->status.seek(0);
    const QString data = d->status.readAll();
    const bool charging = data.compare("Charging", Qt::CaseInsensitive) == 0;
    d->chargingP->publish(charging);
}

QString GamepadController::Impl::gamepadSystemPath() const
{
    QDir dir("/sys/class/power_supply");
    if (!dir.exists()) return QString();

    const auto& list = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
    if (list.isEmpty()) return QString();
    return list.first().absoluteFilePath();
}
