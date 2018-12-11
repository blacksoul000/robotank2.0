#ifndef BLUETOOTH_PAIR_HANDLER_H
#define BLUETOOTH_PAIR_HANDLER_H

#include "abstract_mavlink_handler.h"
#include "abstract_command_handler.h"

namespace domain
{
    class BluetoothPairHandler: public AbstractCommandHandler,
                                   public AbstractMavLinkHandler
    {
    public:
        BluetoothPairHandler(MavLinkCommunicator* communicator);
        void processMessage(const mavlink_message_t& message) override;

    protected:
        void sendCommand(int vehicleId, const CommandPtr& command, int attempt = 0) override;
    };
}

#endif // BLUETOOTH_PAIR_HANDLER_H
