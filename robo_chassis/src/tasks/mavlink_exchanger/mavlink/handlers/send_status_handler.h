#ifndef SEND_STATUS_HANDLER_H
#define SEND_STATUS_HANDLER_H

#include "abstract_mavlink_handler.h"

#include <QScopedPointer>
#include <QVector>
#include <QObject>

struct PointF3D;
struct BluetoothDeviceInfo;

namespace domain
{
    class SendStatusHandler: public QObject, public AbstractMavLinkHandler
    {
    public:
        SendStatusHandler(MavLinkCommunicator* communicator);
        void processMessage(const mavlink_message_t& message) override;

    protected:
        void timerEvent(QTimerEvent *event) override;

        void onArduinoStatus(const bool& online);

        void onJoyButtons(const quint16& buttons);
        void onJoyStatus(const bool& connected);
        void onJoyCapacity(const quint8& capacity);
        void onJoyCharging(const bool& charging);

        void onTrackerStatus(const bool& tracking);

        void onChassisVoltage(const quint16& voltage);
        void onHeadlightChanged(const bool& on);
        void onPointerChanged(const bool& on);

        void onBluetoothScanStatus(const bool& scanning);
        void onBluetoothPairStatus(const bool& pairing);
        void onBluetoothDevices(const QVector< BluetoothDeviceInfo >& devices);

    private:
        struct Impl;
        QScopedPointer< Impl > d;
    };
}

#endif // SEND_STATUS_HANDLER_H
