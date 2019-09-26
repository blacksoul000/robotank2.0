#include "arduino_exchanger.h"

//msgs
#include "influence.h"
#include "pointf3d.h"
#include "joy_axes.h"
#include "empty.h"

#include "pub_sub.h"

#include "uart.h"
#include "i2c_slave.h"

#include <QElapsedTimer>
#include <QPointF>
#include <QTimer>
#include <QDebug>

namespace
{
#pragma pack(push, 1)
    struct ArduinoPkg
    {
      int16_t voltage = 0;
      int16_t currentLeft;
      int16_t currentRight;
      int16_t currentTower;
      float yaw = 0;
      float pitch = 0;
      float roll = 0;
      float towerH = 0;
      uint8_t chassisImuOnline : 1;
      uint8_t chassisImuReady : 1;
      uint8_t towerImuOnline : 1;
      uint8_t towerImuReady : 1;
      uint8_t reserve : 4;
      uint16_t crc = 0;
    };

    struct RaspberryPkg
    {
        RaspberryPkg(){}

        uint8_t powerDown: 1;
        uint8_t light: 1;
        uint8_t enginesPower : 1;
        uint8_t stab : 1;
        uint8_t tracking : 1;
        uint8_t reserve: 3;
        JoyAxes axes;
        float deviationX = 0.0;
        uint16_t crc = 0;
    };
#pragma pack(pop)

    const uint8_t slaveAddress = 0x03;
    const int timeout = 1500; // ms
} // namespace

class ArduinoExchanger::Impl
{
public:
    IExchanger* arduino = nullptr;
    QTimer* timer = nullptr;

    PointF3D yprOffsets = {0, 0, 0};
    float towerHOffset = 0;
    ArduinoPkg lastData;
    RaspberryPkg package;

    double towerH = 0;

    Publisher< bool >* arduinoStatusP = nullptr;
    Publisher< quint16 >* voltageP = nullptr;
    Publisher< bool >* headlightP = nullptr;
    Publisher< bool >* chassisOnlineP = nullptr;
    Publisher< bool >* chassisReadyP = nullptr;
    Publisher< bool >* towerOnlineP = nullptr;
    Publisher< bool >* towerReadyP = nullptr;
    Publisher< float >* gunHP = nullptr;
    Publisher< PointF3D >* yprP = nullptr;

    void sendData();
    void readData();
    void setArduinoOnline(bool set);
    bool isArduinoOnline() const;
    void onLightTriggered();

    bool isValid(const ArduinoPkg* pkg) const;
    uint16_t crc16(const unsigned char* data, unsigned short len) const;

private:
    bool arduinoOnline = true;
};

ArduinoExchanger::ArduinoExchanger():
    ITask(),
    d(new Impl)
{
    d->arduino = new I2CSlave(::slaveAddress, this);
    connect(d->arduino, &IExchanger::dataAvailable, this, &ArduinoExchanger::onNewData);

    d->timer = new QTimer(this);
    d->timer->setInterval(::timeout);
    connect(d->timer, &QTimer::timeout, this, [&](){ d->setArduinoOnline(false); });

    d->arduinoStatusP = PubSub::instance()->advertise< bool >("arduino/status");
    d->voltageP = PubSub::instance()->advertise< quint16 >("chassis/voltage");
    d->headlightP = PubSub::instance()->advertise< bool >("chassis/headlight");
    d->yprP = PubSub::instance()->advertise< PointF3D >("chassis/ypr");
    d->gunHP = PubSub::instance()->advertise< float >("gun/position/hirizontal");
    d->chassisOnlineP = PubSub::instance()->advertise< bool >("chassis/imu/online");
    d->chassisReadyP = PubSub::instance()->advertise< bool >("chassis/imu/ready");
    d->towerOnlineP = PubSub::instance()->advertise< bool >("tower/imu/online");
    d->towerReadyP = PubSub::instance()->advertise< bool >("tower/imu/ready");

    PubSub::instance()->subscribe("joy/axes", &ArduinoExchanger::onJoyEvent, this);
    PubSub::instance()->subscribe("tracker/status", &ArduinoExchanger::onTrackerStatusChanged, this);
    PubSub::instance()->subscribe("tracker/deviation", &ArduinoExchanger::onTrackerDeviation, this);
    PubSub::instance()->subscribe("joy/buttons", &ArduinoExchanger::onJoyButtonsEvent, this);
    PubSub::instance()->subscribe("core/powerDown", &ArduinoExchanger::onPowerDown, this);
    PubSub::instance()->subscribe("gun/calibrate", &ArduinoExchanger::onGunCalibrate, this);
    PubSub::instance()->subscribe("ypr/calibrate", &ArduinoExchanger::onGyroCalibrate, this);

    d->setArduinoOnline(false);
    d->package.powerDown = 0;
    d->package.light = 0;
    d->package.stab = 0;
    d->package.tracking = 0;
    d->package.enginesPower = 1;  // TODO - from GUI or gamepad button
}

