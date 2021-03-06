#ifndef ATTITUDE_HANDLER_H
#define ATTITUDE_HANDLER_H

#include "abstract_mavlink_handler.h"

#include <QScopedPointer>

namespace domain
{
    class RoboModel;

    class AttitudeHandler: public AbstractMavLinkHandler
    {
    public:
        AttitudeHandler(MavLinkCommunicator* communicator, domain::RoboModel* model);
        void processMessage(const mavlink_message_t& message) override;

    private:
        struct Impl;
        QScopedPointer< Impl > d;
    };
}

#endif // ATTITUDE_HANDLER_H
