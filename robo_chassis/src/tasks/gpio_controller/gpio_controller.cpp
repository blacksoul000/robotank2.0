#include "gpio_controller.h"

#include "pub_sub.h"
#include "influence.h"
#include "pointf3d.h"
#include "empty.h"

#include "mpu6050_raw.h"
#include "mpu6050_dmp.h"

#include <pigpio.h>

#include <QtMath>
#include <QPoint>
#include <QDebug>

#define ENABLE_SERVO
#define ENABLE_GYRO

namespace
{
    const uint8_t shotFinishedPin1 = 20;
    const uint8_t shotFinishedPin2 = 21;

    const uint8_t resetPin = 4;
    const uint8_t pointerPin = 26;

    const uint8_t gunVPin = 19;
    const uint8_t cameraVPin = 13;

    const int tickInterval = 100;
    constexpr double gunTickCoef = 60.0 / SHRT_MAX;
    constexpr double cameraTickCoef = 20.0 / SHRT_MAX;
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
        int8_t tick;
        uint16_t zeroLift;
        float pulsePerDegree;
    };

    Publisher< bool >* shotStatusP = nullptr;
    Publisher< QPointF >* gunPositionP = nullptr;
    Publisher< QPointF >* cameraPositionP = nullptr;
    Publisher< bool >* pointerP = nullptr;

    QScopedPointer< IImu > imu;

    bool shoting = false;
    bool shotClosing = false;
    bool pointer = false;

    double towerH = 0;
    double towerHOffset = 0;

    PointF3D chassisGyroData;
    QMap< uint8_t, ServoInfo > servo;

    void onShotStatusChanged(bool shot);
    bool initMpu();
    void onPointerTriggered();
};

GpioController::GpioController():
    ITask(),
    d(new Impl)
{
    d->shotStatusP = PubSub::instance()->advertise< bool >("core/shot");
    d->gunPositionP = PubSub::instance()->advertise< QPointF >("gun/position");
    d->cameraPositionP = PubSub::instance()->advertise< QPointF >("camera/position");
    d->pointerP = PubSub::instance()->advertise< bool >("chassis/pointer");
}

GpioController::~GpioController()
{
    gpioWrite(::shotFinishedPin1, 0);
    gpioTerminate();

    delete d->gunPositionP;
    delete d->cameraPositionP;
    delete d->shotStatusP;
    delete d->pointerP;
    delete d;
}

void GpioController::start()
{
#ifdef ENABLE_GYRO
    d->imu.reset(new Mpu6050Dmp);

    if (!d->imu->init())
    {
    	qWarning() << Q_FUNC_INFO << "Failed to init IMU";
    }
#endif //ENABLE_GYRO

    if (gpioInitialise() >= 0)
    {
#ifdef ENABLE_SERVO
        gpioSetTimerFuncEx(0, ::tickInterval, servoTickProxy, this);

        d->servo.insert(::gunVPin, {900, 1400, 1300, 0, 1300, 18});
        d->servo.insert(::cameraVPin, {1820, 1990, 1930, 0, 1930, 9.08}); // ~376 pulses = camera field of view(41.41 deg)
#endif //ENABLE_SERVO

        gpioSetMode(::shotFinishedPin1, PI_OUTPUT);
        gpioSetMode(::shotFinishedPin2, PI_INPUT);
    }
    else
    {
        qWarning() << "Failed to initialize GPIO";
    }

    d->onShotStatusChanged(false);

    PubSub::instance()->subscribe("joy/buttons", &GpioController::onJoyEvent, this);
    PubSub::instance()->subscribe("arduino/status", &GpioController::onArduinoStatusChanged, this);
    PubSub::instance()->subscribe("core/influence", &GpioController::onInfluence, this);
    PubSub::instance()->subscribe("core/deviationV", &GpioController::onDeviation, this);
    PubSub::instance()->subscribe("gun/calibrate", &GpioController::onGunCalibrate, this);
    PubSub::instance()->subscribe("camera/calibrate", &GpioController::onCameraCalibrate, this);
    PubSub::instance()->subscribe("ypr/calibrate", &GpioController::onGyroCalibrate, this);
    PubSub::instance()->subscribe("chassis/ypr", &GpioController::onChassisGyro, this);
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

#ifdef ENABLE_GYRO
    this->readGyroData();
#endif //ENABLE_GYRO

    d->gunPositionP->publish(QPointF(d->towerH - d->towerHOffset, 
                                -1.0 * (d->servo[::gunVPin].pulse - d->servo[::gunVPin].zeroLift) 
                                        / d->servo[::gunVPin].pulsePerDegree));
    d->cameraPositionP->publish(QPointF(0, (d->servo[::cameraVPin].pulse - d->servo[::cameraVPin].zeroLift) 
                                        / d->servo[::cameraVPin].pulsePerDegree));
}

