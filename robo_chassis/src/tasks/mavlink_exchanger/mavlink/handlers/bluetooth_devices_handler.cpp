#include "bluetooth_devices_handler.h"

#include "pub_sub.h"

#include "bluetooth_device_info.h"
#include "strlcpy.h"

// Internal
#include "mavlink_communicator.h"
#include "abstract_link.h"
#include "mavlink_protocol_helpers.h"

// MAVLink
#include <mavlink.h>

// Qt
#include <QVector>
#include <QByteArray>
#include <QDebug>

using namespace domain;
using data_source::AbstractLink;

struct BluetoothDevicesHandler::Impl
{
    QVector< BluetoothDeviceInfo > devices;
};

BluetoothDevicesHandler::BluetoothDevicesHandler(MavLinkCommunicator* communicator):
    AbstractMavLinkHandler(communicator),
    d(new Impl)
{
    PubSub::instance()->subscribe("bluetooth/devices",
                                  &BluetoothDevicesHandler::onBluetoothDevices, this);
}

void BluetoothDevicesHandler::onBluetoothDevices(const QVector< BluetoothDeviceInfo >& devices)
{
    d->devices = devices;
}

void BluetoothDevicesHandler::processMessage(const mavlink_message_t& message)
{
    if (message.msgid != MAVLINK_MSG_ID_COMMAND_LONG) return;

    mavlink_command_long_t cmd;
    mavlink_msg_command_long_decode(&message, &cmd);

    if (cmd.command != MAVLINK_MSG_ID_BLUETOOTH_DEVICES) return;

    AbstractLink* link = m_communicator->mavSystemLink(message.sysid);
    if (!link) return;

    mavlink_message_t reply;
    mavlink_bluetooth_devices_t bt;

    bt.total_count = d->devices.count();
    bt.first_index = cmd.param1;

    int count = qMin< int >(bt.total_count - bt.first_index, cmd.param2);
    QByteArray result;

    int index = bt.first_index;
    while (bt.count < count && index < bt.total_count)
    {
        QByteArray ba;
        QDataStream out(&ba, QIODevice::WriteOnly);
        out << d->devices.at(index);
        if (ba.size() + result.size() >= int(sizeof(bt.device_list)) - 1) break;

        result.append(ba);
        ++bt.count;
        ++index;
    }

    domain::strlcpy(bt.device_list, result.data(), sizeof(bt.device_list));

    mavlink_msg_bluetooth_devices_encode_chan(m_communicator->systemId(),
                                              m_communicator->componentId(),
                                              m_communicator->linkChannel(link),
                                              &reply, &bt);

    m_communicator->sendMessage(reply, link);
////    qDebug() << Q_FUNC_INFO << "Send to " << link->send().address() << link->send().port() << message.msgid;
}
