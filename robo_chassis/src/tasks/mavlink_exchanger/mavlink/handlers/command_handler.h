#ifndef COMMAND_HANDLER_H
#define COMMAND_HANDLER_H

// Internal
#include "mavlink_command_handler.h"

namespace domain
{
    class CommandHandler: public MavLinkCommandHandler
    {
    public:
        explicit CommandHandler(MavLinkCommunicator* communicator);
        ~CommandHandler() override;

    protected:
        void processCommand(const mavlink_message_t& message) override;
        void onGamepadStatusChanged(const bool& status);

    private:
        class Impl;
        QScopedPointer< Impl > const d;
    };
}

#endif // COMMAND_HANDLER_H
