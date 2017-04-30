#ifndef BLUETOOTH_DEVICE_H
#define BLUETOOTH_DEVICE_H

#include <QObject>

namespace presentation
{
    class BluetoothDevice : public QObject
    {
        Q_OBJECT
        Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
        Q_PROPERTY(QString address READ address WRITE setAddress NOTIFY addressChanged)
        Q_PROPERTY(bool isPaired READ isPaired WRITE setIsPaired NOTIFY isPairedChanged)

    public:
        BluetoothDevice(QObject* parent = nullptr);
        ~BluetoothDevice();

        QString name() const;
        QString address() const;
        bool isPaired() const;

    public slots:
        void setName(const QString& name);
        void setAddress(const QString& address);
        void setIsPaired(bool isPaired);

    signals:
        void nameChanged(const QString& name);
        void addressChanged(const QString& address);
        void isPairedChanged(bool isPaired);

    private:
        class Impl;
        Impl* d;
    };
} // namespace presentation

#endif // BLUETOOTH_DEVICE_H
