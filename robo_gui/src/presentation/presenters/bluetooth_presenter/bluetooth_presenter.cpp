#include "bluetooth_presenter.h"

#include "robo_model.h"
#include "bluetooth_model.h"
#include "bluetooth_device.h"
#include "chassis_packet.h"

#include <QtQml>
#include <QDebug>

using presentation::BluetoothPresenter;
using presentation::BluetoothDevice;
using domain::BluetoothModel;

class BluetoothPresenter::Impl
{
public:
    BluetoothModel* model = nullptr;

    QList< BluetoothDevice* > devices;
};

BluetoothPresenter::BluetoothPresenter(domain::RoboModel* model, QObject* parent) :
    QObject(parent),
    d(new Impl)
{
    qmlRegisterUncreatableType< BluetoothDevice >("Robotank", 1, 0, "BluetoothDevice", "");

    d->model = model->bluetooth();

    connect(d->model, &BluetoothModel::scanStatusChanged,
            this, &BluetoothPresenter::scanStatusChanged);
    connect(d->model, &BluetoothModel::devicesChanged,
            this, &BluetoothPresenter::onDevicesChanged);
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

void BluetoothPresenter::onDevicesChanged(const QVector< DeviceInfo >& devices)
{
    const bool countChanged = (d->devices.count() != devices.count());

    while(d->devices.count() > devices.count()) delete d->devices.takeLast();
    while(d->devices.count() < devices.count()) d->devices.append(new BluetoothDevice(this));

    for (int i = 0; i < devices.count(); ++i)
    {
        BluetoothDevice* dst = d->devices[i];
        const DeviceInfo& src = devices[i];

        dst->setAddress(src.address);
        dst->setName(src.name);
        dst->setIsPaired(src.isPaired);
    }
    if (countChanged)
    {
        emit deviceCountChanged(devices.count());
    }
}

void BluetoothPresenter::requestScan()
{
    d->model->requsestScan();
}

void BluetoothPresenter::requestPair(const QString& address, bool paired)
{
    qDebug() << Q_FUNC_INFO  << address << paired;
    d->model->requestPair(address, paired);
}
