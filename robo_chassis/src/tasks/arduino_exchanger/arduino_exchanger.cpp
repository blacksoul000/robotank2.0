#include "arduino_exchanger.h"

//msgs
#include "influence.h"
#include "empty.h"

#include "pub_sub.h"

#include "uart.h"
#include "i2c_slave.h"

#include <QElapsedTimer>
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
        uint16_t crc = 0;
    };

    struct RaspberryPkg
    {
        RaspberryPkg(){}

        uint8_t powerDown: 1;
        uint8_t light: 1;
        uint8_t reserve: 6;
        int16_t leftEngine = 0;
        int16_t rightEngine = 0;
        int16_t towerH = 0;
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

    ArduinoPkg offsets;
    ArduinoPkg lastData;
    RaspberryPkg package;

    Publisher< bool >* arduinoStatusP = nullptr;
    Publisher< quint16 >* voltageP = nullptr;
    Publisher< bool >* headlightP = nullptr;

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

    PubSub::instance()->subscribe("core/influence", &ArduinoExchanger::onInfluence, this);
    PubSub::instance()->subscribe("joy/buttons", &ArduinoExchanger::onJoyEvent, this);
    PubSub::instance()->subscribe("core/powerDown", &ArduinoExchanger::onPowerDown, this);

    d->setArduinoOnline(false);
    d->package.powerDown = 0;
    d->package.light = 0;
}

ArduinoExchanger::~ArduinoExchanger()
{
    if (d->arduino->isOpen()) d->arduino->close();
    delete d->arduinoStatusP;
    delete d->voltageP;
    delete d->headlightP;
    delete d;
}

void ArduinoExchanger::execute()
{
    if (!d->arduino->isOpen() && !d->arduino->open()) return;
    d->sendData();
}

void ArduinoExchanger::onInfluence(const Influence& influence)
{
    d->package.leftEngine = influence.leftEngine;
    d->package.rightEngine = influence.rightEngine;
    d->package.towerH = influence.towerH;
}

void ArduinoExchanger::onJoyEvent(const quint16& joy)
{
    if (((joy >> 4) & 1) == 1) d->onLightTriggered(); // L1 button
}

void ArduinoExchanger::onNewData(const QByteArray& data)
{
    d->timer->start();
    d->setArduinoOnline(true);

    const ::ArduinoPkg* pkg = reinterpret_cast<const ::ArduinoPkg *>(data.data());
    if (!d->isValid(pkg)) return;
    d->voltageP->publish(pkg->voltage);
//    qDebug() << Q_FUNC_INFO << pkg->currentLeft << pkg->currentRight << pkg->currentTower;
}

void ArduinoExchanger::onPowerDown(const Empty&)
{
    d->package.powerDown = 1;
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
    if (arduinoOnline) this->sendData();
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
