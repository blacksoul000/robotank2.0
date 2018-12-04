#include "send_attitude_handler.h"

#include "pointf3d.h"

#include "pub_sub.h"

// Internal
#include "mavlink_communicator.h"
#include "abstract_link.h"
#include "mavlink_protocol_helpers.h"

// MAVLink
#include <mavlink.h>

// Qt
#include <QDebug>

namespace
{
    const int gscMavId = 255;
}  // namespace

using namespace domain;
using data_source::AbstractLink;

struct SendAttitudeHandler::Impl
{
    PointF3D ypr;
};

SendAttitudeHandler::SendAttitudeHandler(MavLinkCommunicator* communicator):
    AbstractMavLinkHandler(communicator),
    d(new Impl)
{
    this->startTimer(40); // 25 Hz
    PubSub::instance()->subscribe("chassis/ypr", &SendAttitudeHandler::onYpr, this);
}

void SendAttitudeHandler::onYpr(const PointF3D& ypr)
{
    d->ypr = ypr;
}

void SendAttitudeHandler::processMessage(const mavlink_message_t& message)
{
    Q_UNUSED(message)
}

void SendAttitudeHandler::timerEvent(QTimerEvent* event)
{
    Q_UNUSED(event)

    AbstractLink* link = m_communicator->mavSystemLink(::gscMavId);
    if (!link) return;

    mavlink_message_t message;
    mavlink_attitude_t attitude;

    attitude.yaw = data_source::encodeYpr(d->ypr.x);
    attitude.pitch = data_source::encodeYpr(d->ypr.y);
    attitude.roll = data_source::encodeYpr(d->ypr.z);

    mavlink_msg_attitude_encode(m_communicator->systemId(),
                                m_communicator->componentId(),
                                &message, &attitude);

    m_communicator->sendMessage(message, link);
//    qDebug() << Q_FUNC_INFO << "Send to " << link->send().address() << link->send().port() << message.msgid;
}
