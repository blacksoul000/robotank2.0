#ifndef BLUETOOTH_PRESENTER_H
#define BLUETOOTH_PRESENTER_H

#include <QObject>

struct BluetoothDeviceInfo;

namespace domain
{
    class RoboModel;
}

namespace presentation
{
    class BluetoothDevice;

    class BluetoothPresenter : public QObject
    {
        Q_OBJECT
    public:
        Q_PROPERTY(QList< QObject* > devices READ devices NOTIFY deviceCountChanged)
        Q_PROPERTY(bool scanStatus READ scanStatus NOTIFY scanStatusChanged)

        BluetoothPresenter(domain::RoboModel* model, QObject* parent = nullptr);
        ~BluetoothPresenter() override;

        bool scanStatus() const;
        QList< QObject* > devices() const;


    public slots:
        void start();
        void stop();
        void requestStatus();
        void requestScan();
        void requestPair(const QString& address, bool paired);

    signals:
        void deviceCountChanged(int count);
        void scanStatusChanged(bool isScanning);

    private slots:
        void onDevicesChanged(const QVector< BluetoothDeviceInfo >& devices);

    private:
        class Impl;
        Impl* d;
    };
}

#endif //BLUETOOTH_PRESENTER_H
