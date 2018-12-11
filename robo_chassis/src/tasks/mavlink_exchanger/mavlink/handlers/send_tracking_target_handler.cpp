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

namespace
{
    const int gscMavId = 255; // FIXME
}  // namespace

using namespace domain;
using data_source::AbstractLink;

SendTrackingTargetHandler::SendTrackingTargetHandler(MavLinkCommunicator* communicator):
    AbstractMavLinkHandler(communicator)
{
    PubSub::instance()->subscribe("tracker/target", &SendTrackingTargetHandler::onTrackerTarget, this);
}

void SendTrackingTargetHandler::onTrackerTarget(const QRectF& target)
{
    AbstractLink* link = m_communicator->mavSystemLink(::gscMavId);
    if (!link) return;

    mavlink_message_t message;
    mavlink_tracking_target_rect_t rect;

    rect.x = target.x();
    rect.y = target.y();
    rect.width = target.width();
    rect.height = target.height();

    mavlink_msg_tracking_target_rect_encode_chan(m_communicator->systemId(),
                                                 m_communicator->componentId(),
                                                 m_communicator->linkChannel(link),
                                                 &message, &rect);

    m_communicator->sendMessage(message, link);
//    qDebug() << Q_FUNC_INFO << "Send to " << link->send().address() << link->send().port() << message.msgid;
}

void SendTrackingTargetHandler::processMessage(const mavlink_message_t& message)
{
    Q_UNUSED(message)
}
