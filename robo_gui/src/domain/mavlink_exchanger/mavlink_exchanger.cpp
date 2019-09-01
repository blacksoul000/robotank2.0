#include "mavlink_exchanger.h"

#include "robo_model.h"
#include "bluetooth_model.h"
#include "status_model.h"
#include "gamepad_model.h"
#include "settings_model.h"
#include "track_model.h"

#include "gamepad.h"

#include "endpoint.h"
#include "udp_link.h"
#include "mavlink_communicator.h"
#include "communicator_builder.h"
#include "vehicle_registry.h"
#include "vehicle.h"
#include "command_service.h"
#include "command.h"
#include "mavlink_traits.h"

#include <mavlink.h>

#include <QBluetoothAddress>
#include <QNetworkInterface>
#include <QHostAddress>
#include <QRectF>
#include <QDebug>

using domain::MavlinkExchanger;
using domain::Command;

class MavlinkExchanger::Impl
{
public:
    domain::MavLinkCommunicator* communicator = nullptr;
    domain::RoboModel* model = nullptr;
    domain::VehiclePtr vehicle;
};

MavlinkExchanger::MavlinkExchanger(domain::RoboModel* model, QObject *parent) :
    QObject(parent),
    d(new Impl)
{
    d->model = model;

    connect(d->model->bluetooth(), &domain::BluetoothModel::requestScan,
            this, &MavlinkExchanger::onRequestScan);
    connect(d->model->bluetooth(), &domain::BluetoothModel::requestPair,
            this, &MavlinkExchanger::onRequestPair);

    connect(d->model->settings(), &domain::SettingsModel::enginePowerChanged,
            this, &MavlinkExchanger::onEnginePowerChanged);
    connect(d->model->settings(), &domain::SettingsModel::calibrateGun,
            this, &MavlinkExchanger::onCalibrateGun);
    connect(d->model->settings(), &domain::SettingsModel::calibrateGyro,
            this, &MavlinkExchanger::onCalibrateGyro);
    connect(d->model->settings(), &domain::SettingsModel::qualityChanged,
            this, &MavlinkExchanger::onImageSettingsChanged);
    connect(d->model->settings(), &domain::SettingsModel::brightnessChanged,
            this, &MavlinkExchanger::onImageSettingsChanged);
    connect(d->model->settings(), &domain::SettingsModel::contrastChanged,
            this, &MavlinkExchanger::onImageSettingsChanged);

    connect(d->model->track(), &domain::TrackModel::trackRequest,
            this, &MavlinkExchanger::onTrackToggle);

    connect(d->model->track(), &domain::TrackModel::trackRequest,
            this, &MavlinkExchanger::onTrackToggle);

    connect(d->model->gamepad(), &domain::GamepadModel::joyChanged,
            this, &MavlinkExchanger::onJoyChanged);
}

MavlinkExchanger::~MavlinkExchanger()
{
    delete d->communicator;
    delete d;
}

void MavlinkExchanger::start()
{
    CommunicatorBuilder builder(d->model);
    builder.initCommunicator();
    builder.buildIdentification(255, 0, MAV_TYPE_GCS);
    builder.buildHandlers(); // later
    d->communicator = builder.getCommunicator();
    connect(d->communicator->vehicleRegistry().data(), &domain::VehicleRegistry::vehicleAdded,
            this, &MavlinkExchanger::onVehicleAdded);
    connect(d->communicator->vehicleRegistry().data(), &domain::VehicleRegistry::vehicleRemoved,
            this, &MavlinkExchanger::onVehicleRemoved);

    for (const QNetworkInterface& iface: QNetworkInterface::allInterfaces())
    {
        if (iface.flags().testFlag(QNetworkInterface::IsUp)
                && !iface.flags().testFlag(QNetworkInterface::IsLoopBack))
        {
            for (const QNetworkAddressEntry& entry: iface.addressEntries())
            {
                if (!entry.broadcast().isNull())
                {
                    auto link = new data_source::UdpLink({entry.broadcast(), 14550},
                                                         {QHostAddress::Any, 14550});
                    d->communicator->addHeartbeatLink(link);
                    link->connectLink();
                }
            }
        }
    }
}

void MavlinkExchanger::onVehicleAdded(VehiclePtr vehicle)
{
    if (d->vehicle) return;
    if (!vehicle) return;

    d->vehicle = vehicle;
    connect(vehicle.data(), &domain::Vehicle::onlineChanged,
            this, &MavlinkExchanger::onVehicleOnlineChanged);
}

void MavlinkExchanger::onVehicleRemoved(VehiclePtr vehicle)
{
    if (!d->vehicle || !vehicle) return;
    if (vehicle->sysId() != vehicle->sysId()) return;

    disconnect(d->vehicle.data(), &domain::Vehicle::onlineChanged,
            this, &MavlinkExchanger::onVehicleOnlineChanged);
    d->vehicle.clear();
}

void MavlinkExchanger::onVehicleOnlineChanged(bool online)
{
    if (!d->vehicle) return;

    if (online)
    {
        d->model->status()->setChassisStatus(true);

        domain::CommandPtr command = domain::CommandPtr::create();
        command->setType(MAVLINK_MSG_ID_COMMAND_LONG);
        command->setCommandId(MAV_CMD_REQUEST_SETTINGS);
        d->communicator->commandService()->executeCommand(d->vehicle->sysId(), command);
    }
    else
    {
        d->model->status()->setChassisStatus(false);
        d->model->gamepad()->setConnected(false);
        d->model->status()->setPointerStatus(false);
        d->model->status()->setHeadlightStatus(false);
        d->model->bluetooth()->setScanStatus(false);
    }
}

