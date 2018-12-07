#ifndef SYS_STATUS_HANDLER_H
#define SYS_STATUS_HANDLER_H

#include "abstract_mavlink_handler.h"

#include <QScopedPointer>

struct PointF3D;

namespace domain
{
    class RoboModel;

    class SysStatusHandler: public AbstractMavLinkHandler
    {
    public:
        SysStatusHandler(MavLinkCommunicator* communicator, domain::RoboModel* model);
        void processMessage(const mavlink_message_t& message) override;

    private:
        struct Impl;
        QScopedPointer< Impl > d;
    };
}

#endif // SEND_STATUS_HANDLER_H
