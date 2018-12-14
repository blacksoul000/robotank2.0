#include "send_tracking_target_handler.h"

#include "pub_sub.h"

// Internal
#include "mavlink_communicator.h"
#include "abstract_link.h"

// MAVLink
#include <mavlink.h>

// Qt
#include <QRectF>
#include <QDebug>

using namespace domain;
using data_source::AbstractLink;

SendTrackingTargetHandler::SendTrackingTargetHandler(MavLinkCommunicator* communicator):
    QObject(communicator),
    AbstractMavLinkHandler(communicator)
{
    PubSub::instance()->subscribe("tracker/target", &SendTrackingTargetHandler::onTrackerTarget, this);
}

void SendTrackingTargetHandler::onTrackerTarget(const QRectF& target)
{
    mavlink_message_t message;
    mavlink_tracking_target_rect_t rect;

    rect.x = target.x();
    rect.y = target.y();
    rect.width = target.width();
    rect.height = target.height();

    for (AbstractLink* link: m_communicator->links())
    {
        mavlink_msg_tracking_target_rect_encode_chan(m_communicator->systemId(),
                                                     m_communicator->componentId(),
                                                     m_communicator->linkChannel(link),
                                                     &message, &rect);
        m_communicator->sendMessage(message, link);
    }
}

void SendTrackingTargetHandler::processMessage(const mavlink_message_t& message)
{
    Q_UNUSED(message)
}
