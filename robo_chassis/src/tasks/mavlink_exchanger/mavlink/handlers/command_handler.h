#ifndef COMMAND_HANDLER_H
#define COMMAND_HANDLER_H

// Internal
#include "mavlink_command_handler.h"

namespace domain
{
    class CommandHandler: public MavLinkCommandHandler
    {
        Q_OBJECT

    public:
        explicit CommandHandler(MavLinkCommunicator* communicator);
        ~CommandHandler() override;

    protected:
        void processCommand(const mavlink_message_t& message) override;

    private:
        class Impl;
        QScopedPointer< Impl > const d;
    };
}

#endif // COMMAND_HANDLER_H
