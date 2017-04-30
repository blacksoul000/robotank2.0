#include "bluetooth_device.h"

using presentation::BluetoothDevice;

class BluetoothDevice::Impl
{
public:
    QString name;
    QString address;
    bool isPaired;
};

BluetoothDevice::BluetoothDevice(QObject* parent) :
    QObject(parent),
    d(new Impl)
{}

BluetoothDevice::~BluetoothDevice()
{
    delete d;
}

QString BluetoothDevice::name() const
{
    return d->name;
}
QString BluetoothDevice::address() const
{
    return d->address;
}

bool BluetoothDevice::isPaired() const
{
    return d->isPaired;
}

void BluetoothDevice::setName(const QString& name)
{
    if (d->name == name)
        return;

    d->name = name;
    emit nameChanged(name);
}
void BluetoothDevice::setAddress(const QString& address)
{
    if (d->address == address)
        return;

    d->address = address;
    emit addressChanged(address);
}

void BluetoothDevice::setIsPaired(bool isPaired)
{
    if (d->isPaired == isPaired)
        return;

    d->isPaired = isPaired;
    emit isPairedChanged(isPaired);
}
