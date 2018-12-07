#include "tracking_target_handler.h"

#include "robo_model.h"
#include "track_model.h"

// Internal
#include "mavlink_communicator.h"
#include "mavlink_protocol_helpers.h"

// MAVLink
#include <mavlink.h>

// Qt
#include <QDebug>
#include <QRectF>

using namespace domain;

struct TrackingTargetHandler::Impl
{
    domain::RoboModel* model = nullptr;
};

TrackingTargetHandler::TrackingTargetHandler(MavLinkCommunicator* communicator,
                                             domain::RoboModel* model):
    AbstractMavLinkHandler(communicator),
    d(new Impl)
{
    d->model = model;
}

void TrackingTargetHandler::processMessage(const mavlink_message_t& message)
{
    if (message.msgid != MAVLINK_MSG_ID_TRACKING_TARGET_RECT) return;

    mavlink_tracking_target_rect_t rect;
    mavlink_msg_tracking_target_rect_decode(&message, &rect);

    const auto& track = d->model->track();
    track->setTargetRect(QRectF(rect.x, rect.y, rect.width, rect.height));
//    qDebug() << Q_FUNC_INFO << message.sysid << track->targetRect();
}
