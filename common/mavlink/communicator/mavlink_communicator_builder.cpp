#include "mavlink_communicator_builder.h"

// Internal
#include "mavlink_communicator.h"

using namespace domain;

MavLinkCommunicatorBuilder::MavLinkCommunicatorBuilder():
    m_communicator(nullptr)
{}

MavLinkCommunicatorBuilder::~MavLinkCommunicatorBuilder()
{
    if (m_communicator) delete m_communicator;
}

void MavLinkCommunicatorBuilder::initCommunicator()
{
    m_communicator = new MavLinkCommunicator();
}

MavLinkCommunicator* MavLinkCommunicatorBuilder::getCommunicator()
{
    MavLinkCommunicator* communicator = m_communicator;
    m_communicator = nullptr;
    return communicator;
}

void MavLinkCommunicatorBuilder::buildIdentification(quint8 systemId, quint8 componentId, quint8 mavType)
{
    m_communicator->setSystemId(systemId);
    m_communicator->setComponentId(componentId);
    m_communicator->setMavType(mavType);
}
