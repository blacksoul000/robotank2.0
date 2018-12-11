#include "bluetooth_pair_handler.h"

#include "bluetooth_pair_request.h"

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

class BluetoothPairHandler::Impl
{
public:
    // publishers
    Publisher< BluetoothPairRequest >* bluetoothPairP = nullptr;
};

BluetoothPairHandler::BluetoothPairHandler(MavLinkCommunicator* communicator):
        MavLinkCommandHandler(communicator),
    d(new Impl())
{
    d->bluetoothPairP = PubSub::instance()->advertise< BluetoothPairRequest >("bluetooth/pair");
}

BluetoothPairHandler::~BluetoothPairHandler()
{
    delete d->bluetoothPairP;
}

void BluetoothPairHandler::processCommand(const mavlink_message_t& message)
{
    if (message.msgid != MAVLINK_MSG_ID_COMMAND_BLUETOOTH_PAIR) return;

    mavlink_command_bluetooth_pair_t cmd;
    mavlink_msg_command_bluetooth_pair_decode(&message, &cmd);

    auto vehicle = m_communicator->vehicleRegistry()->vehicle(message.sysid);
    if (!vehicle) return;

    qDebug() << Q_FUNC_INFO << message.sysid << cmd.command;
    mavlink_message_t reply;
    mavlink_command_ack_t ack;

    ack.command = cmd.command;

    switch (cmd.command)
    {
        case MAV_CMD_BLUETOOTH_PAIR:
        {
            const quint64 address = cmd.address;
            const quint8 pair = cmd.pair;

            d->bluetoothPairP->publish({QBluetoothAddress(address).toString(), pair});
            ack.result = MAV_RESULT_ACCEPTED;
            break;
        }
        default:
            return;
    }

    AbstractLink* link = m_communicator->mavSystemLink(message.sysid);
    if (!link) return;

    mavlink_msg_command_ack_encode_chan(m_communicator->systemId(),
                                        m_communicator->componentId(),
                                        m_communicator->linkChannel(link),
                                        &reply, &ack);
    m_communicator->sendMessage(reply, link);
}
