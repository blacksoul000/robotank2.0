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

    bool isPairing = false;
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

    const bool isPairing = systemStatus.system_state & BLUETOOTH_PAIRING;
    if (systemStatus.bluetooth_devices_count != d->devices.count() || (d->isPairing && !isPairing))
    {
        auto vehicle = m_communicator->vehicleRegistry()->vehicle(message.sysid);
        if (!vehicle) return;

        d->devices.clear();
        d->devices.resize(systemStatus.bluetooth_devices_count);
        this->requestDevices(vehicle->sysId(), 0, systemStatus.bluetooth_devices_count); // request all
    }
    d->isPairing = isPairing;
}

void BluetoothDevicesHandler::processDevices(const mavlink_message_t& message)
{
    mavlink_bluetooth_devices_t devices;
    mavlink_msg_bluetooth_devices_decode(&message, &devices);

    this->ackCommand(message.sysid, MAV_CMD_REQUEST_BLUETOOTH_DEVICES, Command::Completed);

    QByteArray ba = QByteArray(devices.device_list, devices.data_length);
    QDataStream in(ba);

    BluetoothDeviceInfo device;
    for (quint8 index = 0; index < devices.count; ++index)
    {
        in >> device;
        d->devices[devices.first_index + index] = device;
        qDebug() << Q_FUNC_INFO << device.address << device.isPaired;
    }

    qDebug() << Q_FUNC_INFO << devices.first_index << devices.count << devices.total_count;
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
    command->setType(MAVLINK_MSG_ID_COMMAND_LONG);
    command->setCommandId(MAV_CMD_REQUEST_BLUETOOTH_DEVICES);
    command->addArgument(from);
    command->addArgument(count);

    m_communicator->commandService()->executeCommand(sysId, command);
}
