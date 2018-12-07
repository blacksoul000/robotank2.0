#ifndef SETTINGS_HANDLER_H
#define SETTINGS_HANDLER_H

#include "abstract_mavlink_handler.h"
#include "abstract_command_handler.h"

#include <QScopedPointer>

namespace domain
{
    class RoboModel;

    class SettingsHandler: public AbstractCommandHandler,
                           public AbstractMavLinkHandler
    {
    public:
        SettingsHandler(MavLinkCommunicator* communicator, domain::RoboModel* model);
        void processMessage(const mavlink_message_t& message) override;

    protected:
        void sendCommand(int vehicleId, const CommandPtr& command, int attempt = 0) override;

    private:
        struct Impl;
        QScopedPointer< Impl > d;
    };
}

#endif // SETTINGS_HANDLER_H
