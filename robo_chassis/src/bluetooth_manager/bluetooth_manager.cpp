#include "bluetooth_manager.h"

#include "chassis_packet.h"

#include <QBluetoothDeviceDiscoveryAgent>
#include <QBluetoothLocalDevice>

#include <QDebug>

using domain::BluetoothManager;

class BluetoothManager::Impl
{
public:
    QBluetoothDeviceDiscoveryAgent* agent = nullptr;
    QBluetoothLocalDevice* localDevice = nullptr;

    QVector< DeviceInfo > devices;
    bool isScanning = false;
};

BluetoothManager::BluetoothManager(QObject* parent) :
    QObject(parent),
    d(new Impl)
{}

BluetoothManager::~BluetoothManager()
{
    delete d;
}

void BluetoothManager::start()
{
    d->localDevice = new QBluetoothLocalDevice(this);
    d->localDevice->setHostMode(QBluetoothLocalDevice::HostConnectable);
    d->localDevice->powerOn();

    d->agent = new QBluetoothDeviceDiscoveryAgent(this);
    connect(d->agent, &QBluetoothDeviceDiscoveryAgent::deviceDiscovered,
            this, &BluetoothManager::addDevice);
    connect(d->agent, &QBluetoothDeviceDiscoveryAgent::finished,
            this, &BluetoothManager::onScanFinished);
    connect(d->localDevice, &QBluetoothLocalDevice::pairingFinished,
            this, &BluetoothManager::onPairingFinished);
    connect(d->agent, static_cast<void(QBluetoothDeviceDiscoveryAgent::*)
            (QBluetoothDeviceDiscoveryAgent::Error)>(&QBluetoothDeviceDiscoveryAgent::error),
            [&](){
        qDebug() << Q_FUNC_INFO << __LINE__  << d->agent->errorString();
    });
    connect(d->localDevice, &QBluetoothLocalDevice::pairingDisplayConfirmation,
            this, [](){
        qDebug() << Q_FUNC_INFO << __LINE__ ;
    });
//            this, &BluetoothManager::onPairingDisplayConfirmation);
    connect(d->localDevice, &QBluetoothLocalDevice::pairingDisplayPinCode,
            this, [](){
        qDebug() << Q_FUNC_INFO << __LINE__ ;
    });
//            this, &BluetoothManager::onPairingDisplayConfirmation);
}

void BluetoothManager::scan()
{
    qDebug() << Q_FUNC_INFO;
    d->devices.clear();
    d->agent->start();
}

void BluetoothManager::addDevice(const QBluetoothDeviceInfo& info)
{
    qDebug() << Q_FUNC_INFO << info.address().toString();
    DeviceInfo device;
    device.address = info.address().toString();
    device.name = info.name();
    const QBluetoothLocalDevice::Pairing status = d->localDevice->pairingStatus(info.address());
    device.isPaired = (status == QBluetoothLocalDevice::Paired
                     || status == QBluetoothLocalDevice::AuthorizedPaired);
    d->devices.append(device);
}

void BluetoothManager::requestPairing(const QString& address, bool paired)
{
    d->localDevice->requestPairing(QBluetoothAddress(address),
                                   paired ? QBluetoothLocalDevice::AuthorizedPaired
                                          : QBluetoothLocalDevice::Unpaired);
}

void BluetoothManager::onPairingFinished(const QBluetoothAddress& address,
                                         QBluetoothLocalDevice::Pairing pairing)
{
    const bool status = (pairing == QBluetoothLocalDevice::Paired
                         || pairing == QBluetoothLocalDevice::AuthorizedPaired);

    for (auto& info: d->devices)
    {
        if (info.address == address.toString())
        {
            info.isPaired = status;
            break;
        }
    }
}

QVector< DeviceInfo > BluetoothManager::devices() const
{
    return d->devices;
}

bool BluetoothManager::isScanning() const
{
    return d->agent->isActive();
}

void BluetoothManager::onScanFinished()
{
    qDebug() << Q_FUNC_INFO ;
}

void BluetoothManager::onPairingDisplayConfirmation(const QBluetoothAddress& address,
                                                    const QString& pin)
{
    qDebug() << Q_FUNC_INFO << address.toString() << pin;
    d->localDevice->pairingConfirmation(true);
}
