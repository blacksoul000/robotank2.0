#ifndef MAVLINK_COMMAND_HANDLER_H
#define MAVLINK_COMMAND_HANDLER_H

// Internal
#include "abstract_command_handler.h"
#include "abstract_mavlink_handler.h"

namespace domain
{
    class MavLinkCommandHandler: public domain::AbstractCommandHandler,
                                 public AbstractMavLinkHandler
    {
        Q_OBJECT

    public:
        explicit MavLinkCommandHandler(MavLinkCommunicator* communicator);
        ~MavLinkCommandHandler() override;

        void processMessage(const mavlink_message_t& message) override;

    protected:
        void sendCommand(int vehicleId, const CommandPtr& command, int attempt = 0) override;

        virtual void processCommand(const mavlink_message_t& message);
        virtual void processCommandAck(const mavlink_message_t& message);
    };
}

#endif // MAVLINK_COMMAND_HANDLER_H
