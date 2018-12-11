#include "communicator_builder.h"

#include "heartbeat_handler.h"
#include "attitude_handler.h"
#include "tracking_target_handler.h"
#include "sys_status_handler.h"
#include "mavlink_command_handler.h"
#include "settings_handler.h"
#include "bluetooth_devices_handler.h"
#include "bluetooth_pair_handler.h"

struct CommunicatorBuilder::Impl
{
    domain::RoboModel* model = nullptr;
};

CommunicatorBuilder::CommunicatorBuilder(domain::RoboModel* model) :
    MavLinkCommunicatorBuilder(),
    d(new Impl)
{
    d->model = model;
}

CommunicatorBuilder::~CommunicatorBuilder()
{
    delete d;
}

void CommunicatorBuilder::buildHandlers()
{
    using namespace domain;

    new HeartbeatHandler(m_communicator);
    new MavLinkCommandHandler(m_communicator);
    new BluetoothPairHandler(m_communicator);
    new AttitudeHandler(m_communicator, d->model);
    new TrackingTargetHandler(m_communicator, d->model);
    new SysStatusHandler(m_communicator, d->model);
    new SettingsHandler(m_communicator, d->model);
    new BluetoothDevicesHandler(m_communicator, d->model);
}
