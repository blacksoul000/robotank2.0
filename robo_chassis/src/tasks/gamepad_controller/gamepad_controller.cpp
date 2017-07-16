#include "gamepad_controller.h"

//msgs
#include "joy_axes.h"

#include "pub_sub.h"

//Qt
#include <QSocketNotifier>
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
    JoyAxes axes;
    quint16 buttons = 0;

    Publisher< bool >* joyStatusP = nullptr;
    Publisher< JoyAxes >* axesP = nullptr;
    Publisher< quint16 >* buttonsP = nullptr;

    const QString device = "/dev/input/js0";

    int fd = -1;
    bool isOpened = false;

    int infoFd = -1;
};

GamepadController::GamepadController() :
    ITask(),
    d(new Impl)
{
    d->joyStatusP = PubSub::instance()->advertise< bool >("joy/status");
    d->axesP = PubSub::instance()->advertise< JoyAxes >("joy/axes");
    d->buttonsP = PubSub::instance()->advertise< quint16 >("joy/buttons");
}

GamepadController::~GamepadController()
{
    close(d->fd);
    delete d->joyStatusP;
    delete d->axesP;
    delete d->buttonsP;
    delete d;
}

void GamepadController::execute()
{
    if (!d->isOpened && !this->open()) return;
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

bool GamepadController::open()
{
    d->fd = ::open(d->device.toStdString().c_str(), O_RDONLY | O_NONBLOCK);
    d->isOpened = (d->fd >= 0);
    if (d->isOpened)
    {
        qDebug() << Q_FUNC_INFO << "Connected";
        d->joyStatusP->publish(true);
    }
    return d->isOpened;
}

void GamepadController::readData()
{
    struct js_event event;

    while (read(d->fd, &event, sizeof(event)) == sizeof(event))
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
        default:
            break;
        }
    }
}

void GamepadController::onGamepadStatusChanged()
{

}
