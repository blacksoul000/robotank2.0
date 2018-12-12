#include "command_handler.h"

#include "image_settings.h"
#include "bluetooth_pair_request.h"
#include "pointf3d.h"
#include "empty.h"

#include "pub_sub.h"

// Internal
#include "command_service.h"
#include "command.h"

#include "vehicle_registry.h"
#include "vehicle.h"

#include "mavlink_communicator.h"
#include "abstract_link.h"

// MAVLink
#include <mavlink.h>

// Qt
#include <QBluetoothAddress>
#include <QRectF>
#include <QPoint>
#include <QDebug>

using namespace domain;
using data_source::AbstractLink;

class CommandHandler::Impl
{
public:
    // publishers
    Publisher< QRectF >* trackRectP = nullptr;
    Publisher< quint8 >* trackSelectorP = nullptr;
    Publisher< ImageSettings >* imageSettingsP = nullptr;
    Publisher< Empty >* calibrateGunP = nullptr;
    Publisher< Empty >* calibrateYprP = nullptr;
    Publisher< QPoint >* enginePowerP = nullptr;
    Publisher< Empty >* powerDownP = nullptr;
    Publisher< Empty >* bluetoothScanP = nullptr;
};

CommandHandler::CommandHandler(MavLinkCommunicator* communicator):
    MavLinkCommandHandler(communicator),
    d(new Impl())
{
    d->trackRectP = PubSub::instance()->advertise< QRectF >("tracker/toggle");
    d->trackSelectorP = PubSub::instance()->advertise< quint8 >("tracker/selector");
    d->imageSettingsP = PubSub::instance()->advertise< ImageSettings >("camera/settings");
    d->calibrateGunP = PubSub::instance()->advertise< Empty >("gun/calibrate");
    d->calibrateYprP = PubSub::instance()->advertise< Empty >("ypr/calibrate");
    d->enginePowerP = PubSub::instance()->advertise< QPoint >("core/enginePower");
    d->powerDownP = PubSub::instance()->advertise< Empty >("core/powerDown");
    d->bluetoothScanP = PubSub::instance()->advertise< Empty >("bluetooth/scan");
}

CommandHandler::~CommandHandler()
{
    delete d->trackRectP;
    delete d->trackSelectorP;
    delete d->imageSettingsP;
    delete d->calibrateGunP;
    delete d->calibrateYprP;
    delete d->enginePowerP;
    delete d->powerDownP;
    delete d->bluetoothScanP;
}

void CommandHandler::processCommand(const mavlink_message_t& message)
{
    if (message.msgid != MAVLINK_MSG_ID_COMMAND_LONG) return;

    mavlink_command_long_t cmd;
    mavlink_msg_command_long_decode(&message, &cmd);

    auto vehicle = m_communicator->vehicleRegistry()->vehicle(message.sysid);
    if (!vehicle) return;

    mavlink_message_t reply;
    mavlink_command_ack_t ack;

    ack.command = cmd.command;

    switch (cmd.command)
    {
        case MAV_CMD_BLUETOOTH_SCAN:
        {
            d->bluetoothScanP->publish(Empty());
            ack.result = MAV_RESULT_ACCEPTED;
            break;
        }
        case MAV_CMD_CALIBRATE_GUN:
        {
            d->calibrateGunP->publish(Empty());
            ack.result = MAV_RESULT_ACCEPTED;
            break;
        }
        case MAV_CMD_CALIBRATE_YPR:
        {
            d->calibrateYprP->publish(Empty());
            ack.result = MAV_RESULT_ACCEPTED;
            break;
        }
        case MAV_CMD_ENGINE_POWER:
        {
            const quint8 left = cmd.param1;
            const quint8 right = cmd.param2;

            d->enginePowerP->publish(QPoint(left, right));
            ack.result = MAV_RESULT_ACCEPTED;
            break;
        }
        case MAV_CMD_IMAGE_SETTINGS:
        {
            const quint8 quality = cmd.param1;
            const quint8 brightness = cmd.param2;
            const quint8 contrast = cmd.param3;

            d->imageSettingsP->publish(ImageSettings {quality, brightness, contrast});
            ack.result = MAV_RESULT_ACCEPTED;
            break;
        }
        case MAV_CMD_POWER_DOWN:
        {
            d->powerDownP->publish(Empty());
            ack.result = MAV_RESULT_ACCEPTED;
            break;
        }
        case MAV_CMD_SET_TRACKER_CODE:
        {
            const quint8 code = cmd.param1;
            d->trackSelectorP->publish(code);
            ack.result = MAV_RESULT_ACCEPTED;
            break;
        }
        case MAV_CMD_TRACK:
        {
            const float x = cmd.param1;
            const float y = cmd.param2;
            const float width = cmd.param3;
            const float height = cmd.param4;

            d->trackRectP->publish(QRectF(x, y, width, height));
            ack.result = MAV_RESULT_ACCEPTED;
            break;
        }
        default:
            return;
    }

    qDebug() << Q_FUNC_INFO << message.sysid << cmd.command;

    AbstractLink* link = m_communicator->mavSystemLink(message.sysid);
    if (!link) return;

    mavlink_msg_command_ack_encode_chan(m_communicator->systemId(),
                                        m_communicator->componentId(),
                                        m_communicator->linkChannel(link),
                                        &reply, &ack);
    m_communicator->sendMessage(reply, link);
}