void GpioController::readGyroData()
{
    d->imu->readData();
//    qDebug() << Q_FUNC_INFO << d->imu->pitch() << d->imu->roll() << d->imu->yaw();
    d->towerH = d->imu->yaw() - d->chassisGyroData.x;
//    qDebug() << "=========================================" << d->towerH;
}

void GpioController::onJoyEvent(const quint16& joy)
{
    if ((((joy >> 6) & 1) == 1) && (((joy >> 7) & 1) == 1)) // both triggers
    {
        d->onShotStatusChanged(!d->shoting);
    }
    if (((joy >> 5) & 1) == 1) d->onPointerTriggered();
}

void GpioController::onInfluence(const Influence& influence)
{
    d->servo[::gunVPin].tick = -std::ceil(influence.gunV * ::gunTickCoef);
    d->servo[::cameraVPin].tick = -std::ceil(influence.cameraV * ::cameraTickCoef);
//    qDebug() << Q_FUNC_INFO << __LINE__ << influence.gunV << ::tickCoef << d->servo[::gunVPin].tick << d->servo[::gunVPin].pulse;
//    qDebug() << Q_FUNC_INFO << __LINE__ << influence.cameraV << ::cameraTickCoef << d->servo[::cameraVPin].tick << d->servo[::cameraVPin].pulse;
}

void GpioController::onDeviation(const QPointF& point)
{
//    qDebug() << Q_FUNC_INFO << __LINE__ << point
//             << (d->servo[::cameraVPin].pulsePerDegree * point.y())
//             << d->servo[::cameraVPin].pulse;

    d->servo[::gunVPin].tick = 0;
    d->servo[::cameraVPin].tick = 0;

//    d->servo[::gunVPin].pulse += d->servo[::gunVPin].pulsePerDegree * point.x();
    d->servo[::cameraVPin].pulse += 1.0 * point.y();
}

void GpioController::onGunCalibrate(const Empty&)
{
    d->servo[::gunVPin].zeroLift = d->servo[::gunVPin].pulse;
}

void GpioController::onCameraCalibrate(const Empty&)
{
    d->servo[::cameraVPin].zeroLift = d->servo[::cameraVPin].pulse;
}

void GpioController::onGyroCalibrate(const Empty&)
{
    d->towerHOffset = d->towerH;
}

void GpioController::onChassisGyro(const PointF3D& ypr)
{
    d->chassisGyroData = ypr;
//    printf("ypr2  %7.2f %7.2f %7.2f\n", ypr.x, ypr.y, ypr.z);
}

void GpioController::onArduinoStatusChanged(const bool& online)
{
    Q_UNUSED(online)
    return;
}

void GpioController::servoTickProxy(void* data)
{
    reinterpret_cast< GpioController* >(data)->servoTick();
}

void GpioController::servoTick()
{
    for (auto it = d->servo.begin(), end = d->servo.end(); it != end; ++it)
    {
        it.value().pulse = qBound< uint16_t >(it.value().minPulse, it.value().pulse + it.value().tick, it.value().maxPulse);
//        qDebug() << it.key() << it.value().pulse << it.value().minPulse << it.value().maxPulse;
        gpioServo(it.key(), it.value().pulse);
    }
}

//------------------------------------------------------------------------------------
void GpioController::Impl::onShotStatusChanged(bool shot)
{
    qDebug() << Q_FUNC_INFO << shot;
    gpioWrite(::shotFinishedPin1, shot);
    shotStatusP->publish(shot);
    shoting = shot;
}

void GpioController::Impl::onPointerTriggered()
{
    pointer = !pointer;
    qDebug() << Q_FUNC_INFO << pointer;
    gpioWrite(::pointerPin, pointer);
    pointerP->publish(pointer);
}
