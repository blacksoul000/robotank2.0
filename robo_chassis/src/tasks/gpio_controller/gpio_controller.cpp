#include "gpio_controller.h"

#include "pub_sub.h"
#include "influence.h"
#include "empty.h"

#include <pigpio.h>
#include <cmath>

#include <QDebug>

#define ENABLE_SERVO

namespace
{
    const uint8_t shotPin = 27;
    const uint8_t shotFinishedPin1 = 20;
    const uint8_t shotFinishedPin2 = 21;

    const uint8_t resetPin = 4;
    const uint8_t pointerPin = 26;

    const uint8_t gunVPin = 23;

    const int tickInterval = 10;
    constexpr double gunTickCoef = 60.0 / SHRT_MAX;
    constexpr double positionCoef = 360.0 / 32767;
}

class GpioController::Impl
{
public:
    struct ServoInfo
    {
        uint16_t minPulse;
        uint16_t maxPulse;
        uint16_t pulse;
        uint16_t realPulse;
        int8_t tick;
        uint16_t zeroLift;
        float pulsePerDegree;
    };

    Publisher< float >* gunVP = nullptr;
    Publisher< bool >* pointerP = nullptr;

    bool shoting = false;
    bool shotClosing = false;
    bool pointer = false;

    QMap< uint8_t, ServoInfo > servo;

    void onShotStatusChanged(bool shot);
    void onPointerTriggered();
};

GpioController::GpioController():
    ITask(),
    d(new Impl)
{
    d->gunVP = PubSub::instance()->advertise< float >("gun/position/vertical");
    d->pointerP = PubSub::instance()->advertise< bool >("chassis/pointer");
}

GpioController::~GpioController()
{
    gpioWrite(::shotFinishedPin1, 0);
    gpioTerminate();

    delete d->gunVP;
    delete d->pointerP;
    delete d;
}

void GpioController::start()
{
    if (gpioInitialise() >= 0)
    {
#ifdef ENABLE_SERVO
        gpioSetTimerFuncEx(0, ::tickInterval, servoTickProxy, this);

        d->servo.insert(::gunVPin, {1200, 1670, 1600, 0, 0, 1600, 33.3201});
//        d->servo.insert(::gunVPin, {900, 1400, 1300, 0, 1300, 18});
#endif //ENABLE_SERVO

        gpioSetMode(::shotPin, PI_OUTPUT);
        gpioSetMode(::shotFinishedPin1, PI_OUTPUT);
        gpioSetMode(::shotFinishedPin2, PI_INPUT);
    }
    else
    {
        qWarning() << "Failed to initialize GPIO";
    }


    PubSub::instance()->subscribe("joy/buttons", &GpioController::onJoyEvent, this);
    PubSub::instance()->subscribe("core/influence", &GpioController::onInfluence, this);
    PubSub::instance()->subscribe("core/deviationV", &GpioController::onDeviation, this);
    PubSub::instance()->subscribe("gun/calibrate", &GpioController::onGunCalibrate, this);
}

void GpioController::execute()
{
    if (d->shoting)
    {
        if (gpioRead(::shotFinishedPin2))
        {
            d->shotClosing = true;
        }
        else if (d->shotClosing)
        {
            d->shotClosing = false;
            d->onShotStatusChanged(false);
        }
    }

//    qDebug() << Q_FUNC_INFO << gunPosition.y() << d->servo[::gunVPin].realPulse << d->servo[::gunVPin].maxPulse;
    d->gunVP->publish(((d->servo[::gunVPin].maxPulse - d->servo[::gunVPin].realPulse) / d->servo[::gunVPin].pulsePerDegree));
}

void GpioController::onJoyEvent(const quint16& joy)
{
    if ((((joy >> 6) & 1) == 1) && (((joy >> 7) & 1) == 1)) // both triggers
    {
        d->onShotStatusChanged(!d->shoting); // if we are already shoting - stop!
    }
    if (((joy >> 5) & 1) == 1) d->onPointerTriggered();
}

void GpioController::onInfluence(const Influence& influence)
{
    d->servo[::gunVPin].tick = std::ceil(influence.gunV * ::gunTickCoef);
//    qDebug() << Q_FUNC_INFO << __LINE__ << influence.gunV << ::tickCoef << d->servo[::gunVPin].tick << d->servo[::gunVPin].pulse;
}

void GpioController::onDeviation(const double& value)
{
    d->servo[::gunVPin].tick = 0;
    d->servo[::gunVPin].pulse = d->servo[::gunVPin].maxPulse - value * d->servo[::gunVPin].pulsePerDegree;
//    qDebug() << Q_FUNC_INFO << __LINE__ << value << ((d->servo[::gunVPin].maxPulse - d->servo[::gunVPin].pulse) / d->servo[::gunVPin].pulsePerDegree);
}

void GpioController::onGunCalibrate(const Empty&)
{
    d->servo[::gunVPin].zeroLift = d->servo[::gunVPin].pulse;
}

void GpioController::servoTickProxy(void* data)
{
    reinterpret_cast< GpioController* >(data)->servoTick();
}

void GpioController::servoTick()
{
    for (auto it = d->servo.begin(), end = d->servo.end(); it != end; ++it)
    {
        it.value().realPulse = qBound< uint16_t >(it.value().minPulse, it.value().pulse + it.value().tick, it.value().maxPulse);
//        qDebug() << it.key() << it.value().pulse << it.value().minPulse << it.value().maxPulse << ((it.value().maxPulse - it.value().pulse) / it.value().pulsePerDegree);
        gpioServo(it.key(), it.value().realPulse);
    }
}

//------------------------------------------------------------------------------------
void GpioController::Impl::onShotStatusChanged(bool shot)
{
    qDebug() << Q_FUNC_INFO << shot;
    gpioWrite(::shotFinishedPin1, shot);
    gpioWrite(::shotPin, shot);
    shoting = shot;
}

void GpioController::Impl::onPointerTriggered()
{
    pointer = !pointer;
    qDebug() << Q_FUNC_INFO << pointer;
    gpioWrite(::pointerPin, pointer);
    pointerP->publish(pointer);
}
