#include "sys_status_handler.h"

#include "robo_model.h"
#include "status_model.h"
#include "bluetooth_model.h"
#include "track_model.h"
#include "gamepad_model.h"

// Internal
#include "mavlink_communicator.h"
#include "abstract_link.h"

// MAVLink
#include <mavlink.h>

// Qt
#include <QDebug>

using namespace domain;
using data_source::AbstractLink;

struct SysStatusHandler::Impl
{
    domain::RoboModel* model = nullptr;
};

SysStatusHandler::SysStatusHandler(MavLinkCommunicator* communicator, domain::RoboModel* model):
    AbstractMavLinkHandler(communicator),
    d(new Impl)
{
    d->model = model;
}

void SysStatusHandler::processMessage(const mavlink_message_t& message)
{
    if (message.msgid != MAVLINK_MSG_ID_SYS_STATUS) return;

    mavlink_sys_status_t systemStatus;
    mavlink_msg_sys_status_decode(&message, &systemStatus);

    const auto& status = d->model->status();
    const auto& track = d->model->track();
    const auto& bluetooth = d->model->bluetooth();
    const auto& gamepad = d->model->gamepad();

    status->setArduinoStatus(systemStatus.system_state & ARDUINO_ONLINE);
    gamepad->setConnected(systemStatus.system_state & GAMEPAD_CONNECTED);
    gamepad->setCapacity(systemStatus.gamepad_capacity);
    gamepad->setCharging(systemStatus.system_state & GAMEPAD_CHARGING);
    status->setRobotBatteryLevel(systemStatus.voltage);
    status->setHeadlightStatus(systemStatus.system_state & HEADLIGHT);
    status->setPointerStatus(systemStatus.system_state & POINTER);
    status->setButtons(systemStatus.gamepad_buttons);

    track->setTracking(systemStatus.system_state & TRACKING);

    bluetooth->setScanStatus(systemStatus.system_state & BLUETOOTH_SCANNING);
    bluetooth->setPairStatus(systemStatus.system_state & BLUETOOTH_PAIRING);

//    qDebug() << Q_FUNC_INFO << message.sysid << message.msgid << systemStatus.system_state
//            << systemStatus.voltage << systemStatus.gamepad_capacity
//            << systemStatus.gamepad_buttons;
}
