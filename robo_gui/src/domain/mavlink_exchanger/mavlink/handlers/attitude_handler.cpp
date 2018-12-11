#include "attitude_handler.h"

#include <mavlink.h>

#include "robo_model.h"
#include "status_model.h"

// Internal
#include "mavlink_communicator.h"
#include "mavlink_protocol_helpers.h"

// Qt
#include <QDebug>

using namespace domain;

struct AttitudeHandler::Impl
{
    domain::RoboModel* model = nullptr;
};

AttitudeHandler::AttitudeHandler(MavLinkCommunicator* communicator, domain::RoboModel* model):
    AbstractMavLinkHandler(communicator),
    d(new Impl)
{
    d->model = model;
}

void AttitudeHandler::processMessage(const mavlink_message_t& message)
{
    if (message.msgid != MAVLINK_MSG_ID_ATTITUDE) return;

    mavlink_attitude_t attitude;
    mavlink_msg_attitude_decode(&message, &attitude);

    const auto& status = d->model->status();

    status->setYaw(data_source::decodeYpr(attitude.yaw));
    status->setPitch(data_source::decodeYpr(attitude.pitch));
    status->setRoll(data_source::decodeYpr(attitude.roll));

    status->setGunPositionH(data_source::decodeYpr(attitude.gunH));
    status->setGunPositionV(data_source::decodeYpr(attitude.gunV));
//    qDebug() << Q_FUNC_INFO << message.sysid << status->yaw()
//              << status->pitch() << status->roll()
//              << status->gunPositionH() << status->gunPositionV();
}

