#include "arduino_exchanger.h"

//msgs
#include "influence.h"
#include "point3d.h"
#include "empty.h"

#include "pub_sub.h"

#include <QPoint>
#include <QSerialPort>
#include <QTimer>
#include <QDebug>

namespace
{
#pragma pack(push, 1)
    struct ArduinoPkg
    {
        int16_t gunH = 0;
        int16_t gunV = 0;
        int16_t cameraV = 0;
        int16_t yaw = 0;
        int16_t pitch = 0;
        int16_t roll = 0;
    };
    struct RaspberryPkg
    {
        RaspberryPkg(){}
        RaspberryPkg(const Influence& influence)
        {
            angleType = influence.angleType;
            shot = influence.shot;
            gunV = influence.gunV;
            cameraV = influence.cameraV;
            leftEngine = influence.leftEngine;
            rightEngine = influence.rightEngine;
            towerH = influence.towerH;
        }

        uint8_t angleType: 3;
        uint8_t shot: 1;
        uint8_t reserve: 4;
        int16_t gunV = 0;
        int16_t cameraV = 0;
        int16_t leftEngine = 0;
        int16_t rightEngine = 0;
        int16_t towerH = 0;
    };
#pragma pack(pop)

    const int timeout = 500; // ms
} // namespace

class ArduinoExchanger::Impl
{
public:
    QSerialPort* serial = nullptr;
    QTimer* timer = nullptr;

    QByteArray buffer;
    QByteArray prefix = QByteArray(2, 0x55);
    bool waitPrefix = true;

    ArduinoPkg offsets;
    ArduinoPkg lastData;

    Publisher< QPoint >* gunPositionP = nullptr;
    Publisher< QPoint >* cameraPositionP = nullptr;
    Publisher< Point3D >* yprP = nullptr;
    Publisher< bool >* arduinoStatusP = nullptr;

    void readData();
    void setArduinoOnline(bool set);

private:
    bool arduinoOnline = true;
};

ArduinoExchanger::ArduinoExchanger():
    ITask(),
    d(new Impl)
{
    d->serial = new QSerialPort("/dev/ttyAMA0", this);
    d->serial->setBaudRate(115200);
    d->serial->setDataBits(QSerialPort::Data8);
    d->serial->setParity(QSerialPort::NoParity);
    d->serial->setStopBits(QSerialPort::OneStop);
    connect(d->serial, &QSerialPort::readyRead, [&](){ d->readData(); });

    d->timer = new QTimer(this);
    d->timer->setInterval(::timeout);
    connect(d->timer, &QTimer::timeout, [&](){ d->setArduinoOnline(false); });

    d->gunPositionP = PubSub::instance()->advertise< QPoint >("gun/position");
    d->cameraPositionP = PubSub::instance()->advertise< QPoint >("camera/position");
    d->yprP = PubSub::instance()->advertise< Point3D >("robo/ypr");
    d->arduinoStatusP = PubSub::instance()->advertise< bool >("arduino/status");

    PubSub::instance()->subscribe("core/influence", &ArduinoExchanger::onInfluence, this);
    PubSub::instance()->subscribe("gun/calibrate", &ArduinoExchanger::onGunCalibrate, this);
    PubSub::instance()->subscribe("camera/calibrate", &ArduinoExchanger::onCameraCalibrate, this);
    PubSub::instance()->subscribe("ypr/calibrate", &ArduinoExchanger::onGyroCalibrate, this);

    d->setArduinoOnline(false);
}

ArduinoExchanger::~ArduinoExchanger()
{
    if (d->serial->isOpen()) d->serial->close();
    delete d->gunPositionP;
    delete d->cameraPositionP;
    delete d->yprP;
    delete d->arduinoStatusP;
    delete d;
}

void ArduinoExchanger::execute()
{
    if (!d->serial->isOpen()) d->serial->open(QIODevice::ReadWrite);
}

void ArduinoExchanger::start()
{
    d->serial->open(QIODevice::ReadWrite);
}

void ArduinoExchanger::onInfluence(const Influence& influence)
{
    if (!d->serial->isOpen()) return;

    ::RaspberryPkg pkg(influence);

    QByteArray data(d->prefix);
    data.append(QByteArray(reinterpret_cast<const char *>(&pkg), sizeof(pkg)));

    if (d->serial->write(data) != data.size())
    {
        qDebug() << Q_FUNC_INFO << "Failed to write to the bus.";
    }
}

void ArduinoExchanger::onGunCalibrate(const Empty&)
{
    d->offsets.gunH = d->lastData.gunH;
    d->offsets.gunV = d->lastData.gunV;
}

void ArduinoExchanger::onCameraCalibrate(const Empty &)
{
    d->offsets.cameraV = d->lastData.cameraV;
}

void ArduinoExchanger::onGyroCalibrate(const Empty&)
{
    d->offsets.yaw = d->lastData.yaw;
    d->offsets.pitch = d->lastData.pitch;
    d->offsets.roll = d->lastData.roll;
}

//------------------------------------------------------------------------------------

void ArduinoExchanger::Impl::readData()
{
    QByteArray data = serial->readAll();
    if (serial->error() != QSerialPort::NoError) return;

    buffer.append(data);
    if (waitPrefix)
    {
        if (buffer.size() < prefix.size()) return;

        auto idx = buffer.indexOf(prefix);
        if (idx == -1)
        {
            buffer.remove(0, buffer.size() - prefix.size());
            return;
        }

        buffer.remove(0, idx);
        waitPrefix = false;
    }

    if (!waitPrefix)
    {
        const int packetSize = prefix.size() + sizeof(::ArduinoPkg);
        if (buffer.size() < packetSize) return;

        ::ArduinoPkg pkg = *reinterpret_cast<::ArduinoPkg *>(
                    buffer.mid(prefix.size(), sizeof(::ArduinoPkg)).data());
        buffer.remove(0, packetSize);
        waitPrefix = true;

        gunPositionP->publish(QPoint(pkg.gunH - offsets.gunH, pkg.gunV - offsets.gunV));
        cameraPositionP->publish(QPoint(0, pkg.cameraV - offsets.cameraV));
        yprP->publish(Point3D({pkg.yaw - offsets.yaw,
                               pkg.pitch - offsets.pitch,
                               pkg.roll - offsets.roll}));

//        qDebug() << Q_FUNC_INFO << pkg.yaw << pkg.pitch << pkg.roll << data.toHex();

        memcpy(&lastData, &pkg, sizeof(ArduinoPkg));

        timer->start();
        this->setArduinoOnline(true);
    }

}

void ArduinoExchanger::Impl::setArduinoOnline(bool online)
{
    if (online == arduinoOnline) return;
    qDebug() << (online ? "Arduino online" : "Arduino offline");
    arduinoOnline = online;
    arduinoStatusP->publish(online);
}
