#include "bluetooth_presenter.h"

#include "robo_model.h"
#include "bluetooth_model.h"
#include "bluetooth_device.h"
#include "bluetooth_device_info.h"

#include <QtQml>
#include <QTimer>
#include <QBluetoothAddress>
#include <QDebug>

using presentation::BluetoothPresenter;
using presentation::BluetoothDevice;
using domain::BluetoothModel;

namespace
{
    const int statusInterval = 1000;
}

class BluetoothPresenter::Impl
{
public:
    BluetoothModel* model = nullptr;
    QTimer* statusTimer = nullptr;

    QList< BluetoothDevice* > devices;
};

BluetoothPresenter::BluetoothPresenter(domain::RoboModel* model, QObject* parent) :
    QObject(parent),
    d(new Impl)
{
    qmlRegisterUncreatableType< BluetoothDevice >("Robotank", 1, 0, "BluetoothDevice", "");

    d->model = model->bluetooth();
    d->statusTimer = new QTimer(this);
    d->statusTimer->setInterval(::statusInterval);

    connect(d->model, &BluetoothModel::scanStatusChanged,
            this, &BluetoothPresenter::scanStatusChanged);
    connect(d->model, &BluetoothModel::devicesChanged,
            this, &BluetoothPresenter::onDevicesChanged);
    connect(d->statusTimer, &QTimer::timeout, this, &BluetoothPresenter::requestStatus);

    this->onDevicesChanged(d->model->devices());
}

BluetoothPresenter::~BluetoothPresenter()
{
    delete d;
}

bool BluetoothPresenter::scanStatus() const
{
    return d->model->scanStatus();
}

QList< QObject* > BluetoothPresenter::devices() const
{
    QList< QObject* > devices;
    for (auto device: d->devices)
    {
        devices.append(device);
    }
    return devices;
}

void BluetoothPresenter::start()
{
    d->statusTimer->start();
}

void BluetoothPresenter::stop()
{
    d->statusTimer->stop();
}

void BluetoothPresenter::onDevicesChanged(const QVector< BluetoothDeviceInfo >& devices)
{
    const bool countChanged = (d->devices.count() != devices.count());
    qDebug() << Q_FUNC_INFO << devices.count() << d->devices.count() << countChanged;

    while(d->devices.count() > devices.count()) delete d->devices.takeLast();
    while(d->devices.count() < devices.count()) d->devices.append(new BluetoothDevice(this));

    for (int i = 0; i < devices.count(); ++i)
    {
        BluetoothDevice* dst = d->devices[i];
        const BluetoothDeviceInfo& src = devices[i];

        dst->setAddress(QBluetoothAddress(src.address).toString());
        dst->setName(src.name);
        dst->setIsPaired(src.isPaired);
    }
    if (countChanged)
    {
        emit deviceCountChanged(devices.count());
    }
}

void BluetoothPresenter::requestStatus()
{
    d->model->requestStatus();
}

void BluetoothPresenter::requestScan()
{
    d->model->requestScan();
}

void BluetoothPresenter::requestPair(const QString& address, bool paired)
{
    d->model->requestPair(QBluetoothAddress(address).toUInt64(), paired);
}
