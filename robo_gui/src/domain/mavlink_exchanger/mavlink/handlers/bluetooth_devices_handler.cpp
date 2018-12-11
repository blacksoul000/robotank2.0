#include "bluetooth_devices_handler.h"

#include "robo_model.h"
#include "bluetooth_model.h"

#include "bluetooth_device_info.h"

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

struct BluetoothDevicesHandler::Impl
{
    domain::RoboModel* model = nullptr;
    QVector< BluetoothDeviceInfo > devices;
};

BluetoothDevicesHandler::BluetoothDevicesHandler(MavLinkCommunicator* communicator, domain::RoboModel* model):
    AbstractCommandHandler(communicator),
    AbstractMavLinkHandler(communicator),
    d(new Impl)
{
    d->model = model;
    communicator->commandService()->addHandler(this);
}

void BluetoothDevicesHandler::processMessage(const mavlink_message_t& message)
{
    if (message.msgid == MAVLINK_MSG_ID_SYS_STATUS) this->processSysStatus(message);
    if (message.msgid == MAVLINK_MSG_ID_BLUETOOTH_DEVICES) this->processDevices(message);
}

void BluetoothDevicesHandler::processSysStatus(const mavlink_message_t& message)
{
    mavlink_sys_status_t systemStatus;
    mavlink_msg_sys_status_decode(&message, &systemStatus);

    if (systemStatus.system_state & BLUETOOTH_SCANNING)
    {
        qDebug() << Q_FUNC_INFO << "scanning...";

        auto vehicle = m_communicator->vehicleRegistry()->vehicle(message.sysid);
        if (!vehicle) return;

        if (systemStatus.bluetooth_devices_count == 0)
        {
            d->devices.clear();
        }
        else
        {
            this->requestDevices(vehicle->sysId(), 0,
                                 systemStatus.bluetooth_devices_count); // request all
        }
    }
}

void BluetoothDevicesHandler::processDevices(const mavlink_message_t& message)
{
    mavlink_bluetooth_devices_t devices;
    mavlink_msg_bluetooth_devices_decode(&message, &devices);

    this->ackCommand(message.sysid, MAV_CMD_REQUEST_BLUETOOTH_DEVICES, Command::Completed);

    QByteArray ba = devices.device_list;
    QDataStream in(ba);

    BluetoothDeviceInfo device;
    for (quint8 index = 0; index < devices.count; ++index)
    {
        in >> device;
        d->devices.append(device);
    }

    qDebug() << Q_FUNC_INFO << devices.first_index << devices.count << devices.total_count;

    if (d->devices.count() < devices.total_count)
    {
        auto vehicle = m_communicator->vehicleRegistry()->vehicle(message.sysid);
        if (!vehicle) return;

        this->requestDevices(vehicle->sysId(), d->devices.count(),
                                               devices.total_count - d->devices.count());
    }

    d->model->bluetooth()->setDevices(d->devices);
}

void BluetoothDevicesHandler::sendCommand(int vehicleId, const CommandPtr& command, int attempt)
{
    Q_UNUSED(vehicleId)
    Q_UNUSED(command)
    Q_UNUSED(attempt)
}

void BluetoothDevicesHandler::requestDevices(int sysId, int from, int count)
{
    qDebug() << Q_FUNC_INFO << sysId << from << count;
    domain::CommandPtr command = domain::CommandPtr::create();
    command->setCommandId(MAV_CMD_REQUEST_BLUETOOTH_DEVICES);
    command->addArgument(from);
    command->addArgument(count);

    m_communicator->commandService()->executeCommand(sysId, command);
}
