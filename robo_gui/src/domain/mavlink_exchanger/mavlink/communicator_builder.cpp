#include "communicator_builder.h"

#include "heartbeat_handler.h"
#include "attitude_handler.h"

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
    new AttitudeHandler(m_communicator, d->model);
}
