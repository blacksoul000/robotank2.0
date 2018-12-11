#ifndef BLUETOOTH_MANAGER_H
#define BLUETOOTH_MANAGER_H

#include "i_task.h"

#include <QBluetoothLocalDevice>
#include <QBluetoothDeviceInfo>

struct DeviceInfo;
struct Empty;
struct BluetoothPairRequest;

class BluetoothManager : public ITask
{
public:
    BluetoothManager();
    ~BluetoothManager();

    void start() override;

private:
    void onRequestScan(const Empty&);
    void onRequestPair(const BluetoothPairRequest& request);

private slots:
    void addDevice(const QBluetoothDeviceInfo& info);
    void onPairingFinished(const QBluetoothAddress& address, QBluetoothLocalDevice::Pairing);
    void onScanFinished();
    void onPairingDisplayConfirmation(const QBluetoothAddress& address, const QString& pin);

private:
    class Impl;
    Impl* d;
};

#endif //BLUETOOTH_MANAGER_H
