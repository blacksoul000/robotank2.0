#include "arduino_exchanger.h"

//msgs
#include "influence.h"
#include "empty.h"

#include "pub_sub.h"

#include "uart.h"
#include "i2c_master.h"

#include <QElapsedTimer>
#include <QTimer>
#include <QDebug>

namespace
{
#pragma pack(push, 1)
    struct ArduinoPkg
    {
        int16_t voltage = 0;
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
    };
#pragma pack(pop)

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

private:
    bool arduinoOnline = true;
};

ArduinoExchanger::ArduinoExchanger():
    ITask(),
    d(new Impl)
{
//    d->arduino = new Uart("/dev/ttyAMA0", sizeof(::ArduinoPkg), this);
    d->arduino = new I2CMaster("/dev/i2c-1", sizeof(::ArduinoPkg), this);
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
    if (!d->isArduinoOnline()) return;

    d->sendData();
}

void ArduinoExchanger::onInfluence(const Influence& influence)
{
    if (!d->isArduinoOnline()) return;

    d->package.leftEngine = influence.leftEngine;
    d->package.rightEngine = influence.rightEngine;
    d->package.towerH = influence.towerH;

    d->sendData();
}

void ArduinoExchanger::onJoyEvent(const quint16& joy)
{
    if (((joy >> 4) & 1) == 1) d->onLightTriggered(); // L1 button
}

void ArduinoExchanger::onNewData(const QByteArray& data)
{
    d->timer->start();
    d->setArduinoOnline(true);

    ::ArduinoPkg pkg = *reinterpret_cast<const ::ArduinoPkg *>(data.data());
    d->voltageP->publish(pkg.voltage);
}

void ArduinoExchanger::onPowerDown(const Empty&)
{
    if (!d->isArduinoOnline()) return;

    d->package.powerDown = 1;
    d->sendData();
}

void ArduinoExchanger::Impl::setArduinoOnline(bool online)
{
    if (online == arduinoOnline) return;
    qDebug() << (online ? "Arduino online" : "Arduino offline");
    arduinoOnline = online;
    arduinoStatusP->publish(online);
    if (arduinoOnline) this->sendData();
}

void ArduinoExchanger::Impl::sendData()
{
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
