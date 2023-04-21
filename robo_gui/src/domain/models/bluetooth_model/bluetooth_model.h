#ifndef BLUETOOTH_MODEL_H
#define BLUETOOTH_MODEL_H

#include <QObject>
#include <QVector>

#include "bluetooth_device_info.h"

namespace domain
{
    class BluetoothModel : public QObject
    {
        Q_OBJECT
    public:
        BluetoothModel(QObject* parent = nullptr);
        ~BluetoothModel();

        void setScanStatus(bool isScanning);
        bool scanStatus() const;

        void setPairStatus(bool isPairing);
        bool pairStatus() const;

        void setDevices(const QVector< BluetoothDeviceInfo >& devices);
        QVector< BluetoothDeviceInfo > devices() const;

    signals:
        void requestStatus();
        void requestScan();
        void requestPair(quint64 address, bool paired);

        void scanStatusChanged(bool isScanning);
        void pairStatusChanged(bool isPairing);
        void devicesChanged(const QVector< BluetoothDeviceInfo >& devices);

    private:
        class Impl;
        Impl* d;
    };
} // namespace domain

#endif // BLUETOOTH_MODEL_H
