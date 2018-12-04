#ifndef CHASSIS_EXCHANGER_H
#define CHASSIS_EXCHANGER_H

#include <QObject>

namespace domain
{
    class RoboModel;

    class ChassisExchanger : public QObject
    {
        Q_OBJECT
    public:
        explicit ChassisExchanger(RoboModel* model, QObject* parent = nullptr);
        ~ChassisExchanger();

    signals:
        void buttonsUpdated(quint16 buttons);

    public slots:
        void onImageSettingsChanged();
        void onSelectedTrackerChanged(quint8 code);
        void onTrackToggle(const QRectF& rect);
        void onCalibrateGun();
        void onCalibrateCamera();
        void onCalibrateGyro();
        void onEnginePowerChanged();
        void onRequestConfig();

        void onRequestScan();
        void onRequestPair(const QString& address, bool paired);
        void onRequestBluetoothStatus();

    private slots:
        void send();
        void onReadyRead();
        void processPacket(const QByteArray& data);
        void connectionLost();

    private:
        class Impl;
        Impl* d;
    };

} // namespace domain

#endif // CHASSIS_EXCHANGER_H
