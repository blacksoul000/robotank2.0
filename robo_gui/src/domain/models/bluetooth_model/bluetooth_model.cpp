#include "bluetooth_model.h"

#include "bluetooth_device_info.h"

#include <QDebug>

using domain::BluetoothModel;

class BluetoothModel::Impl
{
public:
    bool isScanning = false;
    bool isPairing = false;
    QVector< BluetoothDeviceInfo > devices;
};

BluetoothModel::BluetoothModel(QObject* parent) :
    QObject(parent),
    d(new Impl)
{
}

BluetoothModel::~BluetoothModel()
{
    delete d;
}

void BluetoothModel::setScanStatus(bool isScanning)
{
    if (d->isScanning == isScanning) return;

    d->isScanning = isScanning;
    emit scanStatusChanged(isScanning);
}

bool BluetoothModel::scanStatus() const
{
    return d->isScanning;
}

void BluetoothModel::setPairStatus(bool isPairing)
{
    if (d->isPairing == isPairing) return;

    d->isPairing = isPairing;
    emit pairStatusChanged(isPairing);
}

bool BluetoothModel::pairStatus() const
{
    return d->isPairing;
}

void BluetoothModel::setDevices(const QVector< BluetoothDeviceInfo >& devices)
{
    d->devices = devices;
    emit devicesChanged(devices);
}

QVector< BluetoothDeviceInfo > BluetoothModel::devices() const
{
    return d->devices;
}
