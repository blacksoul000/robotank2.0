#include "gpio_controller.h"

#include "pub_sub.h"

#include <wiringPi.h>

namespace
{
    const uint8_t shotFinishedPin1 = 28;
    const uint8_t shotFinishedPin2 = 29;

    const uint8_t resetPin = 4;
}

class GpioController::Impl
{
public:
    Publisher< bool >* shotStatusP = nullptr;

    bool shoting = false;
    bool shotClosing = false;

    void onShotStatusChanged(bool shot);
};

GpioController::GpioController():
    ITask(),
    d(new Impl)
{
    d->shotStatusP = PubSub::instance()->advertise< bool >("core/shot");

    PubSub::instance()->subscribe("joy/buttons", &GpioController::onJoyEvent, this);
    PubSub::instance()->subscribe("arduino/status", &GpioController::onArduinoStatusChanged, this);

    wiringPiSetup();

    pinMode (::shotFinishedPin1, OUTPUT);
    pinMode (::shotFinishedPin2, INPUT);

    d->onShotStatusChanged(false);
}

GpioController::~GpioController()
{
    digitalWrite (::shotFinishedPin1, LOW);
    delete d->shotStatusP;
    delete d;
}

void GpioController::execute()
{
    if (!d->shoting) return;

    if(digitalRead(::shotFinishedPin2) == HIGH)
    {
        d->shotClosing = true;
    }
    else if (d->shotClosing)
    {
        d->shotClosing = false;
        d->onShotStatusChanged(false);
    }
}

void GpioController::onJoyEvent(const quint16& joy)
{
    if (d->shoting) return;

    if (((joy >> 6) & 1 == 1) && ((joy >> 7) & 1 == 1)) // both triggers
    {
        d->onShotStatusChanged(true);
    }
}

void GpioController::onArduinoStatusChanged(const bool& online)
{
    qDebug() << Q_FUNC_INFO << online;
    if (online) return;

    return; // TODO - test it
    digitalWrite (::resetPin, HIGH);
    delay(50);
    digitalWrite (::resetPin, LOW);
}

//------------------------------------------------------------------------------------
void GpioController::Impl::onShotStatusChanged(bool shot)
{
    qDebug() << Q_FUNC_INFO << shot;
    shotStatusP->publish(shot);
    digitalWrite (::shotFinishedPin1, shot ? HIGH : LOW);
}
