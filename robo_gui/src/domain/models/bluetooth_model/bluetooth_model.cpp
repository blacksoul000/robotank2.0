#include "bluetooth_model.h"

#include "chassis_packet.h"

using domain::BluetoothModel;

class BluetoothModel::Impl
{
public:
    bool isScanning = false;
    QVector< DeviceInfo > devices;
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

void BluetoothModel::setDevices(const QVector< DeviceInfo >& devices)
{
    d->devices = devices;
    emit devicesChanged(devices);
}

QVector< DeviceInfo > BluetoothModel::devices() const
{
    return d->devices;
}
