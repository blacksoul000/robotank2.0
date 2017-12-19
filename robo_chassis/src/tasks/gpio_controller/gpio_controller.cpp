#include "gpio_controller.h"
#include "MPU6050_6Axis_MotionApps20.h"

#include "pub_sub.h"
#include "influence.h"
#include "pointf3d.h"
#include "empty.h"

#include <pigpio.h>

#include <QTimer>
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

    const uint8_t gunVPin = 13;
    const uint8_t cameraVPin = 19;

    const int maxVerticalSpeed = 300; // pulse per sec
    const int tickInterval = 100;
    constexpr double tickCoef = tickInterval * maxVerticalSpeed / 1000.0 / SHRT_MAX;
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

    struct MpuData
    {
        // MPU control/status vars
        bool dmpReady = false;  // set true if DMP init was successful
        uint8_t devStatus;      // return status after each device operation (0 = success, !0 = error)
        uint16_t packetSize;    // expected DMP packet size (default is 42 bytes)
        uint16_t fifoCount;     // count of all bytes currently in FIFO
        uint8_t fifoBuffer[64]; // FIFO storage buffer

        // orientation/motion vars
        Quaternion q;           // [w, x, y, z]         quaternion container
        VectorInt16 aa;         // [x, y, z]            accel sensor measurements
        VectorInt16 aaReal;     // [x, y, z]            gravity-free accel sensor measurements
        VectorInt16 aaWorld;    // [x, y, z]            world-frame accel sensor measurements
        VectorFloat gravity;    // [x, y, z]            gravity vector
        float ypr[3];           // [yaw, pitch, roll]   yaw/pitch/roll container and gravity vector
    } mpuData;

    Publisher< bool >* shotStatusP = nullptr;
    Publisher< QPointF >* gunPositionP = nullptr;
    Publisher< QPointF >* cameraPositionP = nullptr;

    QScopedPointer< MPU6050 > mpu;

    bool shoting = false;
    bool shotClosing = false;

    double towerH = 0;
    double towerHOffset = 0;

    PointF3D chassisGyroData;
    QMap< uint8_t, ServoInfo > servo;

    void onShotStatusChanged(bool shot);
    bool initMpu();
};

GpioController::GpioController():
    ITask(),
    d(new Impl)
{
    d->shotStatusP = PubSub::instance()->advertise< bool >("core/shot");
    d->gunPositionP = PubSub::instance()->advertise< QPointF >("gun/position");
    d->cameraPositionP = PubSub::instance()->advertise< QPointF >("camera/position");
}

GpioController::~GpioController()
{
    gpioWrite(::shotFinishedPin1, 0);
    gpioTerminate();

    delete d->gunPositionP;
    delete d->cameraPositionP;
    delete d->shotStatusP;
    delete d;
}

