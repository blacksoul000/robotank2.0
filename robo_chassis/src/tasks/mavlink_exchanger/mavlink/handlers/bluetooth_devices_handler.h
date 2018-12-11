#ifndef BLUETOOTH_DEVICES_HANDLER_H
#define BLUETOOTH_DEVICES_HANDLER_H

#include "abstract_mavlink_handler.h"

#include <QScopedPointer>
#include <QVector>
#include <QObject>

struct PointF3D;
struct BluetoothDeviceInfo;

namespace domain
{
    class BluetoothDevicesHandler: public QObject, public AbstractMavLinkHandler
    {
    public:
        BluetoothDevicesHandler(MavLinkCommunicator* communicator);
        void processMessage(const mavlink_message_t& message) override;

    protected:
        void onBluetoothDevices(const QVector< BluetoothDeviceInfo >& devices);

    private:
        struct Impl;
        QScopedPointer< Impl > d;
    };
}

#endif // BLUETOOTH_DEVICES_HANDLER_H
