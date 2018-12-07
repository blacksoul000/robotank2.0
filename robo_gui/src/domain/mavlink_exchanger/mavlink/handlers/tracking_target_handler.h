#ifndef TRACKING_TARGET_HANDLER_H
#define TRACKING_TARGET_HANDLER_H

#include "abstract_mavlink_handler.h"

#include <QScopedPointer>

struct PointF3D;

namespace domain
{
    class RoboModel;

    class TrackingTargetHandler: public AbstractMavLinkHandler
    {
    public:
        TrackingTargetHandler(MavLinkCommunicator* communicator, domain::RoboModel* model);
        void processMessage(const mavlink_message_t& message) override;

    private:
        struct Impl;
        QScopedPointer< Impl > d;
    };
}

#endif // TRACKING_TARGET_HANDLER_H
