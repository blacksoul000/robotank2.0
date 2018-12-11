#ifndef BLUETOOTH_PAIR_HANDLER_H
#define BLUETOOTH_PAIR_HANDLER_H

// Internal
#include "mavlink_command_handler.h"

namespace domain
{
    class BluetoothPairHandler: public MavLinkCommandHandler
    {
        Q_OBJECT

    public:
        explicit BluetoothPairHandler(MavLinkCommunicator* communicator);
        ~BluetoothPairHandler() override;

        void processCommand(const mavlink_message_t& message) override;

    protected:
        void processMessage(const mavlink_message_t& message) override;

    private:
        class Impl;
        QScopedPointer< Impl > const d;
    };
}

#endif // BLUETOOTH_PAIR_HANDLER_H
