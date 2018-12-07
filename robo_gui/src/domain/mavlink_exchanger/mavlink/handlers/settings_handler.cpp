#include "settings_handler.h"

#include "robo_model.h"
#include "settings_model.h"

// MAVLink
#include <mavlink.h>

// Internal
#include "mavlink_communicator.h"
#include "mavlink_protocol_helpers.h"
#include "command_service.h"
#include "command.h"
#include "vehicle_registry.h"
#include "vehicle.h"

// Qt
#include <QDebug>

using namespace domain;

struct SettingsHandler::Impl
{
    domain::RoboModel* model = nullptr;
};

SettingsHandler::SettingsHandler(MavLinkCommunicator* communicator, domain::RoboModel* model):
    AbstractCommandHandler(communicator),
    AbstractMavLinkHandler(communicator),
    d(new Impl)
{
    d->model = model;
    communicator->commandService()->addHandler(this);
}

void SettingsHandler::processMessage(const mavlink_message_t& message)
{
    if (message.msgid != MAVLINK_MSG_ID_SETTINGS) return;

    mavlink_settings_t settings;
    mavlink_msg_settings_decode(&message, &settings);

    const auto& model = d->model->settings();

    model->setQuality(settings.image_quality);
    model->setBrightness(settings.image_brightness);
    model->setContrast(settings.image_contrast);
    model->setEnginePower(SettingsModel::Engine::Left, settings.left_engine_power);
    model->setEnginePower(SettingsModel::Engine::Right, settings.right_engine_power);
    model->setTracker(settings.selected_tracker);
    model->setVideoSource(settings.video_source);

//    qDebug() << Q_FUNC_INFO << message.sysid << status->yaw()
//              << status->pitch() << status->roll()
//              << status->gunPositionH() << status->gunPositionV();


    auto vehicle = m_communicator->vehicleRegistry()->vehicle(message.sysid);
    if (!vehicle) return;

    this->ackCommand(vehicle->sysId(), MAV_CMD_REQUEST_SETTINGS, Command::Completed);
}

void SettingsHandler::sendCommand(int vehicleId, const CommandPtr& command, int attempt)
{
    Q_UNUSED(vehicleId)
    Q_UNUSED(command)
    Q_UNUSED(attempt)
}