ArduinoExchanger::~ArduinoExchanger()
{
    if (d->arduino->isOpen()) d->arduino->close();
    delete d->arduinoStatusP;
    delete d->voltageP;
    delete d->headlightP;
    delete d->yprP;
    delete d->gunHP;
    delete d->chassisOnlineP;
    delete d->chassisReadyP;
    delete d->towerOnlineP;
    delete d->towerReadyP;
    delete d;
}

void ArduinoExchanger::execute()
{
    if (!d->arduino->isOpen() && !d->arduino->open()) return;
    d->sendData();
}

void ArduinoExchanger::onJoyButtonsEvent(const quint16& buttons)
{
    if (((buttons >> 4) & 1) == 1) d->onLightTriggered(); // L1 button
}

void ArduinoExchanger::onNewData(const QByteArray& data)
{
    d->timer->start();
    d->setArduinoOnline(true);

    if (data.size() < sizeof(::ArduinoPkg)) return;

    const ::ArduinoPkg* pkg = reinterpret_cast<const ::ArduinoPkg *>(data.data());
//    qDebug() << Q_FUNC_INFO << data.toHex() << d->isValid(pkg);
    if (!d->isValid(pkg)) return;

    qDebug() << Q_FUNC_INFO << pkg->voltage;
    d->voltageP->publish(pkg->voltage);

    const PointF3D ypr = {pkg->yaw, pkg->pitch, pkg->roll};
    d->yprP->publish(ypr - d->yprOffsets);

    d->gunHP->publish(pkg->towerH - d->towerHOffset);

    d->chassisOnlineP->publish(pkg->chassisImuOnline);
    d->chassisReadyP->publish(pkg->chassisImuReady);
    d->towerOnlineP->publish(pkg->towerImuOnline);
    d->towerReadyP->publish(pkg->towerImuReady);
//    qDebug() << Q_FUNC_INFO << pkg->currentLeft << pkg->currentRight << pkg->currentTower;
//    qDebug() << Q_FUNC_INFO << pkg->yaw << pkg->pitch << pkg->roll << pkg->towerH << pkg->chassisImuOnline << pkg->chassisImuReady << pkg->towerImuOnline << pkg->towerImuReady;
}

void ArduinoExchanger::onPowerDown(const Empty&)
{
    d->package.powerDown = 1;
}

void ArduinoExchanger::onGunCalibrate(const Empty&)
{
    d->towerHOffset = d->lastData.towerH;
}

void ArduinoExchanger::onJoyEvent(const JoyAxes& joy)
{
//    qDebug() << Q_FUNC_INFO << joy.x1 << joy.y1 << joy.x2 << joy.y2;
    d->package.axes = joy;
}

void ArduinoExchanger::onGyroCalibrate(const Empty&)
{
    d->yprOffsets = {d->lastData.yaw, d->lastData.pitch, d->lastData.roll};
}

void ArduinoExchanger::onTrackerDeviation(const QPointF& deviation)
{
    d->package.deviationX = deviation.x();
}

void ArduinoExchanger::onTrackerStatusChanged(const bool& status)
{
    d->package.tracking = status;
    d->package.deviationX = 0;
}

void ArduinoExchanger::Impl::setArduinoOnline(bool online)
{
    if (online == arduinoOnline) return;
    qDebug() << (online ? "Arduino online" : "Arduino offline");
    arduinoOnline = online;
    arduinoStatusP->publish(online);
}

void ArduinoExchanger::Impl::sendData()
{
    package.crc = this->crc16(reinterpret_cast< unsigned char* >(&package), sizeof(RaspberryPkg) - sizeof(package.crc));
    if (!arduino->sendData(QByteArray(reinterpret_cast<const char *>(&package), sizeof(package))))
    {
        qDebug() << Q_FUNC_INFO << "Failed to write to the bus.";
    }
}

bool ArduinoExchanger::Impl::isArduinoOnline() const
{
    return arduinoOnline;
}

void ArduinoExchanger::Impl::onLightTriggered()
{
    package.light = !package.light;
    headlightP->publish(package.light);
}

uint16_t ArduinoExchanger::Impl::crc16(const unsigned char* data, unsigned short len) const
{
    unsigned short crc = 0xFFFF;
    unsigned char i;

    while (len--)
    {
        crc ^= *data++ << 8;

        for (i = 0; i < 8; i++)
            crc = crc & 0x8000 ? (crc << 1) ^ 0x1021 : crc << 1;
    }
    return crc;
}

bool ArduinoExchanger::Impl::isValid(const ArduinoPkg* pkg) const
{
    return pkg->crc == crc16(reinterpret_cast< const unsigned char* > (pkg), sizeof(ArduinoPkg) - sizeof(pkg->crc));
}
