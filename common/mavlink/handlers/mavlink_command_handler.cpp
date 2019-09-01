#include "mavlink_command_handler.h"

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
#include <QDebug>

using namespace domain;
using data_source::AbstractLink;

namespace
{
    const QMultiMap< quint8, Command::CommandStatus > mavStatusMap =
    {
        { MAV_RESULT_DENIED, Command::Rejected },
        { MAV_RESULT_TEMPORARILY_REJECTED, Command::Rejected },
        { MAV_RESULT_UNSUPPORTED, Command::Rejected },
        { MAV_RESULT_FAILED, Command::Rejected },
        { MAV_RESULT_IN_PROGRESS, Command::InProgress },
        { MAV_RESULT_ACCEPTED, Command::Completed }
    };
}

MavLinkCommandHandler::MavLinkCommandHandler(MavLinkCommunicator* communicator):
    AbstractCommandHandler(communicator),
    AbstractMavLinkHandler(communicator)
{
    communicator->commandService()->addHandler(this);
}

MavLinkCommandHandler::~MavLinkCommandHandler()
{
}

void MavLinkCommandHandler::processMessage(const mavlink_message_t& message)
{
    if (message.msgid == MAVLINK_MSG_ID_COMMAND_LONG) this->processCommand(message);
    if (message.msgid == MAVLINK_MSG_ID_COMMAND_ACK) this->processCommandAck(message);
}

void MavLinkCommandHandler::processCommand(const mavlink_message_t& message)
{
    mavlink_command_long_t cmd;
    mavlink_msg_command_long_decode(&message, &cmd);

    mavlink_message_t reply;
    mavlink_command_ack_t ack;

    ack.command = cmd.command;
    ack.result = MAV_RESULT_UNSUPPORTED;

    AbstractLink* link = m_communicator->mavSystemLink(message.sysid);
    if (!link) return;

    mavlink_msg_command_ack_encode_chan(m_communicator->systemId(),
                                        m_communicator->componentId(),
                                        m_communicator->linkChannel(link),
                                        &reply, &ack);
    m_communicator->sendMessage(reply, link);
}

void MavLinkCommandHandler::processCommandAck(const mavlink_message_t& message)
{
    mavlink_command_ack_t ack;
    mavlink_msg_command_ack_decode(&message, &ack);

    auto vehicle = m_communicator->vehicleRegistry()->vehicle(message.sysid);
    if (!vehicle) return;

    this->ackCommand(vehicle->sysId(), ack.command,
                     ::mavStatusMap.value(ack.result, Command::Idle));
}

void MavLinkCommandHandler::sendCommand(int vehicleId, const domain::CommandPtr& command, int attempt)
{
    if (command->type() != MAVLINK_MSG_ID_COMMAND_LONG) return;

//    qDebug() << "MAV:" << vehicleId << command->commandId() << command->arguments() << attempt;

    VehiclePtr vehicle = m_communicator->vehicleRegistry()->vehicle(vehicleId);
    if (vehicle.isNull()) return;

    mavlink_message_t message;
    mavlink_command_long_t mavCommand;

    mavCommand.target_system = vehicle->sysId();
    mavCommand.target_component = 0;
    mavCommand.confirmation = attempt;
    mavCommand.command = command->commandId();

    const auto& args = command->arguments();
    if (args.count() > 0) mavCommand.param1 = args.at(0).toFloat();
    if (args.count() > 1) mavCommand.param2 = args.at(1).toFloat();
    if (args.count() > 2) mavCommand.param3 = args.at(2).toFloat();
    if (args.count() > 3) mavCommand.param4 = args.at(3).toFloat();
    if (args.count() > 4) mavCommand.param5 = args.at(4).toFloat();
    if (args.count() > 5) mavCommand.param6 = args.at(5).toFloat();
    if (args.count() > 6) mavCommand.param7 = args.at(6).toFloat();

    AbstractLink* link = m_communicator->mavSystemLink(vehicle->sysId());
    if (!link) return;

    mavlink_msg_command_long_encode_chan(m_communicator->systemId(),
                                         m_communicator->componentId(),
                                         m_communicator->linkChannel(link),
                                         &message, &mavCommand);
    m_communicator->sendMessage(message, link);
}
