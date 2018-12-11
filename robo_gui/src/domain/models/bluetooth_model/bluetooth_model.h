#ifndef BLUETOOTH_MODEL_H
#define BLUETOOTH_MODEL_H

#include <QObject>
#include <QVector>

struct BluetoothDeviceInfo;

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

        void setDevices(const QVector< BluetoothDeviceInfo >& devices);
        QVector< BluetoothDeviceInfo > devices() const;

    signals:
        void requestStatus();
        void requestScan();
        void requestPair(const QString& address, bool paired);

        void scanStatusChanged(bool isScanning);
        void devicesChanged(const QVector< BluetoothDeviceInfo >& devices);

    private:
        class Impl;
        Impl* d;
    };
} // namespace domain

#endif // BLUETOOTH_MODEL_H
