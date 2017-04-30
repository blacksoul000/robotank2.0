#ifndef BLUETOOTH_MANAGER_H
#define BLUETOOTH_MANAGER_H

#include <QBluetoothLocalDevice>
#include <QBluetoothDeviceInfo>
#include <QObject>

struct DeviceInfo;

namespace domain
{
    class BluetoothManager : public QObject
    {
    public:
        BluetoothManager(QObject* parent = nullptr);
        ~BluetoothManager();

        void start();
        void scan();
        void requestPairing(const QString& address, bool paired);

        bool isScanning() const;
        QVector< DeviceInfo > devices() const;

    private slots:
        void addDevice(const QBluetoothDeviceInfo& info);
        void onPairingFinished(const QBluetoothAddress& address, QBluetoothLocalDevice::Pairing);
        void onScanFinished();
        void onPairingDisplayConfirmation(const QBluetoothAddress& address, const QString& pin);

    private:
        class Impl;
        Impl* d;
    };
}

#endif //BLUETOOTH_MANAGER_H
