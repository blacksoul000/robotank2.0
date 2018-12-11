#ifndef BLUETOOTH_DEVICES_HANDLER_H
#define BLUETOOTH_DEVICES_HANDLER_H

#include "abstract_mavlink_handler.h"
#include "abstract_command_handler.h"

#include <QScopedPointer>

namespace domain
{
    class RoboModel;

    class BluetoothDevicesHandler: public AbstractCommandHandler,
                                   public AbstractMavLinkHandler
    {
    public:
        BluetoothDevicesHandler(MavLinkCommunicator* communicator, domain::RoboModel* model);
        void processMessage(const mavlink_message_t& message) override;

    protected:
        void sendCommand(int vehicleId, const CommandPtr& command, int attempt = 0) override;

        void processDevices(const mavlink_message_t& message);
        void processSysStatus(const mavlink_message_t& message);

        void requestDevices(int sysId, int from, int count);

    private:
        struct Impl;
        QScopedPointer< Impl > d;
    };
}

#endif // BLUETOOTH_DEVICES_HANDLER_H
