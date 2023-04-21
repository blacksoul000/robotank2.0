#include "bluetooth_model.h"

#include <QMutex>
#include <QMutexLocker>
#include <QDebug>

using domain::BluetoothModel;

class BluetoothModel::Impl
{
public:
    bool isScanning = false;
    bool isPairing = false;
    QVector< BluetoothDeviceInfo > devices;
    QMutex mutex;
};

BluetoothModel::BluetoothModel(QObject* parent) :
    QObject(parent),
    d(new Impl)
{
    qRegisterMetaType< QVector< BluetoothDeviceInfo > >("QVector<BluetoothDeviceInfo>");
}

BluetoothModel::~BluetoothModel()
{
    delete d;
}

void BluetoothModel::setScanStatus(bool isScanning)
{
    QMutexLocker locker(&d->mutex);
    if (d->isScanning == isScanning) return;

    d->isScanning = isScanning;
    locker.unlock();

    emit scanStatusChanged(isScanning);
}

bool BluetoothModel::scanStatus() const
{
    QMutexLocker locker(&d->mutex);
    return d->isScanning;
}

void BluetoothModel::setPairStatus(bool isPairing)
{
    QMutexLocker locker(&d->mutex);
    if (d->isPairing == isPairing) return;

    d->isPairing = isPairing;
    locker.unlock();

    emit pairStatusChanged(isPairing);
}

bool BluetoothModel::pairStatus() const
{
    QMutexLocker locker(&d->mutex);
    return d->isPairing;
}

void BluetoothModel::setDevices(const QVector< BluetoothDeviceInfo >& devices)
{
    QMutexLocker locker(&d->mutex);
    d->devices = devices;
    locker.unlock();

    emit devicesChanged(d->devices);
}

QVector< BluetoothDeviceInfo > BluetoothModel::devices() const
{
    QMutexLocker locker(&d->mutex);
    return d->devices;
}
