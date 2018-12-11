#include "bluetooth_pair_handler.h"

// MAVLink
#include <mavlink.h>

// Internal
#include "mavlink_communicator.h"
#include "command_service.h"
#include "command.h"
#include "vehicle_registry.h"
#include "vehicle.h"
#include "abstract_link.h"

// Qt
#include <QDebug>

using namespace domain;
using data_source::AbstractLink;

BluetoothPairHandler::BluetoothPairHandler(MavLinkCommunicator* communicator):
    AbstractCommandHandler(communicator),
    AbstractMavLinkHandler(communicator)
{
    communicator->commandService()->addHandler(this);
}

void BluetoothPairHandler::processMessage(const mavlink_message_t& message)
{
    Q_UNUSED(message)
}

void BluetoothPairHandler::sendCommand(int vehicleId, const CommandPtr& command, int attempt)
{
    if (command->type() != MAVLINK_MSG_ID_COMMAND_BLUETOOTH_PAIR) return;

    const auto& args = command->arguments();
    if (args.count() < 2) return;

    qDebug() << "MAV:" << vehicleId << command->commandId() << command->arguments() << attempt;

    VehiclePtr vehicle = m_communicator->vehicleRegistry()->vehicle(vehicleId);
    if (vehicle.isNull()) return;

    mavlink_message_t message;
    mavlink_command_bluetooth_pair_t mavCommand;

    mavCommand.target_system = vehicle->sysId();
    mavCommand.target_component = 0;
    mavCommand.confirmation = attempt;
    mavCommand.command = command->commandId();

    mavCommand.address = args.at(0).toULongLong();
    mavCommand.pair = args.at(1).toUInt();

    AbstractLink* link = m_communicator->mavSystemLink(vehicle->sysId());
    if (!link) return;

    mavlink_msg_command_bluetooth_pair_encode_chan(m_communicator->systemId(),
                                                   m_communicator->componentId(),
                                                   m_communicator->linkChannel(link),
                                                   &message, &mavCommand);
    m_communicator->sendMessage(message, link);
}
