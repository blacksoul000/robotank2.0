#ifndef SEND_TRACKING_TARGET_HANDLER_H
#define SEND_TRACKING_TARGET_HANDLER_H

#include "abstract_mavlink_handler.h"

#include <QScopedPointer>
#include <QObject>

class QRectF;
namespace domain
{
    class SendTrackingTargetHandler: public QObject, public AbstractMavLinkHandler
    {
    public:
        SendTrackingTargetHandler(MavLinkCommunicator* communicator);
        void processMessage(const mavlink_message_t& message) override;

    protected:
        void onTrackerTarget(const QRectF& target);
    };
}

#endif // SEND_TRACKING_TARGET_HANDLER_H
