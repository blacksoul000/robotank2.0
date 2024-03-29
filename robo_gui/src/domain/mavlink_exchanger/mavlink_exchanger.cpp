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

#include <QSettings>
#include <QNetworkConfigurationManager>
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
    QNetworkConfigurationManager manager;
    domain::MavLinkCommunicator* communicator = nullptr;
    domain::RoboModel* model = nullptr;
    domain::VehiclePtr vehicle;
    int vehicleId = 0;
};

MavlinkExchanger::MavlinkExchanger(domain::RoboModel* model) :
    QObject(model),
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
    connect(d->model->settings(), &domain::SettingsModel::trackerChanged,
            this, &MavlinkExchanger::onSelectedTrackerChanged);

    connect(d->model->track(), &domain::TrackModel::trackRequest,
            this, &MavlinkExchanger::onTrackToggle);

    connect(d->model->gamepad(), &domain::GamepadModel::joyChanged,
            this, &MavlinkExchanger::onJoyChanged);
}

MavlinkExchanger::~MavlinkExchanger()
{
    this->onDismiss();
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
    connect(&d->manager, &QNetworkConfigurationManager::configurationChanged,
            this, &MavlinkExchanger::onNetworkConfigurationChanged);

    for (const QNetworkInterface& iface: QNetworkInterface::allInterfaces())
    {
        if (iface.flags().testFlag(QNetworkInterface::IsUp)
                && !iface.flags().testFlag(QNetworkInterface::IsLoopBack))
        {
            this->onInterfaceOnlineChanged(iface, !iface.addressEntries().empty());
        }
    }

    d->vehicleId = QSettings().value("vehicleId", 0).toInt();
}

void MavlinkExchanger::onVehicleAdded(VehiclePtr vehicle)
{
    if (d->vehicle) return;

    if (vehicle && vehicle->sysId() == d->vehicleId)
    {
        this->setCurrentVehicle(vehicle);
    }
}

void MavlinkExchanger::onVehicleRemoved(VehiclePtr vehicle)
{
    if (!d->vehicle || !vehicle) return;
    if (d->vehicle->sysId() != vehicle->sysId()) return;

    disconnect(d->vehicle.data(), &domain::Vehicle::onlineChanged,
            this, &MavlinkExchanger::onVehicleOnlineChanged);
    d->vehicle.clear();
}

QMap< int, domain::VehiclePtr > MavlinkExchanger::vehicles() const
{
    return d->communicator->vehicleRegistry()->vehicles();
}

void MavlinkExchanger::onVehicleOnlineChanged()
{
    if (!d->vehicle) return;

    if (d->vehicle->isOnline())
    {
        d->model->status()->setChassisStatus(true);
        this->onHandshake();

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

void MavlinkExchanger::onNetworkConfigurationChanged(const QNetworkConfiguration &config)
{
    auto iface = QNetworkInterface::interfaceFromName(config.name());
    if (!iface.isValid()) return;
    if (iface.flags().testFlag(QNetworkInterface::IsLoopBack)) return;

    this->onInterfaceOnlineChanged(iface, !iface.addressEntries().empty());
}

void MavlinkExchanger::onInterfaceOnlineChanged(const QNetworkInterface& iface, bool online)
{
    if (online)
    {
        for (const QNetworkAddressEntry& entry: iface.addressEntries())
        {
            if (!entry.broadcast().isNull())
            {
                if (entry.broadcast().toString() != "192.168.1.255") continue;  // FIXME
                auto link = new data_source::UdpLink(iface.name(), {entry.broadcast(), 14550},
                                                     {QHostAddress::AnyIPv4, 14550});
                d->communicator->addHeartbeatLink(link);
                link->connectLink();
            }
        }
    } else {
        for (const auto& link: d->communicator->heartbeatLinks())
        {
            if (link->interface() == iface.name())
            {
                link->disconnectLink();
                d->communicator->removeHeartbeatLink(link);
            }
        }
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

void MavlinkExchanger::onHandshake()
{
    if (!d->vehicle) return;
    qDebug() << Q_FUNC_INFO;

    domain::CommandPtr command = domain::CommandPtr::create();
    command->setType(MAVLINK_MSG_ID_COMMAND_LONG);
    command->setCommandId(MAV_CMD_HANDSHAKE);
    d->communicator->commandService()->executeCommand(d->vehicle->sysId(), command);
}

void MavlinkExchanger::onDismiss()
{
    if (!d->vehicle) return;
    qDebug() << Q_FUNC_INFO;

    domain::CommandPtr command = domain::CommandPtr::create();
    command->setType(MAVLINK_MSG_ID_COMMAND_LONG);
    command->setCommandId(MAV_CMD_DISMISS);
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

    command->addArgument(axes.value(Axes::DigitalX, 0)
			? axes.value(Axes::DigitalX, 0) : axes.value(Axes::X1, 0));
    command->addArgument(axes.value(Axes::DigitalY, 0)
    		? -axes.value(Axes::DigitalY, 0) : -axes.value(Axes::Y1, 0));
    command->addArgument(axes.value(Axes::X2, 0));
    command->addArgument(-axes.value(Axes::Y2, 0));
    command->addArgument(model->buttons());

    d->communicator->commandService()->executeCommand(d->vehicle->sysId(), command, true);
}

domain::VehiclePtr MavlinkExchanger::currentVehicle() const
{
    return d->vehicle;
}

void MavlinkExchanger::setCurrentVehicle(const domain::VehiclePtr& vehicle)
{
    qDebug() << Q_FUNC_INFO << vehicle;
    if (vehicle == d->vehicle) return;
    
    if (d->vehicle)
    {
        this->onDismiss();
        disconnect(vehicle.data(), &domain::Vehicle::onlineChanged,
            this, &MavlinkExchanger::onVehicleOnlineChanged);
        d->vehicleId = 0;
    }

    d->vehicle = vehicle;

    if (d->vehicle)
    {
        d->vehicleId = d->vehicle->sysId();
        connect(vehicle.data(), &domain::Vehicle::onlineChanged,
            this, &MavlinkExchanger::onVehicleOnlineChanged);
        this->onHandshake();
    }

    QSettings().setValue("vehicleId", d->vehicleId);
    emit currentVehicleChanged();
}