void MavlinkExchanger::onSelectedTrackerChanged()
{
    if (!d->vehicle) return;

    domain::CommandPtr command = domain::CommandPtr::create();
    command->setType(MAVLINK_MSG_ID_COMMAND_LONG);
    command->setCommandId(MAV_CMD_SET_TRACKER_CODE);
    command->addArgument(d->model->settings()->tracker());

    d->communicator->commandService()->executeCommand(d->vehicle->sysId(), command);
}

void MavlinkExchanger::onTrackToggle(const QRectF& rect)
{
    if (!d->vehicle) return;

    domain::CommandPtr command = domain::CommandPtr::create();
    command->setType(MAVLINK_MSG_ID_COMMAND_LONG);
    command->setCommandId(MAV_CMD_TRACK);
    command->addArgument(rect.x());
    command->addArgument(rect.y());
    command->addArgument(rect.width());
    command->addArgument(rect.height());

    d->communicator->commandService()->executeCommand(d->vehicle->sysId(), command);
}

void MavlinkExchanger::onRequestScan()
{
    if (!d->vehicle) return;

    domain::CommandPtr command = domain::CommandPtr::create();
    command->setType(MAVLINK_MSG_ID_COMMAND_LONG);
    command->setCommandId(MAV_CMD_BLUETOOTH_SCAN);

    d->communicator->commandService()->executeCommand(d->vehicle->sysId(), command);
}

void MavlinkExchanger::onRequestPair(quint64 address, bool paired)
{
    if (!d->vehicle) return;

    domain::CommandPtr command = domain::CommandPtr::create();
    command->setType(MAVLINK_MSG_ID_COMMAND_BLUETOOTH_PAIR);
    command->setCommandId(MAV_CMD_BLUETOOTH_PAIR);
    command->addArgument(address);
    command->addArgument(paired);

    d->communicator->commandService()->executeCommand(d->vehicle->sysId(), command);
}

void MavlinkExchanger::onEnginePowerChanged()
{
    if (!d->vehicle) return;

    domain::CommandPtr command = domain::CommandPtr::create();
    command->setType(MAVLINK_MSG_ID_COMMAND_LONG);
    command->setCommandId(MAV_CMD_ENGINE_POWER);
    const auto& s = d->model->settings();
    command->addArgument(s->enginePower(domain::SettingsModel::Left));
    command->addArgument(s->enginePower(domain::SettingsModel::Right));

    d->communicator->commandService()->executeCommand(d->vehicle->sysId(), command);
}

void MavlinkExchanger::onCalibrateGun()
{
    if (!d->vehicle) return;

    domain::CommandPtr command = domain::CommandPtr::create();
    command->setType(MAVLINK_MSG_ID_COMMAND_LONG);
    command->setCommandId(MAV_CMD_CALIBRATE_GUN);

    d->communicator->commandService()->executeCommand(d->vehicle->sysId(), command);
}

void MavlinkExchanger::onCalibrateGyro()
{
    if (!d->vehicle) return;

    domain::CommandPtr command = domain::CommandPtr::create();
    command->setType(MAVLINK_MSG_ID_COMMAND_LONG);
    command->setCommandId(MAV_CMD_CALIBRATE_YPR);

    d->communicator->commandService()->executeCommand(d->vehicle->sysId(), command);
}

void MavlinkExchanger::onImageSettingsChanged()
{
    if (!d->vehicle) return;

    domain::CommandPtr command = domain::CommandPtr::create();
    command->setType(MAVLINK_MSG_ID_COMMAND_LONG);
    command->setCommandId(MAV_CMD_IMAGE_SETTINGS);
    const auto& s = d->model->settings();
    command->addArgument(s->quality());
    command->addArgument(s->brightness());
    command->addArgument(s->contrast());

    d->communicator->commandService()->executeCommand(d->vehicle->sysId(), command);
}

void MavlinkExchanger::onJoyChanged()
{
    if (!d->vehicle) return;

	using Axes = domain::GamepadModel::Axes;

    domain::CommandPtr command = domain::CommandPtr::create();
    command->setType(MAVLINK_MSG_ID_COMMAND_LONG);
    command->setCommandId(MAV_CMD_JOY_EVENT);
    const auto& model = d->model->gamepad();
    const auto& axes = model->axes();

    command->addArgument(axes.at(Axes::DigitalX)
			? axes.at(Axes::DigitalX) : axes.at(Axes::X1));
    command->addArgument(axes.at(Axes::DigitalY)
    		? -axes.at(Axes::DigitalY) : -axes.at(Axes::Y1));
    command->addArgument(axes.at(Axes::X2));
    command->addArgument(-axes.at(Axes::Y2));
    command->addArgument(model->buttons());

//    qDebug() << Q_FUNC_INFO << __LINE__
//    		<< (axes.at(Axes::DigitalX)
//				? axes.at(Axes::DigitalX) : axes.at(Axes::X1))
//			<< (axes.at(Axes::DigitalY)
//				? axes.at(Axes::DigitalY) : axes.at(Axes::Y1))
//			<< axes.at(Axes::X2)
//			<< axes.at(Axes::Y2)
//			<< axes
//			<< model->buttons();

    d->communicator->commandService()->executeCommand(d->vehicle->sysId(), command, true);
}
