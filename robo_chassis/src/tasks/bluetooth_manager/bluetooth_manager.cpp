#include "bluetooth_manager.h"

#include "pub_sub.h"
#include "empty.h"
#include "bluetooth_pair_request.h"

#include "bluetooth_device_info.h"

#include <QBluetoothDeviceDiscoveryAgent>
#include <QBluetoothLocalDevice>

#include <QDebug>

class BluetoothManager::Impl
{
public:
    Publisher< bool >* scanStatusP = nullptr;
    Publisher< QVector< BluetoothDeviceInfo > >* deviceListP = nullptr;

    QBluetoothDeviceDiscoveryAgent* agent = nullptr;
    QBluetoothLocalDevice* localDevice = nullptr;

    QVector< BluetoothDeviceInfo > devices;
    bool isScanning = false;
};

BluetoothManager::BluetoothManager() :
    ITask(),
    d(new Impl)
{
    d->scanStatusP = PubSub::instance()->advertise< bool >("bluetooth/scanning");
    d->deviceListP = PubSub::instance()->advertise< QVector< BluetoothDeviceInfo > >("bluetooth/devices");

    PubSub::instance()->subscribe("bluetooth/scan", &BluetoothManager::onRequestScan, this);
    PubSub::instance()->subscribe("bluetooth/pair", &BluetoothManager::onRequestPair, this);
}

BluetoothManager::~BluetoothManager()
{
    delete d->scanStatusP;
    delete d->deviceListP;
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
}

void BluetoothManager::onRequestScan(const Empty&)
{
    qDebug() << Q_FUNC_INFO;
    d->devices.clear();
    d->agent->start();
    d->scanStatusP->publish(true);
    d->deviceListP->publish(d->devices);
}

void BluetoothManager::onRequestPair(const BluetoothPairRequest& request)
{
    d->localDevice->requestPairing(QBluetoothAddress(request.device),
                             request.pair ? QBluetoothLocalDevice::AuthorizedPaired
                                          : QBluetoothLocalDevice::Unpaired);
}

void BluetoothManager::addDevice(const QBluetoothDeviceInfo& info)
{
    qDebug() << Q_FUNC_INFO << info.address().toString();
    BluetoothDeviceInfo device;
    device.address = info.address().toString();
    device.name = info.name();
    const QBluetoothLocalDevice::Pairing status = d->localDevice->pairingStatus(info.address());
    device.isPaired = (status == QBluetoothLocalDevice::Paired
                     || status == QBluetoothLocalDevice::AuthorizedPaired);
    d->devices.append(device);
    d->deviceListP->publish(d->devices);
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

//QVector< DeviceInfo > BluetoothManager::devices() const
//{
//    return d->devices;
//}
//
//bool BluetoothManager::isScanning() const
//{
//    return d->agent->isActive();
//}

void BluetoothManager::onScanFinished()
{
    qDebug() << Q_FUNC_INFO;
    d->scanStatusP->publish(false);
}

void BluetoothManager::onPairingDisplayConfirmation(const QBluetoothAddress& address,
                                                    const QString& pin)
{
    qDebug() << Q_FUNC_INFO << address.toString() << pin;
    d->localDevice->pairingConfirmation(true);
}
