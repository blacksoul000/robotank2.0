#include "communicator_builder.h"

#include "heartbeat_handler.h"
#include "command_handler.h"
#include "send_attitude_handler.h"
#include "send_tracking_target_handler.h"
#include "send_status_handler.h"
#include "send_settings_handler.h"
#include "bluetooth_devices_handler.h"
#include "bluetooth_pair_handler.h"

using namespace domain;

CommunicatorBuilder::CommunicatorBuilder() :
    MavLinkCommunicatorBuilder()
{}

void CommunicatorBuilder::buildHandlers()
{
    new HeartbeatHandler(m_communicator);
    new CommandHandler(m_communicator);
    new SendAttitudeHandler(m_communicator);
    new SendTrackingTargetHandler(m_communicator);
    new SendStatusHandler(m_communicator);
    new SendSettingsHandler(m_communicator);
    new BluetoothDevicesHandler(m_communicator);
    new BluetoothPairHandler(m_communicator);
}
