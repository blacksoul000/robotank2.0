#ifndef BLUETOOTH_MODEL_H
#define BLUETOOTH_MODEL_H

#include <QObject>

struct DeviceInfo;

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

        void setDevices(const QVector< DeviceInfo >& devices);
        QVector< DeviceInfo > devices() const;

    signals:
        void requestStatus();
        void requsestScan();
        void requestPair(const QString& address, bool paired);

        void scanStatusChanged(bool isScanning);
        void devicesChanged(const QVector< DeviceInfo >& devices);

    private:
        class Impl;
        Impl* d;
    };
} // namespace domain

#endif // BLUETOOTH_MODEL_H
