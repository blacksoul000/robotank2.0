#include "gpio_controller.h"

#include "pub_sub.h"
#include "influence.h"
#include "pointf3d.h"
#include "empty.h"

#include "mpu6050_raw.h"
#include "mpu6050_dmp.h"

#include <pigpio.h>

#include <QPoint>
#include <QDebug>

#define ENABLE_SERVO
#define ENABLE_GYRO

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

    struct ImuData
    {
    	~ImuData()
    	{
    		delete onlineP;
    		delete readyP;
    	}

		QScopedPointer< IImu > imu;
		Publisher< bool >* onlineP = nullptr;
		Publisher< bool >* readyP = nullptr;
		bool ready = false;
		double yawOffset = 0;
    };

    Publisher< QPointF >* gunPositionP = nullptr;
    Publisher< bool >* pointerP = nullptr;
    Publisher< PointF3D >* yprP = nullptr;

    ImuData chassis;
    ImuData tower;

    bool shoting = false;
    bool shotClosing = false;
    bool pointer = false;

    double towerH = 0;

    PointF3D chassisGyroData;
    QMap< uint8_t, ServoInfo > servo;

    void onShotStatusChanged(bool shot);
    void onPointerTriggered();
};

GpioController::GpioController():
    ITask(),
    d(new Impl)
{
    d->gunPositionP = PubSub::instance()->advertise< QPointF >("gun/position");
    d->pointerP = PubSub::instance()->advertise< bool >("chassis/pointer");
    d->yprP = PubSub::instance()->advertise< PointF3D >("chassis/ypr");
    d->chassis.onlineP = PubSub::instance()->advertise< bool >("chassis/gyro/online");
    d->chassis.readyP = PubSub::instance()->advertise< bool >("chassis/gyro/ready");
    d->tower.onlineP = PubSub::instance()->advertise< bool >("tower/gyro/online");
    d->tower.readyP = PubSub::instance()->advertise< bool >("tower/gyro/ready");
}

GpioController::~GpioController()
{
    gpioWrite(::shotFinishedPin1, 0);
    gpioTerminate();

    delete d->gunPositionP;
    delete d->pointerP;
    delete d->yprP;
    delete d;
}

void GpioController::start()
{
#ifdef ENABLE_GYRO
    d->tower.imu.reset(new Mpu6050Dmp(0x69, 14, -53, 25, 825, 304, 1254));
    d->chassis.imu.reset(new Mpu6050Dmp(0x68, 68, -17, -31, -2364, -983, 987));

    for (auto& imuData: {&d->chassis, &d->tower})
    {
		if (imuData->imu->init())
		{
			imuData->onlineP->publish(true);
		}
		else
		{
			qWarning() << Q_FUNC_INFO << "Failed to init IMU";
		}
    }

#endif //ENABLE_GYRO

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
    PubSub::instance()->subscribe("arduino/status", &GpioController::onArduinoStatusChanged, this);
    PubSub::instance()->subscribe("core/influence", &GpioController::onInfluence, this);
    PubSub::instance()->subscribe("core/deviationV", &GpioController::onDeviation, this);
    PubSub::instance()->subscribe("gun/calibrate", &GpioController::onGunCalibrate, this);
    PubSub::instance()->subscribe("ypr/calibrate", &GpioController::onGyroCalibrate, this);
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

    const float towerH = d->chassis.imu->isReady() && d->tower.imu->isReady()
    				? d->towerH - d->tower.yawOffset : 0;
    const auto gunPosition = QPointF(towerH,
                                ((d->servo[::gunVPin].maxPulse - d->servo[::gunVPin].realPulse) / d->servo[::gunVPin].pulsePerDegree));
//    qDebug() << Q_FUNC_INFO << gunPosition.y() << d->servo[::gunVPin].realPulse << d->servo[::gunVPin].maxPulse;
    d->gunPositionP->publish(gunPosition);
}

void GpioController::readGyroData()
{
    for (auto& imuData: {&d->chassis, &d->tower})
    {
    	imuData->imu->readData();

    	if (imuData->imu->isReady() != imuData->ready)
		{
    		imuData->ready = imuData->imu->isReady();
			imuData->readyP->publish(imuData->ready);
			imuData->yawOffset = imuData->imu->yaw();
		}
    }

//    qDebug() << d->tower.imu->yaw() << d->tower.imu->pitch() << d->tower.imu->roll();

    // imu is rotated. pitch and roll swapped.
    if (d->chassis.ready)
    {
    	d->yprP->publish(PointF3D({d->chassis.imu->yaw() - d->chassis.yawOffset,
    							   d->chassis.imu->roll(),
								   d->chassis.imu->pitch()}));
    }
    d->towerH = d->tower.imu->yaw() - d->chassis.imu->yaw();
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
    d->tower.yawOffset = d->towerH;
}

void GpioController::onGyroCalibrate(const Empty&)
{
    d->chassis.yawOffset = d->chassis.imu->yaw();
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
