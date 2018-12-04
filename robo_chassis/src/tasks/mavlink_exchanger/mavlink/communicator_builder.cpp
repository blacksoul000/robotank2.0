#include "communicator_builder.h"

#include "heartbeat_handler.h"
#include "send_attitude_handler.h"
#include "send_gps_raw_handler.h"

using namespace domain;

CommunicatorBuilder::CommunicatorBuilder() :
    MavLinkCommunicatorBuilder()
{}

void CommunicatorBuilder::buildHandlers()
{
    new HeartbeatHandler(m_communicator);
    new SendAttitudeHandler(m_communicator);
    new SendGpsRawHandler(m_communicator);
}