void GpioController::start()
{
#ifdef ENABLE_GYRO
    d->initMpu();

    // supply your own gyro offsets here, scaled for min sensitivity
    d->mpu->setXGyroOffset(14);
    d->mpu->setYGyroOffset(-53);
    d->mpu->setZGyroOffset(25);
    d->mpu->setXAccelOffset(825);
    d->mpu->setYAccelOffset(304);
    d->mpu->setZAccelOffset(1254);
#endif //ENABLE_GYRO

    if (gpioInitialise() >= 0)
    {
#ifdef ENABLE_SERVO
        gpioSetTimerFuncEx(0, ::tickInterval, servoTickProxy, this);

        // Battery can't hold sufficient power and RPi is going down if we init both servos at the same time
        d->servo.insert(::gunVPin, {900, 1400, 1300, 0, 1300, 18});
        QTimer::singleShot(1000, [&](){
            d->servo.insert(::cameraVPin, {1600, 2300, 1900, 0, 1900, 9.08}); // ~376 pulses = camera field of view(41.41 deg)
        });
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
    // if programming failed, don't try to do anything
    if (!d->mpuData.dmpReady) return;

    // get current FIFO count
    d->mpuData.fifoCount = d->mpu->getFIFOCount();

    if (d->mpuData.fifoCount == 1024) 
    {
        // reset so we can continue cleanly
        d->mpu->resetFIFO();
        printf("FIFO overflow!\n");

    // otherwise, check for DMP data ready interrupt (this should happen frequently)
    } 
    else if (d->mpuData.fifoCount >= 42) 
    {
        // read a packet from FIFO
        d->mpu->getFIFOBytes(d->mpuData.fifoBuffer, d->mpuData.packetSize);

        // display Euler angles in degrees
        d->mpu->dmpGetQuaternion(&d->mpuData.q, d->mpuData.fifoBuffer);
        d->mpu->dmpGetGravity(&d->mpuData.gravity, &d->mpuData.q);
        d->mpu->dmpGetYawPitchRoll(d->mpuData.ypr, &d->mpuData.q, &d->mpuData.gravity);
        printf("ypr1  %7.2f %7.2f %7.2f    ", d->mpuData.ypr[0] * 180/M_PI, d->mpuData.ypr[1] * 180/M_PI, d->mpuData.ypr[2] * 180/M_PI);

        #ifdef OUTPUT_READABLE_REALACCEL
            // display real acceleration, adjusted to remove gravity
            d->mpu->dmpGetQuaternion(&d->mpuData.q, d->mpuData.fifoBuffer);
            d->mpu->dmpGetAccel(&d->mpuData.aa, d->mpuData.fifoBuffer);
            d->mpu->dmpGetGravity(&d->mpuData.gravity, &d->mpuData.q);
            d->mpu->dmpGetLinearAccel(&d->mpuData.aaReal, &d->mpuData.aa, &d->mpuData.gravity);
            printf("areal %6d %6d %6d    ", d->mpuData.aaReal.x, d->mpuData.aaReal.y, d->mpuData.aaReal.z);
        #endif

        #ifdef OUTPUT_READABLE_WORLDACCEL
            // display initial world-frame acceleration, adjusted to remove gravity
            // and rotated based on known orientation from quaternion
            d->mpu->dmpGetQuaternion(&d->mpuData.q, d->mpuData.fifoBuffer);
            d->mpu->dmpGetAccel(&d->mpuData.aa, d->mpuData.fifoBuffer);
            d->mpu->dmpGetGravity(&d->mpuData.gravity, &d->mpuData.q);
            d->mpu->dmpGetLinearAccelInWorld(&d->mpuData.aaWorld, &d->mpuData.aaReal, &d->mpuData.q);
            printf("aworld %6d %6d %6d    ", d->mpuData.aaWorld.x, d->mpuData.aaWorld.y, d->mpuData.aaWorld.z);
        #endif
        printf("\n");
    }

    d->towerH = d->mpuData.ypr[0] * 180/M_PI - d->chassisGyroData.x;
    if (d->towerH < 0) d->towerH += 360;
//    qDebug() << "=========================================" << d->towerH;
}

void GpioController::onJoyEvent(const quint16& joy)
{
    if (d->shoting) return;

    if (((joy >> 6) & 1 == 1) && ((joy >> 7) & 1 == 1)) // both triggers
    {
        d->onShotStatusChanged(true);
    }
}

void GpioController::onInfluence(const Influence& influence)
{
    d->servo[::gunVPin].tick = -std::ceil(influence.gunV * ::tickCoef);
    d->servo[::cameraVPin].tick = -std::ceil(influence.cameraV * ::tickCoef);
//    qDebug() << Q_FUNC_INFO << __LINE__ << influence.gunV << ::tickCoef << d->servo[::gunVPin].tick << d->servo[::gunVPin].pulse;
//    qDebug() << Q_FUNC_INFO << __LINE__ << influence.cameraV << ::tickCoef << d->servo[::cameraVPin].tick << d->servo[::cameraVPin].pulse;
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
    if (online) return;
    return;

//    digitalWrite (::resetPin, HIGH);
//    delay(50);
//    digitalWrite (::resetPin, LOW);
}

void GpioController::servoTickProxy(void* data)
{
    reinterpret_cast< GpioController* >(data)->servoTick();
}

void GpioController::servoTick()
{
//    auto it = d->servo.begin();
//    ++it;
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
    shotStatusP->publish(shot);
    gpioWrite(::shotFinishedPin1, shot);
    shoting = shot;
}

bool GpioController::Impl::initMpu()
{
    mpu.reset(new MPU6050);
    // initialize device
    printf("Initializing I2C devices...\n");
    mpu->initialize();

    // verify connection
    printf("Testing device connections...\n");
    printf(mpu->testConnection() ? "MPU6050 connection successful\n" : "MPU6050 connection failed\n");

    // load and configure the DMP
    printf("Initializing DMP...\n");
    mpuData.devStatus = mpu->dmpInitialize();

    // make sure it worked (returns 0 if so)
    if (mpuData.devStatus == 0) 
    {
        // turn on the DMP, now that it's ready
        printf("Enabling DMP...\n");
        mpu->setDMPEnabled(true);

        // set our DMP Ready flag so the main loop() function knows it's okay to use it
        printf("DMP ready!\n");
        mpuData.dmpReady = true;

        // get expected DMP packet size for later comparison
        mpuData.packetSize = mpu->dmpGetFIFOPacketSize();
        return true;
    } 
    else 
    {
        // ERROR!
        // 1 = initial memory load failed
        // 2 = DMP configuration updates failed
        // (if it's going to break, usually the code will be 1)
        printf("DMP Initialization failed (code %d)\n", mpuData.devStatus);
        return false;
    }
}